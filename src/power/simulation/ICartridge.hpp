#pragma once

class ICameraManager;

class ICartridgeActorLoader;

class ICartridge {
public:
	ICartridge() {
		
	}
	
	virtual ~ICartridge() = default;
	
	virtual ICartridgeActorLoader& GetActorLoader() = 0;
	virtual ICameraManager& GetCameraManager() = 0;
};
