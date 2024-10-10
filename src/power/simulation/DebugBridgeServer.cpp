// DebugBridgeServer.cpp

#include "DebugBridgeServer.hpp"

#include "simulation/Cartridge.hpp"
#include "simulation/CartridgeActorLoader.hpp"
#include "simulation/ILoadedCartridge.hpp"

#include <nanogui/common.h>

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cstdio>   // For std::remove
#include <cstring>  // For std::memcpy

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>  // For memfd_create
#include <sys/stat.h>
#endif

#include <cstdlib>  // For system()

CartridgeBridge::CartridgeBridge(uint16_t port, ICartridge& cartridge, CartridgeActorLoader& actorLoader, std::function<void(std::optional<std::reference_wrapper<ILoadedCartridge>>)> onCartridgeInsertedCallback)
: m_port(port), mCartridge(cartridge), mActorLoader(actorLoader), mOnCartridgeInsertedCallback(onCartridgeInsertedCallback),
#ifdef __linux__
m_mem_fd(-1),
#elif defined(__APPLE__)
m_ram_disk_path(""),
#endif
mLoadedCartridge(nullptr),
#ifdef _WIN32
mSharedObjectHandle(nullptr) // Initialize for Windows
#else
mSharedObjectHandle(nullptr) // Initialize for Unix-like systems
#endif

{
	m_server.init_asio();
	
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
		
		// Initialize the disk at server start
		initialize_disk();
		
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
		
		// Cleanup the disk when stopping the server
		erase_disk();
		
		std::cout << "DebugBridgeServer has stopped." << std::endl;
	} catch (const websocketpp::exception& e) {
		std::cerr << "WebSocket++ exception on stop: " << e.what() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Standard exception on stop: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown exception on stop." << std::endl;
	}
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
			// Before loading a new cartridge, erase/eject the existing disk
			erase_disk();
			
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

#ifdef _WIN32
void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
	// Close existing DLL if loaded
	if (mSharedObjectHandle) {
		FreeLibrary(mSharedObjectHandle);
		mSharedObjectHandle = nullptr;
		std::cout << "Previous DLL unloaded successfully." << std::endl;
	}
	
	// Save the received data to a temporary DLL file
	const char* temp_filename = "temp_module.dll";
	std::ofstream ofs(temp_filename, std::ios::binary);
	if (!ofs) {
		std::cerr << "Failed to create temporary DLL file." << std::endl;
		return;
	}
	ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
	ofs.close();
	
	// Load the DLL
	mSharedObjectHandle = LoadLibraryA(temp_filename);
	if (!mSharedObjectHandle) {
		std::cerr << "Failed to load DLL: " << GetLastError() << std::endl;
		std::remove(temp_filename);
		return;
	}
	
	// Get the say_hello function
	typedef void (*SayHelloFunc)();
	SayHelloFunc say_hello = (SayHelloFunc)GetProcAddress(mSharedObjectHandle, "say_hello");
	if (!say_hello) {
		std::cerr << "Failed to find say_hello function: " << GetLastError() << std::endl;
		FreeLibrary(mSharedObjectHandle);
		mSharedObjectHandle = nullptr;
		std::remove(temp_filename);
		return;
	}
	
	// Call the say_hello function
	try {
		say_hello();
	} catch (...) {
		std::cerr << "Exception occurred while executing say_hello." << std::endl;
	}
	
	// Do not unload the DLL here. It will be unloaded when a new cartridge is loaded or on server stop.
	
	// Optionally, remove the temporary file if it's no longer needed
	// std::remove(temp_filename);
}

#elif defined(__linux__) || defined(__APPLE__)
void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
	// Close existing shared object if loaded
	if (mSharedObjectHandle) {
		dlclose(mSharedObjectHandle);
		mSharedObjectHandle = nullptr;
		std::cout << "Previous shared object unloaded successfully." << std::endl;
	}
	
	// Check for 'SOLO' magic number and remove it
	size_t offset = 0;
	if (data.size() >= 4 && data[0] == 'S' && data[1] == 'O' && data[2] == 'L' && data[3] == 'O') {
		offset = 4;
	}
	
	size_t so_size = data.size() - offset;
	if (so_size == 0) {
		std::cerr << "No shared object data to execute." << std::endl;
		return;
	}
	
