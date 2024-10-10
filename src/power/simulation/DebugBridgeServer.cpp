// DebugBridgeServer.cpp

#include "DebugBridgeServer.hpp"

#include "simulation/Cartridge.hpp"
#include "simulation/ILoadedCartridge.hpp"

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

CartridgeBridge::CartridgeBridge(uint16_t port, ICartridge& cartridge, std::function<void(ILoadedCartridge&)> onCartridgeInsertedCallback) : m_port(port), mCartridge(cartridge), mOnCartridgeInsertedCallback(onCartridgeInsertedCallback) {
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

void CartridgeBridge::run() {
	try {
		m_server.listen(m_port);
		m_server.start_accept();
		
		std::cout << "DebugBridgeServer is running on port " << m_port << std::endl;
		m_server.run();
	} catch (const websocketpp::exception& e) {
		std::cerr << "WebSocket++ exception: " << e.what() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Standard exception: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Other exception." << std::endl;
	}
}

void CartridgeBridge::stop() {
	try {
		m_server.stop_listening();
		m_server.stop();
		std::cout << "DebugBridgeServer has stopped." << std::endl;
	} catch (const websocketpp::exception& e) {
		std::cerr << "WebSocket++ exception on stop: " << e.what() << std::endl;
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
// Windows-specific implementation for loading and executing a DLL
void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
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
	HMODULE hModule = LoadLibraryA(temp_filename);
	if (!hModule) {
		std::cerr << "Failed to load DLL: " << GetLastError() << std::endl;
		std::remove(temp_filename);
		return;
	}
	
	// Get the say_hello function
	typedef void (*SayHelloFunc)();
	SayHelloFunc say_hello = (SayHelloFunc)GetProcAddress(hModule, "say_hello");
	if (!say_hello) {
		std::cerr << "Failed to find say_hello function: " << GetLastError() << std::endl;
		FreeLibrary(hModule);
		std::remove(temp_filename);
		return;
	}
	
	// Call the say_hello function
	try {
		say_hello();
	} catch (...) {
		std::cerr << "Exception occurred while executing say_hello." << std::endl;
	}
	
	// Unload the DLL and remove the temporary file
	FreeLibrary(hModule);
	std::remove(temp_filename);
}
#elif defined(__linux__) || defined(__APPLE__)
// Unix-like system implementation for loading and executing a shared object

void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
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
	int mem_fd = memfd_create("in_memory_so", MFD_CLOEXEC);
	if (mem_fd == -1) {
		std::cerr << "memfd_create failed: " << strerror(errno) << std::endl;
		return;
	}
	
	// Write the .so data into the memory file
	ssize_t written = write(mem_fd, data.data() + offset, so_size);
	if (written != static_cast<ssize_t>(so_size)) {
		std::cerr << "Failed to write all data to memfd: " << strerror(errno) << std::endl;
		close(mem_fd);
		return;
	}
	
	// Retrieve the file descriptor's path using /proc/self/fd/
	// Note: dlopen can accept a path like "/proc/self/fd/<fd>"
	std::string fd_path = "/proc/self/fd/" + std::to_string(mem_fd);
	
	// Load the shared object
	void* handle = dlopen(fd_path.c_str(), RTLD_NOW);
	if (!handle) {
		std::cerr << "dlopen failed: " << dlerror() << std::endl;
		close(mem_fd);
		return;
	}
	
	// Clear any existing errors
	dlerror();
	
	// Get the say_hello function
	typedef void (*SayHelloFunc)();
	SayHelloFunc say_hello = (SayHelloFunc)dlsym(handle, "say_hello");
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		std::cerr << "Failed to find say_hello function: " << dlsym_error << std::endl;
		dlclose(handle);
		close(mem_fd);
		return;
	}
	
	// Call the say_hello function
	try {
		say_hello();
	} catch (...) {
		std::cerr << "Exception occurred while executing say_hello." << std::endl;
	}
	
	// Close the shared object and the memory file descriptor
	dlclose(handle);
	close(mem_fd);
#elif defined(__APPLE__)
	// macOS-specific implementation using a RAM Disk
	
	// Step 1: Create a RAM Disk
	// You can create a RAM disk using the `hdiutil` command
	// Here, we'll create it programmatically
	
	// Define RAM disk size (e.g., 10MB)
	const int ram_disk_size_mb = 10;
	const int sectors = ram_disk_size_mb * 2048; // 1 sector = 512 bytes
	
	// Create the RAM disk
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
	
	// Format the RAM disk as HFS+
	std::string format_ram_disk_cmd = "diskutil erasevolume HFS+ 'RAMDisk' " + std::string(ram_disk_path);
	int ret = system(format_ram_disk_cmd.c_str());
	if (ret != 0) {
		std::cerr << "Failed to format RAM disk." << std::endl;
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
		system(detach_cmd.c_str());
		return;
	}
	
	// Step 2: Write the shared object to the RAM disk
	// Generate a unique temporary file name
	char temp_filename_template[] = "/Volumes/RAMDisk/temp_module_XXXXXX.dylib";
	char temp_filename[sizeof(temp_filename_template)];
	strcpy(temp_filename, temp_filename_template);
	
	int fd = mkstemps(temp_filename, 6); // 6 for ".dylib"
	if (fd == -1) {
		std::cerr << "Failed to create temporary dylib file on RAM disk: " << strerror(errno) << std::endl;
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
		system(detach_cmd.c_str());
		return;
	}
	
	// Write the .dylib data to the temporary file
	ssize_t written = write(fd, data.data() + offset, so_size);
	if (written != static_cast<ssize_t>(so_size)) {
		std::cerr << "Failed to write all data to temporary dylib: " << strerror(errno) << std::endl;
		close(fd);
		std::remove(temp_filename);
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
		system(detach_cmd.c_str());
		return;
	}
	
	// Close the file descriptor
	close(fd);
	
	// Step 3: Load the shared object
	void* handle = dlopen(temp_filename, RTLD_NOW);
	if (!handle) {
		std::cerr << "dlopen failed: " << dlerror() << std::endl;
		std::remove(temp_filename);
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
		system(detach_cmd.c_str());
		return;
	}
	
	// Clear any existing errors
	dlerror();
	
	// Get the say_hello function
	typedef ILoadedCartridge* (*GetLoadedCartridgeFunc)(ICartridge& cartridge);
	GetLoadedCartridgeFunc load_cartridge = (GetLoadedCartridgeFunc)dlsym(handle, "load_cartridge");
	
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		std::cerr << "Failed to find say_hello function: " << dlsym_error << std::endl;
		dlclose(handle);
		std::remove(temp_filename);
		// Detach the RAM disk
		std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
		system(detach_cmd.c_str());
		return;
	}
	
	// Call the say_hello function
	try {
		mLoadedCartridge = std::unique_ptr<ILoadedCartridge>(load_cartridge(mCartridge));
		
		mOnCartridgeInsertedCallback(*mLoadedCartridge);
		
	} catch (...) {
		std::cerr << "Exception occurred while executing say_hello." << std::endl;
	}
	
	// Step 4: Cleanup
	dlclose(handle);
	std::remove(temp_filename);
	
	// Detach the RAM disk
	std::string detach_cmd = "hdiutil detach " + std::string(ram_disk_path);
	system(detach_cmd.c_str());
#else
	std::cerr << "In-memory shared object loading is not supported on this platform." << std::endl;
#endif // __APPLE__
}

#else
// Unsupported platform
void CartridgeBridge::execute_shared_object(const std::vector<uint8_t>& data) {
	std::cerr << "Shared object execution not supported on this platform." << std::endl;
}
#endif

// Optionally remove or comment out the raw memory execution methods if they are no longer needed
/*
 void DebugBridgeServer::execute_raw_memory(const std::vector<uint8_t>& data) {
 // Original raw memory execution code...
 }
 */
