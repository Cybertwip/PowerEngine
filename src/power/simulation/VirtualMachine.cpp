#include "VirtualMachine.hpp"
#include <cstring> // for memcpy
#include <stdexcept>

static constexpr uint64_t MAX_CALL_INSTR = 32'000'000ull;

// Definition of function_map
std::unordered_map<uint64_t, FunctionHandler> function_map;

VirtualMachine::VirtualMachine() : mMachine(nullptr) {
}

VirtualMachine::~VirtualMachine() {
	stop();
	
	function_map.clear();
}

void VirtualMachine::start(std::vector<uint8_t> executable_data) {
	stop();
	
	mMachine = std::make_unique<riscv::Machine<riscv::RISCV64>>(executable_data);
		
	// Set up Linux environment
	mMachine->setup_linux({});
	
	// Add basic Linux system calls
	mMachine->setup_linux_syscalls();
	
	// Set up the custom syscall handler
	setup_syscall_handler(*mMachine);

	// start debugging session
	printf("GDB server is listening on localhost:%u\n", 3333);
	mDebugServer = std::make_unique<riscv::RSP<riscv::RISCV64>>(*mMachine, 3333);
	
	gdb_poll();
}

void VirtualMachine::gdb_poll()
{
	// Accept new client if any
	auto client = mDebugServer->accept(0); // 0 timeout for non-blocking
	if (client) {
		printf("GDB connected\n");
		mDebugClient = std::move(client);
	}
	
	// Process existing client
	if (mDebugClient && !mDebugClient->is_closed()) {
		while (mDebugClient->process_one()) {
			// Continue processing available data
		}
	}
}



void VirtualMachine::reset() {
	if (mMachine) {
		mMachine->reset();
	}
}

void VirtualMachine::stop() {
	if (mMachine) {
		mMachine->stop();
	}
}

void VirtualMachine::update() {
	if (mMachine) {
		mMachine->vmcall(mCartridgeHook.updater_function_ptr, mCartridgeHook.cartridge_ptr);
	}
	
	// Poll GDB in each update cycle
	gdb_poll();

}

void VirtualMachine::register_callback(uint64_t this_ptr, const std::string& function_name,
									   std::function<void(uint64_t, const std::vector<std::any>&, std::vector<unsigned char>&)> func) {

	uint64_t hash = FUNCTION_HASH(function_name.c_str(), function_name.size());
	function_map[hash] = [this_ptr, func, function_name](uint64_t ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
		if (ptr != this_ptr) {
			throw std::runtime_error("Invalid 'this' pointer in function call: " + function_name);
		}
		func(ptr, args, output_buffer);
	};
}

