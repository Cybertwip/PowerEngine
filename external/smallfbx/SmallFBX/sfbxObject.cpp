#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

#include <atomic>

namespace sfbx {

// Initialize the static ID counter
std::atomic<int64> Object::s_next_id(1);  // Start from 1 (or any preferred starting ID)

ObjectClass GetObjectClass(string_view n)
{
    if (n.empty()) {
        return ObjectClass::Unknown;
    }
#define Case(T) else if (n == sfbxS_##T) { return ObjectClass::T; }
    sfbxEachObjectClass(Case)
#undef Case
    else {
        sfbxPrint("GetFbxObjectClass(): unknown type \"%s\"\n", std::string(n).c_str());
        return ObjectClass::Unknown;
    }
}
ObjectClass GetObjectClass(Node* n)
{
    return GetObjectClass(n->getName());
}

string_view GetObjectClassName(ObjectClass t)
{
    switch (t) {
#define Case(T) case ObjectClass::T: return sfbxS_##T;
        sfbxEachObjectClass(Case)
#undef Case
    default:
        return "";
    }
}


ObjectSubClass GetObjectSubClass(string_view n)
{
    if (n.empty()) {
        return ObjectSubClass::Unknown;
    }
#define Case(T) else if (n == sfbxS_##T) { return ObjectSubClass::T; }
    sfbxEachObjectSubClass(Case)
#undef Case
    else {
        sfbxPrint("GetFbxObjectSubClass(): unknown subtype \"%s\"\n", std::string(n).c_str());
        return ObjectSubClass::Unknown;
    }
}

ObjectSubClass GetObjectSubClass(Node* n)
{
    if (GetPropertyCount(n) == 3)
        return GetObjectSubClass(GetPropertyString(n, 2));
#ifdef sfbxEnableLegacyFormatSupport
    else if (GetPropertyCount(n) == 2)
        return GetObjectSubClass(GetPropertyString(n, 1));
#endif
    else
        return ObjectSubClass::Unknown;
}

string_view GetObjectSubClassName(ObjectSubClass t)
{
    switch (t) {
#define Case(T) case ObjectSubClass::T: return sfbxS_##T;
        sfbxEachObjectSubClass(Case)
#undef Case
    default: return "";
    }
}


static constexpr inline string_view GetInternalObjectClassName(ObjectClass t, ObjectSubClass st)
{
#define Case1(T, ST, Ret) if (t == ObjectClass::T && st == ObjectSubClass::ST) return Ret
#define Case2(T, Ret) if (t == ObjectClass::T) return Ret

    Case1(Deformer, Cluster, sfbxS_SubDeformer);
    Case1(Deformer, BlendShapeChannel, sfbxS_SubDeformer);
    Case2(AnimationStack, sfbxS_AnimStack);
    Case2(AnimationLayer, sfbxS_AnimLayer);
    Case2(AnimationCurveNode, sfbxS_AnimCurveNode);
    Case2(AnimationCurve, sfbxS_AnimCurve);

#undef Case1
#undef Case2

    return GetObjectClassName(t);
}


std::string MakeFullName(string_view display_name, string_view class_name)
{
    std::string ret;
    size_t pos = display_name.find('\0'); // ignore class name part
    if (pos == std::string::npos)
        ret = display_name;
    else
        ret.assign(display_name.data(), pos);

    ret += (char)0x00;
    ret += (char)0x01;
    ret += class_name;
    return ret;
}
std::string MakeFullName(string_view display_name, ObjectClass c, ObjectSubClass sc)
{
    return MakeFullName(display_name, GetInternalObjectClassName(c, sc));
}

bool IsFullName(string_view name)
{
    size_t n = name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i)
            if (name[i] == 0x00 && name[i + 1] == 0x01)
                return true;
    }
    return false;
}

bool SplitFullName(string_view full_name, string_view& display_name, string_view& class_name)
{
    const char* str = full_name.data();
    size_t n = full_name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i) {
            if (str[i] == 0x00 && str[i + 1] == 0x01) {
                display_name = string_view(str, i);
                i += 2;
                class_name = string_view(str + i, n - i);
                return true;
            }
        }
    }
    display_name = string_view(full_name);
    return false;
}



Object::Object()
{
	// Use atomic fetch and increment to ensure unique IDs
	m_id = s_next_id.fetch_add(1);
	m_document = nullptr;
}

