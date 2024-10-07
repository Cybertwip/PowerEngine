#pragma once


#include <nanogui/vector.h>

#include <memory>
#include <functional>


namespace nanogui {
class Texture;
}

class Mesh;

class Batch {
	
public:
	virtual ~Batch() = default;
	
	virtual void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) = 0;
	
	static void init_dummy_texture();
	
	static std::shared_ptr<nanogui::Texture> get_dummy_texture() {
		return mDummyTexture;
	}
	
private:
	static std::shared_ptr<nanogui::Texture> mDummyTexture;
};
