#pragma once

class ICameraManager;

class ICartridgeActorLoader;

class ICartridge {
public:
	ICartridge(ICartridgeActorLoader& actorLoader, ICameraManager& cameraManager) : mActorLoader(actorLoader), mCameraManager(cameraManager) {
		
	}
	
	virtual ~ICartridge() = default;
	
protected:
	ICartridgeActorLoader& mActorLoader;
	ICameraManager& mCameraManager;
};
