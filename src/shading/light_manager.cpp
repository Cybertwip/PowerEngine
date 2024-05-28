#include "light_manager.h"
#include "shader.h" // Ensure you include your shader class here
#include <graphics/opengl/framebuffer.h>

#include "entity/entity.h"
#include "components/renderable/light_component.h"

LightManager* LightManager::instance = nullptr;

LightManager* LightManager::getInstance() {
	if (instance == nullptr) {
		instance = new LightManager();
	}
	return instance;
}

void LightManager::addPointLight(const PointLight& light) {
	pointLights.push_back(light);
}

const std::vector<LightManager::PointLight>& LightManager::getPointLights() const {
	return pointLights;
}

void LightManager::addDirectionalLight(std::shared_ptr<anim::Entity> entity) {
	directionalLights.push_back(entity);
}

const std::vector<std::shared_ptr<anim::Entity>>& LightManager::getDirectionalLights() const {
	return directionalLights;
}

void LightManager::updateLightParameters(anim::Shader& shader) {
	int numDirectionLights = static_cast<int>(directionalLights.size());
	shader.set_int("numDirectionLights", numDirectionLights);

	for (size_t i = 0; i < directionalLights.size(); ++i) {
		auto light = directionalLights[i]->get_component<anim::DirectionalLightComponent>();
		auto& parameters = light->get_parameters();
		
		// Note: Ensure your shader class has these methods defined or adjust accordingly
		shader.set_vec3("dir_lights[" + std::to_string(i) + "].position", parameters.position);
		shader.set_vec3("dir_lights[" + std::to_string(i) + "].direction", parameters.direction);
		shader.set_vec3("dir_lights[" + std::to_string(i) + "].ambient", parameters.ambient);
		shader.set_vec3("dir_lights[" + std::to_string(i) + "].diffuse", parameters.diffuse);
		shader.set_vec3("dir_lights[" + std::to_string(i) + "].specular", parameters.specular);
	}
	
	int numPointLights = static_cast<int>(pointLights.size());
	shader.set_int("numPointLights", numPointLights);
	
	for (int i = 0; i < numPointLights; ++i) {
		std::string iStr = std::to_string(i); // Convert loop index to string for dynamic uniform names
		
		shader.set_vec3("pointLights[" + iStr + "].position", pointLights[i].position);
		shader.set_vec3("pointLights[" + iStr + "].ambient", pointLights[i].ambient);
		shader.set_vec3("pointLights[" + iStr + "].diffuse", pointLights[i].diffuse);
		shader.set_vec3("pointLights[" + iStr + "].specular", pointLights[i].specular);
		shader.set_float("pointLights[" + iStr + "].constant", pointLights[i].constant);
		shader.set_float("pointLights[" + iStr + "].linear", pointLights[i].linear);
		shader.set_float("pointLights[" + iStr + "].quadratic", pointLights[i].quadratic);
	}
}
