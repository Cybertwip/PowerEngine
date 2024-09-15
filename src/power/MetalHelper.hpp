#pragma once

class MetalHelper {
public:
	static void readPixelsFromMetal(int x, int y, int width, int height, int* pixels);
	
	// Setter functions for depth and stencil
	static void setDepthClear(void* render_pass);
	static void setStencilClear(void* render_pass);
	static void setDepthStore(void* render_pass);
	static void setStencilStore(void* render_pass);
	static void disableDepth(void* render_pass);
	static void disableStencil(void* render_pass);

	static void enableDepth(void* render_pass);
};
