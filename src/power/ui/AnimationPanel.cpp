#include "ui/AnimationPanel.hpp"

#include <nanogui/canvas.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/renderpass.h>
#include <nanogui/screen.h>
#include <nanogui/toolbutton.h>

#include "ShaderManager.hpp"

#include "actors/Actor.hpp"
#include "components/DrawableComponent.hpp"
#include "components/PlaybackComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/CameraComponent.hpp"

#include "filesystem/CompressedSerialization.hpp"

#include "graphics/drawing/Batch.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SelfContainedMeshCanvas.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "import/SkinnedFbx.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

AnimationPanel::AnimationPanel(nanogui::Widget& parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt), mCurrentTime(0) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 10, 10));
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::initialize() {
	Panel::initialize();
	
	
	auto playbackLayout = std::make_shared<nanogui::GridLayout>(nanogui::Orientation::Horizontal, 2,
																nanogui::Alignment::Middle, 0, 0);
	
	mPlaybackPanel = std::make_shared<Widget>(*this);
	
	
	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	
	mPlaybackPanel->set_layout(playbackLayout);
	
	mCanvasPanel = std::make_shared<Widget>(*this);
	mCanvasPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
	
	mReversePlayButton = std::make_shared<nanogui::ToolButton>(mPlaybackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");
	
	mReversePlayButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			
			playback.update_state(active ? PlaybackState::Play : PlaybackState::Pause, PlaybackModifier::Reverse, PlaybackTrigger::None, playback.getPlaybackData());
		}
	});
	
	mPlayPauseButton = std::make_shared<nanogui::ToolButton>(mPlaybackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
	
	mPlayPauseButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			
			playback.update_state(active ? PlaybackState::Play : PlaybackState::Pause, PlaybackModifier::Forward, PlaybackTrigger::None, playback.getPlaybackData());
		}
	});
	
	mPreviewCanvas = std::make_shared<SelfContainedMeshCanvas>(mCanvasPanel);
	
	set_active_actor(std::nullopt);
}

void AnimationPanel::parse_file(const std::string& path) {
	if (mActiveActor.has_value()) {
		
		CompressedSerialization::Deserializer deserializer;
		
		// Load the serialized file
		if (!deserializer.load_from_file(path)) {
			std::cerr << "Failed to load serialized file: " << path << "\n";
		} else {
			auto animation = std::make_unique<Animation>();
			
			animation->deserialize(deserializer);

			auto& playbackComponent = mActiveActor->get().get_component<PlaybackComponent>();

			auto playbackData = std::make_shared<PlaybackData>(playbackComponent.getPlaybackData()->mSkeleton, std::move(animation));
			
			playbackComponent.setPlaybackData(playbackData);
			
			mPlayPauseButton->set_pushed(false);
			mReversePlayButton->set_pushed(false);
		}
	}
}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	if (mActiveActor.has_value()) {
		Actor& actorRef = mActiveActor->get();
		
		if (actorRef.find_component<SkinnedAnimationComponent>()) {
			set_title("Animation");
			set_visible(true);
			
			mReversePlayButton->set_visible(true);
			mPlayPauseButton->set_visible(true);

			parent()->perform_layout(screen()->nvg_context());
			
			auto& playback = actorRef.get_component<PlaybackComponent>();
			
			auto state = playback.get_state();
			
			if (state.getPlaybackState() == PlaybackState::Pause) {
				mPlayPauseButton->set_pushed(false);
				mReversePlayButton->set_pushed(false);
			} else if (state.getPlaybackState() == PlaybackState::Play &&
					   state.getPlaybackModifier() == PlaybackModifier::Forward) {
				mPlayPauseButton->set_pushed(true);
				mReversePlayButton->set_pushed(false);
			} else if (state.getPlaybackState() == PlaybackState::Play &&
					   state.getPlaybackModifier() == PlaybackModifier::Reverse) {
				mPlayPauseButton->set_pushed(false);
				mReversePlayButton->set_pushed(true);
			}
			
		} else if (actorRef.find_component<DrawableComponent>()) {
			DrawableComponent& drawableComponent = actorRef.get_component<DrawableComponent>();
			const Drawable& drawableRef = drawableComponent.drawable();
			
			// Attempt to cast to MeshComponent using dynamic_cast
			const MeshComponent* meshComponent = dynamic_cast<const MeshComponent*>(&drawableRef);
			if (meshComponent) {
				set_title("Mesh");
				set_visible(true);
				mReversePlayButton->set_visible(false);
				mPlayPauseButton->set_visible(false);

				parent()->perform_layout(screen()->nvg_context());
			}
			else {
				// Handle the case where drawable is not a MeshComponent
				std::cerr << "Error: DrawableComponent does not contain a MeshComponent." << std::endl;
				set_visible(false);
				parent()->perform_layout(screen()->nvg_context());
				return;
			}
		} else {
			set_visible(false);
			parent()->perform_layout(screen()->nvg_context());
		}
	} else {
		set_visible(false);
		parent()->perform_layout(screen()->nvg_context());
	}
	
	mPreviewCanvas->set_active_actor(mActiveActor);
}