#ifdef __linux__
	// Linux-specific implementation using memfd_create
	m_mem_fd = memfd_create("cartridge_disk", MFD_CLOEXEC);
	if (m_mem_fd == -1) {
		std::cerr << "memfd_create failed: " << strerror(errno) << std::endl;
		return;
	}
	
	// Write the .so data into the memory file
	ssize_t written = write(m_mem_fd, data.data() + offset, so_size);
	if (written != static_cast<ssize_t>(so_size)) {
		std::cerr << "Failed to write all data to memfd: " << strerror(errno) << std::endl;
		close(m_mem_fd);
		m_mem_fd = -1;
		return;
	}
	
	// Retrieve the file descriptor's path using /proc/self/fd/
	std::string fd_path = "/proc/self/fd/" + std::to_string(m_mem_fd);
	
	// Load the shared object
	mSharedObjectHandle = dlopen(fd_path.c_str(), RTLD_NOW);
	if (!mSharedObjectHandle) {
		std::cerr << "dlopen failed: " << dlerror() << std::endl;
		close(m_mem_fd);
		m_mem_fd = -1;
		return;
	}
	
#elif defined(__APPLE__)
	// macOS-specific implementation using a RAM Disk
	
	// Step 1: Create a RAM Disk
	const int ram_disk_size_mb = 256;
	const int sectors = ram_disk_size_mb * 2048; // 1 sector = 512 bytes
	
	std::string create_ram_disk_cmd = "hdiutil attach -nomount ram://" + std::to_string(sectors);
	FILE* pipe = popen(create_ram_disk_cmd.c_str(), "r");
	if (!pipe) {
		std::cerr << "Failed to create RAM disk." << std::endl;
		return;
	}
	
	char ram_disk_path[256];
	if (!fgets(ram_disk_path, sizeof(ram_disk_path), pipe)) {
		std::cerr << "Failed to get RAM disk path." << std::endl;
		pclose(pipe);
		return;
	}
	pclose(pipe);
	
	// Remove trailing newline
	size_t len = strlen(ram_disk_path);
	if (len > 0 && ram_disk_path[len - 1] == '\n') {
		ram_disk_path[len - 1] = '\0';
	}
	
	m_ram_disk_path = std::string(ram_disk_path);
	
	// Format the RAM disk as HFS+
	std::string format_ram_disk_cmd = "diskutil erasevolume HFS+ 'RAMDisk' " + m_ram_disk_path;
	int ret = system(format_ram_disk_cmd.c_str());
	if (ret != 0) {
		std::cerr << "Failed to format RAM disk." << std::endl;
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
		return;
	}
	
	// Step 2: Write the shared object to the RAM disk
	char temp_filename_template[] = "/Volumes/RAMDisk/temp_module_XXXXXX.dylib";
	char temp_filename[sizeof(temp_filename_template)];
	strcpy(temp_filename, temp_filename_template);
	
	int fd = mkstemps(temp_filename, 6); // 6 for ".dylib"
	if (fd == -1) {
		std::cerr << "Failed to create temporary dylib file on RAM disk: " << strerror(errno) << std::endl;
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
		return;
	}
	
	// Write the .dylib data to the temporary file
	ssize_t written = write(fd, data.data() + offset, so_size);
	if (written != static_cast<ssize_t>(so_size)) {
		std::cerr << "Failed to write all data to temporary dylib: " << strerror(errno) << std::endl;
		close(fd);
		std::remove(temp_filename);
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
		return;
	}
	
	// Close the file descriptor
	close(fd);
	
	// Step 3: Load the shared object
	mSharedObjectHandle = dlopen(temp_filename, RTLD_NOW);
	if (!mSharedObjectHandle) {
		std::cerr << "dlopen failed: " << dlerror() << std::endl;
		std::remove(temp_filename);
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
		return;
	}
#endif // Platform-specific loading
	
	// Clear any existing errors
	dlerror();
	
	// Get the load_cartridge function
	typedef ILoadedCartridge* (*LoadCartridgeFunc)(ICartridge& cartridge);
	LoadCartridgeFunc load_cartridge = (LoadCartridgeFunc)dlsym(mSharedObjectHandle, "load_cartridge");
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		std::cerr << "Failed to find load_cartridge function: " << dlsym_error << std::endl;
#ifdef _WIN32
		FreeLibrary(mSharedObjectHandle);
