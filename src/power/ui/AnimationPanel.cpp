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
#include "graphics/drawing/SelfContainedMeshCanvas.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "import/SkinnedFbx.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

AnimationPanel::AnimationPanel(nanogui::Widget &parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt), mCurrentTime(0) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 10, 10));
	
	auto *playbackLayout = new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
												   nanogui::Alignment::Middle, 0, 0);
	
	Widget *playbackPanel = new Widget(this);
	
	
	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	
	playbackPanel->set_layout(playbackLayout);
	
	Widget *canvasPanel = new Widget(this);
	canvasPanel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
	
	mReversePlayButton = new nanogui::ToolButton(playbackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");
	
	mReversePlayButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			
			playback.update_state(active ? SkinnedAnimationComponent::PlaybackState::Play : SkinnedAnimationComponent::PlaybackState::Pause, SkinnedAnimationComponent::PlaybackModifier::Reverse, SkinnedAnimationComponent::PlaybackTrigger::None);
		}
	});
	
	
	mPlayPauseButton = new nanogui::ToolButton(playbackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
	
	mPlayPauseButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			
			playback.update_state(active ? SkinnedAnimationComponent::PlaybackState::Play : SkinnedAnimationComponent::PlaybackState::Pause, SkinnedAnimationComponent::PlaybackModifier::Forward, SkinnedAnimationComponent::PlaybackTrigger::None);
		}
	});
	
	mPreviewCanvas = new SelfContainedMeshCanvas(canvasPanel);
	
	set_active_actor(std::nullopt);
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::parse_file(const std::string& path) {
	if (mActiveActor.has_value()) {
		
		CompressedSerialization::Deserializer deserializer;
		
		// Load the serialized file
		if (!deserializer.load_from_file(path)) {
			std::cerr << "Failed to load serialized file: " << path << "\n";
		} else {
			auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo> ();
			
			auto animation = std::make_unique<Animation>();
			
			animation->deserialize(deserializer);
			
			pdo->mAnimationData.push_back(std::move(animation));

			auto& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			skinnedComponent.set_pdo(std::move(pdo));
			
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
			parent()->perform_layout(screen()->nvg_context());
			
			auto& playback = actorRef.get_component<PlaybackComponent>();
			
			auto state = playback.get_state();
			
			if (state.getPlaybackState() == SkinnedAnimationComponent::PlaybackState::Pause) {
				mPlayPauseButton->set_pushed(false);
				mReversePlayButton->set_pushed(false);
			} else if (state.getPlaybackState() == SkinnedAnimationComponent::PlaybackState::Play &&
					   state.getPlaybackModifier() == SkinnedAnimationComponent::PlaybackModifier::Forward) {
				mPlayPauseButton->set_pushed(true);
				mReversePlayButton->set_pushed(false);
			} else if (state.getPlaybackState() == SkinnedAnimationComponent::PlaybackState::Play &&
					   state.getPlaybackModifier() == SkinnedAnimationComponent::PlaybackModifier::Reverse) {
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
