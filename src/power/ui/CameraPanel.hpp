#pragma once

#include "Panel.hpp"

#include <nanogui/textbox.h>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <optional>
#include <functional>

// Forward declarations to avoid including full headers
class Actor;
class CameraComponent;

namespace nanogui {
class Widget;
class Label;
}

/**
 * @class CameraPanel
 * @brief A NanoGUI panel for editing the properties of a CameraComponent.
 *
 * This panel displays controls for Field of View (FOV), near and far clipping planes,
 * aspect ratio, and projection type. It also provides a button to toggle a "Control" state.
 * The panel automatically shows or hides itself based on whether the selected Actor
 * has a CameraComponent.
 */
class CameraPanel : public Panel {
public:
	/**
	 * @brief Constructs a new CameraPanel.
	 * @param parent The parent widget in the NanoGUI hierarchy.
	 */
	CameraPanel(nanogui::Widget& parent);
	
	/**
	 * @brief Destructor for the CameraPanel.
	 */
	~CameraPanel();
	
	/**
	 * @brief Populates the panel's UI fields with values from a CameraComponent.
	 * @param camera The component to read values from.
	 */
	void update_values_from(const CameraComponent& camera);
	
	/**
	 * @brief Applies the values from the panel's UI fields to a CameraComponent.
	 * @param camera The component to modify.
	 */
	void gather_values_into(CameraComponent& camera);
	
	/**
	 * @brief Sets the currently active actor for the panel.
	 * If the actor has a CameraComponent, the panel becomes visible and displays its properties.
	 * Otherwise, the panel is hidden.
	 * @param actor An optional reference to the selected actor.
	 */
	void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
private:
	// UI elements for camera properties
	std::shared_ptr<nanogui::Widget> mSettingsPanel;
	std::shared_ptr<nanogui::Label> mFovLabel;
	std::shared_ptr<nanogui::FloatBox<float>> mFovBox;
	std::shared_ptr<nanogui::Label> mNearLabel;
	std::shared_ptr<nanogui::FloatBox<float>> mNearBox;
	std::shared_ptr<nanogui::Label> mFarLabel;
	std::shared_ptr<nanogui::FloatBox<float>> mFarBox;
	std::shared_ptr<nanogui::Label> mAspectLabel;
	std::shared_ptr<nanogui::FloatBox<float>> mAspectBox;
	
	// Control button
	std::shared_ptr<nanogui::Button> mControlButton;
	bool mIsControlling;
	
	// Checkboxes for boolean flags
	std::shared_ptr<nanogui::Widget> mCheckboxContainer;
	std::shared_ptr<nanogui::CheckBox> mOrthoCheck;
	std::shared_ptr<nanogui::CheckBox> mDefaultCheck;
	
	// Data members
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
};
