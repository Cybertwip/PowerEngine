#include "UiCommon.hpp"

#include "actors/Actor.hpp"

#include "components/UiComponent.hpp"

#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/StatusBarPanel.hpp"
#include "ui/TransformPanel.hpp"

UiCommon::UiCommon(nanogui::Widget& parent) {
    auto mainWrapper = new nanogui::Window(&parent, "");

    mainWrapper->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                       nanogui::Alignment::Fill, 0, 0));

    auto sceneWrapper = new nanogui::Window(mainWrapper, "");
    sceneWrapper->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
                                       nanogui::Alignment::Fill, 0, 0));

    sceneWrapper->set_fixed_width(parent.size().x());
    sceneWrapper->set_fixed_height(parent.size().y() - 56);
    
    mStatusBarPanel = new StatusBarPanel(*mainWrapper);
    
    mScenePanel = new ScenePanel(*sceneWrapper);
    auto rightWrapper = new nanogui::Window(sceneWrapper, "");
    rightWrapper->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));

    mHierarchyPanel = new HierarchyPanel(*rightWrapper);
    mTransformPanel = new TransformPanel(*rightWrapper);
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
