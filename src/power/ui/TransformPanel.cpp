#include "ui/TransformPanel.hpp"

#include "actors/Actor.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include "components/TransformComponent.hpp"

TransformPanel::TransformPanel(nanogui::Widget &parent) : Panel(parent, "Transform"), mActiveActor(std::nullopt) {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());

    // Translation section
    new nanogui::Label(this, "Translation", "sans-bold");

    Widget *translatePanel = new Widget(this);
    translatePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                       nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(translatePanel, "X:", "sans-bold");
    mXTranslate = new nanogui::IntBox<int>(translatePanel);
    mXTranslate->set_editable(true);
    mXTranslate->set_fixed_size(nanogui::Vector2i(100, 20));
    mXTranslate->set_value(0);
    mXTranslate->set_default_value("0");
    mXTranslate->set_font_size(16);
    mXTranslate->set_format("[-]?[0-9]*");
    mXTranslate->set_spinnable(true);
    mXTranslate->set_value_increment(1);

    new nanogui::Label(translatePanel, "Y:", "sans-bold");
    mYTranslate = new nanogui::IntBox<int>(translatePanel);
    mYTranslate->set_editable(true);
    mYTranslate->set_fixed_size(nanogui::Vector2i(100, 20));
    mYTranslate->set_value(0);
    mYTranslate->set_default_value("0");
    mYTranslate->set_font_size(16);
    mYTranslate->set_format("[-]?[0-9]*");
    mYTranslate->set_spinnable(true);
    mYTranslate->set_value_increment(1);

    new nanogui::Label(translatePanel, "Z:", "sans-bold");
    mZTranslate = new nanogui::IntBox<int>(translatePanel);
    mZTranslate->set_editable(true);
    mZTranslate->set_fixed_size(nanogui::Vector2i(100, 20));
    mZTranslate->set_value(0);
    mZTranslate->set_default_value("0");
    mZTranslate->set_font_size(16);
    mZTranslate->set_format("[-]?[0-9]*");
    mZTranslate->set_spinnable(true);
    mZTranslate->set_value_increment(1);

    // Rotation section
    new nanogui::Label(this, "Rotation", "sans-bold");

    Widget *rotatePanel = new Widget(this);
    rotatePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                    nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(rotatePanel, "Pitch", "sans-bold");
    mPitchRotate = new nanogui::IntBox<int>(rotatePanel);
    mPitchRotate->set_editable(true);
    mPitchRotate->set_fixed_size(nanogui::Vector2i(100, 20));
    mPitchRotate->set_value(0);
    mPitchRotate->set_default_value("0");
    mPitchRotate->set_font_size(16);
    mPitchRotate->set_format("[-]?[0-9]*");
    mPitchRotate->set_spinnable(true);
    mPitchRotate->set_value_increment(1);

    new nanogui::Label(rotatePanel, "Yaw", "sans-bold");
    mYawRotate = new nanogui::IntBox<int>(rotatePanel);
    mYawRotate->set_editable(true);
    mYawRotate->set_fixed_size(nanogui::Vector2i(100, 20));
    mYawRotate->set_value(0);
    mYawRotate->set_default_value("0");
    mYawRotate->set_font_size(16);
    mYawRotate->set_format("[-]?[0-9]*");
    mYawRotate->set_spinnable(true);
    mYawRotate->set_value_increment(1);

    new nanogui::Label(rotatePanel, "Roll", "sans-bold");
    mRollRotate = new nanogui::IntBox<int>(rotatePanel);
    mRollRotate->set_editable(true);
    mRollRotate->set_fixed_size(nanogui::Vector2i(100, 20));
    mRollRotate->set_value(0);
    mRollRotate->set_default_value("0");
    mRollRotate->set_font_size(16);
    mRollRotate->set_format("[-]?[0-9]*");
    mRollRotate->set_spinnable(true);
    mRollRotate->set_value_increment(1);

    // Scale section
    new nanogui::Label(this, "Scale", "sans-bold");

    Widget *scalePanel = new Widget(this);
    scalePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                   nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(scalePanel, "X", "sans-bold");
    mXScale = new nanogui::IntBox<int>(scalePanel);
    mXScale->set_editable(true);
    mXScale->set_fixed_size(nanogui::Vector2i(100, 20));
    mXScale->set_value(1);
    mXScale->set_default_value("1");
    mXScale->set_font_size(16);
    mXScale->set_format("[1-9][0-9]*");
    mXScale->set_spinnable(true);
    mXScale->set_value_increment(1);

    new nanogui::Label(scalePanel, "Y", "sans-bold");
    mYScale = new nanogui::IntBox<int>(scalePanel);
    mYScale->set_editable(true);
    mYScale->set_fixed_size(nanogui::Vector2i(100, 20));
    mYScale->set_value(1);
    mYScale->set_default_value("1");
    mYScale->set_font_size(16);
    mYScale->set_format("[1-9][0-9]*");
    mYScale->set_spinnable(true);
    mYScale->set_value_increment(1);

    new nanogui::Label(scalePanel, "Z", "sans-bold");
    mZScale = new nanogui::IntBox<int>(scalePanel);
    mZScale->set_editable(true);
    mZScale->set_fixed_size(nanogui::Vector2i(100, 20));
    mZScale->set_value(1);
    mZScale->set_default_value("1");
    mZScale->set_font_size(16);
    mZScale->set_format("[1-9][0-9]*");
    mZScale->set_spinnable(true);
    mZScale->set_value_increment(1);
}

void TransformPanel::gather_values_into(TransformComponent &transform) {
    transform.set_translation(
        glm::vec3(mXTranslate->value(), mYTranslate->value(), mZTranslate->value()));

    glm::quat rotation = glm::quat(glm::vec3(glm::radians((float)mPitchRotate->value()),
                                             glm::radians((float)mYawRotate->value()),
                                             glm::radians((float)mRollRotate->value())));
    transform.set_rotation(rotation);

    transform.transform.scale = ozz::math::Float3((float)mXScale->value(), (float)mYScale->value(),
                                                  (float)mZScale->value());
}

void TransformPanel::update_values_from(TransformComponent &transform) {
    glm::vec3 translation = transform.get_translation();
    mXTranslate->set_value((int)translation.x);
    mYTranslate->set_value((int)translation.y);
    mZTranslate->set_value((int)translation.z);

    glm::vec3 eulerAngles = glm::eulerAngles(transform.get_rotation());
    mPitchRotate->set_value((int)glm::degrees(eulerAngles.x));
    mYawRotate->set_value((int)glm::degrees(eulerAngles.y));
    mRollRotate->set_value((int)glm::degrees(eulerAngles.z));

    ozz::math::Float3 scale = transform.transform.scale;
    mXScale->set_value((int)scale.x);
    mYScale->set_value((int)scale.y);
    mZScale->set_value((int)scale.z);
}


void TransformPanel::set_active_actor(std::reference_wrapper<Actor> actor) {
    mActiveActor = actor;
    
    update_values_from(mActiveActor->get().get_component<TransformComponent>());
}

void TransformPanel::update() {
    if (mActiveActor.has_value()){
        auto& transform = mActiveActor->get().get_component<TransformComponent>();
        gather_values_into(transform);
    }
}
