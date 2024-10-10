#pragma once

class IActorManager;
class ICameraManager;

class ICartridge {
public:
	ICartridge(IActorManager& actorManager, ICameraManager& cameraManager) : mActorManager(actorManager), mCameraManager(cameraManager) {
		
	}
	
	virtual ~ICartridge() = default;
	
protected:
	IActorManager& mActorManager;
	ICameraManager& mCameraManager;
};
