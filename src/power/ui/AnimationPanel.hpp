#pragma once

#include "Panel.hpp"

#include "animation/AnimationTimeProvider.hpp"

namespace nanogui {
class Canvas;
class ToolButton;
class Widget;
class Screen;
}

class Actor;
class SelfContainedMeshCanvas;
class TransformComponent;

class AnimationPanel : public Panel {
public:
	AnimationPanel(nanogui::Widget& parent, nanogui::Screen& screen, AnimationTimeProvider& previewTimeProvider);
	~AnimationPanel();

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
	void parse_file(const std::string& path);
	
	void update_with(int currentTime) {
		mPreviewTimeProvider.SetTime(currentTime);
	}
    
private:
	AnimationTimeProvider mPreviewTimeProvider;
	
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	std::shared_ptr<nanogui::ToolButton> mReversePlayButton; // New reverse play button
	std::shared_ptr<nanogui::ToolButton> mPlayPauseButton;

	std::shared_ptr<nanogui::Widget> mCanvasPanel;

	std::shared_ptr<nanogui::Widget> mPlaybackPanel;

	std::shared_ptr<SelfContainedMeshCanvas> mPreviewCanvas;
};
