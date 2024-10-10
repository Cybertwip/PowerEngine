#pragma once

class ILoadedCartridge {
public:
	ILoadedCartridge() {
		
	}
	
	virtual ~ICartridge() = default;
	
	virtual void update() = 0;
};
