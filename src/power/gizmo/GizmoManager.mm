#import "GizmoManager.hpp"
#import "actors/Actor.hpp"
#import "components/TransformComponent.hpp"

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <nanogui/icons.h>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <vector>

// Helper utility to convert a GLM matrix to a Metal-compatible SIMD matrix.
// Both are column-major, so a direct memory copy is efficient and safe.
static simd_float4x4 glm_to_simd(const glm::mat4& m) {
    return *(reinterpret_cast<const simd_float4x4*>(glm::value_ptr(m)));
}

GizmoManager::GizmoManager(nanogui::Widget& parent, id<MTLDevice> device, unsigned long colorPixelFormat, ActorManager& actorManager)
    : mDevice(device), mActorManager(actorManager) {

    // --- 1. Create Metal Render Pipeline State from the .metal shader file ---
    NSError *error = nil;
    // Assumes your shaders are compiled into a default library in the app's main bundle.
    id<MTLLibrary> library = [mDevice newDefaultLibraryWithBundle:[NSBundle mainBundle] error:&error];
    if (!library) {
        NSLog(@"[GizmoManager] FATAL: Could not create Metal library. Error: %@", error.localizedDescription);
        // In a real app, you might throw an exception or handle this more gracefully.
        return;
    }

    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"gizmo_vertex"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"gizmo_fragment"];

    MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.label = @"Gizmo Pipeline";
    pipelineDescriptor.vertexFunction = vertexFunc;
    pipelineDescriptor.fragmentFunction = fragmentFunc;
    pipelineDescriptor.colorAttachments[0].pixelFormat = (MTLPixelFormat)colorPixelFormat;

    mPipelineState = [mDevice newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!mPipelineState) {
        NSLog(@"[GizmoManager] FATAL: Failed to create Gizmo pipeline state. Error: %@", error.localizedDescription);
    }
    
    // --- 2. Create a Depth Stencil State to draw the gizmo on top of other objects ---
    MTLDepthStencilDescriptor *depthDesc = [MTLDepthStencilDescriptor new];
    depthDesc.depthCompareFunction = MTLCompareFunctionAlways; // Pass regardless of depth
    depthDesc.depthWriteEnabled = NO;                          // Don't write to the depth buffer
    mDepthDisabledState = [mDevice newDepthStencilStateWithDescriptor:depthDesc];

    // --- 3. Generate the procedural geometry for the gizmos ---
    create_procedural_gizmos();

    // --- 4. Set up NanoGUI buttons for mode switching ---
    mTranslationButton = std::make_shared<nanogui::Button>(parent, "", FA_ARROWS_ALT);
    mTranslationButton->set_callback([this]() { set_mode(GizmoMode::Translation); });

    mRotationButton = std::make_shared<nanogui::Button>(parent, "", FA_REDO);
    mRotationButton->set_callback([this]() { set_mode(GizmoMode::Rotation); });

    mScaleButton = std::make_shared<nanogui::Button>(parent, "", FA_EXPAND_ARROWS_ALT);
    mScaleButton->set_callback([this]() { set_mode(GizmoMode::Scale); });

    mTranslationButton->set_size({48, 48});
    mRotationButton->set_size({48, 48});
    mScaleButton->set_size({48, 48});

    mTranslationButton->set_position({10, parent.height() - 58});
    mRotationButton->set_position({mTranslationButton->position().x() + 53, parent.height() - 58});
    mScaleButton->set_position({mRotationButton->position().x() + 53, parent.height() - 58});

    // --- 5. Initialize state ---
    select(std::nullopt);
    set_mode(GizmoMode::None);
}

GizmoManager::~GizmoManager() {
    // With ARC (Automatic Reference Counting), we don't need to manually call [release].
    // The `id` pointers will be automatically released when this object is destroyed.
    // The void* pointers are just holding the addresses, and ARC manages the underlying objects.
}

void GizmoManager::create_procedural_gizmos() {
    create_translation_gizmo();
    create_rotation_gizmo();
    create_scale_gizmo();
}

// --- GEOMETRY CREATION METHODS ---

