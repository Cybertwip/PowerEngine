// DebugBridgeServer.cpp

#include "DebugBridgeServer.hpp"

#include "simulation/Cartridge.hpp"
#include "simulation/CartridgeActorLoader.hpp"
#include "simulation/VirtualMachine.hpp"

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

// Define INITIAL_MEMORY_SIZE for elf memory on macOS
#define INITIAL_MEMORY_SIZE (10 * 1024 * 1024) // 10 MB, adjust as needed

CartridgeBridge::CartridgeBridge(uint16_t port, VirtualMachine& virtualMachine, CartridgeActorLoader& actorLoader, std::function<void(std::optional<std::reference_wrapper<VirtualMachine>>)> onCartridgeInsertedCallback)
: m_port(port), mVirtualMachine(virtualMachine), mActorLoader(actorLoader), mOnVirtualMachineLoadedCallback(onCartridgeInsertedCallback)
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
		
		mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
		mVirtualMachine.reset();

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
			execute_elf(data);
			std::string ack = "Elf object executed successfully.";
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

void CartridgeBridge::execute_elf(const std::vector<uint8_t>& data) {
	// Existing elf unloading logic
	// If a cartridge was loaded, reset it
	mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
	mActorLoader.cleanup();

	// Prepare data by removing the 'SOLO' magic number if present
	size_t offset = 0;
	if (data.size() >= 4 && data[0] == 'S' && data[1] == 'O' && data[2] == 'L' && data[3] == 'O') {
		offset = 4;
	}
	
	size_t elf_size = data.size() - offset;
	if (elf_size == 0) {
		std::cerr << "No elf data to execute." << std::endl;
		return;
	}
	
	if (!data.empty()) {
		
		std::vector<uint8_t> elf_data;
		
		elf_data.assign(data.begin() + offset, data.end());
		
		// Call the load_cartridge function in the main thread
		nanogui::async([this, binary_data = std::move(elf_data)](){
			try {
				mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
				mVirtualMachine.start(std::move(binary_data));

				// one frame to warmup the virtual machine thread
				mOnVirtualMachineLoadedCallback(mVirtualMachine);

			}
			catch (const riscv::MachineTimeoutException& ex)
			{
				std::cerr << "Exception occurred while executing load_cartridge >> " << ex.what() << std::endl;
				mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
				mVirtualMachine.reset();
			}
			catch (const riscv::MachineException& ex)
			{
				std::cerr << "Exception occurred while executing load_cartridge >> " << ex.what() << std::endl;
				mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
				mVirtualMachine.reset();
			}
			catch (std::exception& ex) {
				std::cerr << "Exception occurred while executing load_cartridge >> " << ex.what() << std::endl;
				mOnVirtualMachineLoadedCallback(std::nullopt); // Eject cartridge to prevent updating
				mVirtualMachine.reset();
			}
		});
	} else {
		std::cerr << "Invalid ELF." << std::endl;
		return;
	}
	
}

