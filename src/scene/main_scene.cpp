#include "main_scene.h"
#include <glcpp/camera.h>
#include <entity/entity.h>
#include <graphics/opengl/image.h>
#include <graphics/opengl/framebuffer.h>
#include <graphics/shader.h>

#include <util/log.h>
#include <graphics/shader.h>
#include <graphics/opengl/image.h>
#include <graphics/opengl/grid.h>
#include <graphics/opengl/framebuffer.h>

#include <imgui.h>

#include "UI/ui_context.h"

#include "../entity/components/component.h"
#include "../entity/components/pose_component.h"
#include "../entity/components/renderable/mesh_component.h"
#include "../entity/components/renderable/light_component.h"

#include "physics/PhysicsWorld.h"

#include "utility.h"
#include "shading/light_manager.h"
#include "scene/scene.hpp"

#include <filesystem>
#include <graphics/post_processing.h>



namespace {
const float SHADOW_SIZE = 1024;

}

namespace fs = std::filesystem;

MainScene::MainScene(uint32_t width, uint32_t height)
{
	anim::LOG("MainScene()");
	resources_ = std::make_shared<anim::SharedResources>(static_cast<Scene&>(*this));
	resources_->initialize();
	
	init_shadow_map(get_mutable_shared_resources()->get_light_manager());
	
	
	init_framebuffer(width, height);
	grid_framebuffer_.reset(new anim::Grid());

	init_camera();
	
	resources_->create_light();

	LightManager::PointLight keyLight = {
		glm::vec3(2.0f, 2.0f, 2.0f), // Position to the right, above, and in front of the subject
		glm::vec3(0.05f), // Ambient
		glm::vec3(0.8f), // Diffuse
		glm::vec3(1.0f), // Specular
		1.0f, // Constant attenuation
		0.09f, // Linear attenuation
		0.032f // Quadratic attenuation
	};
	get_mutable_shared_resources()->get_light_manager().addPointLight(keyLight);

	LightManager::PointLight fillLight = {
		glm::vec3(-2.0f, 0.5f, 2.0f), // Position to the left, slightly above, and in front of the subject
		glm::vec3(0.05f), // Ambient
		glm::vec3(0.5f), // Diffuse
		glm::vec3(0.5f), // Specular
		1.0f, // Constant attenuation
		0.09f, // Linear attenuation
		0.032f // Quadratic attenuation
	};
	get_mutable_shared_resources()->get_light_manager().addPointLight(fillLight);
	
	LightManager::PointLight backLight = {
		glm::vec3(0.0f, 2.0f, -2.0f), // Position directly behind and above the subject
		glm::vec3(0.05f), // Ambient
		glm::vec3(0.6f), // Diffuse
		glm::vec3(0.7f), // Specular
		1.0f, // Constant attenuation
		0.09f, // Linear attenuation
		0.032f // Quadratic attenuation
	};
	get_mutable_shared_resources()->get_light_manager().addPointLight(backLight);
	_physics_world = physics::PhysicsWorld::create();

	
	set_size(width, height);

}

void MainScene::init_shadow_map(LightManager& lightManager) {
	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	// Define variables for shadow map texture and framebuffer
	GLuint shadowMapFBO, shadowMap;
	
	const int shadowMapTextureUnit = 15; // Example texture unit dedicated for shadow map

	// Generate framebuffer and texture IDs
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &shadowMap);
	
	glActiveTexture(GL_TEXTURE0 + shadowMapTextureUnit);

	glBindTexture(GL_TEXTURE_2D, shadowMap);
	
	// Specify texture parameters
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	
	// Bind the framebuffer and attach the depth texture to it
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	
	// Set draw and read buffers to none since we only need depth information
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	
	// Unbind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// Store the shadow map texture ID and framebuffer ID for later use
	this->shadowMap = shadowMap;
	this->shadowMapFBO = shadowMapFBO;
	
	// Set the shadow map texture ID in the LightManager for later use
	lightManager.setShadowMapTextureID(shadowMap);
}


