// DebugBridgeServer.cpp

#include "DebugBridgeServer.hpp"

#include "simulation/Cartridge.hpp"
#include "simulation/CartridgeActorLoader.hpp"
#include "simulation/ILoadedCartridge.hpp"

#include <nanogui/common.h>

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cstdio>   // For std::remove, mkstemp
#include <cstring>  // For std::memcpy

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>  // For shm_open on macOS
#include <sys/stat.h>
#endif

#include <cstdlib>  // For system()
#include <vector>
#include <mutex>
#include <thread>
#include <functional>
#include <optional>
#include <span>

// Define INITIAL_MEMORY_SIZE for shared memory on macOS
#define INITIAL_MEMORY_SIZE (10 * 1024 * 1024) // 10 MB, adjust as needed

CartridgeBridge::CartridgeBridge(uint16_t port, ICartridge& cartridge, CartridgeActorLoader& actorLoader, std::function<void(std::optional<std::reference_wrapper<ILoadedCartridge>>)> onCartridgeInsertedCallback)
: m_port(port), mCartridge(cartridge), mActorLoader(actorLoader), mOnCartridgeInsertedCallback(onCartridgeInsertedCallback),
#ifdef __linux__
m_mem_fd(-1),
#elif defined(__APPLE__)
m_mem_fd(-1),
m_mapped_memory(nullptr),
m_temp_so_path(""),
#endif
mLoadedCartridge(nullptr),
mStore(mWasmEngine)
{
	m_server.init_asio();
	
	m_server.set_validate_handler(
								 websocketpp::lib::bind(
														&CartridgeBridge::validate_connection,
														this,
														websocketpp::lib::placeholders::_1
														)
								 );

	m_server.set_message_handler(
								 websocketpp::lib::bind(
														&CartridgeBridge::on_message,
														this,
														websocketpp::lib::placeholders::_1,
														websocketpp::lib::placeholders::_2
														)
								 );
	
	// Disable logging for production
	m_server.clear_access_channels(websocketpp::log::alevel::all);
	m_server.set_access_channels(websocketpp::log::alevel::none);
}

CartridgeBridge::~CartridgeBridge() {
	stop();
}

void CartridgeBridge::run() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_running.load()) {
		std::cerr << "Server is already running." << std::endl;
		return;
	}
	
	try {
		m_server.listen(m_port);
		m_server.start_accept();
		
		std::cout << "DebugBridgeServer is running on port " << m_port << std::endl;
		
		// Initialize memory mappings at server start
		initialize_memory();
		
		m_running.store(true);
		m_thread = std::thread([this]() {
			try {
				m_server.run();
			} catch (const websocketpp::exception& e) {
				std::cerr << "WebSocket++ exception in thread: " << e.what() << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Standard exception in thread: " << e.what() << std::endl;
			} catch (...) {
				std::cerr << "Unknown exception in thread." << std::endl;
			}
			
			m_running.store(false);
			std::cout << "DebugBridgeServer thread has exited." << std::endl;
		});
	} catch (const websocketpp::exception& e) {
		std::cerr << "WebSocket++ exception: " << e.what() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Standard exception: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Other exception." << std::endl;
	}
}

void CartridgeBridge::stop() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_running.load()) {
		std::cerr << "Server is not running." << std::endl;
		return;
	}
	
	try {
		m_server.stop_listening();
		m_server.stop();
		
		if (m_thread.joinable()) {
			m_thread.join();
		}
		
		// Cleanup memory mappings when stopping the server
		erase_memory(true); // Force erase
		
		std::cout << "DebugBridgeServer has stopped." << std::endl;
	} catch (const websocketpp::exception& e) {
		std::cerr << "WebSocket++ exception on stop: " << e.what() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Standard exception on stop: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown exception on stop." << std::endl;
	}
}

bool CartridgeBridge::validate_connection(websocketpp::connection_hdl hdl) {
	return true;
}

void CartridgeBridge::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
	std::lock_guard<std::mutex> lock(m_mutex);
	try {
		if (msg->get_opcode() != websocketpp::frame::opcode::binary) {
			std::cerr << "Received non-binary message. Ignoring." << std::endl;
			return;
		}
		
		std::vector<uint8_t> data(msg->get_payload().begin(), msg->get_payload().end());
		
		// Check for magic number 'SOLO' at the start (optional)
		if (data.size() >= 4 && data[0] == 'S' && data[1] == 'O' && data[2] == 'L' && data[3] == 'O') {
			// Shared Object execution
			// Before loading a new cartridge, erase/eject the existing memory mapping
			erase_memory(true); // Force erase
			
			execute_shared_object(data);
			std::string ack = "Shared object executed successfully.";
			m_server.send(hdl, ack, websocketpp::frame::opcode::text);
		} else {
			// Assume it's a DebugCommand
			DebugCommand cmd = DebugCommand::deserialize(data);
			
			std::cout << "Received command of type " << static_cast<int>(cmd.type)
			<< " with payload: " << cmd.payload << std::endl;
			
			DebugCommand response = process_command(cmd);
			
			std::vector<uint8_t> response_bytes = response.serialize();
			m_server.send(hdl, response_bytes.data(), response_bytes.size(), websocketpp::frame::opcode::binary);
		}
	} catch (const std::exception& e) {
		std::cerr << "Error handling message: " << e.what() << std::endl;
	}
}

