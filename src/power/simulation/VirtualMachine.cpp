#include "VirtualMachine.hpp"
#include <cstring> // for memcpy
#include <stdexcept>

// Definition of function_map
std::unordered_map<uint64_t, FunctionHandler> function_map;

VirtualMachine::VirtualMachine() {
}

VirtualMachine::~VirtualMachine() {
	stop();
	function_map.clear();
}

void VirtualMachine::start(const std::vector<uint8_t>& executable_data, uint64_t loader_ptr) {
	mMachine = std::make_unique<riscv::Machine<riscv::RISCV64>>(executable_data);
		
	// Set up Linux environment
	mMachine->setup_linux({});
	
	// Add basic Linux system calls
	mMachine->setup_linux_syscalls();
	
	// Set up the custom syscall handler
	setup_syscall_handler(*mMachine);

	uint64_t cartridgePtr = mMachine->vmcall("load_cartridge", loader_ptr);
	
	mMachine->memory.memcpy_out(&mCartridgeHook, cartridgePtr, sizeof(CartridgeHook));
}

void VirtualMachine::reset() {
	mMachine->reset();
}

void VirtualMachine::stop() {
	mMachine->stop();
}

void VirtualMachine::update() {
	mMachine->vmcall(mCartridgeHook.updater_function_ptr, mCartridgeHook.cartridge_ptr);
}

void VirtualMachine::register_callback(uint64_t this_ptr, const std::string& function_name,
									   std::function<void(uint64_t, const std::vector<std::any>&, std::vector<unsigned char>&)> func) {
	uint64_t hash = FUNCTION_HASH(function_name.c_str(), function_name.size());
	function_map[hash] = [this_ptr, func](uint64_t ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
		if (ptr != this_ptr) {
			throw std::runtime_error("Invalid 'this' pointer in function call");
		}
		func(ptr, args, output_buffer);
	};
}