void MainScene::init_framebuffer(uint32_t width, uint32_t height)
{
	framebuffer_.reset(new anim::Framebuffer(
											 anim::FramebufferSpec(
																   width,
																   height,
																   1,
																   {anim::FramebufferTextureFormat::RGBA8,anim::FramebufferTextureFormat::RED_INTEGER,
															   anim::FramebufferTextureFormat::RED_INTEGER},				   anim::FramebufferTextureFormat::Depth)));
	if (framebuffer_->error())
	{
		anim::LOG("framebuffer error");
		framebuffer_.reset(new anim::Framebuffer{anim::FramebufferSpec(width,
																	   height,
																	   1,
																	   {anim::FramebufferTextureFormat::RGBA8},
																	   anim::FramebufferTextureFormat::Depth)});
	}
	
	
	entity_framebuffer_.reset(new anim::Framebuffer(
											 anim::FramebufferSpec(
																   width,
																   height,
																   1,
																   {anim::FramebufferTextureFormat::RGBA8,anim::FramebufferTextureFormat::RED_INTEGER, anim::FramebufferTextureFormat::RED_INTEGER},				   anim::FramebufferTextureFormat::Depth)));
	if (entity_framebuffer_->error())
	{
		anim::LOG("framebuffer error");
		entity_framebuffer_.reset(new anim::Framebuffer{anim::FramebufferSpec(width,
																	   height,
																	   1,
																	   {anim::FramebufferTextureFormat::RGBA8},
																	   anim::FramebufferTextureFormat::Depth)});
	}

}

void MainScene::init_camera()
{
	auto camera = resources_->create_camera();
	camera_ = camera;
}

