#include "VirtualMachine.hpp"
#include <cstring> // for memcpy
#include <stdexcept>


#include <libriscv/rsp_server.hpp>

template <int W>
void gdb_listen(uint16_t port, Machine<W>& machine)
{
	printf("GDB server is listening on localhost:%u\n", port);
	riscv::RSP<W> server { machine, port };
	auto client = server.accept();
	if (client != nullptr) {
		printf("GDB connected\n");
		while (client->process_one());
	}
}

static constexpr uint64_t MAX_CALL_INSTR = 32'000'000ull;

// Definition of function_map
std::unordered_map<uint64_t, FunctionHandler> function_map;

VirtualMachine::VirtualMachine() : mMachine(nullptr) {
}

VirtualMachine::~VirtualMachine() {
	stop();
	function_map.clear();
}

void VirtualMachine::start(std::vector<uint8_t> executable_data, uint64_t loader_ptr) {
	
	stop();
	
	mMachine = std::make_unique<riscv::Machine<riscv::RISCV64>>(executable_data);
		
	// Set up Linux environment
	mMachine->setup_linux({});
	
	// Add basic Linux system calls
	mMachine->setup_linux_syscalls();
	
	// Set up the custom syscall handler
	setup_syscall_handler(*mMachine);

	uint64_t cartridgePtr = mMachine->preempt(MAX_CALL_INSTR, "load_cartridge", loader_ptr);
	
	mMachine->memory.memcpy_out(&mCartridgeHook, cartridgePtr, sizeof(CartridgeHook));
	
	mSimulationThread = std::thread([this]() {
		mMachine->simulate();
	});
	
	// start debugging session
	mDebugThread = std::thread([this]() {
		gdb_listen(2159, *mMachine);
	});

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

