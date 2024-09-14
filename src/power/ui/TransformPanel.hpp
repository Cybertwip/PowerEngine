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

    void set_active_actor(std::reference_wrapper<Actor> actor);
    
private:
    nanogui::IntBox<int> *mXTranslate;
    nanogui::IntBox<int> *mYTranslate;
    nanogui::IntBox<int> *mZTranslate;

    nanogui::IntBox<int> *mPitchRotate;
    nanogui::IntBox<int> *mYawRotate;
    nanogui::IntBox<int> *mRollRotate;

    nanogui::IntBox<int> *mXScale;
    nanogui::IntBox<int> *mYScale;
    nanogui::IntBox<int> *mZScale;
	
	nanogui::Widget *mTranslateWidget;
	nanogui::Widget *mRotateWidget;
	nanogui::Widget *mScaleWidget;
    
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	int mTransformRegistrationId;
};
