set(BUNDLE)
set(APP_ICON)
set(BUNDLE_MENU)

if(APPLE)
    set(BUNDLE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_LIST_DIR}/../../AppIcon.icns)

    # Specify the icon file location
    set(APP_ICON ${CMAKE_CURRENT_LIST_DIR}/../../AppIcon.icns)
    set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
        
    # Specify the menu file location
    set(BUNDLE_MENU ${CMAKE_CURRENT_LIST_DIR}/../platform/mac/MainMenu.xib)
    set_source_files_properties(${BUNDLE_MENU} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

# Add the shaders
file(GLOB SHADERS ${CMAKE_CURRENT_LIST_DIR}/shaders/*.vs ${CMAKE_CURRENT_LIST_DIR}/shaders/*.fs)

add_executable(PowerEngine
    ${BUNDLE}
    ${APP_ICON}
    ${BUNDLE_MENU}
    ${CMAKE_CURRENT_LIST_DIR}/Application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Application.hpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Canvas.hpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderManager.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderManager.hpp


    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/MaterialProperties.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/Texture.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/ShaderWrapper.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/shading/ShaderWrapper.cpp

    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Drawable.hpp

    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Mesh.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Mesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/SkinnedMesh.cpp

    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.hpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/drawing/Billboard.cpp

    ${CMAKE_CURRENT_LIST_DIR}/import/Fbx.hpp
    ${CMAKE_CURRENT_LIST_DIR}/import/Fbx.cpp

    ${CMAKE_CURRENT_LIST_DIR}/ui/ScenePanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/ScenePanel.cpp

    ${CMAKE_CURRENT_LIST_DIR}/ui/TransformPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/TransformPanel.cpp

    ${CMAKE_CURRENT_LIST_DIR}/ui/HierarchyPanel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HierarchyPanel.cpp

    ${SHADERS}
)

target_include_directories(PowerEngine PUBLIC ../../external/smallfbx)
target_include_directories(PowerEngine PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(PowerEngine PRIVATE nanogui ozz_animation EnTT imgui-feature-layout imgui-node-editor json fbx2ozz SmallFBX)

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

