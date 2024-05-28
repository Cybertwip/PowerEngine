#ifndef SRC_ANIM_RESOURCES_EXPORTEER_H
#define SRC_ANIM_RESOURCES_EXPORTEER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <json/json.h>

#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <string>

struct aiScene;
struct aiNode;

namespace anim
{
    class Model;
    class Animation;
    class Framebuffer;
    class Entity;
    class SharedResources;
    class Exporter
    {
    public:
        Exporter() = default;
        ~Exporter() = default;
        void to_png(Framebuffer *framebuffer, const char *save_path);
        void to_json(std::shared_ptr<Entity> entity, const char *save_path);
         std::string to_json(std::shared_ptr<Entity> model);

        void to_glft2(std::shared_ptr<Entity> entity, const char *save_path, const char *model_path);

        bool is_linear_{true};

    private:
        Json::Value dfs(std::shared_ptr<Entity> node, const std::string &parent_name);

        void to_ai_node(aiNode *ai_node, std::shared_ptr<Entity> entity, aiNode *parent_ai_node = nullptr);
    };
    Json::Value get_quat_json(const glm::quat &r);
    Json::Value get_vec_json(const glm::vec3 &p);
    void to_json_all_animation_data(const char *save_path, std::shared_ptr<Entity> entity, SharedResources *resources);
}

#endif
