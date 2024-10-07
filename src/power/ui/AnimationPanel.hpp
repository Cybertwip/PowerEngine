#pragma once

#include "Panel.hpp"

namespace nanogui {
class Canvas;
class ToolButton;
class Widget;
}

class Actor;
class SelfContainedMeshCanvas;
class TransformComponent;

class AnimationPanel : public Panel {
public:
	AnimationPanel(nanogui::Widget& parent, nanogui::Screen& screen);
	~AnimationPanel();

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
	void parse_file(const std::string& path);
	
	void update_with(int currentTime) {
		mCurrentTime = currentTime;
	}
    
private:
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	std::shared_ptr<nanogui::ToolButton> mReversePlayButton; // New reverse play button
	std::shared_ptr<nanogui::ToolButton> mPlayPauseButton;

	std::shared_ptr<nanogui::Widget> mCanvasPanel;

	std::shared_ptr<nanogui::Widget> mPlaybackPanel;

	std::shared_ptr<SelfContainedMeshCanvas> mPreviewCanvas;
	
	int mCurrentTime;

};
