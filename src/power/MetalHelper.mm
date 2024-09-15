#include "MetalHelper.hpp"

#import <Metal/Metal.h>
#include <vector>
#include <algorithm>

void MetalHelper::readPixelsFromMetal(int x, int y, int width, int height, int* pixels) {
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	if (!device) {
		return; // Metal is not supported on this device
	}
	
	id<MTLCommandQueue> commandQueue = [device newCommandQueue];
	
	MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
																					width:width
																				   height:height
																				mipmapped:NO];
	
	id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
	
	if (!texture) {
		return;
	}
	
	MTLRegion region = MTLRegionMake2D(x, y, width, height);
	size_t bytesPerRow = sizeof(int) * width;
	std::vector<int> pixelBuffer(width * height);
	
	id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
	id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
	
	[blitEncoder copyFromTexture:texture
					 sourceSlice:0
					 sourceLevel:0
					sourceOrigin:MTLOriginMake(0, 0, 0)
					  sourceSize:MTLSizeMake(width, height, 1)
						 toBytes:pixelBuffer.data()
					 bytesPerRow:bytesPerRow
				   bytesPerImage:bytesPerRow * height
			   destinationOrigin:MTLOriginMake(0, 0, 0)];
	
	[blitEncoder endEncoding];
	
	[commandBuffer commit];
	[commandBuffer waitUntilCompleted];
	
	std::copy(pixelBuffer.begin(), pixelBuffer.end(), pixels);
}

void MetalHelper::setDepthClear(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
		passDescriptor.depthAttachment.clearDepth = 1.0;
	}
}

void MetalHelper::setStencilClear(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
		passDescriptor.stencilAttachment.clearStencil = 0;
	}
}

void MetalHelper::setDepthStore(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
	}
}

void MetalHelper::setStencilStore(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.stencilAttachment.storeAction = MTLStoreActionStore;
	}
}

void MetalHelper::disableDepth(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
		passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
	}
}

void MetalHelper::disableStencil(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.stencilAttachment.loadAction = MTLLoadActionDontCare;
		passDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
	}
}

// New function: Enable depth testing
void MetalHelper::enableDepth(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		// Enable depth testing by ensuring the depth attachment will load and store data
		passDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;  // Load existing depth buffer content
		passDescriptor.depthAttachment.storeAction = MTLStoreActionStore;  // Store depth buffer content after rendering
	}
}
