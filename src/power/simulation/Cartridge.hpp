#pragma once

#include "simulation/ICartridge.hpp"

class Cartridge : public ICartridge {
public:
	Cartridge(ICartridgeActorLoader& actorLoader, ICameraManager& cameraManager) : mActorLoader(actorLoader), mCameraManager(cameraManager) {
		
	}
	
	~Cartridge() = default;
	
	ICartridgeActorLoader& GetActorLoader() override {
		return mActorLoader;
	}
	
	ICameraManager& GetCameraManager() override {
		return mCameraManager;
	}

private:
	ICartridgeActorLoader& mActorLoader;
	ICameraManager& mCameraManager;
};
