#pragma once

#include "Panel.hpp"

#include <nanogui/textbox.h>

class Actor;
class TransformComponent;

class TransformPanel : public Panel {
public:
    TransformPanel(nanogui::Widget& parent);
	~TransformPanel();
    
    void gather_values_into(TransformComponent& transform);
    void update_values_from(const TransformComponent& transform);

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
    
private:
    nanogui::IntBox<int> *mXTranslate;
    nanogui::IntBox<int> *mYTranslate;
    nanogui::IntBox<int> *mZTranslate;

    nanogui::IntBox<int> *mPitchRotate;
    nanogui::IntBox<int> *mYawRotate;
    nanogui::IntBox<int> *mRollRotate;

	std::shared_ptr<nanogui::FloatBox<float>> mXScale;
	std::shared_ptr<nanogui::FloatBox<float>> mYScale;
	std::shared_ptr<nanogui::FloatBox<float>> mZScale;
	
	std::shared_ptr<nanogui::Widget> mTranslateWidget;
	std::shared_ptr<nanogui::Widget> mRotateWidget;
	std::shared_ptr<nanogui::Widget> mScaleWidget;
    
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	int mTransformRegistrationId;
};
