#import <Cocoa/Cocoa.h>

#include "Application.hpp"

#include "CameraActorLoader.hpp"
#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "RenderCommon.hpp"
#include "ShaderManager.hpp"
#include "UiCommon.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "actors/IActorSelectedRegistry.hpp"

#include "animation/HeuristicSkeletonPoser.hpp"  // debug
#include "animation/Skeleton.hpp"  // debug

#include "components/CameraComponent.hpp"

#include "components/DrawableComponent.hpp"  // debug

#include "components/TransformComponent.hpp"

#include "components/SkinnedMeshComponent.hpp"  // debug
#include "components/SkeletonComponent.hpp"  // debug

#include "execution/BlueprintManager.hpp"
#include "execution/ExecutionManager.hpp"

#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"

#include "import/ModelImporter.hpp"
#include "serialization/SerializationModule.hpp"
#include "serialization/SceneSerializer.hpp"
#include "simulation/Cartridge.hpp"
#include "simulation/CartridgeActorLoader.hpp"
#include "simulation/SimulationServer.hpp"
#include "simulation/VirtualMachine.hpp"
#include "ui/AnimationPanel.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
//#include "ui/SceneTimeBar.hpp"
#include "ui/StatusBarPanel.hpp"
#include "ui/ResourcesPanel.hpp"
#include "ui/TransformPanel.hpp"
#include "ui/UiManager.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/theme.h>
#include <nanogui/window.h>

#include <cmath>
#include <functional>


// === ADDED SECTION: Objective-C Category to handle menu actions ===
// Global pointer to our C++ Application instance.
static Application* g_App = nullptr;

// A "Category" adds new methods to an existing class.
// We add our action method directly to NSApplication.
@interface NSApplication (PowerEngineActions)
- (IBAction)newProjectAction:(id)sender;
- (IBAction)newSceneAction:(id)sender;
- (IBAction)saveSceneAction:(id)sender;
- (IBAction)loadSceneAction:(id)sender;
@end

@implementation NSApplication (PowerEngineActions)
- (IBAction)newProjectAction:(id)sender {
	// This Objective-C method calls the C++ method on our app instance.
	if (g_App) {
		g_App->new_project_action();
	}
}
- (IBAction)newSceneAction:(id)sender {
	// This Objective-C method calls the C++ method on our app instance.
	if (g_App) {
		g_App->new_scene_action();
	}
}

- (IBAction)saveSceneAction:(id)sender {
	// This Objective-C method calls the C++ method on our app instance.
	if (g_App) {
		g_App->save_scene_action();
	}
}
- (IBAction)loadSceneAction:(id)sender {
	// This Objective-C method calls the C++ method on our app instance.
	if (g_App) {
		g_App->load_scene_action();
	}
}
@end

Application::Application()
: nanogui::DraggableScreen("Power Engine")
, mGlobalAnimationTimeProvider(60 * 30)
{
	g_App = this;

	Batch::init_dummy_texture();
	
	mEntityRegistry = std::make_unique<entt::registry>();
	
	mCameraManager = std::make_unique<CameraManager>();
	
	mActorManager  = std::make_unique<ActorManager>(*mEntityRegistry, *mCameraManager);

}

