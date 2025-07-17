#include "ui/CameraPanel.hpp"

#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>

#include "actors/Actor.hpp"
#include "components/CameraComponent.hpp"
#include "components/UiComponent.hpp"

// Constructor: Sets up the entire UI layout and initializes members.
CameraPanel::CameraPanel(nanogui::Widget& parent)
    : Panel(parent, "Camera"), mActiveActor(std::nullopt), mIsControlling(false) {

    // Set the main layout for this panel.
    set_layout(std::make_unique<nanogui::GroupLayout>());

    // A callback function to be triggered when any value box is changed.
    // It gathers all UI values and applies them to the active camera component.
    auto gatherValuesCallback = [this](float) {
        if (mActiveActor.has_value()) {
            // Check if the actor actually has a camera component before proceeding.
            if (mActiveActor->get().find_component<CameraComponent>()) {
                auto& camera = mActiveActor->get().get_component<CameraComponent>();
                gather_values_into(camera);
            }
        }
    };

    // Create a container widget for all the settings.
    mSettingsPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
    
    // Use a grid layout to arrange labels and value boxes neatly in two columns.
    auto settingsLayout = std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Vertical, 2, nanogui::Alignment::Fill, 5, 5);
    settingsLayout->set_col_alignment({ nanogui::Alignment::Maximum, nanogui::Alignment::Fill });
    mSettingsPanel->set_layout(std::move(settingsLayout));

    // -- Field of View (FOV) --
    mFovLabel = std::make_shared<nanogui::Label>(*mSettingsPanel, "FOV", "sans-bold");
    mFovBox = std::make_shared<nanogui::FloatBox<float>>(*mSettingsPanel);
    mFovBox->set_editable(true);
    mFovBox->set_font_size(16);
    mFovBox->set_value(45.0f);
    mFovBox->set_default_value("45.0");
    mFovBox->set_spinnable(true);
    mFovBox->set_value_increment(1.0f);
    mFovBox->number_format("%.1f");
    mFovBox->set_callback(gatherValuesCallback);

    // -- Near Plane --
    mNearLabel = std::make_shared<nanogui::Label>(*mSettingsPanel, "Near", "sans-bold");
    mNearBox = std::make_shared<nanogui::FloatBox<float>>(*mSettingsPanel);
    mNearBox->set_editable(true);
    mNearBox->set_font_size(16);
    mNearBox->set_value(0.1f);
    mNearBox->set_default_value("0.1");
    mNearBox->set_spinnable(true);
    mNearBox->set_value_increment(0.1f);
    mNearBox->number_format("%.2f");
    mNearBox->set_callback(gatherValuesCallback);

    // -- Far Plane --
    mFarLabel = std::make_shared<nanogui::Label>(*mSettingsPanel, "Far", "sans-bold");
    mFarBox = std::make_shared<nanogui::FloatBox<float>>(*mSettingsPanel);
    mFarBox->set_editable(true);
    mFarBox->set_font_size(16);
    mFarBox->set_value(10000.0f);
    mFarBox->set_default_value("10000.0");
    mFarBox->set_spinnable(true);
    mFarBox->set_value_increment(100.0f);
    mFarBox->number_format("%.1f");
    mFarBox->set_callback(gatherValuesCallback);

    // -- Aspect Ratio --
    mAspectLabel = std::make_shared<nanogui::Label>(*mSettingsPanel, "Aspect", "sans-bold");
    mAspectBox = std::make_shared<nanogui::FloatBox<float>>(*mSettingsPanel);
    mAspectBox->set_editable(true);
    mAspectBox->set_font_size(16);
    mAspectBox->set_value(16.0f / 9.0f);
    mAspectBox->set_default_value("1.77");
    mAspectBox->set_spinnable(true);
    mAspectBox->set_value_increment(0.1f);
    mAspectBox->number_format("%.2f");
    mAspectBox->set_callback(gatherValuesCallback);

    // -- Control Button --
    mControlButton = std::make_shared<nanogui::Button>(*this, "Control");
    mControlButton->set_callback([this] {
        mIsControlling = !mIsControlling;
        mControlButton->set_caption(mIsControlling ? "Release" : "Control");
		
		if (mActiveActor.has_value()) {
			// Check if the actor has the required component.
			if (mActiveActor->get().find_component<CameraComponent>()) {
				auto& cameraComponent = mActiveActor->get().get_component<CameraComponent>();
				cameraComponent.set_active(mIsControlling);
				
				if (mActiveActor->get().find_component<UiComponent>()) {
					mActiveActor->get().get_component<UiComponent>().select();
				}
			}
		}
		
        // The actual control/release logic would be implemented here or in a controller system.
    });

    // Start with the panel hidden.
    set_active_actor(std::nullopt);
}

// Destructor: Clean-up is handled by std::shared_ptr.
CameraPanel::~CameraPanel() {
}

// Applies UI values to the CameraComponent.
void CameraPanel::gather_values_into(CameraComponent& camera) {
    camera.set_fov(mFovBox->value());
    camera.set_near(mNearBox->value());
    camera.set_far(mFarBox->value());
    camera.set_aspect(mAspectBox->value());
}

// Populates UI fields from the CameraComponent's state.
void CameraPanel::update_values_from(const CameraComponent& camera) {
    mFovBox->set_value(camera.get_fov());
    mNearBox->set_value(camera.get_near());
    mFarBox->set_value(camera.get_far());
    mAspectBox->set_value(camera.get_aspect());
	mIsControlling = camera.active();
}

// Manages panel visibility and data binding based on the selected actor.
void CameraPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
    // Commit any pending changes in the input boxes before switching context.
    mFovBox->commit();
    mNearBox->commit();
    mFarBox->commit();
    mAspectBox->commit();

    mActiveActor = actor;

    bool shouldBeVisible = false;
    if (mActiveActor.has_value()) {
        // Check if the actor has the required component.
        if (mActiveActor->get().find_component<CameraComponent>()) {
            auto& cameraComponent = mActiveActor->get().get_component<CameraComponent>();
            update_values_from(cameraComponent);
            shouldBeVisible = true;
        }
    }

    // Set visibility and refresh the layout.
    set_visible(shouldBeVisible);
    if (parent()) {
        parent()->get().perform_layout(screen().nvg_context());
    }
}
