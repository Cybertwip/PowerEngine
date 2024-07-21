#include "UiCommon.hpp"

#include "actors/Actor.hpp"

#include "components/UiComponent.hpp"

#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

UiCommon::UiCommon(nanogui::Widget& parent) {
    mScenePanel = new ScenePanel(parent);
    auto wrapper = new nanogui::Window(&parent, "");
    wrapper->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));

    mHierarchyPanel = new HierarchyPanel(*wrapper);
    mTransformPanel = new TransformPanel(*wrapper);
}


void UiCommon::attach_actors(const std::vector<std::reference_wrapper<Actor>> &actors) {
    for(auto& actor : actors){
        actor.get().add_component<UiComponent>([this, actor](){
            mTransformPanel->set_active_actor(actor);
        });
    }
    
    mHierarchyPanel->set_actors(actors);
}

void UiCommon::update(){
    mTransformPanel->update();
}
