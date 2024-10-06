set(BUNDLE)
set(APP_ICON)
set(BUNDLE_MENU)

if(APPLE)
    set(BUNDLE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_LIST_DIR}/../../AppIcon.icns)

#Specify the icon file location
    set(APP_ICON ${CMAKE_CURRENT_LIST_DIR}/../../AppIcon.icns)
    set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")

#Specify the menu file location
    set(BUNDLE_MENU ${CMAKE_CURRENT_LIST_DIR}/../platform/mac/MainMenu.xib)
    set_source_files_properties(${BUNDLE_MENU} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

#Add the shaders
file(GLOB SHADERS ${CMAKE_CURRENT_LIST_DIR}/shaders/*.vs ${CMAKE_CURRENT_LIST_DIR}/shaders/*.fs)

set(platform_specific_libraries)

if(APPLE)
    add_library(power_metal
    ${CMAKE_CURRENT_LIST_DIR}/MetalHelper.hpp
    ${CMAKE_CURRENT_LIST_DIR}/MetalHelper.mm)

    set(platform_specific_libraries power_metal)
    
    target_link_libraries(power_metal nanogui)
endif()


add_executable(PowerEngine
    ${BUNDLE}
    ${APP_ICON}
    ${BUNDLE_MENU}
    ${PLATFORM_SPECIFIC_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/Application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Application.hpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraActorLoader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MeshActorLoader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MeshActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderCommon.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/UiCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/UiCommon.hpp

    ${CMAKE_CURRENT_LIST_DIR}/ai/DeepMotionSettingsWindow.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ai/DeepMotionSettingsWindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ai/DeepMotionApiClient.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ai/DeepMotionApiClient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ai/PromptWindow.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ai/PromptWindow.cpp

    ${CMAKE_CURRENT_LIST_DIR}/actors/IActorSelectedCallback.hpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/IActorSelectedRegistry.hpp

    ${CMAKE_CURRENT_LIST_DIR}/actors/Actor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/Actor.hpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/ActorManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/ActorManager.hpp
    
    ${CMAKE_CURRENT_LIST_DIR}/animation/Animation.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/AnimationTimeProvider.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/Skeleton.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/StackableAnimationTimeProvider.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/Transform.hpp

    ${CMAKE_CURRENT_LIST_DIR}/components/AnimationComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedAnimationComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/TransformAnimationComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/TimelineComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/CameraComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/CameraComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/Component.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/Component.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/ColorComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/ColorComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/DrawableComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/DrawableComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/MetadataComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/PlaybackComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/TransformComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/UiComponent.hpp


    ${CMAKE_CURRENT_LIST_DIR}/components/MeshComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/MeshComponent.hpp

    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedMeshComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedMeshComponent.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/CompressedSerialization.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/DirectoryNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/DirectoryNode.cpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/ImageUtils.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/ImageUtils.cpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshActorImporter.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshActorImporter.cpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshActorExporter.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshActorExporter.cpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshDeserializer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/MeshDeserializer.cpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/VectorConversion.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/UrlOpener.hpp

    ${CMAKE_CURRENT_LIST_DIR}/gizmo/GizmoManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/gizmo/GizmoManager.cpp

    ${CMAKE_CURRENT_LIST_DIR}/grok/Client.hpp
    ${CMAKE_CURRENT_LIST_DIR}/grok/Client.cpp

    ${CMAKE_CURRENT_LIST_DIR}/grok/Message.hpp

    ${CMAKE_CURRENT_LIST_DIR}/grok/PromptBox.hpp
    ${CMAKE_CURRENT_LIST_DIR}/grok/PromptBox.cpp

    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MaterialProperties.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MaterialProperties.cpp    
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MeshData.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MeshVertex.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MeshVertex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/ShaderWrapper.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/ShaderWrapper.cpp


    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Batch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Batch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/BatchUnit.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Drawable.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Grid.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Grid.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/NullDrawable.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Mesh.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Mesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/IMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshBatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedMeshBatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedMeshCanvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedMeshCanvas.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SharedSelfContainedMeshCanvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SharedSelfContainedMeshCanvas.cpp    
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/ISkinnedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMeshBatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedSkinnedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SelfContainedSkinnedMeshBatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/CameraActorBuilder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/CameraActorBuilder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshActorBuilder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshActorBuilder.cpp

    ${CMAKE_CURRENT_LIST_DIR}/import/Fbx.hpp
    ${CMAKE_CURRENT_LIST_DIR}/import/Fbx.cpp

    ${CMAKE_CURRENT_LIST_DIR}/import/SkinnedFbx.hpp
    ${CMAKE_CURRENT_LIST_DIR}/import/SkinnedFbx.cpp


    ${CMAKE_CURRENT_LIST_DIR}/ui/AnimationPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/AnimationPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HierarchyPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HierarchyPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ImportWindow.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ImportWindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/MeshPicker.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/MeshPicker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/Panel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/Panel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ResourcesPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ResourcesPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ScenePanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ScenePanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/SceneTimeBar.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/SceneTimeBar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/StatusBarPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/StatusBarPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/TransformPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/TransformPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/UiManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/UiManager.cpp

    ${SHADERS}
)

find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto SSL)

target_include_directories(PowerEngine PUBLIC ../../external/smallfbx)
target_include_directories(PowerEngine PUBLIC ../../external/stb)
target_include_directories(PowerEngine PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(PowerEngine 
PRIVATE 
nanogui 
EnTT 
imgui-feature-layout 
imgui-node-editor 
json 
SmallFBX 
httplib::httplib 
OpenSSL::SSL
OpenSSL::Crypto
ozz_animation_tools
ozz_animation_fbx
zlib
#grpc++ 
${platform_specific_libraries})

# Set the resource files to be included in the bundle
set(RESOURCE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/resources
)
set(RESOURCE_FILES
    imgui.ini
    LICENSE
)

# Copy resource and shader files to the binary directory after the build
foreach(resource_file ${RESOURCE_DIRECTORIES})
    add_custom_command(TARGET PowerEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${resource_file} $<TARGET_FILE_DIR:PowerEngine>/../Resources)
endforeach()

foreach(resource_file ${RESOURCE_FILES})
    add_custom_command(TARGET PowerEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/${resource_file} $<TARGET_FILE_DIR:PowerEngine>/../Resources/${resource_file})
endforeach()

# Copy shader files
foreach(shader_file ${SHADERS})
    add_custom_command(TARGET PowerEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${shader_file} $<TARGET_FILE_DIR:PowerEngine>/shaders/)
endforeach()

if(APPLE)
    # Set macOS bundle properties
    set_target_properties(PowerEngine PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist  # Provide your Info.plist file
    )

    # Disable code signing for the entire project
    set_target_properties(PowerEngine PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )

    set_target_properties(PowerEngine PROPERTIES 
                      XCODE_GENERATE_SCHEME TRUE
                      XCODE_SCHEME_WORKING_DIRECTORY  "$<TARGET_FILE_DIR:PowerEngine>/../Resources")


    # Set release build type and enable optimizations
    set(CMAKE_BUILD_TYPE Release)
endif()

set_target_properties(json PROPERTIES UNITY_BUILD ON)
set_target_properties(PowerEngine PROPERTIES UNITY_BUILD ON)

 