DebugCommand CartridgeBridge::process_command(const DebugCommand& cmd) {
	if (cmd.type == DebugCommand::CommandType::EXECUTE) {
		std::string result = "Executed command: " + cmd.payload;
		return DebugCommand(DebugCommand::CommandType::RESPONSE, result);
	}
	
	return DebugCommand(DebugCommand::CommandType::RESPONSE, "Unknown command type.");
}

void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
	// Platform-specific shared object loading
#ifdef _WIN32
	// [Windows-specific loading code remains unchanged]
#elif defined(__linux__) || defined(__APPLE__)
	// Prepare data by removing the 'SOLO' magic number if present
	size_t offset = 0;
	if (data.size() >= 4 && data[0] == 'S' && data[1] == 'O' && data[2] == 'L' && data[3] == 'O') {
		offset = 4;
	}
	
	size_t so_size = data.size() - offset;
	if (so_size == 0) {
		std::cerr << "No shared object data to execute." << std::endl;
		return;
	}
	
#endif
	
	auto copiableData = data;
	wasmtime::Span<uint8_t> span(copiableData.data() + offset, copiableData.size() - offset);
	try {
		// Compile the WebAssembly module
		auto compile_result = wasmtime::Module::compile(mWasmEngine, span);
		if (!compile_result) {
			std::cerr << "Failed to compile WebAssembly module" << std::endl;
			// Handle error as needed
			return;
		}
		wasmtime::Module wasmModule = compile_result.unwrap();
		std::cout << "Instantiating module...\n";
		
		// Configure WASI and store it within our `wasmtime_store_t`
		wasmtime::WasiConfig wasi;
		wasi.preopen_dir("..", "/");
		wasi.inherit_argv();
		wasi.inherit_env();
		wasi.inherit_stdin();
		wasi.inherit_stdout();
		wasi.inherit_stderr();
		auto wasi_set_result = mStore.context().set_wasi(std::move(wasi));
		if (!wasi_set_result) {
			std::cerr << "Failed to set WASI configuration" << std::endl;
			// Handle error as needed
			return;
		}
		
		// Create our linker and define WASI
		wasmtime::Linker linker(mWasmEngine);
		auto define_wasi_result = linker.define_wasi();
		if (!define_wasi_result) {
			std::cerr << "Failed to define WASI in linker" << std::endl;
			// Handle error as needed
			return;
		}
		
		// Instantiate the module
		auto instantiate_result = linker.instantiate(mStore, wasmModule);
		if (!instantiate_result) {
			std::cerr << "Failed to instantiate module" << std::endl;
			// Handle error as needed
			return;
		}
		mWasmInstance = std::make_shared<wasmtime::Instance>(std::move(instantiate_result.unwrap()));
		
		// Get the 'load_cartridge' function
		auto load_cartridge_result = mWasmInstance->get(mStore, "load_cartridge");
		if (!load_cartridge_result) {
			std::cerr << "Failed to get 'load_cartridge' function from the module." << std::endl;
			// Handle error as needed
			return;
		}
		
		if (!load_cartridge_result.has_value()) {
			std::cerr << "'load_cartridge' is not a function in the module." << std::endl;
			return;
		}
		
		wasmtime::Func load_cartridge = std::get<wasmtime::Func>(load_cartridge_result.value());
		
		// Call the 'load_cartridge' function asynchronously
		nanogui::async([this, load_cartridge]() {
			try {
				
				wasmtime::ExternRef externref(&mCartridge);

				auto results = load_cartridge.call(mStore, {externref}).unwrap();
				
				wasmtime::ExternRef val = *results[0].externref();

				ILoadedCartridge* loadedCartridge = std::any_cast<ILoadedCartridge*>(val.data());
				
				mLoadedCartridge = std::unique_ptr<ILoadedCartridge>(loadedCartridge); // load new cartridge
				
				mOnCartridgeInsertedCallback(*mLoadedCartridge); // insert cartridge

			} catch (const std::exception& e) {
				std::cerr << "Exception occurred while executing 'load_cartridge': " << e.what() << std::endl;
				// Handle error as needed
			} catch (...) {
				std::cerr << "Unknown exception occurred while executing 'load_cartridge'." << std::endl;
				// Handle error as needed
			}
		});
		
	} catch (const std::exception& e) {
		std::cerr << "Standard exception: " << e.what() << std::endl;
		// Handle error as needed
		return;
	} catch (...) {
		std::cerr << "Unknown exception occurred during module compilation and instantiation." << std::endl;
		// Handle error as needed
		return;
	}
}

