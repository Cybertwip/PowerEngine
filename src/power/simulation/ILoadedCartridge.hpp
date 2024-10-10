#pragma once

class ILoadedCartridge {
public:
	ILoadedCartridge() {
		
	}
	
	virtual ~ILoadedCartridge() = default;
	
	virtual void update() = 0;
};