void Application::initialize() {
	@autoreleasepool {
		// Ensure the native application object is running
		[NSApplication sharedApplication];
		// Load the XIB. The owner is the NSApplication instance ('NSApp'),
		// which now directly contains our action method via the category.
		[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:NSApp topLevelObjects:nil];
	}
	
	DraggableScreen::initialize();

	mUiCommon = std::make_shared<UiCommon>(*this, screen(), *mActorManager, mGlobalAnimationTimeProvider);
	
	mRenderCommon = std::make_shared<RenderCommon>(*mUiCommon->scene_panel(), *this, *mEntityRegistry, *mActorManager, *mCameraManager);
	
	mMeshBatch = std::make_unique<MeshBatch>(mRenderCommon->canvas()->render_pass());
	
	mSkinnedMeshBatch = std::make_unique<SkinnedMeshBatch>(mRenderCommon->canvas()->render_pass());

	mGizmoMeshBatch = std::make_unique<MeshBatch>(mRenderCommon->canvas()->render_pass());
	
	mGizmoSkinnedMeshBatch = std::make_unique<SkinnedMeshBatch>(mRenderCommon->canvas()->render_pass());

	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);

	mGizmoBatchUnit = std::make_unique<BatchUnit>(*mGizmoMeshBatch, *mGizmoSkinnedMeshBatch);

	mMeshShader = std::make_unique<ShaderWrapper>(mRenderCommon->shader_manager().get_shader("mesh"));
	
	mSkinnedShader = std::make_unique<ShaderWrapper>(mRenderCommon->shader_manager().get_shader("skinned_mesh"));
	
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
		
	mMeshActorLoader = std::make_unique<MeshActorLoader>(*mActorManager, mRenderCommon->shader_manager(), *mBatchUnit, *mMeshActorBuilder);
	
	mGizmoActorLoader = std::make_unique<MeshActorLoader>(*mActorManager, mRenderCommon->shader_manager(), *mGizmoBatchUnit, *mMeshActorBuilder);
	
	mGizmoManager = std::make_unique<GizmoManager>(*mRenderCommon->canvas(), mRenderCommon->shader_manager(), *mActorManager, *mGizmoActorLoader);
	
	mUiManager = std::make_unique<UiManager>(*this,
											 mUiCommon->hierarchy_panel(),
											 mUiCommon->hierarchy_panel(),
											 *mActorManager,
											 *mMeshActorLoader,
											 *mGizmoActorLoader,
											 mRenderCommon->shader_manager(),
											 mUiCommon->scene_panel(),
											 mRenderCommon->canvas(),
											 mUiCommon->status_bar(),
											 mUiCommon->animation_panel(),
//											 mUiCommon->scene_time_bar(),
											 mGlobalAnimationTimeProvider,
											 *mCameraManager,
											 *mGizmoManager,
											 [this](std::function<void(int, int)> callback){
												 auto callbackWrapee = [this, callback](bool down, int width, int height, int x, int y){
													 callback(x, y);
												 };
												 register_click_callback(callbackWrapee);
											 }
											 );
	
	theme().m_window_drop_shadow_size = 0;
	
	set_layout(std::make_unique<nanogui::GroupLayout>(0, 0, 0, 0));
	
	std::vector<std::reference_wrapper<Actor>> actors;
	
	mRenderCommon->camera_actor_loader().setup_engine_camera(mGlobalAnimationTimeProvider,
															 45.0f, 0.01f, 1e5f,
															 mRenderCommon->canvas()->fixed_size().x() /
															 static_cast<float>(mRenderCommon->canvas()->fixed_size().y()));
	
	mUiCommon->hierarchy_panel()->add_actors(std::move(actors));
	
	set_background(mRenderCommon->canvas()->background_color());
		
	mVirtualMachine = std::make_unique<VirtualMachine>();
	
	mCartridgeActorLoader = std::make_unique<CartridgeActorLoader>(*mVirtualMachine, *mMeshActorLoader, *mActorManager, *mUiCommon->hierarchy_panel(), mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader);
	
	mCartridge = std::make_unique<Cartridge>(*mVirtualMachine, *mCartridgeActorLoader, *mCameraManager);

	mSimulationServer = std::make_unique<SimulationServer>(9003, *mVirtualMachine, *mCartridgeActorLoader, [this](std::optional<std::reference_wrapper<VirtualMachine>> virtualMachine) {
		mLoadedVirtualMachine = virtualMachine;
		
		if (mLoadedVirtualMachine) {
			mExecutionManager->set_execution_mode(ExecutionManager::EExecutionMode::Laboratory);
		}
	});
	
	mSerializationModule = std::make_unique<SerializationModule>(*mActorManager, *mMeshActorBuilder, mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader);
	
	mBlueprintManager = std::make_unique<BlueprintManager>(*mRenderCommon->canvas(), mUiCommon->hierarchy_panel(), *mActorManager);
	
	mExecutionManager = std::make_unique<ExecutionManager>(*mRenderCommon->canvas(), *mSimulationServer, *mBlueprintManager);
	
	mSimulationServer->run();

	perform_layout();
}