void GizmoManager::create_translation_gizmo() {
    const float line_length = 1.0f;
    const float cone_height = 0.2f;
    const float cone_radius = 0.08f;
    const int cone_segments = 12;

    for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
        std::vector<glm::vec3> vertices;
        glm::vec3 axis(i == 0, i == 1, i == 2);
        
        // Part 1: The line segment for the arrow shaft
        vertices.push_back(glm::vec3(0.0f));
        vertices.push_back(axis * line_length);

        // Part 2: The triangle-based cone for the arrowhead
        glm::vec3 cone_base = axis * line_length;
        glm::vec3 up = (i == 1) ? glm::vec3(-1, 0, 0) : glm::vec3(0, 1, 0);
        glm::vec3 p1 = glm::normalize(glm::cross(axis, up));
        glm::vec3 p2 = glm::normalize(glm::cross(axis, p1));

        for (int j = 0; j < cone_segments; ++j) {
            float angle1 = 2.0f * M_PI * j / cone_segments;
            float angle2 = 2.0f * M_PI * (j + 1) / cone_segments;
            glm::vec3 c1 = cone_base + (p1 * cosf(angle1) + p2 * sinf(angle1)) * cone_radius;
            glm::vec3 c2 = cone_base + (p1 * cosf(angle2) + p2 * sinf(angle2)) * cone_radius;
            
            // Triangle for the cone side
            vertices.push_back(c1);
            vertices.push_back(c2);
            vertices.push_back(cone_base + axis * cone_height);
            
            // Triangle for the cone base
            vertices.push_back(c1);
            vertices.push_back(cone_base);
            vertices.push_back(c2);
        }

        mTranslationMeshes[i].vertex_count = vertices.size();
        mTranslationMeshes[i].primitiveType = MTLPrimitiveTypeTriangle;
        
        id<MTLBuffer> buffer = [mDevice newBufferWithBytes:vertices.data()
                                                   length:vertices.size() * sizeof(glm::vec3)
                                                  options:MTLResourceStorageModeShared];
        buffer.label = [NSString stringWithFormat:@"GizmoTranslateAxis%d", i];
        mTranslationMeshes[i].vbo = buffer;
    }
}

void GizmoManager::create_rotation_gizmo() {
    const int segments = 64;
    for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
        std::vector<glm::vec3> vertices;
        for (int j = 0; j <= segments; ++j) {
            float angle = 2.0f * M_PI * j / segments;
            float x = cosf(angle);
            float y = sinf(angle);
            if (i == 0) vertices.push_back({0, x, y}); // X-axis ring on YZ plane
            if (i == 1) vertices.push_back({x, 0, y}); // Y-axis ring on XZ plane
            if (i == 2) vertices.push_back({x, y, 0}); // Z-axis ring on XY plane
        }
        
        mRotationMeshes[i].vertex_count = vertices.size();
        mRotationMeshes[i].primitiveType = MTLPrimitiveTypeLineStrip;
        id<MTLBuffer> buffer = [mDevice newBufferWithBytes:vertices.data() length:vertices.size() * sizeof(glm::vec3) options:MTLResourceStorageModeShared];
        buffer.label = [NSString stringWithFormat:@"GizmoRotateAxis%d", i];
        mRotationMeshes[i].vbo = buffer;
    }
}

void GizmoManager::create_scale_gizmo() {
    const float line_length = 1.0f;
    const float cube_size = 0.1f;
    for (int i = 0; i < 3; ++i) { // 0=X, 1=Y, 2=Z
        std::vector<glm::vec3> vertices;
        glm::vec3 axis(i == 0, i == 1, i == 2);
        
        // Part 1: Line segment
        vertices.push_back(glm::vec3(0.0f));
        vertices.push_back(axis * line_length);

        // Part 2: Cube at the end of the line
        glm::vec3 c = axis * line_length;
        float s = cube_size / 2.0f;
        glm::vec3 p[8] = {
            c + glm::vec3(-s, -s, -s), c + glm::vec3( s, -s, -s), c + glm::vec3( s,  s, -s), c + glm::vec3(-s,  s, -s),
            c + glm::vec3(-s, -s,  s), c + glm::vec3( s, -s,  s), c + glm::vec3( s,  s,  s), c + glm::vec3(-s,  s,  s)
        };
        // 12 lines for the cube edges
        int indices[] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
        for(int idx : indices) {
            vertices.push_back(p[idx]);
        }

        mScaleMeshes[i].vertex_count = vertices.size();
        mScaleMeshes[i].primitiveType = MTLPrimitiveTypeLine;
        id<MTLBuffer> buffer = [mDevice newBufferWithBytes:vertices.data() length:vertices.size() * sizeof(glm::vec3) options:MTLResourceStorageModeShared];
        buffer.label = [NSString stringWithFormat:@"GizmoScaleAxis%d", i];
        mScaleMeshes[i].vbo = buffer;
    }
}

// --- DRAWING AND LOGIC METHODS ---

