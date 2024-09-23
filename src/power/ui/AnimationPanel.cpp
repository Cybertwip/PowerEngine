#include "ui/AnimationPanel.hpp"

#include <nanogui/toolbutton.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include "actors/Actor.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/TransformComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "import/SkinnedFbx.hpp"

AnimationPanel::AnimationPanel(nanogui::Widget &parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::GroupLayout());

	auto *playbackLayout = new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
													nanogui::Alignment::Middle, 0, 0);
	
	Widget *playbackPanel = new Widget(this);

	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	
	playbackPanel->set_layout(playbackLayout);

	mReversePlayButton = new nanogui::ToolButton(playbackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");
	
	mReversePlayButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& animation = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			animation.set_reverse(true);
			animation.set_playing(active);
		}
	});


	mPlayPauseButton = new nanogui::ToolButton(playbackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
	
	mPlayPauseButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& animation = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			animation.set_reverse(false);
			animation.set_playing(active);
		}
	});
	
	set_active_actor(std::nullopt);
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::parse_file(const std::string& path) {
	if (mActiveActor.has_value()) {
		auto model = std::make_unique<SkinnedFbx>(path);
		
		model->LoadModel();
		model->TryImportAnimations();

		std::unique_ptr<Drawable> drawableComponent;
		
		if (model->GetSkeleton() != nullptr) {
			std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
			
			auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo> (std::move(model->GetSkeleton()));
			
			for (auto& animation : model->GetAnimationData()) {
				pdo->mAnimationData.push_back(std::move(animation));
			}
			
			auto& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			
			skinnedComponent.set_pdo(std::move(pdo));
		}
	}
	
	

}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {

	if (actor.has_value()) {
		if (actor->get().find_component<SkinnedAnimationComponent>()) {
			mActiveActor = actor;
			
			set_visible(true);
			parent()->perform_layout(screen()->nvg_context());
		} else {
			set_visible(false);
			parent()->perform_layout(screen()->nvg_context());
		}
	} else {
		set_visible(false);
		parent()->perform_layout(screen()->nvg_context());
	}

}
