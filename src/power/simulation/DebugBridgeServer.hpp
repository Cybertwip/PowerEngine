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

typedef websocketpp::server<websocketpp::config::asio> server;

class IActorManager;
class ICameraManager;
class ICartridge;
class ILoadedCartridge;
class ICartridgeActorLoader;

class CartridgeBridge {
public:
	CartridgeBridge(uint16_t port,
					ICartridge& cartridge,
					std::function<void(ILoadedCartridge&)> onCartridgeInsertedCallback);
	
	~CartridgeBridge();
	void run();
	void stop();
	
private:
	server m_server;
	uint16_t m_port;
	std::mutex m_mutex;
	std::queue<DebugCommand> m_command_queue;
	
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
	DebugCommand process_command(const DebugCommand& cmd);
	
	void execute_shared_object(const std::vector<uint8_t>& data);
	
private:
	std::function<void(ILoadedCartridge&)>  mOnCartridgeInsertedCallback;
	
	std::unique_ptr<ILoadedCartridge> mLoadedCartridge;
	
	ICartridge& mCartridge;
	
	std::thread m_thread;                // Thread to run the server
	std::atomic<bool> m_running;         // Flag to indicate if the server is running

};

