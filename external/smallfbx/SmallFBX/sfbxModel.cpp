#include "sfbxInternal.h"
#include "sfbxDocument.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxMaterial.h"

namespace sfbx {

ObjectClass NodeAttribute::getClass() const { return ObjectClass::NodeAttribute; }
ObjectSubClass NullAttribute::getSubClass() const { return ObjectSubClass::Null; }
ObjectSubClass RootAttribute::getSubClass() const { return ObjectSubClass::Root; }
ObjectSubClass LimbNodeAttribute::getSubClass() const { return ObjectSubClass::LimbNode; }
ObjectSubClass LightAttribute::getSubClass() const { return ObjectSubClass::Light; }
ObjectSubClass CameraAttribute::getSubClass() const { return ObjectSubClass::Camera; }



ObjectClass Model::getClass() const { return ObjectClass::Model; }

void Model::importFBXObjects()
{
    auto n = getNode();
    if (!n)
        return;

    EnumerateProperties(n, [this](Node* p) {
        auto get_int = [p]() -> int {
            if (GetPropertyCount(p) == 5)
                return GetPropertyValue<int32>(p, 4);
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 4) {
                return GetPropertyValue<int32>(p, 3);
            }
#endif
            return 0;
        };

        auto get_float3 = [p]() -> double3 {
            if (GetPropertyCount(p) == 7) {
                return double3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 6) {
                return double3{
                    (float)GetPropertyValue<float64>(p, 3),
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                };
            }
#endif
            return {};
        };

        auto pname = GetPropertyString(p);
        if (pname == sfbxS_Visibility) {
            m_visibility = GetPropertyValue<bool>(p, 4);
        }
        else if (pname == sfbxS_LclTranslation)
            m_position = get_float3();
        else if (pname == sfbxS_RotationOrder)
            m_rotation_order = (RotationOrder)get_int();
        else if (pname == sfbxS_PreRotation)
            m_pre_rotation = get_float3();
        else if (pname == sfbxS_PostRotation)
            m_post_rotation = get_float3();
        else if (pname == sfbxS_LclRotation)
            m_rotation = get_float3();
        else if (pname == sfbxS_LclScale)
            m_scale = get_float3();
        });
}

#define sfbxVector3d(V) (float64)V.x, (float64)V.y, (float64)V.z

void Mesh::exportFBXObjects()
{
	super::exportFBXObjects(); // Calls Model::exportFBXObjects()
	
	Node* prop = m_node->findChild(sfbxS_Properties70);

	if (!prop)
		return;

	prop->createChild(sfbxS_P, "DefaultAttributeIndex", sfbxS_int, sfbxS_Integer, "", 0);

	// Export geometry
	if (m_geom) {
		m_geom->exportFBXObjects();
		document()->createLinkOO(m_geom, shared_from_this(), sfbxS_Geometry);
	}
	
	// Export materials and create connections
	for (auto& material : m_materials) {
		material->exportFBXObjects();
		document()->createLinkOO(material, shared_from_this(), sfbxS_Material);
	}
}

void Model::exportFBXObjects()
{
	if (m_node)
		return; // Already exported
	
	Node* objects = document()->findNode(sfbxS_Objects);
	if (!objects)
		return;
	
	// Create the node for this model
	m_node = objects->createChild(
								  sfbxS_Model,
								  getID(),
								  getFullName(),
								  GetObjectSubClassName(getSubClass())
								  );
	
	// Set up properties
	Node* prop = m_node->createChild(sfbxS_Properties70);
	prop->createChild(sfbxS_P, sfbxS_LclTranslation, sfbxS_LclTranslation, sfbxS_Vector, "", m_position.x, m_position.y, m_position.z);
	prop->createChild(sfbxS_P, sfbxS_LclRotation, sfbxS_LclRotation, sfbxS_Vector, "", m_rotation.x, m_rotation.y, m_rotation.z);
	prop->createChild(sfbxS_P, sfbxS_LclScale, sfbxS_LclScale, sfbxS_Vector, "", m_scale.x, m_scale.y, m_scale.z);
	
	
	// Export attached node attribute
	if (document()) {
		document()->createLinkOO(shared_from_this(), getParent());
	}

	// Export child models
	for (auto& child : m_child_models) {
		child->exportFBXObjects();
	}
}

void Model::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto model = as<Model>(v))
        m_child_models.push_back(model);
}

void Model::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (auto model = as<Model>(v))
        erase(m_child_models, model);
}

