#pragma once

#include <cstdint>
#include <vector>

// Define C++ equivalents for Metal stencil operations and compare functions
enum class StencilOperation {
	Keep = 0,
	Zero = 1,
	Replace = 2,
	IncrementClamp = 3,
	DecrementClamp = 4,
	Invert = 5,
	IncrementWrap = 6,
	DecrementWrap = 7
};

enum class CompareFunction {
	Never = 0,
	Less = 1,
	Equal = 2,
	LessEqual = 3,
	Greater = 4,
	NotEqual = 5,
	GreaterEqual = 6,
	Always = 7
};

class MetalHelper {
public:
	static void readPixelsFromMetal(void* nswin, void *texture, int x, int y, int width, int height, std::vector<int>& pixels);
	
	static void readPixelsFromMetal(void* nswin, void *texture, int x, int y, int width, int height, std::vector<uint8_t>& pixels);

	// Setter functions for depth and stencil
	static void setDepthClear(void* render_pass);
	static void setStencilClear(void* render_pass);
	static void setDepthStore(void* render_pass);
	static void setStencilStore(void* render_pass);
	static void disableDepth(void* render_pass);
	static void disableStencil(void* render_pass);
	
	static void enableDepth(void* render_pass);
	
	// Functions for stencil
	static void enableStencil(void* render_pass);
	static void setStencilFunction(void* render_pass, CompareFunction function, uint32_t referenceValue, uint32_t readMask);
	static void setStencilOperation(void* render_pass, StencilOperation stencilFailOp, StencilOperation depthFailOp, StencilOperation passOp);
	static void setStencilMask(void* render_pass, uint32_t writeMask);
	
	// Creates the depth stencil state and returns a pointer to it
	static void* createDepthStencilState(void* device, CompareFunction compareFunc,
										 StencilOperation stencilFailOp, StencilOperation depthFailOp,
										 StencilOperation passOp, uint32_t writeMask, uint32_t readMask);
	
	static void setDepthStencilState(void* commandEncoder, void* depthStencilState);

};
