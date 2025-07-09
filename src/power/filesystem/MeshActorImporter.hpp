#pragma once

#include "import/Fbx.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <sstream>

class Animation;

class MeshActorImporter {
public:
    // This struct will be the return type, containing the raw loaded data.
    struct FbxData {
        std::unique_ptr<Fbx> mModel;
        std::optional<std::vector<std::unique_ptr<Animation>>> mAnimations;
    };

    MeshActorImporter();

    // These methods will now return the FbxData struct directly.
    std::unique_ptr<FbxData> processFbx(const std::string& path);
    std::unique_ptr<FbxData> processFbx(std::stringstream& data, const std::string& modelName);
};
