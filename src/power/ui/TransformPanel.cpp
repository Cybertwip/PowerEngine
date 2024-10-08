#include "ui/TransformPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/textbox.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>

#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"

TransformPanel::TransformPanel(nanogui::Widget& parent, nanogui::Screen& screen)
: Panel(parent, screen, "Transform"), mActiveActor(std::nullopt), mTransformRegistrationId(-1) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	
	
	auto gatherValuesCallback = [this](int){
		if (mActiveActor.has_value()) {
			auto &transform = mActiveActor->get().get_component<TransformComponent>();
			gather_values_into(transform);
		}
	};
	
	// Translation section
	mTranslationLabel = std::make_shared<nanogui::Label>(*this, screen, "Translation", "sans-bold");
	
	mTranslatePanel = std::make_shared<nanogui::Widget>(*this, screen);
	
	auto translateLayout = std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Vertical, 2,
																 nanogui::Alignment::Middle, 0, 0);
	
	translateLayout->set_col_alignment(nanogui::Alignment::Fill);
	
	mTranslatePanel->set_layout(std::move(translateLayout));
	
	mXLabel = std::make_shared<nanogui::Label>(mTranslatePanel, screen, "X", "sans-bold");
	mXTranslate = std::make_shared<nanogui::IntBox<int>>(mTranslatePanel, screen);
	mXTranslate->set_editable(true);
	mXTranslate->set_value(0);
	mXTranslate->set_default_value("0");
	mXTranslate->set_font_size(16);
	mXTranslate->set_format("[-]?[0-9]*");
	mXTranslate->set_spinnable(true);
	mXTranslate->set_value_increment(1);
	mXTranslate->set_callback(gatherValuesCallback);
	
	mYLabel = std::make_shared<nanogui::Label>(mTranslatePanel, screen, "Y", "sans-bold");
	mYTranslate = std::make_shared<nanogui::IntBox<int>>(mTranslatePanel, screen);
	mYTranslate->set_editable(true);
	mYTranslate->set_value(0);
	mYTranslate->set_default_value("0");
	mYTranslate->set_font_size(16);
	mYTranslate->set_format("[-]?[0-9]*");
	mYTranslate->set_spinnable(true);
	mYTranslate->set_value_increment(1);
	mYTranslate->set_callback(gatherValuesCallback);
	
	mZLabel = std::make_shared<nanogui::Label>(mTranslatePanel, screen, "Z", "sans-bold");
	mZTranslate = std::make_shared<nanogui::IntBox<int>>(mTranslatePanel, screen);
	mZTranslate->set_editable(true);
	mZTranslate->set_value(0);
	mZTranslate->set_default_value("0");
	mZTranslate->set_font_size(16);
	mZTranslate->set_format("[-]?[0-9]*");
	mZTranslate->set_spinnable(true);
	mZTranslate->set_value_increment(1);
	mZTranslate->set_callback(gatherValuesCallback);
	
	// Rotation section
	mRotationLabel = std::make_shared<nanogui::Label>(*this, screen, "Rotation", "sans-bold");
	
	mRotatePanel = std::make_shared<nanogui::Widget>(*this, screen);
	
	auto rotateLayout = std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Vertical, 2,
															  nanogui::Alignment::Middle, 0, 0);
	rotateLayout->set_col_alignment(nanogui::Alignment::Fill);
	
	mRotatePanel->set_layout(std::move(rotateLayout));
	
	mPitchLabel = std::make_shared<nanogui::Label>(mRotatePanel, screen, "Pitch", "sans-bold");
	mPitchRotate = std::make_shared<nanogui::IntBox<int>>(mRotatePanel, screen);
	mPitchRotate->set_editable(true);
	mPitchRotate->set_value(0);
	mPitchRotate->set_default_value("0");
	mPitchRotate->set_font_size(16);
	mPitchRotate->set_format("[-]?[0-9]*");
	mPitchRotate->set_spinnable(true);
	mPitchRotate->set_value_increment(1);
	mPitchRotate->set_callback(gatherValuesCallback);
	
	mYawLabel = std::make_shared<nanogui::Label>(mRotatePanel, screen, "Yaw", "sans-bold");
	mYawRotate = std::make_shared<nanogui::IntBox<int>>(mRotatePanel, screen);
	mYawRotate->set_editable(true);
	mYawRotate->set_value(0);
	mYawRotate->set_default_value("0");
	mYawRotate->set_font_size(16);
	mYawRotate->set_format("[-]?[0-9]*");
	mYawRotate->set_spinnable(true);
	mYawRotate->set_value_increment(1);
	mYawRotate->set_callback(gatherValuesCallback);
	
	mRollLabel = std::make_shared<nanogui::Label>(mRotatePanel, screen, "Roll", "sans-bold");
	mRollRotate = std::make_shared<nanogui::IntBox<int>>(mRotatePanel, screen);
	mRollRotate->set_editable(true);
	mRollRotate->set_value(0);
	mRollRotate->set_default_value("0");
	mRollRotate->set_font_size(16);
	mRollRotate->set_format("[-]?[0-9]*");
	mRollRotate->set_spinnable(true);
	mRollRotate->set_value_increment(1);
	mRollRotate->set_callback(gatherValuesCallback);
	
	// Scale section
	mScaleLabel = std::make_shared<nanogui::Label>(*this, screen, "Scale", "sans-bold");
	
	mScalePanel = std::make_shared<nanogui::Widget>(*this, screen);
	auto scaleLayout = std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Vertical, 2,
															 nanogui::Alignment::Middle, 0, 0);
	scaleLayout->set_col_alignment(nanogui::Alignment::Fill);
	
	mScalePanel->set_layout(std::move(scaleLayout));
	
	mXScaleLabel = std::make_shared<nanogui::Label>(mScalePanel, screen, "X", "sans-bold");
	mXScale = std::make_shared<nanogui::FloatBox<float>>(mScalePanel, screen);
	mXScale->set_editable(true);
	mXScale->set_value(1);
	mXScale->set_default_value("1");
	mXScale->set_font_size(16);
	mXScale->set_spinnable(true);
	mXScale->set_value_increment(0.01f);
	mXScale->number_format("%.2f");
	mXScale->set_callback(gatherValuesCallback);
	
	mYScaleLabel = std::make_shared<nanogui::Label>(mScalePanel, screen, "Y", "sans-bold");
	mYScale = std::make_shared<nanogui::FloatBox<float>>(mScalePanel, screen);
	mYScale->set_editable(true);
	mYScale->set_value(1);
	mYScale->set_default_value("1");
	mYScale->set_font_size(16);
	mYScale->set_spinnable(true);
	mYScale->set_value_increment(0.01f);
	mYScale->number_format("%.2f");
	mYScale->set_callback(gatherValuesCallback);
	
	mZScaleLabel = std::make_shared<nanogui::Label>(mScalePanel, screen, "Z", "sans-bold");
	mZScale = std::make_shared<nanogui::FloatBox<float>>(mScalePanel, screen);
	mZScale->set_editable(true);
	mZScale->set_value(1);
	mZScale->set_default_value("1");
	mZScale->set_font_size(16);
	mZScale->set_spinnable(true);
	mZScale->number_format("%.2f");
	mZScale->set_value_increment(0.01f);
	mZScale->set_callback(gatherValuesCallback);
	
	set_active_actor(std::nullopt);
}