#else
		dlclose(mSharedObjectHandle);
#endif
		mSharedObjectHandle = nullptr;
		
#ifdef __linux__
		close(m_mem_fd);
		m_mem_fd = -1;
#elif defined(__APPLE__)
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
#endif
		return;
	}
	
	// Call the load_cartridge function
	// move the loading to the main thread
	nanogui::async([this, load_cartridge](){
		try {
			mLoadedCartridge = std::unique_ptr<ILoadedCartridge>(load_cartridge(mCartridge)); // load new cartridge
			
			mOnCartridgeInsertedCallback(*mLoadedCartridge); // insert cartridge
			

		} catch (...) {
			std::cerr << "Exception occurred while executing load_cartridge." << std::endl;
#ifdef _WIN32
			FreeLibrary(mSharedObjectHandle);
#else
			dlclose(mSharedObjectHandle);
#endif
			mSharedObjectHandle = nullptr;
			
#ifdef __linux__
			close(m_mem_fd);
			m_mem_fd = -1;
#elif defined(__APPLE__)
			std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
			system(detach_cmd.c_str());
			m_ram_disk_path = "";
#endif
		}
	});

	
	// Do not close the shared object here. It will remain loaded until a new cartridge is loaded or the server stops.
}
#endif


// Initialize the disk when the server starts
void CartridgeBridge::initialize_disk() {
#ifdef __linux__
	// Create an initial memory file (disk)
	m_mem_fd = memfd_create("initial_disk", MFD_CLOEXEC);
	if (m_mem_fd == -1) {
		std::cerr << "memfd_create failed: " << strerror(errno) << std::endl;
		// Handle error as needed
	}
	// Optionally, set up the memory file as needed
#elif defined(__APPLE__)
	// Create an initial RAM disk
	const int ram_disk_size_mb = 10;
	const int sectors = ram_disk_size_mb * 2048; // 1 sector = 512 bytes
	
	std::string create_ram_disk_cmd = "hdiutil attach -nomount ram://" + std::to_string(sectors);
	FILE* pipe = popen(create_ram_disk_cmd.c_str(), "r");
	if (!pipe) {
		std::cerr << "Failed to create initial RAM disk." << std::endl;
		return;
	}
	
	char ram_disk_path[256];
	if (!fgets(ram_disk_path, sizeof(ram_disk_path), pipe)) {
		std::cerr << "Failed to get initial RAM disk path." << std::endl;
		pclose(pipe);
		return;
	}
	pclose(pipe);
	
	// Remove trailing newline
	size_t len = strlen(ram_disk_path);
	if (len > 0 && ram_disk_path[len - 1] == '\n') {
		ram_disk_path[len - 1] = '\0';
	}
	
	m_ram_disk_path = std::string(ram_disk_path);
	
	// Format the RAM disk as HFS+
	std::string format_ram_disk_cmd = "diskutil erasevolume HFS+ 'RAMDisk' " + m_ram_disk_path;
	int ret = system(format_ram_disk_cmd.c_str());
	if (ret != 0) {
		std::cerr << "Failed to format initial RAM disk." << std::endl;
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		system(detach_cmd.c_str());
		m_ram_disk_path = "";
		return;
	}
#endif
}

// Erase and eject the current disk
void CartridgeBridge::erase_disk() {
#ifdef __linux__
	if (m_mem_fd != -1) {
		close(m_mem_fd);
		m_mem_fd = -1;
		std::cout << "Memory file (disk) erased and closed on Linux." << std::endl;
	}
#elif defined(__APPLE__)
	if (!m_ram_disk_path.empty()) {
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + m_ram_disk_path;
		int ret = system(detach_cmd.c_str());
		if (ret != 0) {
			std::cerr << "Failed to detach RAM disk: " << m_ram_disk_path << std::endl;
		} else {
			std::cout << "RAM disk detached successfully: " << m_ram_disk_path << std::endl;
		}
		m_ram_disk_path = "";
	}
#endif
	
	// If a cartridge was loaded, reset it
	if (mLoadedCartridge) {
	
		mOnCartridgeInsertedCallback(std::nullopt); // eject cartridge to prevent updating

		mActorLoader.cleanup();
	
		mLoadedCartridge.reset(); // release cartridge memory
		std::cout << "Loaded cartridge has been ejected." << std::endl;
	}
}

