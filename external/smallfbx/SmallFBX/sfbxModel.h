#pragma once
#include "sfbxObject.h"

namespace sfbx {

// NodeAttribute and its subclasses:
//  (NullAttribute, RootAttribute, LimbNodeAttribute, LightAttribute, CameraAttribute)
// 
// these are for internal use. users do not need to care about them.

class NodeAttribute : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class NullAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void exportFBXObjects() override;
};

class RootAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void exportFBXObjects() override;
};

class LimbNodeAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void exportFBXObjects() override;
};

class LightAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void importFBXObjects() override;
    void exportFBXObjects() override;
};

class CameraAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void importFBXObjects() override;
    void exportFBXObjects() override;
};



// Model and its subclasses:
//  (Null, Root, LimbNode, Light, Camera)

class Model : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

    std::shared_ptr<Model> getParentModel() const;

    bool getVisibility() const;
    RotationOrder getRotationOrder() const;
    double3 getPosition() const;
    double3 getPreRotation() const;
    double3 getRotation() const;
    double3 getPostRotation() const;
    double3 getScale() const;
    double4x4 getLocalMatrix() const;
    double4x4 getGlobalMatrix() const;
    std::string getPath() const;

    void setVisibility(bool v);
    void setRotationOrder(RotationOrder v);
    void setPosition(double3 v);
    void setPreRotation(double3 v);
    void setRotation(double3 v);
    void setPostRotation(double3 v);
    void setScale(double3 v);
	void setLocalMatrix(const double4x4& matrix); // **New Method**

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
    void addParent(ObjectPtr v) override;
    void eraseParent(ObjectPtr v) override;
    void propagateDirty();
    void updateMatrices() const;

    std::shared_ptr<Model> m_parent_model{};
    std::vector<std::shared_ptr<Model>> m_child_models;

    bool m_visibility = true;
    RotationOrder m_rotation_order = RotationOrder::XYZ;
    double3 m_position{};
    double3 m_pre_rotation{};
    double3 m_rotation{};
    double3 m_post_rotation{};
    double3 m_scale = double3::one();

    mutable bool m_matrix_dirty = true;
    mutable double4x4 m_matrix_local = double4x4::identity();
    mutable double4x4 m_matrix_global = double4x4::identity();
	
	std::shared_ptr<NodeAttribute> m_attr{};

};


// Null, Root, and LimbNode represent joints.
// usually Null or Root is the root of joints, but a joint structure with LimbNode root also seems to be valid.

class Null : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

protected:
    void exportFBXObjects() override;

    //std::shared_ptr<NullAttribute> m_attr{};
};

class Root : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

protected:
    void exportFBXObjects() override;

    //std::shared_ptr<RootAttribute> m_attr{};
};

class LimbNode : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

protected:
    void exportFBXObjects() override;

	//std::shared_ptr<LimbNodeAttribute> m_attr{};
};


// Mesh represents polygon mesh objects, but it holds just transform data.
// geometry data is stored in GeomMesh. (see sfbxGeometry.h)

class Mesh : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

	std::shared_ptr<GeomMesh> getGeometry();
	void setGeometry(std::shared_ptr<GeomMesh> geom); // **New Method**

    span<std::shared_ptr<Material>> getMaterials() const;

protected:
    void importFBXObjects() override;
	void exportFBXObjects() override;
	
	std::shared_ptr<GeomMesh> m_geom{};
    std::vector<std::shared_ptr<Material>> m_materials;
};


enum class LightType
{
    Point,
    Directional,
    Spot,
};

class Light : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

    LightType getLightType() const;
    double3 getColor() const;
    float getIntensity() const;
    float getInnerAngle() const; // in degree, for spot light
    float getOuterAngle() const; // in degree, for spot light

    void setLightType(LightType v);
    void setColor(double3 v);
    void setIntensity(float v);
    void setInnerAngle(float v);
    void setOuterAngle(float v);

protected:
    friend class LightAttribute;
    void exportFBXObjects() override;

	//std::shared_ptr<LightAttribute> m_attr{};
    LightType m_light_type = LightType::Point;
    double3 m_color = double3::one();
    float m_intensity = 1.0f;
    float m_inner_angle = 45.0f;
    float m_outer_angle = 45.0f;
};


enum class CameraType
{
    Perspective,
    Orthographic,
};

class Camera : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

    CameraType getCameraType() const;
    float getFocalLength() const;   // in mm
    double2 getFilmSize() const;     // in mm, aka "aperture" or "sensor"
    double2 getFilmOffset() const;   // in mm, aka "lens shift" or "sensor shift"
    double2 getFildOfView() const;   // in degree
    double2 getAspectSize() const;   // in pixel (screen size)
    float getAspectRatio() const;
    float getNearPlane() const;
    float getFarPlane() const;
    double3 getUpVector() const;
    double3 getTargetPosition() const;
    bool getAutoClipPlanes() const;

    // there is no setFildOfView() because fov is computed by aperture and focal length.
    // focal length can be computed by compute_focal_length() in sfbxMath.h with fov and aperture.

    void setCameraType(CameraType v);
    void setFocalLength(float v);   // in mm
    void setFilmSize(double2 v);     // in mm
    void setFilmShift(double2 v);    // in mm
    void setAspectSize(double2 v);   // in pixel (screen size)
    void setNearPlane(float v);
    void setFarPlane(float v);

protected:
    friend class CameraAttribute;
    friend class AnimationCurveNode;
    void importFBXObjects() override;
    void exportFBXObjects() override;

	//std::shared_ptr<CameraAttribute> m_attr{};
    CameraType m_camera_type = CameraType::Perspective;
	double m_focal_length = 50.0f; // in mm
    double2 m_film_size{ 36.0f, 24.0f }; // in mm
    double2 m_film_offset{}; // in mm
    double2 m_aspect{ 1920.0f, 1080.0f };
	double m_near_plane = 0.1f;
	double m_far_plane = 1000.0f;
    double3 m_up_vector = {0, 1, 0};
    double3 m_target_position = {0, 0, 0};
    bool m_auto_clip_planes = false;
};


} // namespace sfbx
