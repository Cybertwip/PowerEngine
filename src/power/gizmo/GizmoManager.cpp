#include "GizmoManager.hpp"
#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"
#include <nanogui/icons.h>
#include <nanogui/opengl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

// Helper to convert GLM matrix to NanoGUI matrix
static nanogui::Matrix4f glm_to_nanogui(const glm::mat4& m) {
	nanogui::Matrix4f result;
	memcpy(result.m, glm::value_ptr(m), 16 * sizeof(float));
	return result;
}

GizmoManager::GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager)
: mActorManager(actorManager) {
	
	// Create or get a simple shader for drawing colored gizmos
	if (!shaderManager.shader_exists("gizmo_shader")) {
		// A simple pass-through shader
		const std::string vs = R"(
			#version 330 core
			layout (location = 0) in vec3 aPos;
			uniform mat4 model;
			uniform mat4 view;
			uniform mat4 projection;
			void main() {
				gl_Position = projection * view * model * vec4(aPos, 1.0);
			}
		)";
		const std::string fs = R"(
			#version 330 core
			out vec4 FragColor;
			uniform vec4 color;
			void main() {
				FragColor = color;
			}
		)";
		shaderManager.create_shader("gizmo_shader", vs, fs);
	}
	mGizmoShader = std::make_unique<ShaderWrapper>(shaderManager.get_shader("gizmo_shader"));
	
	create_procedural_gizmos();
	
}

GizmoManager::~GizmoManager() {
	cleanup();
}

void GizmoManager::cleanup() {
	for (int i = 0; i < 3; ++i) {
		glDeleteVertexArrays(1, &mTranslationMeshes[i].vao);
		glDeleteBuffers(1, &mTranslationMeshes[i].vbo);
		glDeleteVertexArrays(1, &mRotationMeshes[i].vao);
		glDeleteBuffers(1, &mRotationMeshes[i].vbo);
		glDeleteVertexArrays(1, &mScaleMeshes[i].vao);
		glDeleteBuffers(1, &mScaleMeshes[i].vbo);
	}
}

void GizmoManager::create_procedural_gizmos() {
	create_translation_gizmo();
	create_rotation_gizmo();
	create_scale_gizmo();
}

// --- GEOMETRY CREATION ---

void GizmoManager::create_translation_gizmo() {
	const float line_length = 1.0f;
	const float cone_height = 0.2f;
	const float cone_radius = 0.08f;
	const int cone_segments = 12;
	
	for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
		std::vector<glm::vec3> vertices;
		glm::vec3 axis(i == 0, i == 1, i == 2);
		
		// Line part
		vertices.push_back(glm::vec3(0.0f));
		vertices.push_back(axis * line_length);
		
		// Cone part
		glm::vec3 cone_base = axis * line_length;
		glm::vec3 up(0,1,0);
		if (i == 1) up = glm::vec3(-1,0,0); // If axis is Y, choose a different 'up'
		glm::vec3 p1 = glm::normalize(glm::cross(axis, up));
		glm::vec3 p2 = glm::normalize(glm::cross(axis, p1));
		
		for (int j = 0; j < cone_segments; ++j) {
			float angle1 = 2.0f * M_PI * j / cone_segments;
			float angle2 = 2.0f * M_PI * (j + 1) / cone_segments;
			glm::vec3 c1 = cone_base + (p1 * cos(angle1) + p2 * sin(angle1)) * cone_radius;
			glm::vec3 c2 = cone_base + (p1 * cos(angle2) + p2 * sin(angle2)) * cone_radius;
			vertices.push_back(c1);
			vertices.push_back(c2);
			vertices.push_back(cone_base + axis * cone_height);
		}
		
		mTranslationMeshes[i].vertex_count = vertices.size();
		glGenVertexArrays(1, &mTranslationMeshes[i].vao);
		glGenBuffers(1, &mTranslationMeshes[i].vbo);
		glBindVertexArray(mTranslationMeshes[i].vao);
		glBindBuffer(GL_ARRAY_BUFFER, mTranslationMeshes[i].vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}
}

void GizmoManager::create_rotation_gizmo() {
	const int segments = 64;
	for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
		std::vector<glm::vec3> vertices;
		for (int j = 0; j <= segments; ++j) {
			float angle = 2.0f * M_PI * j / segments;
			float x = cos(angle);
			float y = sin(angle);
			if (i == 0) vertices.push_back({0, x, y}); // X-axis ring on YZ plane
			if (i == 1) vertices.push_back({x, 0, y}); // Y-axis ring on XZ plane
			if (i == 2) vertices.push_back({x, y, 0}); // Z-axis ring on XY plane
		}
		
		mRotationMeshes[i].vertex_count = vertices.size();
		glGenVertexArrays(1, &mRotationMeshes[i].vao);
		glGenBuffers(1, &mRotationMeshes[i].vbo);
		glBindVertexArray(mRotationMeshes[i].vao);
		glBindBuffer(GL_ARRAY_BUFFER, mRotationMeshes[i].vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}
}

void GizmoManager::create_scale_gizmo() {
	const float line_length = 1.0f;
	const float cube_size = 0.1f;
	for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
		std::vector<glm::vec3> vertices;
		glm::vec3 axis(i == 0, i == 1, i == 2);
		
		// Line
		vertices.push_back(glm::vec3(0.0f));
		vertices.push_back(axis * line_length);
		
		// Cube vertices
		glm::vec3 c = axis * line_length;
		glm::vec3 u(i == 1 ? 1:0, i == 1 ? 0:1, 0);
		glm::vec3 v = glm::cross(axis, u);
		u *= cube_size; v *= cube_size;
		
		glm::vec3 points[8] = {
			c - u - v, c + u - v, c + u + v, c - u + v,
			c - u - v + axis*cube_size*2, c + u - v + axis*cube_size*2,
			c + u + v + axis*cube_size*2, c - u + v + axis*cube_size*2
		};
		int indices[] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
		for(int idx : indices) vertices.push_back(points[idx]);
		
		mScaleMeshes[i].vertex_count = vertices.size();
		glGenVertexArrays(1, &mScaleMeshes[i].vao);
		glGenBuffers(1, &mScaleMeshes[i].vbo);
		glBindVertexArray(mScaleMeshes[i].vao);
		glBindBuffer(GL_ARRAY_BUFFER, mScaleMeshes[i].vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}
}