Object::~Object()
{
}

ObjectClass Object::getClass() const { return ObjectClass::Unknown; }
ObjectSubClass Object::getSubClass() const { return ObjectSubClass::Unknown; }

void Object::setNode(Node* n)
{
    m_node = n;
    if (n) {
        // do these in importFBXObjects() is too late because of referencing other objects...
        size_t cprops = GetPropertyCount(n);
        if (cprops == 3) {
            m_id = GetPropertyValue<int64>(n, 0);
            m_name = GetPropertyString(n, 1);
        }
#ifdef sfbxEnableLegacyFormatSupport
        else if (cprops == 2) {
            // no ID in legacy format
            m_name = GetPropertyString(n, 0);
        }
#endif
        n->setForceNullTerminate(true);
    }
}

void Object::importFBXObjects()
{
}

void Object::exportFBXObjects()
{
    if (m_id == 0)
        return;

    auto objects = document()->findNode(sfbxS_Objects);
    m_node = objects->createChild(
        GetObjectClassName(getClass()), m_id, getFullName(), GetObjectSubClassName(getSubClass()));

    // FBX SDK seems to require the object node to always null-terminated.
    // (Blender seems not to require this, though)
    m_node->setForceNullTerminate(true);
}

void Object::exportFBXConnections()
{
	VisitedSet visited;
	exportFBXConnections(visited);
}

void Object::exportFBXConnections(VisitedSet& visited) {
	// Check if this object has already been processed
	if (visited.find(shared_from_this()) != visited.end()) {
		return; // Already processed, skip
	}
	
	// Mark this object as processed
	visited.insert(shared_from_this());
	
	// Export connections for this object
	for (auto parent : getParents()) {
		document()->createLinkOO(shared_from_this(), parent);
	}
	
	// Recursively export connections for children
	for (auto& child : getChildren()) {
		child->exportFBXConnections(visited);
	}
}

void Object::addChild(ObjectPtr v)
{
    if (v) {
        m_children.push_back(v);
        m_child_property_names.emplace_back();
        v->addParent(shared_from_this());
		v->setDocument(m_document);
    }
}
void Object::addChild(ObjectPtr v, string_view p)
{
    addChild(v);
    if (v) {
        m_child_property_names.back() = p;
    }
}

void Object::eraseChild(ObjectPtr v)
{
	auto it = std::find(m_children.begin(), m_children.end(), v);
	
	if (it != m_children.end()) {
		size_t index = std::distance(m_children.begin(), it);
		
		// Remove the child property name at the found index
		if (index < m_child_property_names.size()) {
			m_child_property_names.erase(m_child_property_names.begin() + index);
		}
		
		// Remove the child from the m_children vector
		m_children.erase(it);
		
		v->eraseParent(shared_from_this());
	}
}

void Object::addParent(ObjectPtr v)
{
    if (v)
        m_parents.push_back(v);
}

void Object::eraseParent(ObjectPtr v)
{
    erase(m_parents, v);
}


int64 Object::getID() const { return m_id; }
string_view Object::getFullName() const { return m_name; }
string_view Object::getName() const { return m_name.c_str(); }
Node* Object::getNode() const { return m_node; }

span<ObjectPtr> Object::getParents() const  { return make_span(m_parents); }
span<ObjectPtr> Object::getChildren() const { return make_span(m_children); }
ObjectPtr Object::getParent(size_t i) const { return i < m_parents.size() ? m_parents[i] : nullptr; }
ObjectPtr Object::getChild(size_t i) const  { return i < m_children.size() ? m_children[i] : nullptr; }
const std::string& Object::getChildProp(size_t i) const { static std::string nulls; return i < m_child_property_names.size() ? m_child_property_names[i] : nulls; }

ObjectPtr Object::findChild(string_view name) const
{
    if (IsFullName(name))
        return find_if(m_children, [&name](ObjectPtr c) { return c->getFullName() == name; });
    else
        return find_if(m_children, [&name](ObjectPtr c) { return c->getName() == name; });
    return nullptr;
}

void Object::setID(int64 id) { m_id = id; }
void Object::setName(string_view v) { m_name = MakeFullName(v, getClass(), getSubClass()); }


} // namespace sfbx
