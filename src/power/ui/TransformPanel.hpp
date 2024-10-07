#pragma once

#include "Panel.hpp"

#include <nanogui/textbox.h>

class Actor;
class TransformComponent;

class TransformPanel : public Panel {
public:
    TransformPanel(nanogui::Widget&);
	~TransformPanel();
    
    void gather_values_into(TransformComponent& transform);
    void update_values_from(const TransformComponent& transform);

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
    
private:
	void initialize() override;
	
	std::shared_ptr<nanogui::IntBox<int>> mXTranslate;
	std::shared_ptr<nanogui::IntBox<int>> mYTranslate;
	std::shared_ptr<nanogui::IntBox<int>> mZTranslate;

	std::shared_ptr<nanogui::IntBox<int>> mPitchRotate;
	std::shared_ptr<nanogui::IntBox<int>> mYawRotate;
    std::shared_ptr<nanogui::IntBox<int>> mRollRotate;

	std::shared_ptr<nanogui::Label> mScaleLabel;
	std::shared_ptr<nanogui::FloatBox<float>> mXScale;
	std::shared_ptr<nanogui::FloatBox<float>> mYScale;
	std::shared_ptr<nanogui::FloatBox<float>> mZScale;
	
	std::shared_ptr<nanogui::Widget> mTranslateWidget;
	std::shared_ptr<nanogui::Widget> mRotateWidget;
	std::shared_ptr<nanogui::Widget> mScaleWidget;
    
	std::shared_ptr<nanogui::Label> mTranslationLabel;
	std::shared_ptr<nanogui::Label> mXLabel;
	std::shared_ptr<nanogui::Label> mYLabel;
	std::shared_ptr<nanogui::Label> mZLabel;

	std::shared_ptr<nanogui::Label> mXScaleLabel;
	std::shared_ptr<nanogui::Label> mYScaleLabel;
	std::shared_ptr<nanogui::Label> mZScaleLabel;

	std::shared_ptr<nanogui::Label> mRotationLabel;
	std::shared_ptr<nanogui::Label> mYawLabel;
	std::shared_ptr<nanogui::Label> mPitchLabel;
	std::shared_ptr<nanogui::Label> mRollLabel;

	std::shared_ptr<nanogui::Widget> mTranslatePanel;
	std::shared_ptr<nanogui::Widget> mRotatePanel;
	std::shared_ptr<nanogui::Widget> mScalePanel;


    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	int mTransformRegistrationId;
};