void Model::addParent(ObjectPtr v)
{
    super::addParent(v);
    if (auto model = as<Model>(v))
        m_parent_model = model;
}

void Model::eraseParent(ObjectPtr v)
{
    super::eraseParent(v);
    if (v == m_parent_model)
        m_parent_model = nullptr;
}

std::shared_ptr<Model> Model::getParentModel() const { return m_parent_model; }

bool Model::getVisibility() const { return m_visibility; }
RotationOrder Model::getRotationOrder() const { return m_rotation_order; }
double3 Model::getPosition() const { return m_position; }

double3 Model::getPreRotation() const { return m_pre_rotation; }
double3 Model::getRotation() const { return m_rotation; }
double3 Model::getPostRotation() const { return m_post_rotation; }
double3 Model::getScale() const { return m_scale; }

void Model::updateMatrices() const
{
    if (m_matrix_dirty) {
        // scale
        double4x4 r = scale44(m_scale);

        // rotation
        if (m_post_rotation != double3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_post_rotation * DegToRad)));
        if (m_rotation != double3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_rotation * DegToRad)));
        if (m_pre_rotation != double3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_pre_rotation * DegToRad)));

        // translation
        (double3&)r[3] = m_position;

        m_matrix_local = r;
        m_matrix_global = m_matrix_local;
        if (m_parent_model)
            m_matrix_global *= m_parent_model->getGlobalMatrix();

        m_matrix_dirty = false;
    }
}

double4x4 Model::getLocalMatrix() const
{
    updateMatrices();
    return m_matrix_local;
}

void Model::setLocalMatrix(const double4x4& matrix)
{
	if (m_matrix_local != matrix) {
		m_matrix_local = matrix;
		m_matrix_dirty = false;
		updateMatrices();
	}
}


double4x4 Model::getGlobalMatrix() const
{
    updateMatrices();
    return m_matrix_global;
}

std::string Model::getPath() const
{
    if (m_id == 0)
        return{};

    std::string ret;
    if (m_parent_model)
        ret += m_parent_model->getPath();
    ret += "/";
    ret += getName();
    return ret;
}

void Model::setVisibility(bool v) { m_visibility = v; }
void Model::setRotationOrder(RotationOrder v) { m_rotation_order = v; }

void Model::propagateDirty()
{
    if (!m_matrix_dirty) {
        m_matrix_dirty = true;
        for (auto c : m_child_models)
            c->propagateDirty();
    }
}

#define MarkDirty(V, A) if (A != V) { V = A; propagateDirty(); }
void Model::setPosition(double3 v)     { MarkDirty(m_position, v); }
void Model::setPreRotation(double3 v)  { MarkDirty(m_pre_rotation, v); }
void Model::setRotation(double3 v)     { MarkDirty(m_rotation, v); }
void Model::setPostRotation(double3 v) { MarkDirty(m_post_rotation, v); }
void Model::setScale(double3 v)        { MarkDirty(m_scale, v); }
#undef MarkDirty



ObjectSubClass Null::getSubClass() const { return ObjectSubClass::Null; }

void Null::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<NullAttribute>();
    super::exportFBXObjects();
}

void NullAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Null);
}

void Null::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto attr = as<NullAttribute>(v))
        m_attr = attr;
}

void Null::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass Root::getSubClass() const { return ObjectSubClass::Root; }

void Root::exportFBXObjects()
{
	if (m_node)
		return; // Already exported
	
	// Call the base class implementation
	super::exportFBXObjects();
	
	// Create a RootAttribute if it doesn't exist
	if (!m_attr) {
		m_attr = document()->createObject<NodeAttribute>();
		addChild(m_attr);
	}
}

void RootAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
}

void Root::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto attr = as<RootAttribute>(v))
        m_attr = attr;
}

void Root::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass LimbNode::getSubClass() const { return ObjectSubClass::LimbNode; }

void LimbNode::exportFBXObjects()
{
	super::exportFBXObjects();

	if (!m_attr){
		m_attr = createChild<LimbNodeAttribute>();
		m_attr->exportFBXObjects();
	}
}


void LimbNode::exportFBXConnections()
{
	super::exportFBXConnections();
	
	if (m_attr){
		m_attr->exportFBXConnections();
	}
}

void LimbNodeAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Skeleton);
	
	for (auto& child : m_children) {
		child->exportFBXObjects();
	}
}

void LimbNode::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto attr = as<LimbNodeAttribute>(v))
        m_attr = attr;
}


void LimbNode::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass Mesh::getSubClass() const { return ObjectSubClass::Mesh; }

