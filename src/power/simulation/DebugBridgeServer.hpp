// DebugBridgeServer.hpp
#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <queue>
#include <functional>
#include <thread>  // For std::thread
#include <atomic>  // For std::atomic<bool>
#include "DebugBridgeCommon.hpp"
#include "simulation/Cartridge.hpp"

#ifdef __linux__
#include <unistd.h>  // For close()
#elif defined(__APPLE__)
#include <string>
#include <vector>
#include <sys/mman.h> // For mmap on macOS
#include <sys/stat.h> // For shm_open on macOS
#include <fcntl.h>    // For shm_open on macOS
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
	 * @param actorLoader Reference to the CartridgeActorLoader.
	 * @param onCartridgeInsertedCallback Callback invoked when a cartridge is inserted.
	 */
	CartridgeBridge(uint16_t port,
					ICartridge& cartridge,
					CartridgeActorLoader& actorLoader,
					std::function<void(std::optional<std::reference_wrapper<ILoadedCartridge>>)> onCartridgeInsertedCallback);
	
	/**
	 * @brief Destructor. Ensures the server is stopped and resources are cleaned up.
	 */
	~CartridgeBridge();
	
	/**
	 * @brief Starts the server and initializes memory mappings.
	 */
	void run();
	
	/**
	 * @brief Stops the server and cleans up memory mappings.
	 */
	void stop();
	
private:
	// WebSocket++ server instance
	typedef websocketpp::server<websocketpp::config::asio> server;
	server m_server;
	
	// Server configuration
	uint16_t m_port;                            ///< Port number for the server to listen on.
	std::mutex m_mutex;                         ///< Mutex for thread-safe operations.
	std::queue<DebugCommand> m_command_queue;   ///< Queue to store incoming DebugCommands.
	
	/**
	 * @brief Callback for handling incoming WebSocket messages.
	 *
	 * @param hdl The connection handle.
	 * @param msg Pointer to the received message.
	 */
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
	
	/**
	 * @brief Processes a received DebugCommand and generates a response.
	 *
	 * @param cmd The received DebugCommand.
	 * @return The response DebugCommand.
	 */
	DebugCommand process_command(const DebugCommand& cmd);
	
	/**
	 * @brief Executes a shared object (DLL or .so/.dylib) from the received data.
	 *
	 * @param data The binary data containing the shared object.
	 */
	void execute_shared_object(const std::vector<uint8_t>& data);
	
	/**
	 * @brief Initializes memory mappings when the server starts.
	 */
	void initialize_memory();
	
	/**
	 * @brief Erases/ejects the current memory mappings.
	 *
	 * @param force If true, forces the erasure even if not necessary.
	 */
	void erase_memory(bool force = false);
	
#ifdef __APPLE__
	/**
	 * @brief Loads a shared library from a memory buffer.
	 *
	 * @param data Pointer to the shared library data in memory.
	 * @param size Size of the shared library data.
	 * @return Handle to the loaded shared library, or nullptr on failure.
	 */
	void* fdlopen(const void* data, size_t size);
#endif
	
private:
	// Callback function invoked when a cartridge is inserted or ejected
	std::function<void(std::optional<std::reference_wrapper<ILoadedCartridge>>)>  mOnCartridgeInsertedCallback;
	
	// Pointer to the currently loaded cartridge
	std::unique_ptr<ILoadedCartridge> mLoadedCartridge;
	
	// Reference to the cartridge interface
	ICartridge& mCartridge;
	
	// Reference to the CartridgeActorLoader
	CartridgeActorLoader& mActorLoader;
	
	// Thread to run the WebSocket server
	std::thread m_thread;
	
	// Flag to indicate if the server is running
	std::atomic<bool> m_running;
	
	// Memory management member variables
#ifdef __linux__
	int m_mem_fd;  ///< File descriptor for memory file (memfd_create) on Linux.
#elif defined(__APPLE__)
	int m_mem_fd;                ///< File descriptor for shared memory on macOS.
	void* m_mapped_memory;       ///< Pointer to the mapped shared memory region on macOS.
	std::string m_temp_so_path;  ///< Path to the temporary shared object file on macOS.
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