bool Application::keyboard_event(int key, int scancode, int action, int modifiers) {
	Screen::keyboard_event(key, scancode, action, modifiers);
	
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		nanogui::async([this](){
			set_visible(false);
		});
		return true;
	}
	
	// Detect "New Project" trigger (e.g., Ctrl + N)
//	if (key == GLFW_KEY_N && action == GLFW_PRESS && modifiers & GLFW_MOD_CONTROL) {
//		new_project_action();
//		return true;
//	}
//	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
////		mMeshActorLoader->cleanup();
//		auto& skinnedActor = mMeshActorLoader->create_actor("test.fbx", mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader);
//		
//		auto& skeletonComponent = skinnedActor.get_component<SkeletonComponent>();
//		
//		auto& skeleton = static_cast<Skeleton&>(skeletonComponent.get_skeleton());
//		
//		HeuristicSkeletonPoser poser(skeleton);
//		
//		poser.apply();
//
//		DrawableComponent& drawableComponent = skinnedActor.get_component<DrawableComponent>();
//		
//		auto& drawableRef = drawableComponent.drawable();
//		
//		auto& skinnedMeshComponent = static_cast<SkinnedMeshComponent&>(drawableRef);
//
//		
//		MeshActorExporter exporter;
//		
//		std::stringstream meshStream;
//		
//		exporter.exportSkinnedFbxToStream(skinnedMeshComponent.get_model(), meshStream);
//		
//		std::ofstream outFile("SkinnedMesh.fbx");
//		if (outFile.is_open()) {
//			outFile << meshStream.str();
//			outFile.close();
//			std::cout << "File exported successfully." << std::endl;
//		} else {
//			std::cerr << "Unable to open file for writing." << std::endl;
//		}
//
//
//		return true;
//	}
	// delegate to blueprint manager first
	
	if (mBlueprintManager->keyboard_event(key, scancode, action, modifiers)) {
		return true;
	}
	
	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
		mBlueprintManager->commit();
		mUiManager->remove_active_actor();
		mCameraManager->update_from(*mActorManager);
		return true;
	}
    return false;
}

void Application::draw(NVGcontext *ctx) {
    Screen::draw(ctx);
}

void Application::process_events() {
	mGlobalAnimationTimeProvider.Update();
	
	if (mLoadedVirtualMachine.has_value()) {
		mLoadedVirtualMachine->get().update();
	}
	
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [down, w, h, x, y] = mClickQueue.front();
		mClickQueue.pop();
		
		for (auto& callback : mClickCallbacks) {
			callback(down, w, h, x, y);
		}
	}
	
	while (!mEventQueue.empty()) {
		auto callback = mEventQueue.back();
		mEventQueue.pop_back();
		callback();
	}

	mUiCommon->scene_panel()->process_events();
	mUiManager->status_bar_panel()->resources_panel()->process_events();
	
	mUiManager->process_events();
	
	mExecutionManager->process_events();
}

bool Application::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {	
	// Queue the click event
	if (button == GLFW_MOUSE_BUTTON_1) {
		mClickQueue.push(std::make_tuple(down, width(), height(), p.x(), p.y()));
	}
	
	return Widget::mouse_button_event(p, button, down, modifiers);
}

void Application::register_click_callback(std::function<void(bool, int, int, int, int)> callback) {
	mClickCallbacks.push_back(callback);
}