void Mesh::importFBXObjects()
{
#ifdef sfbxEnableLegacyFormatSupport
    // in old fbx, Model::Mesh has geometry data (Geometry::Mesh does not exist)
    auto n = getNode();
    if (n->findChild(sfbxS_Vertices)) {
        getGeometry()->setNode(n);
    }
#endif
}

void Mesh::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto geom = as<GeomMesh>(v))
        m_geom = geom;
    else if (auto material = as<Material>(v))
        m_materials.push_back(material);
}

void Mesh::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_geom)
        m_geom = nullptr;
    else if (auto material = as<Material>(v))
        erase(m_materials, material);
}

std::shared_ptr<GeomMesh> Mesh::getGeometry()
{
    if (!m_geom)
        m_geom = createChild<GeomMesh>(getName());
    return m_geom;
}

// Adding setGeometry to Mesh
void Mesh::setGeometry(std::shared_ptr<GeomMesh> geom)
{
	if (m_geom != geom) {
		m_geom = geom;
		
		m_geom->setDocument(document());
		
		// Establish the connection
		if (document()) {
			document()->createLinkOO(m_geom, shared_from_this());
		}
	}
}


span<std::shared_ptr<Material>> Mesh::getMaterials() const
{
    return make_span(m_materials);
}



ObjectSubClass Light::getSubClass() const { return ObjectSubClass::Light; }

void LightAttribute::importFBXObjects()
{
    auto light = as<Light>(getParent());
    if (!light)
        return;

    EnumerateProperties(getNode(), [light](Node* p) {
        auto name = GetPropertyString(p, 0);
        if (name == sfbxS_LightType)
            light->m_light_type = (LightType)GetPropertyValue<int32>(p, 4);
        else if (name == sfbxS_Color)
            light->m_color = double3{
                    (float32)GetPropertyValue<float64>(p, 4),
                    (float32)GetPropertyValue<float64>(p, 5),
                    (float32)GetPropertyValue<float64>(p, 6)};
        else if (name == sfbxS_Intensity)
            light->m_intensity = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_InnerAngle)
            light->m_inner_angle = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_OuterAngle)
            light->m_outer_angle = (float32)GetPropertyValue<float64>(p, 4);
        });
}

void Light::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<LightAttribute>();
    super::exportFBXObjects();
}

void LightAttribute::exportFBXObjects()
{
    super::exportFBXObjects();

    auto light = as<Light>(getParent());
    if (!light)
        return;

    auto color = light->m_color;
    auto props = getNode()->createChild(sfbxS_Properties70);
    props->createChild(sfbxS_P, sfbxS_LightType, sfbxS_enum, "", "", (int32)light->m_light_type);
    props->createChild(sfbxS_P, sfbxS_Color, sfbxS_Color, "", "A", (float64)color.x, (float64)color.y, (float64)color.z);
    props->createChild(sfbxS_P, sfbxS_Intensity, sfbxS_Number, "", "A", (float64)light->m_intensity);
    if (light->m_light_type == LightType::Spot) {
        props->createChild(sfbxS_P, sfbxS_InnerAngle, sfbxS_Number, "", "A", (float64)light->m_inner_angle);
        props->createChild(sfbxS_P, sfbxS_OuterAngle, sfbxS_Number, "", "A", (float64)light->m_outer_angle);
    }
}

void Light::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto attr = as<LightAttribute>(v))
        m_attr = attr;
}

void Light::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}

LightType Light::getLightType() const { return m_light_type; }
double3 Light::getColor() const { return m_color; }
float Light::getIntensity() const { return m_intensity; }
float Light::getInnerAngle() const { return m_inner_angle; }
float Light::getOuterAngle() const { return m_outer_angle; }

void Light::setLightType(LightType v) { m_light_type = v; }
void Light::setColor(double3 v) { m_color = v; }
void Light::setIntensity(float v) { m_intensity = v; }
void Light::setInnerAngle(float v) { m_inner_angle = v; }
void Light::setOuterAngle(float v) { m_outer_angle = v; }



ObjectSubClass Camera::getSubClass() const { return ObjectSubClass::Camera; }

void Camera::importFBXObjects()
{
    
    if (m_target_position != double3::zero())
        m_target_position = m_target_position - getPosition();
}