TransformPanel::~TransformPanel() {
	if (mActiveActor.has_value()) {
		auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
		transformComponent.unregister_on_transform_changed_callback(mTransformRegistrationId);
	}
	
}

void TransformPanel::gather_values_into(TransformComponent &transform) {
	
	transform.transform.translation = glm::vec3(mXTranslate->value(), mYTranslate->value(), mZTranslate->value());

	auto rotation = glm::quat(glm::vec3(glm::radians((float)mPitchRotate->value()),
									glm::radians((float)mYawRotate->value()),
									glm::radians((float)mRollRotate->value())));
	transform.transform.rotation = rotation;
	
	transform.transform.scale = glm::vec3((float)mXScale->value(), (float)mYScale->value(),
												  (float)mZScale->value());
	
	transform.set_translation(transform.get_translation()); // force event firing
}

void TransformPanel::update_values_from(const TransformComponent &transform) {
	glm::vec3 translation = transform.get_translation();
	mXTranslate->set_value((int)translation.x);
	mYTranslate->set_value((int)translation.y);
	mZTranslate->set_value((int)translation.z);
	
	glm::vec3 eulerAngles = glm::eulerAngles(transform.get_rotation());
	mPitchRotate->set_value((int)glm::degrees(eulerAngles.x));
	mYawRotate->set_value((int)glm::degrees(eulerAngles.y));
	mRollRotate->set_value((int)glm::degrees(eulerAngles.z));
	
	glm::vec3 scale = transform.get_scale();
	mXScale->set_value(scale.x);
	mYScale->set_value(scale.y);
	mZScale->set_value(scale.z);
}

void TransformPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {

	mXTranslate->commit();
	mYTranslate->commit();
	mZTranslate->commit();
	mPitchRotate->commit();
	mYawRotate->commit();
	mRollRotate->commit();
	mXScale->commit();
	mYScale->commit();
	mZScale->commit();
	mActiveActor = actor;
	
	if (mActiveActor.has_value()) {
		auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
		transformComponent.unregister_on_transform_changed_callback(mTransformRegistrationId);
		
		mTransformRegistrationId = transformComponent.register_on_transform_changed_callback([this](const TransformComponent& transform) {
			update_values_from(transform);
		});

		update_values_from(transformComponent);
		
		set_visible(true);
		parent().perform_layout(screen().nvg_context());
	} else {
		set_visible(false);
		parent().perform_layout(screen().nvg_context());
	}
}