// --- LOGIC ---

void GizmoManager::select(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
}

void GizmoManager::set_mode(GizmoMode mode) {
	mCurrentMode = mode;
}

void GizmoManager::select_axis(GizmoAxis gizmoId) {
	mActiveAxis = gizmoId;
}

void GizmoManager::draw(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	if (!mActiveActor.has_value() || mCurrentMode == GizmoMode::None) {
		return;
	}
	
	auto& actor = mActiveActor->get();
	auto& transform = actor.get_component<TransformComponent>();
	
	// Gizmo model matrix calculation
	glm::vec3 actor_pos = transform.get_translation();
	
	// Constant screen size scaling
	auto view_inv = view.inverse();
	glm::vec3 cam_pos(view_inv(0, 3), view_inv(1, 3), view_inv(2, 3));
	float distance = glm::distance(cam_pos, actor_pos);
	float scale_factor = distance * 0.1f; // Tweak this factor for desired on-screen size
	
	glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
	glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), actor_pos);
	
	// For rotation gizmo, align with the object's rotation
	glm::mat4 rot_mat = glm::mat4(1.0f);
	if (mCurrentMode == GizmoMode::Rotation) {
		rot_mat = glm::mat4_cast(transform.get_rotation());
	}
	
	glm::mat4 model = trans_mat * rot_mat * scale_mat;
	
	// --- Drawing ---
	mGizmoShader->bind();
	mGizmoShader->set_uniform("view", view);
	mGizmoShader->set_uniform("projection", projection);
	mGizmoShader->set_uniform("model", glm_to_nanogui(model));
	
	// Select which gizmo mesh set to use
	GizmoMesh* active_meshes = nullptr;
	if (mCurrentMode == GizmoMode::Translation) active_meshes = mTranslationMeshes;
	else if (mCurrentMode == GizmoMode::Rotation) active_meshes = mRotationMeshes;
	else if (mCurrentMode == GizmoMode::Scale) active_meshes = mScaleMeshes;
	
	// Save previous GL state
	GLboolean was_depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST); // Draw gizmo on top of everything
	
	// Draw each axis with its color
	for (int i = 0; i < 3; i++) {
		glm::vec4 color(i == 0, i == 1, i == 2, 1.0f); // R, G, B
		mGizmoShader->set_uniform("color", nanogui::Color(color.r, color.g, color.b, 1.0f));
		
		glBindVertexArray(active_meshes[i].vao);
		if (mCurrentMode == GizmoMode::Translation && i==0) { // X Arrow
			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLES, 2, active_meshes[i].vertex_count - 2);
		} else if(mCurrentMode == GizmoMode::Rotation) {
			glDrawArrays(GL_LINE_STRIP, 0, active_meshes[i].vertex_count);
		} else { // Fallback for other modes / axes
			glDrawArrays(GL_LINES, 0, active_meshes[i].vertex_count);
		}
	}
	glBindVertexArray(0);
	
	// Restore GL state
	if (was_depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
	}
}


// --- TRANSFORMATION LOGIC (Unchanged from original post) ---

void GizmoManager::translate(float dx, float dy) {
	auto& actor = mActiveActor.value().get();
	auto& transform = actor.get_component<TransformComponent>();
	auto translation = transform.get_translation();
	switch (mActiveAxis) {
		case GizmoAxis::X: translation.x += dx; break;
		case GizmoAxis::Y: translation.y += dy; break;
		case GizmoAxis::Z: translation.z -= dy; break;
		default: break;
	}
	transform.set_translation(translation);
}

void GizmoManager::rotate(float dx, float dy) {
	float angle = ((dx + dy) / 2.0f);
	auto& actor = mActiveActor.value().get();
	auto& transform = actor.get_component<TransformComponent>();
	switch (mActiveAxis) {
		case GizmoAxis::X: transform.rotate(glm::vec3(1.0f, 0.0f, 0.0f), -angle); break;
		case GizmoAxis::Y: transform.rotate(glm::vec3(0.0f, 1.0f, 0.0f), angle); break;
		case GizmoAxis::Z: transform.rotate(glm::vec3(0.0f, 0.0f, -1.0f), -angle); break;
		default: break;
	}
}

void GizmoManager::scale(float dx, float dy) {
	auto& actor = mActiveActor.value().get();
	auto& transform = actor.get_component<TransformComponent>();
	auto scale = transform.get_scale();
	switch (mActiveAxis) {
		case GizmoAxis::X: scale.x += dx; break;
		case GizmoAxis::Y: scale.y += dy; break;
		case GizmoAxis::Z: scale.z -= dy; break;
		default: break;
	}
	transform.set_scale(scale);
}

void GizmoManager::transform(float dx, float dy) {
	if (mActiveActor.has_value() && mActiveAxis != GizmoAxis::None) {
		switch (mCurrentMode) {
			case GizmoMode::Translation: translate(dx, dy); break;
			case GizmoMode::Rotation: rotate(dx, dy); break;
			case GizmoMode::Scale: scale(dx * 0.01f, dy * 0.01f); break;
			default: break;
		}
	}
}