void GizmoManager::draw(id<MTLRenderCommandEncoder> encoder, const glm::mat4& view, const glm::mat4& projection) {
    if (!mActiveActor.has_value() || mCurrentMode == GizmoMode::None) {
        return;
    }

    auto& transform = mActiveActor->get().get_component<TransformComponent>();
    glm::vec3 actor_pos = transform.get_translation();
    
    // Calculate a scale factor to keep the gizmo a consistent size on screen
    glm::vec3 cam_pos = glm::vec3(glm::inverse(view)[3]);
    float distance = glm::distance(cam_pos, actor_pos);
    float scale_factor = distance * 0.1f; // This factor can be tweaked for desired size

    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
    glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), actor_pos);
    
    // The rotation gizmo should align with the actor's rotation. Others are world-aligned.
    glm::mat4 rot_mat = (mCurrentMode == GizmoMode::Rotation) ? glm::mat4_cast(transform.get_rotation()) : glm::mat4(1.0f);
    glm::mat4 model_mat = trans_mat * rot_mat * scale_mat;

    // --- Encode Drawing Commands for Metal ---
    [encoder pushDebugGroup:@"DrawGizmo"];
    [encoder setRenderPipelineState:(id<MTLRenderPipelineState>)mPipelineState];
    [encoder setDepthStencilState:(id<MTLDepthStencilState>)mDepthDisabledState];
    
    // Set uniforms for the vertex shader (matrices)
    struct {
        simd_float4x4 model;
        simd_float4x4 view;
        simd_float4x4 projection;
    } uniforms;
    
    uniforms.model = glm_to_simd(model_mat);
    uniforms.view = glm_to_simd(view);
    uniforms.projection = glm_to_simd(projection);

    [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];

    // Select which set of gizmo meshes to draw
    GizmoMesh* active_meshes = nullptr;
    if (mCurrentMode == GizmoMode::Translation) active_meshes = mTranslationMeshes;
    else if (mCurrentMode == GizmoMode::Rotation) active_meshes = mRotationMeshes;
    else if (mCurrentMode == GizmoMode::Scale) active_meshes = mScaleMeshes;

    // Draw each axis of the active gizmo
    for (int i = 0; i < 3; ++i) {
        // Set the color uniform for the fragment shader
        simd_float4 color = { (float)(i == 0), (float)(i == 1), (float)(i == 2), 1.0f }; // R, G, B
        [encoder setFragmentBytes:&color length:sizeof(color) atIndex:0];

        // Set the vertex buffer for this axis
        [encoder setVertexBuffer:(id<MTLBuffer>)active_meshes[i].vbo offset:0 atIndex:0];
        
        // Issue the draw call
        [encoder drawPrimitives:(MTLPrimitiveType)active_meshes[i].primitiveType 
                    vertexStart:0 
                    vertexCount:active_meshes[i].vertex_count];
    }
    [encoder popDebugGroup];
}


// --- LOGIC METHODS (pure C++, unchanged) ---

void GizmoManager::select(std::optional<std::reference_wrapper<Actor>> actor) {
    mActiveActor = actor;
}

void GizmoManager::set_mode(GizmoMode mode) {
    mCurrentMode = mode;
}

void GizmoManager::select_axis(GizmoAxis gizmoId) {
    mActiveAxis = gizmoId;
}

void GizmoManager::translate(float dx, float dy) {
    auto& transform = mActiveActor->get().get_component<TransformComponent>();
    auto translation = transform.get_translation();
    switch (mActiveAxis) {
        case GizmoAxis::X: translation.x += dx; break;
        case GizmoAxis::Y: translation.y += dy; break;
        case GizmoAxis::Z: translation.z -= dy; break; // Invert Z for intuitive screen-space movement
        default: break;
    }
    transform.set_translation(translation);
}

void GizmoManager::rotate(float dx, float dy) {
    float angle = ((dx - dy) * 0.5f); // Use subtraction for more intuitive rotation
    auto& transform = mActiveActor->get().get_component<TransformComponent>();
    switch (mActiveAxis) {
        case GizmoAxis::X: transform.rotate(glm::vec3(1.0f, 0.0f, 0.0f), angle); break;
        case GizmoAxis::Y: transform.rotate(glm::vec3(0.0f, 1.0f, 0.0f), angle); break;
        case GizmoAxis::Z: transform.rotate(glm::vec3(0.0f, 0.0f, 1.0f), angle); break;
        default: break;
    }
}

void GizmoManager::scale(float dx, float dy) {
    auto& transform = mActiveActor->get().get_component<TransformComponent>();
    auto scale = transform.get_scale();
    float amount = (dx + dy) * 0.5f; // Uniform scaling based on average mouse movement
    switch (mActiveAxis) {
        case GizmoAxis::X: scale.x += amount; break;
        case GizmoAxis::Y: scale.y += amount; break;
        case GizmoAxis::Z: scale.z += amount; break;
        default: break;
    }
    transform.set_scale(scale);
}

void GizmoManager::transform(float dx, float dy) {
    if (mActiveActor.has_value() && mActiveAxis != GizmoAxis::None) {
        // Adjust sensitivity for better user experience
        const float translate_sensitivity = 0.01f;
        const float rotate_sensitivity = 0.01f;
        const float scale_sensitivity = 0.01f;

        switch (mCurrentMode) {
            case GizmoMode::Translation: translate(dx * translate_sensitivity, dy * translate_sensitivity); break;
            case GizmoMode::Rotation:    rotate(dx * rotate_sensitivity, dy * rotate_sensitivity); break;
            case GizmoMode::Scale:       scale(dx * scale_sensitivity, dy * scale_sensitivity); break;
            default: break;
        }
    }
}