void CameraAttribute::importFBXObjects()
{
    

    auto cam = as<Camera>(getParent());
    if (!cam)
        return;

    EnumerateProperties(getNode(), [cam](Node* p) {

        auto get_float3 = [p]() -> double3 {
            if (GetPropertyCount(p) == 7) {
                return double3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 6) {
                return double3{
                    (float)GetPropertyValue<float64>(p, 3),
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                };
            }
#endif
            return {};
        };

        auto name = GetPropertyString(p, 0);
        if (name == sfbxS_CameraProjectionType)
            cam->m_camera_type = (CameraType)GetPropertyValue<int32>(p, 4);
        else if (name == sfbxS_FocalLength)
            cam->m_focal_length = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_FilmWidth)
            cam->m_film_size.x = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmHeight)
            cam->m_film_size.y = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmOffsetX)
            cam->m_film_offset.x = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmOffsetY)
            cam->m_film_offset.y = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_NearPlane)
            cam->m_near_plane = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_FarPlane)
            cam->m_far_plane = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_UpVector)
            cam->m_up_vector = get_float3();
        else if (name == sfbxS_InterestPosition)
            cam->m_target_position = get_float3();
        else if (name == sfbxS_AutoComputeClipPanes)
            cam->m_auto_clip_planes = GetPropertyValue<bool>(p, 4) || GetPropertyValue<int>(p, 4);
        });
}

void Camera::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<CameraAttribute>();
    super::exportFBXObjects();
}

void CameraAttribute::exportFBXObjects()
{
    super::exportFBXObjects();

    auto cam = as<Camera>(getParent());
    if (!cam)
        return;

    auto props = getNode()->createChild(sfbxS_Properties70);
    props->createChild(sfbxS_P, sfbxS_CameraProjectionType, sfbxS_enum, "", "", (int32)cam->m_camera_type);
    props->createChild(sfbxS_P, sfbxS_FocalLength, sfbxS_Number, "", "A", (float64)cam->m_focal_length);
    props->createChild(sfbxS_P, sfbxS_FilmWidth, sfbxS_Number, "", "A", (float64)cam->m_film_size.x * MillimeterToInch);
    props->createChild(sfbxS_P, sfbxS_FilmHeight, sfbxS_Number, "", "A", (float64)cam->m_film_size.y * MillimeterToInch);
    if (cam->m_film_offset.x != 0.0f)
        props->createChild(sfbxS_P, sfbxS_FilmOffsetX, sfbxS_Number, "", "A", (float64)cam->m_film_offset.x * MillimeterToInch);
    if (cam->m_film_offset.y != 0.0f)
        props->createChild(sfbxS_P, sfbxS_FilmOffsetY, sfbxS_Number, "", "A", (float64)cam->m_film_offset.y * MillimeterToInch);
    props->createChild(sfbxS_P, sfbxS_NearPlane, sfbxS_Number, "", "A", (float64)cam->m_near_plane);
    props->createChild(sfbxS_P, sfbxS_FarPlane, sfbxS_Number, "", "A", (float64)cam->m_far_plane);
}

void Camera::addChild(ObjectPtr v)
{
    super::addChild(v);
    if (auto attr = as<CameraAttribute>(v))
        m_attr = attr;
}

void Camera::eraseChild(ObjectPtr v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}

CameraType Camera::getCameraType() const { return m_camera_type; }
float Camera::getFocalLength() const { return m_focal_length; }
double2 Camera::getFilmSize() const { return m_film_size; }
double2 Camera::getFilmOffset() const { return m_film_offset; }
double2 Camera::getFildOfView() const
{
    return double2{
        compute_fov(m_film_size.x, m_focal_length),
        compute_fov(m_film_size.y, m_focal_length),
    };
}

double2 Camera::getAspectSize() const { return m_aspect; }
float Camera::getAspectRatio() const
{
    return m_film_size.x / m_film_size.y;
}

float Camera::getNearPlane() const { return m_near_plane; }
float Camera::getFarPlane() const { return m_far_plane; }

double3 Camera::getUpVector() const { return m_up_vector; }
double3 Camera::getTargetPosition() const { return m_target_position; }

bool Camera::getAutoClipPlanes() const
{
    return m_auto_clip_planes;
}


void Camera::setCameraType(CameraType v) { m_camera_type = v; }
void Camera::setFocalLength(float v) { m_focal_length = v; }
void Camera::setFilmSize(double2 v) { m_film_size = v; }
void Camera::setFilmShift(double2 v) { m_film_offset = v; }
void Camera::setAspectSize(double2 v) { m_aspect = v; }
void Camera::setNearPlane(float v) { m_near_plane = v; }
void Camera::setFarPlane(float v) { m_far_plane = v; }

} // namespace sfbx
