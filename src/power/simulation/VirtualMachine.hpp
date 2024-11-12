#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <any>
#include <stdexcept>
#include <thread>

// Include riscv machine implementation
#include <libriscv/machine.hpp>
#include <libriscv/rsp_server.hpp>

#define SYS_CLASS_FUNCTION_HOOK 386

// Structure matching FunctionCallData
struct __attribute__((packed)) FunctionCallData {
	uint64_t this_ptr;
	uint64_t func_offset;
	uint64_t arg_count;
	uint64_t args[10]; // Adjust size as needed
	uint64_t buffer_offset;
};

struct CartridgeHook {
	uint64_t cartridge_ptr;
	uint64_t updater_function_ptr;
};

// Compile-time FNV-1a hash function
constexpr uint64_t fnv1a_hash(const char* s, size_t count, uint64_t hash = 14695981039346656037ULL) {
	return count ? fnv1a_hash(s + 1, count - 1, (hash ^ static_cast<uint64_t>(s[0])) * 1099511628211ULL) : hash;
}

#define FUNCTION_HASH(str, size) fnv1a_hash(str, size)

// Function map to store function handlers
using FunctionHandler = std::function<void(uint64_t, const std::vector<std::any>&, std::vector<unsigned char>&)>;

extern std::unordered_map<uint64_t, FunctionHandler> function_map;


// Custom syscall handler
template <int W>
void setup_syscall_handler(riscv::Machine<W>& machine) {
	machine.install_syscall_handler(SYS_CLASS_FUNCTION_HOOK,
									[](riscv::Machine<W>& machine) {
		auto& mem = machine.memory;
		auto& regs = machine.cpu.registers();
		
		// Retrieve pointer to FunctionCallData
		uint64_t data_ptr = regs.get(riscv::REG_ARG0);
		
		// Read FunctionCallData from emulated memory
		FunctionCallData data;
		mem.memcpy_out(&data, data_ptr, sizeof(FunctionCallData));
		
		// Extract arguments into std::vector<std::any>
		std::vector<std::any> args;
		for (uint64_t i = 0; i < data.arg_count; ++i) {
			args.push_back(static_cast<uint64_t>(data.args[i]));
		}
		
		// Prepare output_buffer
		std::vector<unsigned char> output_buffer(32);
		mem.memcpy_out(output_buffer.data(), data.buffer_offset, output_buffer.size());
		
		// Find the function in the function_map
		auto it = function_map.find(data.func_offset);
		if (it != function_map.end()) {
			// Call the registered function
			it->second(data.this_ptr, args, output_buffer);
			
			// If output_buffer contains data, write it back to emulated memory
			mem.memcpy(data.buffer_offset, output_buffer.data(), output_buffer.size());
			
			// Set the result to 0 (success)
			machine.set_result(0);
		} else {
			throw std::runtime_error("Unknown function hash in syscall handler");
		}
	});
}

class VirtualMachine {
public:
	VirtualMachine();
	~VirtualMachine();
	
	void start(std::vector<uint8_t> executable_data, uint64_t loader_ptr);
	void gdb_listen(uint16_t port);
	void reset();
	void stop();
	void update();
	
	// Function to register callbacks
	void register_callback(uint64_t this_ptr, const std::string& function_name,
						   std::function<void(uint64_t, const std::vector<std::any>&, std::vector<unsigned char>&)> func);
	
private:
	std::unique_ptr<riscv::Machine<riscv::RISCV64>> mMachine;
	CartridgeHook mCartridgeHook;
	std::unique_ptr<riscv::RSPClient<riscv::RISCV64>> mDebugClient;
	std::mutex mMachineMutex;
	std::atomic<bool> mRunning;
};