void CartridgeBridge::initialize_memory() {
#ifdef __linux__
	if (m_mem_fd == -1) { // Check if mem_fd is already created
		m_mem_fd = memfd_create("initial_memory", MFD_CLOEXEC);
		if (m_mem_fd == -1) {
			std::cerr << "memfd_create failed: " << strerror(errno) << std::endl;
			// Handle error as needed
		}
		
		// Optionally, set up the memory file as needed
	}
#elif defined(__APPLE__)
	// Define a unique shared memory name
	const char* shm_name = "/cartridge_bridge_shm";
	
	// Create or open the shared memory object
	m_mem_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0600);
	if (m_mem_fd == -1) {
		std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
		// Handle error as needed
		return;
	}
	
	// Set the size of the shared memory object
	if (ftruncate(m_mem_fd, INITIAL_MEMORY_SIZE) == -1) {
		std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
		close(m_mem_fd);
		shm_unlink(shm_name);
		// Handle error as needed
		return;
	}
	
	// Map the shared memory object into the process's address space
	m_mapped_memory = mmap(nullptr, INITIAL_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_mem_fd, 0);
	if (m_mapped_memory == MAP_FAILED) {
		std::cerr << "mmap failed: " << strerror(errno) << std::endl;
		close(m_mem_fd);
		shm_unlink(shm_name);
		// Handle error as needed
		return;
	}
	
	// Optionally, initialize the memory region
	std::memset(m_mapped_memory, 0, INITIAL_MEMORY_SIZE);
	
	std::cout << "Shared memory initialized successfully on macOS." << std::endl;
#endif
}

void CartridgeBridge::erase_memory(bool force) {
	// If a cartridge was loaded, reset it
	if (mLoadedCartridge) {
		mOnCartridgeInsertedCallback(std::nullopt); // Eject cartridge to prevent updating
		mActorLoader.cleanup();
		mLoadedCartridge.reset(); // Release cartridge memory
		std::cout << "Loaded cartridge has been ejected." << std::endl;
	}
	
#ifdef __linux__
	if (m_mem_fd != -1 && force) { // Only erase if forced
		close(m_mem_fd);
		m_mem_fd = -1;
		std::cout << "Memory file (memfd) erased and closed on Linux." << std::endl;
	}
#elif defined(__APPLE__)
	if (m_mapped_memory && force) { // Only erase if forced
		if (munmap(m_mapped_memory, INITIAL_MEMORY_SIZE) == -1) {
			std::cerr << "munmap failed: " << strerror(errno) << std::endl;
		} else {
			std::cout << "Shared memory unmapped successfully on macOS." << std::endl;
		}
		m_mapped_memory = nullptr;
	}
	
	if (m_mem_fd != -1 && force) {
		close(m_mem_fd);
		shm_unlink("/cartridge_bridge_shm"); // Ensure the name matches shm_name in initialize_memory
		m_mem_fd = -1;
		std::cout << "Shared memory object unlinked and closed on macOS." << std::endl;
	}
	
	if (!m_temp_so_path.empty() && force) { // Only unlink if forced
		// Remove the temporary dylib file
		if (std::remove(m_temp_so_path.c_str()) != 0) {
			std::cerr << "Failed to remove temporary dylib file: " << m_temp_so_path << std::endl;
		} else {
			std::cout << "Temporary dylib file removed successfully: " << m_temp_so_path << std::endl;
		}
		
		m_temp_so_path.clear();
	}
#endif
}

#ifdef __APPLE__
void* CartridgeBridge::fdlopen(const void* data, size_t size) {
	if (!data || size == 0) {
		std::cerr << "Invalid data or size for fdlopen." << std::endl;
		return nullptr;
	}
	
	// Create a unique temporary file with .dylib extension
	char temp_filename_template[] = "/tmp/cartridge_bridge_so_XXXXXX.dylib";
	int temp_fd = mkstemps(temp_filename_template, 6); // 6 for ".dylib"
	if (temp_fd == -1) {
		std::cerr << "Failed to create temporary dylib file: " << strerror(errno) << std::endl;
		return nullptr;
	}
	
	// Write the shared library data to the temporary file
	ssize_t written = write(temp_fd, data, size);
	if (written != static_cast<ssize_t>(size)) {
		std::cerr << "Failed to write all data to temporary dylib: " << strerror(errno) << std::endl;
		close(temp_fd);
		std::remove(temp_filename_template);
		return nullptr;
	}
	
	// Close the file descriptor
	close(temp_fd);
	
	// Load the shared library using dlopen
	void* handle = dlopen(temp_filename_template, RTLD_NOW);
	if (!handle) {
		std::cerr << "dlopen failed: " << dlerror() << std::endl;
		std::remove(temp_filename_template);
		return nullptr;
	}
	
	// Store the temporary file path for later cleanup
	m_temp_so_path = std::string(temp_filename_template);
	
	return handle;
}
#endif
