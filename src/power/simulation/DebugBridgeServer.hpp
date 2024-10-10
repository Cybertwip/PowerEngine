// DebugBridgeServer.hpp
#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <queue>
#include <functional>
#include <mutex>   // For std::thread
#include <thread>   // For std::thread
#include <atomic>   // For std::atomic<bool>
#include "DebugBridgeCommon.hpp"
#include "simulation/Cartridge.hpp"


#ifdef __linux__
#include <unistd.h>  // For close()
#elif defined(__APPLE__)
#include <string>
#endif

typedef websocketpp::server<websocketpp::config::asio> server;

class IActorManager;
class ICameraManager;
class ICartridge;
class ILoadedCartridge;

class CartridgeActorLoader;

class CartridgeBridge {
public:
	/**
	 * @brief Constructs a CartridgeBridge instance.
	 *
	 * @param port The port number on which the server will listen.
	 * @param cartridge Reference to the ICartridge implementation.
	 * @param onCartridgeInsertedCallback Callback invoked when a cartridge is inserted.
	 */
	CartridgeBridge(uint16_t port,
					ICartridge& cartridge,
					CartridgeActorLoader& actorLoader,
					std::function<void(ILoadedCartridge&)> onCartridgeInsertedCallback);
	
	/**
	 * @brief Destructor. Ensures the server is stopped and resources are cleaned up.
	 */
	~CartridgeBridge();
	
	/**
	 * @brief Starts the server and initializes the disk.
	 */
	void run();
	
	/**
	 * @brief Stops the server and cleans up the disk.
	 */
	void stop();
	
private:
	// WebSocket++ server instance
	typedef websocketpp::server<websocketpp::config::asio> server;
	server m_server;
	
	// Server configuration
	uint16_t m_port;
	std::mutex m_mutex;
	std::queue<DebugCommand> m_command_queue;
	
	// Callback for handling incoming messages
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
	
	// Processes a received DebugCommand and generates a response
	DebugCommand process_command(const DebugCommand& cmd);
	
	// Executes a shared object (DLL or .so/.dylib)
	void execute_shared_object(const std::vector<uint8_t>& data);
	
	// Initializes the disk when the server starts
	void initialize_disk();
	
	// Erases/ejects the current disk
	void erase_disk();
	
private:
	// Callback function invoked when a cartridge is inserted
	std::function<void(ILoadedCartridge&)>  mOnCartridgeInsertedCallback;
	
	// Pointer to the currently loaded cartridge
	std::unique_ptr<ILoadedCartridge> mLoadedCartridge;
	
	// Reference to the cartridge interface
	ICartridge& mCartridge;
	
	CartridgeActorLoader& mActorLoader;
	
	// Thread to run the server
	std::thread m_thread;
	
	// Flag to indicate if the server is running
	std::atomic<bool> m_running;
	
	// Disk management member variables
#ifdef __linux__
	int m_mem_fd;  // File descriptor for memory file (memfd_create)
#elif defined(__APPLE__)
	std::string m_ram_disk_path;  // Path to the RAM disk on macOS
#endif
	
	
#ifdef _WIN32
	HMODULE mSharedObjectHandle; // Handle for Windows DLL
#else
	void* mSharedObjectHandle;   // Handle for Unix-like shared objects
#endif

	
	// Prevent copying
	CartridgeBridge(const CartridgeBridge&) = delete;
	CartridgeBridge& operator=(const CartridgeBridge&) = delete;
};


