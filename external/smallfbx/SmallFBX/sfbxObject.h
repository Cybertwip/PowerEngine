#pragma once
#include "sfbxTypes.h"

#include <unordered_set>
#include <memory>

namespace sfbx {

enum class ObjectClass : int
{
    Unknown,
    NodeAttribute,
    Model,
    Geometry,
    Deformer,
    Pose,
    Video,
    Texture,
    Material,
    AnimationStack,
    AnimationLayer,
    AnimationCurveNode,
    AnimationCurve,
    Implementation,
    BindingTable,
};

enum class ObjectSubClass : int
{
    Unknown,
    Null,
    Root,
    LimbNode,
    Light,
    Camera,
    Mesh,
    Shape,
    BlendShape,
    BlendShapeChannel,
    Skin,
    Cluster,
    BindPose,
    Clip,
};

#define sfbxEachObjectClass(Body)\
    Body(NodeAttribute) Body(Model) Body(Geometry) Body(Deformer) Body(Pose)\
    Body(Video) Body(Texture) Body(Material)\
    Body(AnimationStack) Body(AnimationLayer) Body(AnimationCurveNode) Body(AnimationCurve)\
    Body(Implementation) Body(BindingTable)

#define sfbxEachObjectSubClass(Body)\
    Body(Null) Body(Root) Body(LimbNode) Body(Light) Body(Camera) Body(Mesh) Body(Shape)\
    Body(BlendShape) Body(BlendShapeChannel) Body(Skin) Body(Cluster) Body(BindPose) Body(Clip)

#define sfbxEachObjectType(Body)\
    Body(NodeAttribute) Body(NullAttribute) Body(RootAttribute) Body(LimbNodeAttribute) Body(LightAttribute) Body(CameraAttribute)\
    Body(Model) Body(Null) Body(Root) Body(LimbNode) Body(Light) Body(Camera) Body(Mesh)\
    Body(Geometry) Body(GeomMesh) Body(Shape)\
    Body(Deformer) Body(Skin) Body(Cluster) Body(BlendShape) Body(BlendShapeChannel)\
    Body(Pose) Body(BindPose)\
    Body(Video) Body(Material)\
	Body(Texture) \
    Body(AnimationStack) Body(AnimationLayer) Body(AnimationCurveNode) Body(AnimationCurve)

#define Decl(T) class T;
sfbxEachObjectType(Decl)
#undef Decl


ObjectClass GetObjectClass(string_view n);
ObjectClass GetObjectClass(Node* n);
string_view GetObjectClassName(ObjectClass t);
ObjectSubClass GetObjectSubClass(string_view n);
ObjectSubClass GetObjectSubClass(Node* n);
string_view GetObjectSubClassName(ObjectSubClass t);

// return full name (display name + \x00 \x01 + class name)
std::string MakeFullName(string_view display_name, string_view class_name);
std::string MakeFullName(string_view display_name, ObjectClass c, ObjectSubClass sc);
// true if name is in full name (display name + \x00 \x01 + class name)
bool IsFullName(string_view name);
// split full name into display name & class name (e.g. "hoge\x00\x01Mesh" -> "hoge" & "Mesh")
bool SplitFullName(string_view full_name, string_view& display_name, string_view& class_name);


// base object class

class Object : public std::enable_shared_from_this<Object>
{
	friend class Document;
public:
	using VisitedSet = std::unordered_set<std::shared_ptr<Object>>;

    virtual ~Object();
    virtual ObjectClass getClass() const;
    virtual ObjectSubClass getSubClass() const;

    template<class T> std::shared_ptr<T> createChild(string_view name = {});
    virtual void addChild(ObjectPtr v);
    virtual void addChild(ObjectPtr v, string_view p);
    virtual void eraseChild(ObjectPtr v);

    int64 getID() const;
    string_view getFullName() const; // display name + class name (e.g. "hoge\x00\x01Mesh")
    string_view getName() const; // display name (e.g. "hoge" if object name is "hoge\x00\x01Mesh")
    Node* getNode() const;

    span<ObjectPtr> getParents() const;
    span<ObjectPtr> getChildren() const;
	ObjectPtr getParent(size_t i = 0) const;
	ObjectPtr getChild(size_t i = 0) const;
    const std::string& getChildProp(size_t i = 0) const;
	ObjectPtr findChild(string_view name) const; // name accepts both full name and display name

    void setID(int64 v);
    void setName(string_view v);
    void setNode(Node* v);

	Document* document() const {
		return m_document;
	}
	
	void setDocument(Document* document) {
		m_document = document;
	}
	
	virtual void importFBXObjects();
	virtual void exportFBXObjects();
	// Original exportFBXConnections method initiates the traversal
	virtual void exportFBXConnections();

	void exportFBXConnections(VisitedSet& visited);

protected:
    Object();
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;

    virtual void addParent(ObjectPtr v);
    virtual void eraseParent(ObjectPtr v);

    Node* m_node{};
    int64 m_id{};
    std::string m_name;

    std::vector<ObjectPtr> m_parents;
    std::vector<ObjectPtr> m_children;
    std::vector<std::string> m_child_property_names;
	
private:
	static std::atomic<int64> s_next_id;  // Static atomic counter for unique IDs

	Document* m_document;
};


} // sfbx
