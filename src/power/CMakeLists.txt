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
    ${CMAKE_CURRENT_LIST_DIR}/MetalHelper.mm
    ${CMAKE_CURRENT_LIST_DIR}/platform/Apple/ContextMenu.mm)

    target_include_directories(power_metal PUBLIC ${CMAKE_CURRENT_LIST_DIR}/platform)

    set(platform_specific_libraries power_metal)
    
    target_link_libraries(power_metal nanogui)

    set (PLATFORM_SPECIFIC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/Application.mm)
endif()


add_executable(PowerEngine
    ${BUNDLE}
    ${APP_ICON}
    ${BUNDLE_MENU}
    ${PLATFORM_SPECIFIC_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/Application.hpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraActorLoader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/CameraManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ICameraManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MeshActorLoader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MeshActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderCommon.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/UiCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/UiCommon.hpp

    ${CMAKE_CURRENT_LIST_DIR}/actors/IActorManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/IActorSelectedCallback.hpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/IActorSelectedRegistry.hpp

    ${CMAKE_CURRENT_LIST_DIR}/actors/Actor.hpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/ActorManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/actors/ActorManager.hpp
    
    ${CMAKE_CURRENT_LIST_DIR}/animation/Animation.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/AnimationTimeProvider.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/HeuristicSkeletonPoser.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/HeuristicSkeletonPoser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/IBone.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/ISkeleton.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/Skeleton.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/StackableAnimationTimeProvider.hpp
    ${CMAKE_CURRENT_LIST_DIR}/animation/Transform.hpp

    ${CMAKE_CURRENT_LIST_DIR}/components/AnimationComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/BlueprintComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/BlueprintComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/PrimitiveComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/PrimitiveComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedAnimationComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/SkeletonComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/TransformAnimationComponent.hpp
    #${CMAKE_CURRENT_LIST_DIR}/components/TimelineComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/CameraComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/CameraComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/Component.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/Component.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/ColorComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/DrawableComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/DrawableComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/MetadataComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/ModelMetadataComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/PlaybackComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/TransformComponent.hpp
    ${CMAKE_CURRENT_LIST_DIR}/components/UiComponent.hpp


    ${CMAKE_CURRENT_LIST_DIR}/components/MeshComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/MeshComponent.hpp

    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedMeshComponent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/components/SkinnedMeshComponent.hpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/BlueprintManager.hpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/BlueprintCanvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/BlueprintCanvas.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/BlueprintNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/BlueprintNode.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/NodeProcessor.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/NodeProcessor.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/KeyReleaseNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/KeyReleaseNode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/KeyPressNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/KeyPressNode.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/StringNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/StringNode.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/PrintNode.hpp
    ${CMAKE_CURRENT_LIST_DIR}/execution/PrintNode.cpp

    ${CMAKE_CURRENT_LIST_DIR}/execution/ExecutionManager.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/CompressedSerialization.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/DirectoryNode.hpp

    ${CMAKE_CURRENT_LIST_DIR}/filesystem/ImageUtils.hpp
    ${CMAKE_CURRENT_LIST_DIR}/filesystem/ImageUtils.cpp

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
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/ISkinnedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMeshBatch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMeshBatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/CameraActorBuilder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/CameraActorBuilder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshActorBuilder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/MeshActorBuilder.cpp

    ${CMAKE_CURRENT_LIST_DIR}/import/ModelImporter.hpp
    ${CMAKE_CURRENT_LIST_DIR}/import/ModelImporter.cpp

    ${CMAKE_CURRENT_LIST_DIR}/simulation/DebugBridgeCommon.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/DebugBridgeCommon.hpp

    ${CMAKE_CURRENT_LIST_DIR}/platform/ContextMenu.hpp

    ${CMAKE_CURRENT_LIST_DIR}/serialization/SceneSerializer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/serialization/SceneSerializer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/serialization/UUID.hpp

    ${CMAKE_CURRENT_LIST_DIR}/serialization/SerializationModule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/serialization/SerializationModule.hpp


    ${CMAKE_CURRENT_LIST_DIR}/simulation/Cartridge.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/ICartridge.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/ICartridgeActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/CartridgeActorLoader.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/CartridgeActorLoader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/PrimitiveBuilder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/PrimitiveBuilder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/Primitive.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/SimulationServer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/SimulationServer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/VirtualMachine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/simulation/VirtualMachine.hpp
    

    ${CMAKE_CURRENT_LIST_DIR}/ui/AnimationPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/AnimationPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/CameraPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/CameraPanel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/FileView.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/FileView.cpp
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

target_compile_definitions(PowerEngine PUBLIC -DASIO_STANDALONE)


target_include_directories(PowerEngine PUBLIC ../../external/asio/include)
target_include_directories(PowerEngine PUBLIC ../../external/wasmtime/include)
target_include_directories(PowerEngine PUBLIC ../../external/websocketpp)
target_include_directories(PowerEngine PUBLIC ../../external/smallfbx)
target_include_directories(PowerEngine PUBLIC ../../external/stb)
target_include_directories(PowerEngine PUBLIC ../../external/entt/single_include)
target_include_directories(PowerEngine PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_directories(PowerEngine PUBLIC ../../external/wasmtime/lib)

target_link_libraries(PowerEngine 
PRIVATE 
nanogui 
imgui-feature-layout 
imgui-node-editor 
json 
httplib::httplib 
OpenSSL::SSL
OpenSSL::Crypto
ozz_animation_tools
ozz_animation_fbx
riscv
wasmtime
zlib
assimp
#grpc++ 

${platform_specific_libraries})

# Set the resource files to be included in the bundle
set(RESOURCE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/resources
)
set(RESOURCE_FILES
    LICENSE.md
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

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/Application.mm PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)



file(GLOB FBS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/buffers/*.fbs")
set(GENERATED_HEADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/serialization")
file(MAKE_DIRECTORY ${GENERATED_HEADERS_DIR}) # Ensure this directory exists

# A list to hold the full paths of all generated headers.
set(GENERATED_HEADERS "")

foreach(FBS_FILE ${FBS_FILES})
  # Get the base filename without the extension (e.g., "monster_schema")
  get_filename_component(SCHEMA_NAME ${FBS_FILE} NAME_WE)

  # Define the full path for the output header.
  # Flatbuffers appends "_generated.h" to the schema filename.
  set(OUTPUT_HEADER "${GENERATED_HEADERS_DIR}/${SCHEMA_NAME}_generated.h")
  list(APPEND GENERATED_HEADERS ${OUTPUT_HEADER})

  # add_custom_command defines the rule to generate one header.
  add_custom_command(
    OUTPUT ${OUTPUT_HEADER}
    # COMMAND: The command to run.
    # We use the 'flatc' target to get the path to the compiled compiler.
    #   --cpp: Generate C++ code.
    #   -o:    Specify the output directory.
    #   ${FBS_FILE}: The input schema file.
    COMMAND
      $<TARGET_FILE:flatc> --cpp -o ${GENERATED_HEADERS_DIR} ${FBS_FILE}

    # DEPENDS: This command depends on the input .fbs file and the flatc compiler itself.
    # If either changes, the command will re-run.
    DEPENDS
      ${FBS_FILE}
      flatc

    # COMMENT: A message that will be printed during the build.
    COMMENT "Generating C++ header from ${FBS_FILE}..."
  )
endforeach()

# add_custom_target groups all the generated outputs into a single, dependable target.
# When another target depends on `generate_headers`, CMake ensures all custom commands
# associated with ${GENERATED_HEADERS} are executed first.
add_custom_target(
  generate_headers
  DEPENDS ${GENERATED_HEADERS}
)

add_dependencies(PowerEngine generate_headers)

target_include_directories(PowerEngine
  PUBLIC
    ${GENERATED_HEADERS_DIR}
)

target_link_libraries(PowerEngine
  PRIVATE
    flatbuffers
)

 
