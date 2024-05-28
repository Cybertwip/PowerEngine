#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include <graphics/opengl/framebuffer.h>


// Forward declaration to avoid including the entire shader header
namespace anim {
class Shader;
class Entity;
}

class LightManager {
public:
	struct DirectionalLight {
		glm::vec3 position;
		glm::vec3 direction;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
	};
	
	struct PointLight {
		glm::vec3 position;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		float constant;
		float linear;
		float quadratic;
	};
	
	
private:
	std::vector<std::shared_ptr<anim::Entity>> directionalLights;
	std::vector<PointLight> pointLights;
	static LightManager* instance;
	
	// Existing member variables and methods...
	GLuint shadowMapTextureID = 0; // Add this line to store shadow map texture ID

	
	// Private constructor for singleton
	LightManager() {}
	
public:
	// Delete copy constructor and assignment operator
	LightManager(const LightManager&) = delete;
	LightManager& operator=(const LightManager&) = delete;
	
	
	int getShadowMapTextureId() { return shadowMapTextureID;
	}
	// Existing public methods...
	void setShadowMapTextureID(GLuint textureID) { shadowMapTextureID = textureID; } // Method to set the texture ID
	
	void clearLights(){
		directionalLights.clear();
	}
	
	void removeDirectionalLight(std::shared_ptr<anim::Entity> light){
		auto it = std::find(directionalLights.begin(), directionalLights.end(), light);
		
		if(it != directionalLights.end()){
			directionalLights.erase(it);
		}
	}

	static LightManager* getInstance();
	void addPointLight(const PointLight& light);
	const std::vector<PointLight>& getPointLights() const;
	void addDirectionalLight(std::shared_ptr<anim::Entity> entity);
	const std::vector<std::shared_ptr<anim::Entity>>& getDirectionalLights() const;
	void updateLightParameters(anim::Shader& shader);
};

#endif // LIGHTMANAGER_H