void MainScene::render_to_shadow_map(ui::UiContext& ui_context) {
	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	// Retrieve the first directional light as the primary shadow caster
	auto& directionalLights = get_mutable_shared_resources()->get_light_manager().getDirectionalLights();
	if (directionalLights.empty()) return; // Early return if no directional lights
	
	const auto& entity = directionalLights[0];
	const auto& light = entity->get_component<anim::DirectionalLightComponent>()->get_parameters(); // Assuming the first directional light is used for shadows
	const auto& transform = entity->get_component<anim::TransformComponent>();

	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 0.01f, far_plane = 1000.0f;
	lightProjection = glm::ortho(-SHADOW_SIZE, SHADOW_SIZE, -SHADOW_SIZE, SHADOW_SIZE, near_plane, far_plane);

	glm::vec3 lightPos = transform->get_translation();
	
	auto [t, r, s] = anim::DecomposeTransform(transform->get_mat4());
	

	// The base "down" vector
	glm::vec3 down = glm::vec3(0.0f, 1.0f, 0.0f);
	
	// Rotate the base "down" vector by the quaternion to get the actual "down" direction
	glm::vec3 downDirection = r * down;
	
	// Now use the downDirection to compute the target position
	glm::vec3 targetPosition = lightPos + downDirection; // Here we add because downDirection is already in the correct direction from lightPos

	// Construct the view matrix by combining translation and rotation
	lightView = glm::lookAt(lightPos, targetPosition, glm::vec3(0.0, 0.0, -1.0f));

	lightSpaceMatrix = lightProjection * lightView;

	// Bind the framebuffer for the shadow map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.0f);

	glEnable(GL_DEPTH_TEST);

	auto shadow_shader = resources_->get_mutable_shader("shadow");
	
	shadow_shader->use();
	shadow_shader->set_mat4("lightSpaceMatrix", lightSpaceMatrix);

	if(!ui_context.scene.is_trigger_update_keyframe){
		resources_->get_mutable_animator()->update_sequencer(resources_->get_scene(), ui_context);
	}
	
	if(resources_->get_root_entity()){
		auto children = resources_->get_root_entity()->get_children_recursive();
		
		for(auto& child : children) {
			if(auto component = child->get_component<anim::MeshComponent>(); component){
				if(!component->isInterface){
					component->draw_shadow(child, *shadow_shader);
				}
			}
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the shadow map framebuffer
	glViewport(0, 0, framebuffer_->get_width(), framebuffer_->get_height()); // Restore the viewport

}

void MainScene::pre_draw(ui::UiContext& ui_context)
{
	update_framebuffer();
	auto camera = camera_->get_component<anim::CameraComponent>();
	camera->set_view_and_projection(framebuffer_->get_aspect());
	resources_->set_ubo_view(camera->get_view());
	resources_->set_ubo_projection(camera->get_projection());
	resources_->set_ubo_position(camera->get_position());
	render_to_shadow_map(ui_context);
}

void MainScene::update_framebuffer()
{
//	if (framebuffer_->get_width() != width_ || framebuffer_->get_height() != height_)
//	{
//		init_framebuffer(width_, height_);
//	}
}
void MainScene::draw_to_framebuffer(ui::UiContext& ui_context)
{
	auto grid_shader = resources_->get_mutable_shader("grid");
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glDepthMask(true);
	
	entity_framebuffer_->bind_with_depth_and_stencil(background_color_);
	resources_->update(ui_context, false);
	entity_framebuffer_->unbind();
	
	framebuffer_->bind_with_depth_and_stencil(background_color_);
	
	// Enable stencil test for grid drawing
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // Set stencil to 1 where drawing occurs
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // Replace stencil value on drawing
	glStencilMask(0xFF); // Enable stencil writing
	
	glStencilMask(0x00); // Disable stencil writing

	// Draw the grid
	if (!resources_->get_is_exporting()) {
		
		auto camera_entity = resources_->get_scene().get_active_camera();
		
		auto camera = camera_entity->get_component<anim::CameraComponent>();
		
		grid_framebuffer_->draw(*grid_shader, *camera);
	}
	
	
	auto shadow_shader = resources_->get_mutable_shader("animation");
	
	auto& directionalLights = get_mutable_shared_resources()->get_light_manager().getDirectionalLights();
	
	for(auto entity : directionalLights){
		const auto& transform = entity->get_component<anim::TransformComponent>();
		
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 0.01f, far_plane = 1000.0f;
		lightProjection = glm::ortho(-SHADOW_SIZE, SHADOW_SIZE, -SHADOW_SIZE, SHADOW_SIZE, near_plane, far_plane);
		
		glm::vec3 lightPos = transform->get_translation();
		auto [t, r, s] = anim::DecomposeTransform(transform->get_mat4());
		
		// The base "down" vector
		glm::vec3 down = glm::vec3(0.0f, 1.0f, 0.0f);
		
		// Rotate the base "down" vector by the quaternion to get the actual "down" direction
		glm::vec3 downDirection = r * down;
		
		// Now use the downDirection to compute the target position
		glm::vec3 targetPosition = lightPos + downDirection; // Here we add because downDirection is already in the correct direction from lightPos
		
		// Construct the view matrix by combining translation and rotation
		lightView = glm::lookAt(lightPos, targetPosition, glm::vec3(0.0, 0.0, -1.0f));
		
		lightSpaceMatrix = lightProjection * lightView;
		
		shadow_shader->use();
		shadow_shader->set_mat4("lightSpaceMatrix", lightSpaceMatrix);
		
		break;
	}
	
	resources_->update(ui_context, !ui_context.scene.is_trigger_update_keyframe);
	
	anim::OutlineInfo cellShading;
	cellShading.color = glm::vec3{0.0f, 0.0f, 0.0f};
	cellShading.width = 1.15f;
	cellShading.threshold = 0.8f;
	resources_->mPostProcessing->execute_outline_with_depth_always(framebuffer_.get(), 1, 0xFF, cellShading);
	
	if (selected_entity_)
	{
		anim::OutlineInfo outlineShading;
		
		outlineShading.width = 1.0f;
		outlineShading.threshold = 0.25f;
		
		resources_->mPostProcessing->execute_outline_with_depth_always(framebuffer_.get(), 2, 0xFF, outlineShading);
	}
	
	glClear(GL_STENCIL_BUFFER_BIT);
	
	framebuffer_->unbind();
	
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	
#ifndef NDEBUG
	auto error = glGetError();
	anim::LOG("main scene: " + std::to_string((int)error), error == GL_NO_ERROR);
#endif
}

void MainScene::draw(ui::UiContext& ui_context)
{
	draw_to_framebuffer(ui_context);
}

void MainScene::picking(int x, int y, bool is_edit_mode, bool is_bone_picking_mode)
{
		// @TODO setup for 1280 x 720 frame buffer size
	
	int32_t pixel_i = 0;
	
	if(is_edit_mode){
		pixel_i = entity_framebuffer_->read_pixel(x, y, 2);
	} else {
		pixel_i = entity_framebuffer_->read_pixel(x, y, 1);
	}
	
	//    anim::LOG(std::to_string(pixel_i) + ": " + std::to_string(pixel_x) + " " + std::to_string(pixel_y));
	int32_t pick_id = pixel_i;
	
	int32_t pixel_x = pixel_i >> 8;

	
	if (pick_id == 0x00000000){
		set_selected_entity(nullptr);
	}
	else
	{
		std::shared_ptr<anim::Entity> entity = nullptr;
		
		entity = resources_->get_entity(pixel_x);
		
		if (entity){
			if(is_edit_mode){
				set_selected_entity(nullptr);
				set_selected_entity(entity);
			} else {
				set_selected_entity(nullptr);
				set_selected_entity(entity->get_mutable_root());
			}
		} else {
			set_selected_entity(nullptr);
		}
	}
}
