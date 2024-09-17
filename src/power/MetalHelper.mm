#include "MetalHelper.hpp"

#include <QuartzCore/CAMetalLayer.h>
#include <AppKit/AppKit.h>
#include <Metal/Metal.h>
#include <vector>
#include <algorithm>

#include <nanogui/metal.h>

static MTLStencilOperation toMTLStencilOperation(StencilOperation op) {
	switch (op) {
		case StencilOperation::Keep: return MTLStencilOperationKeep;
		case StencilOperation::Zero: return MTLStencilOperationZero;
		case StencilOperation::Replace: return MTLStencilOperationReplace;
		case StencilOperation::IncrementClamp: return MTLStencilOperationIncrementClamp;
		case StencilOperation::DecrementClamp: return MTLStencilOperationDecrementClamp;
		case StencilOperation::Invert: return MTLStencilOperationInvert;
		case StencilOperation::IncrementWrap: return MTLStencilOperationIncrementWrap;
		case StencilOperation::DecrementWrap: return MTLStencilOperationDecrementWrap;
		default: return MTLStencilOperationKeep;
	}
}

static MTLCompareFunction toMTLCompareFunction(CompareFunction func) {
	switch (func) {
		case CompareFunction::Never: return MTLCompareFunctionNever;
		case CompareFunction::Less: return MTLCompareFunctionLess;
		case CompareFunction::Equal: return MTLCompareFunctionEqual;
		case CompareFunction::LessEqual: return MTLCompareFunctionLessEqual;
		case CompareFunction::Greater: return MTLCompareFunctionGreater;
		case CompareFunction::NotEqual: return MTLCompareFunctionNotEqual;
		case CompareFunction::GreaterEqual: return MTLCompareFunctionGreaterEqual;
		case CompareFunction::Always: return MTLCompareFunctionAlways;
		default: return MTLCompareFunctionAlways;
	}
}

void MetalHelper::readPixelsFromMetal(void *nswin, void *texture, int x, int y, int width, int height, std::vector<int>& pixels) {
	// Static caches for Metal objects to avoid repeated retrievals
	static CAMetalLayer* metalLayer = nullptr;
	static id<MTLDevice> device = nullptr;
	static id<MTLCommandQueue> commandQueue = nullptr;
	static id<MTLBuffer> buffer = nullptr;
	
	// Initialize and cache CAMetalLayer
	if (!metalLayer) {
		metalLayer = static_cast<CAMetalLayer*>(nanogui::metal_window_layer(nswin));
		if (!metalLayer) {
			NSLog(@"Failed to get CAMetalLayer.");
			return;
		}
	}
	
	// Initialize and cache MTLDevice
	if (!device) {
		device = (__bridge id<MTLDevice>)(nanogui::metal_device());
		if (!device) {
			NSLog(@"Failed to get MTLDevice.");
			return;
		}
	}
	
	// Initialize and cache MTLCommandQueue
	if (!commandQueue) {
		commandQueue = (__bridge id<MTLCommandQueue>)(nanogui::metal_command_queue());
		if (!commandQueue) {
			NSLog(@"Failed to get MTLCommandQueue.");
			return;
		}
	}
	
	// Convert the texture pointer to an MTLTexture
	id<MTLTexture> bufferTexture = (__bridge id<MTLTexture>)texture;
	if (!bufferTexture) {
		NSLog(@"Failed to get backbuffer texture.");
		return;
	}
	
	// Calculate bytes per pixel and buffer size
	constexpr size_t bytesPerPixel = sizeof(int32_t); // 4 bytes per pixel for R integer 32
	const size_t bytesPerRow = width * bytesPerPixel;
	const size_t bufferSize = bytesPerRow * height;
	
	// Ensure the pixels vector has enough space
	if (pixels.size() * sizeof(int) < bufferSize) {
		pixels.resize((bufferSize + sizeof(int) - 1) / sizeof(int)); // Round up to accommodate all bytes
	}
	
	// Allocate or reuse the buffer
	if (!buffer || buffer.length < bufferSize) {
		buffer = [device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
		if (!buffer) {
			NSLog(@"Failed to create buffer.");
			return;
		}
	}
	
	// Create a command buffer
	id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
	if (!commandBuffer) {
		NSLog(@"Failed to create command buffer.");
		return;
	}
	
	// Create a blit command encoder
	id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
	if (!blitEncoder) {
		NSLog(@"Failed to create blit encoder.");
		return;
	}
	
	// Perform the copy from the texture to the buffer
	[blitEncoder copyFromTexture:bufferTexture
					 sourceSlice:0
					 sourceLevel:0
					sourceOrigin:MTLOriginMake(x, y, 0) // (x, y, 0) as the origin
					  sourceSize:MTLSizeMake(width, height, 1)
						toBuffer:buffer
			   destinationOffset:0
		  destinationBytesPerRow:bytesPerRow
		destinationBytesPerImage:bufferSize];
	
	// Finalize encoding and commit the command buffer
	[blitEncoder endEncoding];
	[commandBuffer commit];
	[commandBuffer waitUntilCompleted]; // Consider asynchronous handling if possible
	
	// Efficiently copy the data from the buffer to the pixels vector
	std::memcpy(pixels.data(), buffer.contents, bufferSize);
}


void MetalHelper::setDepthClear(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
		passDescriptor.depthAttachment.clearDepth = 1.0;
		passDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
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

void MetalHelper::enableDepth(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
		passDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
	}
}

void MetalHelper::enableStencil(void* render_pass) {
	MTLRenderPassDescriptor* passDescriptor = static_cast<MTLRenderPassDescriptor*>(render_pass);
	if (passDescriptor) {
		passDescriptor.stencilAttachment.loadAction = MTLLoadActionLoad;
		passDescriptor.stencilAttachment.storeAction = MTLStoreActionStore;
	}
}

void* MetalHelper::createDepthStencilState(void* devicePtr, CompareFunction compareFunc,
										   StencilOperation stencilFailOp, StencilOperation depthFailOp,
										   StencilOperation passOp, uint32_t writeMask, uint32_t readMask) {
	id<MTLDevice> device = (__bridge id<MTLDevice>)devicePtr;
	
	MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
	
	// Configure stencil operations for the front face
	MTLStencilDescriptor *stencilFront = [[MTLStencilDescriptor alloc] init];
	stencilFront.stencilCompareFunction = toMTLCompareFunction(compareFunc);
	stencilFront.stencilFailureOperation = toMTLStencilOperation(stencilFailOp);
	stencilFront.depthFailureOperation = toMTLStencilOperation(depthFailOp);
	stencilFront.depthStencilPassOperation = toMTLStencilOperation(passOp);
	stencilFront.writeMask = writeMask;
	stencilFront.readMask = readMask;
	depthStencilDesc.frontFaceStencil = stencilFront;
	
	// Optionally, configure back face (same as front here)
	depthStencilDesc.backFaceStencil = stencilFront;
	
	// Create the depth stencil state
	id<MTLDepthStencilState> depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDesc];
	
	return (__bridge_retained void*)depthStencilState;
}

void MetalHelper::setDepthStencilState(void* commandEncoderPtr, void* depthStencilStatePtr) {
	id<MTLCommandEncoder> commandEncoder = (__bridge id<MTLCommandEncoder>)commandEncoderPtr;
	id<MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)depthStencilStatePtr;
	
	[commandEncoder setDepthStencilState:depthStencilState];
}