bool Application::drop_event(Widget& sender, const std::vector<std::string> & filenames) {

	mEventQueue.push_back([this, &sender, filenames](){
		const std::string& path = filenames[0];

		// 1. Check if the drop occurred on the Animation Panel
		if (mUiCommon->animation_panel()->contains(m_mouse_pos, true, true)) {
			if (path.find(".pan") != std::string::npos) {
				mUiCommon->animation_panel()->parse_file(path);
				return; // Event handled
			}
		}

		// 2. Check if the drop occurred on the main 3D Scene/Canvas
		if (mRenderCommon->canvas()->contains(m_mouse_pos, true, true)) {
			// You can add more specific exclusion zones if needed
			// For example, !mUiManager->some_other_panel()->contains(m_mouse_pos)
			
			if (path.find(".fbx") != std::string::npos) {
				mUiCommon->hierarchy_panel()->add_actor(mMeshActorLoader->create_actor(path, mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader));
				//				mUiCommon->scene_time_bar()->refresh_actors();
				return; // Event handled
			} else if (path.find(".png") != std::string::npos) {
				mUiCommon->hierarchy_panel()->add_actor(mMeshActorLoader->create_sprite_actor("Sprite", path, mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader));
				//				mUiCommon->scene_time_bar()->refresh_actors();
				return; // Event handled
			}
		}
	});
}

void Application::new_project_action() {
	// Using nanogui::async to keep the UI responsive while the dialog is open.
	nanogui::async([this]() {
		nanogui::file_dialog_async({{"", "Folders"}}, true, false,
								   [this](const std::vector<std::string>& folders) {
			if (folders.empty()) {
				return; // User canceled
			}
			std::string projectFolder = folders.front();
			try {
				if (DirectoryNode::createProjectFolder(projectFolder)) {
					// Reset the application state for the new project
					mBlueprintManager->stop();
					mUiCommon->hierarchy_panel()->clear_actors();
					mExecutionManager->set_execution_mode(ExecutionManager::EExecutionMode::Editor);
					
					// Asynchronously refresh the file view to show the new project folder
					nanogui::async([this]() {
						mUiManager->status_bar_panel()->resources_panel()->refresh_file_view();
					});
				}
			} catch (const std::exception& e) {
				std::cerr << "Error creating project folder: " << e.what() << std::endl;
			}
		});
	});
}

void Application::new_scene_action() {
	mUiCommon->hierarchy_panel()->fire_actor_selected_event(std::nullopt);
	mActorManager->clear_actors();
	
	mUiCommon->hierarchy_panel()->clear_actors();
		
	mRenderCommon->camera_actor_loader().setup_engine_camera(mGlobalAnimationTimeProvider,
															 45.0f, 0.01f, 1e5f,
															 mRenderCommon->canvas()->fixed_size().x() /
															 static_cast<float>(mRenderCommon->canvas()->fixed_size().y()));

}

void Application::save_scene_action() {
	nanogui::async([this]() {
		nanogui::file_dialog_async(
								   {{"pwr", "Scene Files"}}, true, false, [this](const std::vector<std::string>& files) {
									   if (files.empty()) {
										   return; // User canceled
									   }
									   
									   std::string destinationFile = files.front();
									   
									   mSerializationModule->save_scene(destinationFile);
								   });
	});

}

void Application::load_scene_action() {
	nanogui::async([this]() {
		nanogui::file_dialog_async(
								   {{"pwr", "Scene Files"}}, false, false, [this](const std::vector<std::string>& files) {
									   if (files.empty()) {
										   return; // User canceled
									   }
									   
									   
									   
									   std::string destinationFile = files.front();

									   mUiCommon->hierarchy_panel()->fire_actor_selected_event(std::nullopt);
									   
									   mActorManager->clear_actors();
									   
									   mUiCommon->hierarchy_panel()->clear_actors();

									   mSerializationModule->load_scene(destinationFile);
									   
									   mRenderCommon->camera_actor_loader().setup_engine_camera(mGlobalAnimationTimeProvider,
																								45.0f, 0.01f, 1e5f,
																								mRenderCommon->canvas()->fixed_size().x() /
																								static_cast<float>(mRenderCommon->canvas()->fixed_size().y()));

									   
									   mUiCommon->hierarchy_panel()->reload();

								   });
	});
}
