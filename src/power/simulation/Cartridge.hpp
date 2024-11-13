#pragma once

#include "simulation/ICartridge.hpp"
#include "simulation/VirtualMachine.hpp"

class Cartridge : public ICartridge {
public:
	Cartridge(VirtualMachine& virtualMachine, ICartridgeActorLoader& actorLoader, ICameraManager& cameraManager) : mVirtualMachine(virtualMachine), mActorLoader(actorLoader), mCameraManager(cameraManager) {
		
		// register here
		mVirtualMachine.register_callback(0, "GetNuclei",
										  [this](uint64_t this_ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
			if (!args.empty()) {
				throw std::runtime_error("Argument count mismatch for GetActorLoader");
			}
			auto& instance = *reinterpret_cast<Cartridge*>(this_ptr);
			uint64_t result = reinterpret_cast<uint64_t>(this);
			
			// Copy result into output_buffer
			unsigned char* data_ptr = reinterpret_cast<unsigned char*>(&result);
			std::memcpy(output_buffer.data(), data_ptr, sizeof(uint64_t));
		});
		

		mVirtualMachine.register_callback(reinterpret_cast<uint64_t>(this), "Cartridge::GetActorLoader",
										  [](uint64_t this_ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
			if (!args.empty()) {
				throw std::runtime_error("Argument count mismatch for GetActorLoader");
			}
			auto& instance = *reinterpret_cast<Cartridge*>(this_ptr);
			uint64_t result = reinterpret_cast<uint64_t>(instance.GetActorLoader());
			
			// Copy result into output_buffer
			unsigned char* data_ptr = reinterpret_cast<unsigned char*>(&result);
			std::memcpy(output_buffer.data(), data_ptr, sizeof(uint64_t));
		});
		
	}
	
	~Cartridge() = default;
	
	ICartridgeActorLoader* GetActorLoader() override {
		return &mActorLoader;
	}
	
//	ICameraManager& GetCameraManager() override {
//		return mCameraManager;
//	}

private:
	VirtualMachine& mVirtualMachine;
	ICartridgeActorLoader& mActorLoader;
	ICameraManager& mCameraManager;
};
