// SimulationServer.hpp
#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <queue>
#include <functional>
#include <string>
#include <thread>  // For std::thread
#include <vector>
#include <atomic>  // For std::atomic<bool>
#include "DebugBridgeCommon.hpp"
#include "simulation/Cartridge.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

class IActorManager;
class ICameraManager;
class ICartridge;

class CartridgeActorLoader;

class VirtualMachine;

class SimulationServer {
public:
	SimulationServer(uint16_t port,
					VirtualMachine& virtualMachine,
					CartridgeActorLoader& actorLoader,
					std::function<void(std::optional<std::reference_wrapper<VirtualMachine>>)> onCartridgeInsertedCallback);
	
	/**
	 * @brief Destructor. Ensures the server is stopped and resources are cleaned up.
	 */
	~SimulationServer();
	
	/**
	 * @brief Starts the server and initializes memory mappings.
	 */
	void run();
	
	/**
	 * @brief Stops the server and cleans up memory mappings.
	 */
	void stop();
	
	void eject();
	
private:
	// WebSocket++ server instance
	typedef websocketpp::server<websocketpp::config::asio> server;
	server m_server;
	
	// Server configuration
	uint16_t m_port;                            ///< Port number for the server to listen on.
	std::mutex m_mutex;                         ///< Mutex for thread-safe operations.
	std::queue<DebugCommand> m_command_queue;   ///< Queue to store incoming DebugCommands.
	bool validate_connection(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
	DebugCommand process_command(const DebugCommand& cmd);
	
	void execute_elf(const std::vector<uint8_t>& data);
	
private:
	// Callback function invoked when a cartridge is inserted or ejected
	std::function<void(std::optional<std::reference_wrapper<VirtualMachine>>)>  mOnVirtualMachineLoadedCallback;
	
	// Pointer to the currently loaded cartridge
	VirtualMachine& mVirtualMachine;
		
	// Reference to the CartridgeActorLoader
	CartridgeActorLoader& mActorLoader;
	
	// Thread to run the WebSocket server
	std::thread m_thread;
	std::thread m_virtual_machine_thread;

	// Flag to indicate if the server is running
	std::atomic<bool> m_running;
	
	// Prevent copying
	SimulationServer(const SimulationServer&) = delete;
	SimulationServer& operator=(const SimulationServer&) = delete;
};
