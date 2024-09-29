#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"
#include "sfbxMaterial.h"
#include "sfbxAnimation.h"
#include "sfbxDocument.h"
#include "sfbxParser.h"
#include <cassert>
#include <fstream>

namespace sfbx {

static const uint8_t g_fbx_header_magic[23]{
    'K', 'a', 'y', 'd', 'a', 'r', 'a', ' ', 'F', 'B', 'X', ' ', 'B', 'i', 'n', 'a', 'r', 'y', ' ', ' ', 0x00, 0x1a, 0x00,
};

// *** these must not be changed. it leads to CRC check failure. ***
static const char g_fbx_time_id[] = "1970-01-01 10:00:00:000";
static const uint8_t g_fbx_file_id[16]{ 0x28, 0xb3, 0x2a, 0xeb, 0xb6, 0x24, 0xcc, 0xc2, 0xbf, 0xc8, 0xb0, 0x2a, 0xa9, 0x2b, 0xfc, 0xf1 };
static const uint8_t g_fbx_footer_magic1[16]{ 0xfa, 0xbc, 0xab, 0x09, 0xd0, 0xc8, 0xd4, 0x66, 0xb1, 0x76, 0xfb, 0x83, 0x1c, 0xf7, 0x26, 0x7e };
static const uint8_t g_fbx_footer_magic2[16]{ 0xf8, 0x5a, 0x8c, 0x6a, 0xde, 0xf5, 0xd9, 0x7e, 0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b };


Document::Document()
{
    initialize();
}

Document::Document(std::istream& input)
{
    read(input);
}

Document::Document(const std::string& path)
{
	global_settings.path = path;
    read(path);
}

bool Document::valid() const
{
    return !m_objects.empty();
}

void Document::initialize()
{
	m_root_model = createObject<Root>("Scene");
	m_root_model->setID(0);
	
	// Set the FBX version to 7500
	m_version = FileVersion::Fbx2016; // Assuming FileVersion::Fbx7500 == 7500

}
bool Document::readAscii(std::istream& is)
{
	{
		std::string line;
		std::getline(is, line);
		
		int major, minor, patch;
		const char* s = std::strstr(line.c_str(), "FBX");
		if (!s || sscanf(s, "FBX %d.%d.%d project file", &major, &minor, &patch) != 3) {
			sfbxPrint("sfbx::Document::read(): not a fbx file\n");
			return false;
		}
		m_version = (FileVersion)(major * 1000 + minor * 100);
	}
	
	try {
		while (!is.eof()) {
			std::string block = read_brace_block(is);
			if (block.empty())
				break;
			
			string_view block_view = block;
			auto node = createNode();
			node->readAscii(block_view);
			if (node->isNull()) {
				eraseNode(node);
				break;
			}
		}
		importFBXObjects();
	}
	catch (const std::runtime_error& e) {
		sfbxPrint("sfbx::Document::read(): exception %s\n", e.what());
		return false;
	}
	
	return true;
}

bool Document::readBinary(std::istream& is)
{
    char magic[std::size(g_fbx_header_magic)];
    readv(is, magic, std::size(g_fbx_header_magic));
    if (memcmp(magic, g_fbx_header_magic, std::size(g_fbx_header_magic)) != 0) {
        sfbxPrint("sfbx::Document::read(): not a fbx file\n");
        return false;
    }
    m_version = (FileVersion)read1<uint32_t>(is);

    try {
        uint64_t pos = std::size(g_fbx_header_magic) + 4;
        for (;;) {
            auto node = createNode();
            pos += node->readBinary(is, pos);
            if (node->isNull()) {
                eraseNode(node);
                break;
            }
        }
        importFBXObjects();
		
    }
    catch (const std::runtime_error& e) {
        sfbxPrint("sfbx::Document::read(): exception %s\n", e.what());
        return false;
    }
    return true;
}

void GlobalSettings::importFBXObjects(Document *doc)
{
    Node* global_settings = doc->findNode(sfbxS_GlobalSettings);
    if (!global_settings)
        return;

//    global_settings->createChild(sfbxS_Version, sfbxI_GlobalSettingsVersion);

    Node *prop = global_settings->findChild(sfbxS_Properties70);
    if (!prop)
        prop = global_settings->findChild(sfbxS_Properties);
    if (!prop)
        return;

    for (auto& c : prop->getChildren()) {
        auto propssize = c->getProperties().size();
        if (c->getName() != sfbxS_P || propssize < 1)
            continue;

        auto name = c->getProperty(0)->getString();

        //TODO helper, impl all
        if (name == sfbxS_DefaultCamera) {
//                        prop->createChild(sfbxS_P, sfbxS_DefaultCamera, sfbxS_KString, "", "", "Producer Perspective");
            assert(propssize >= 5);
            camera = c->getProperty(4)->getString();
        }
        if (name == sfbxS_TimeSpanStop) {
//                        prop->createChild(sfbxS_P, sfbxS_DefaultCamera, sfbxS_KString, "", "", "Producer Perspective");
            assert(propssize >= 5);
            time_stop = c->getProperty(4)->getValue<int64>();
        }

		// Check for the UpAxis property
		if (name == sfbxS_UpAxis) {
			assert(propssize >= 5);
			up_axis = c->getProperty(4)->getValue<int>();
		}
		
		// Check for the UpAxisSign property
		if (name == sfbxS_UpAxisSign) {
			assert(propssize >= 5);
			up_axis_sign = c->getProperty(4)->getValue<int>();
		}

//            prop->createChild(sfbxS_P, sfbxS_FrontAxis, sfbxS_int, sfbxS_Integer, "", 2);
//            prop->createChild(sfbxS_P, sfbxS_FrontAxisSign, sfbxS_int, sfbxS_Integer, "", 1);
//            prop->createChild(sfbxS_P, sfbxS_CoordAxis, sfbxS_int, sfbxS_Integer, "", 0);
//            prop->createChild(sfbxS_P, sfbxS_CoordAxisSign, sfbxS_int, sfbxS_Integer, "", 1);
//            prop->createChild(sfbxS_P, sfbxS_OriginalUpAxis, sfbxS_int, sfbxS_Integer, "", -1);
//            prop->createChild(sfbxS_P, sfbxS_OriginalUpAxisSign, sfbxS_int, sfbxS_Integer, "", 1);

		if (name == sfbxS_UnitScaleFactor) {
			assert(propssize >= 5);
			unit_scale = c->getProperty(4)->getValue<float64>();
		}

		if (name == sfbxS_CustomFrameRate) {
			assert(propssize >= 5);
			frame_rate = c->getProperty(4)->getValue<float64>();
			
			if(frame_rate <= 0){
				frame_rate = 60;
			}
		}

//            prop->createChild(sfbxS_P, sfbxS_UnitScaleFactor, sfbxS_double, sfbxS_Number, "", 1.0);
//            prop->createChild(sfbxS_P, sfbxS_OriginalUnitScaleFactor, sfbxS_double, sfbxS_Number, "", 1.0);
//            prop->createChild(sfbxS_P, sfbxS_AmbientColor, sfbxS_ColorRGB sfbxS_ColorRGB, sfbxS_Color, "", 0.0, 0.0, 0.0);

//            prop->createChild(sfbxS_P, sfbxS_TimeMode, sfbxS_enum, "", "", 0);
//            prop->createChild(sfbxS_P, sfbxS_TimeProtocol, sfbxS_enum, "", "", 2);
//            prop->createChild(sfbxS_P, sfbxS_SnapOnFrameMode, sfbxS_enum, "", "", 0);
//            prop->createChild(sfbxS_P, sfbxS_TimeSpanStart, sfbxS_KTime, sfbxS_Time, "", (int64)0);
//            prop->createChild(sfbxS_P, sfbxS_CustomFrameRate, sfbxS_double, sfbxS_Number, "", -1.0);

		if (name == sfbxS_CustomFrameRate) {
			assert(propssize >= 5);
			double frameRate = c->getProperty(4)->getValue<float64>();
		}

//            prop->createChild(sfbxS_P, sfbxS_TimeMarker, sfbxS_Compound, "", "");
//            prop->createChild(sfbxS_P, sfbxS_CurrentTimeMarker, sfbxS_int, sfbxS_Integer, "", -1);

    }

}


bool Document::read(std::istream& is)
{
    unload();

    char c;
    is.get(c);
    is.unget();

    // ascii FBX must start with ';'. even FBX SDK doesn't recognize without that.
    if (c == ';')
        return readAscii(is);
    else
        return readBinary(is);
}

bool Document::read(const std::string& path)
{
    unload();

    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (file)
        return read(file);
    return false;
}


bool Document::writeBinary(std::ostream& os) 
{
    writev(os, g_fbx_header_magic);
    writev(os, m_version);

    uint64_t pos = std::size(g_fbx_header_magic) + 4;
    for (Node* node : m_root_nodes)
        pos += node->writeBinary(os, pos);
    {
        Node null_node;
		null_node.setDocument(this);
        pos += null_node.writeBinary(os, pos);
    }

    // footer

    writev(os, g_fbx_footer_magic1);
    pos += std::size(g_fbx_footer_magic1);

    // add padding to 16 byte align
    uint64_t pad = 16 - (pos % 16);
    for (uint64_t i = 0; i < pad; ++i)
        writev(os, (int8)0);

    writev(os, (int32)0);
    writev(os, m_version);

    // 120 byte space
    for (uint64_t i = 0; i < 120; ++i)
        writev(os, (int8)0);

    writev(os, g_fbx_footer_magic2);

    return true;
}

bool Document::writeBinary(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeBinary(file);
    return false;
}

bool Document::writeAscii(std::ostream& os) const
{
	char version[128];
	sprintf(version, "; FBX %d.%d.0 project file\n", (int)m_version / 1000 % 10, (int)m_version / 100 % 10);
	
	os << version;
	// Add the missing header lines
	os << "; Created by Power Engine \n";
	os << "; https://x.com/cybertwip \n";
	os << "; -------------------------------------------------\n\n";
	
	for (auto node : getRootNodes()) {
		// these nodes seem required only in binary format.
		if (node->getName() == sfbxS_FileId ||
			node->getName() == sfbxS_CreationTime ||
			node->getName() == sfbxS_Creator)
			continue;
		node->writeAscii(os);
	}
	
	return true;
}

bool Document::writeAscii(const std::string& path) const
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeAscii(file);
    return false;
}



void Document::unload()
{
    m_version = FileVersion::Default;

    m_nodes.clear();
    m_root_nodes.clear();

    m_objects.clear();
    m_anim_stacks.clear();
    m_root_model = {};
    m_current_take = {};
}

FileVersion Document::getFileVersion() const
{
    return m_version;
}


void Document::setFileVersion(FileVersion v)
{
    m_version = v;
}


Node* Document::createNode(string_view name)
{
    auto n = createChildNode(name);
    m_root_nodes.push_back(n);
    return n;
}
Node* Document::createChildNode(string_view name)
{
    auto n = new Node();
	n->setDocument(this);
	n->setName(name);
    m_nodes.push_back(NodePtr(n));
    return n;
}

void Document::eraseNode(Node* n)
{
    erase_if(m_nodes, [n](const NodePtr& p) { return p.get() == n; });
    erase(m_root_nodes, n);
}

Node* Document::findNode(string_view name) const
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&name](const NodePtr& p) { return p->getName() == name; });
    return it != m_nodes.end() ? it->get() : nullptr;
}

span<sfbx::NodePtr> Document::getAllNodes() const { return make_span(m_nodes); }
span<Node*> Document::getRootNodes() const { return make_span(m_root_nodes); }

void Document::createLinkOO(ObjectPtr child, ObjectPtr parent, string_view type)
{
	if (!child || !parent)
		return;
	if (auto c = findNode(sfbxS_Connections)) {
		if (!type.empty()) {
			c->createChild(sfbxS_C, sfbxS_OO, child->getID(), parent->getID(), type);
		} else {
			c->createChild(sfbxS_C, sfbxS_OO, child->getID(), parent->getID());
		}
	}
	// Store the connection
	m_connections.emplace_back(child, parent, Connection::Type::OO, std::string(type));
}


void Document::createLinkOP(ObjectPtr child, ObjectPtr parent, string_view target)
{
	if (!child || !parent)
		return;
	if (auto c = findNode(sfbxS_Connections))
		c->createChild(sfbxS_C, sfbxS_OP, child->getID(), parent->getID(), target);
	// **Store the connection**
	m_connections.emplace_back(child, parent, Connection::Type::OP, std::string(target));
}

void Document::importFBXObjects()
{
	if (Node* objects = findNode(sfbxS_Objects)) {
		initialize();
		for (Node* n : objects->getChildren()) {
			if (ObjectPtr obj = createObject(GetObjectClass(n), GetObjectSubClass(n))) {
				obj->setNode(n);
			}
		}
	}
	
	if (Node* connections = findNode(sfbxS_Connections)) {
		for (Node* n : connections->getChildren()) {
			auto name = n->getName();
			auto ct = GetPropertyString(n, 0);
			if (name == sfbxS_C && ct == sfbxS_OO) {
				ObjectPtr child = FindObjectById(m_objects, GetPropertyValue<int64>(n, 1));
				ObjectPtr parent = FindObjectById(m_objects, GetPropertyValue<int64>(n, 2));
				if (child && parent) {
					parent->addChild(child);
					// **Store the connection**
					m_connections.emplace_back(child, parent, Connection::Type::OO);
				}
			}
			else if (name == sfbxS_C && ct == sfbxS_OP) {
				ObjectPtr child = FindObjectById(m_objects, GetPropertyValue<int64>(n, 1));
				ObjectPtr parent = FindObjectById(m_objects, GetPropertyValue<int64>(n, 2));
				auto p = GetPropertyString(n, 3);
				if (child && parent) {
					parent->addChild(child, p);
					// **Store the connection**
					m_connections.emplace_back(child, parent, Connection::Type::OP, std::string(p));
				}
			}
#ifdef sfbxEnableLegacyFormatSupport
			else if (name == sfbxS_Connect && ct == sfbxS_OO) {
				ObjectPtr child = FindObjectByName(m_objects, GetPropertyString(n, 1));
				ObjectPtr parent = FindObjectByName(m_objects, GetPropertyString(n, 2));
				if (child && parent) {
					parent->addChild(child);
					// **Store the connection**
					m_connections.emplace_back(child, parent, Connection::Type::OO);
				}
			}
#endif
			else {
				sfbxPrint("sfbx::Document::read(): unrecognized connection type %s %s\n",
						  std::string(name).c_str(), std::string(ct).c_str());
			}
		}
	}
	
	global_settings.importFBXObjects(this);
	
	// index-based loop because m_objects may be push_backed in the loop
	for (size_t i = 0; i < m_objects.size(); ++i) {
		auto obj = m_objects[i];
		obj->importFBXObjects();
	}
	
	if (Node* takes = findNode(sfbxS_Takes)) {
		auto current = GetChildPropertyString(takes, sfbxS_Current);
		if (auto t = findAnimationStack(current))
			m_current_take = t;
	}
}

// **Implement the getConnections method**
const std::vector<Connection>& Document::getConnections() const {
	return m_connections;
}


ObjectPtr Document::createObject(ObjectClass c, ObjectSubClass s)
{
    Object* r{};
    switch (c) {
    case ObjectClass::NodeAttribute:
        switch (s) {
        case ObjectSubClass::Null: r = new NullAttribute(); break;
        case ObjectSubClass::Root: r = new RootAttribute(); break;
        case ObjectSubClass::LimbNode: r = new LimbNodeAttribute(); break;
        case ObjectSubClass::Light: r = new LightAttribute(); break;
        case ObjectSubClass::Camera: r = new CameraAttribute(); break;
        default: r = new NodeAttribute(); break;
        }
        break;
    case ObjectClass::Model:
        switch (s) {
        case ObjectSubClass::Null: r = new Null(); break;
        case ObjectSubClass::Root: r = new Root(); break;
        case ObjectSubClass::LimbNode: r = new LimbNode(); break;
        case ObjectSubClass::Mesh: r = new Mesh(); break;
        case ObjectSubClass::Light: r = new Light(); break;
        case ObjectSubClass::Camera: r = new Camera(); break;
        default: r = new Model(); break;
        }
        break;
    case ObjectClass::Geometry:
        switch (s) {
        case ObjectSubClass::Mesh: r = new GeomMesh(); break;
        case ObjectSubClass::Shape: r = new Shape(); break;
        default: r = new Geometry(); break;
        }
        break;
    case ObjectClass::Deformer:
        switch (s) {
        case ObjectSubClass::Skin: r = new Skin(); break;
        case ObjectSubClass::Cluster: r = new Cluster(); break;
        case ObjectSubClass::BlendShape: r = new BlendShape(); break;
        case ObjectSubClass::BlendShapeChannel: r = new BlendShapeChannel(); break;
        default: r = new Deformer(); break;
        }
        break;
    case ObjectClass::Pose:
        switch (s) {
        case ObjectSubClass::BindPose: r = new BindPose(); break;
        default: r = new Pose(); break;
        }
        break;
    case ObjectClass::Video:             r = new Video(); break;
    case ObjectClass::Texture:           r = new Texture(); break;
    case ObjectClass::Material:          r = new Material(); break;
    case ObjectClass::AnimationStack:    r = new AnimationStack(); break;
    case ObjectClass::AnimationLayer:    r = new AnimationLayer(); break;
    case ObjectClass::AnimationCurveNode:r = new AnimationCurveNode(); break;
    case ObjectClass::AnimationCurve:    r = new AnimationCurve(); break;
    case ObjectClass::Implementation:    r = new Implementation(); break;
    case ObjectClass::BindingTable:      r = new BindingTable(); break;
    default: break;
    }

	ObjectPtr pointer = nullptr;
    if (r) {
		pointer = ObjectPtr(r);
        addObject(pointer);
    }
    else {
        sfbxPrint("sfbx::Document::createObject(): unrecongnized type \"%s\"\n", GetObjectClassName(c).data());
    }
	
	pointer->setDocument(this);
	
    return pointer;
}

template<class T>
std::shared_ptr<T> Document::createObject(string_view name)
{
    T* r = new T();
	
	std::shared_ptr<T> pointer(r);
	pointer->setName(name);
    addObject(pointer);
    return pointer;
}

#define Body(T) template std::shared_ptr<T> Document::createObject(string_view name);
sfbxEachObjectType(Body)
#undef Body

void Document::addObject(ObjectPtr obj, bool check)
{
    if (obj) {
        if (check && find(m_objects, obj))
            return;
        if (auto take = as<AnimationStack>(obj))
            m_anim_stacks.push_back(take);
		obj->setDocument(this);
		m_objects.push_back(obj);
	}
}

void Document::eraseObject(ObjectPtr obj)
{
    erase_if(m_objects, [obj](const ObjectPtr& p) { return p == obj; });
    erase(m_anim_stacks, obj);

    //// this should not be happen
    //if (obj == m_root_model)
    //    m_root_model = nullptr;
}

// "The pointer is dangling" warning. can be ignored because m_objects hold a reference.
#pragma warning(push)
#pragma warning(disable:26815)

ObjectPtr Document::findObject(int64 id) const
{
	auto iterator = std::find_if(m_objects.begin(), m_objects.end(), [&id](ObjectPtr p) { return p->getID() == id; });
	
	if(iterator != m_objects.end()){
		return *iterator;
	} else {
		return nullptr;
	}
}

#pragma warning(pop)

ObjectPtr Document::findObject(string_view name) const
{
    return FindObjectByName(m_objects, name);
}

span<ObjectPtr> Document::getAllObjects() const { return make_span(m_objects); }
std::shared_ptr<Model> Document::getRootModel() const { return m_root_model; }

span<std::shared_ptr<AnimationStack>> Document::getAnimationStacks() const
{
    return make_span(m_anim_stacks);
}

std::shared_ptr<AnimationStack> Document::findAnimationStack(string_view name) const
{
    return FindObjectByName(m_anim_stacks, name);
}

std::shared_ptr<AnimationStack> Document::getCurrentTake() const { return m_current_take; }
void Document::setCurrentTake(std::shared_ptr<AnimationStack> v) { m_current_take = v; }


bool Document::mergeAnimations(Document* doc)
{
    if (!doc || !doc->valid())
        return false;

    size_t num_merged = 0;
    for (auto take : doc->getAnimationStacks()) {
        if (take->remap(this))
            ++num_merged;
    }
    if (!m_current_take && !m_anim_stacks.empty())
        m_current_take = m_anim_stacks.front();
    return num_merged != 0;
}

bool Document::mergeAnimations(std::istream& input)
{
    return mergeAnimations(MakeDocument(input));
}

bool Document::mergeAnimations(const std::string& path)
{
    return mergeAnimations(MakeDocument(path));
}

void Document::exportFBXNodes()
{
	m_nodes.clear();
	m_root_nodes.clear();
	
	std::time_t t = std::time(nullptr);
	std::tm* now = std::localtime(&t);
	std::string take_name{ m_current_take ? m_current_take->getName() : "" };
	
	// Create the FBXHeaderExtension node
	auto header_extension = createNode(sfbxS_FBXHeaderExtension);
	{
		header_extension->createChild(sfbxS_FBXHeaderVersion, (int32_t)1003);
		header_extension->createChild(sfbxS_FBXVersion, (int32_t)getFileVersion());
		header_extension->createChild(sfbxS_EncryptionType, (int32_t)0);
		{
			auto timestamp = header_extension->createChild(sfbxS_CreationTimeStamp);
			timestamp->createChild(sfbxS_Version, 1000);
			timestamp->createChild(sfbxS_Year, now->tm_year + 1900);
			timestamp->createChild(sfbxS_Month, now->tm_mon + 1);
			timestamp->createChild(sfbxS_Day, now->tm_mday);
			timestamp->createChild(sfbxS_Hour, now->tm_hour);
			timestamp->createChild(sfbxS_Minute, now->tm_min);
			timestamp->createChild(sfbxS_Second, now->tm_sec);
			timestamp->createChild(sfbxS_Millisecond, 0);
		}
		header_extension->createChild(sfbxS_Creator, "Power Engine Alpha 1.0");
		{
			auto other_flags = header_extension->createChild(sfbxS_OtherFlags);
			other_flags->createChild(sfbxS_TCDefinition, sfbxI_TCDefinition);
		}
		{
			auto scene_info = header_extension->createChild(sfbxS_SceneInfo, MakeFullName(sfbxS_GlobalInfo, sfbxS_SceneInfo), sfbxS_UserData);
			scene_info->createChild(sfbxS_Type, sfbxS_UserData);
			scene_info->createChild(sfbxS_Version, 100);
			{
				auto meta = scene_info->createChild(sfbxS_MetaData);
				meta->createChild(sfbxS_Version, 100);
				meta->createChild(sfbxS_Title, "");
				meta->createChild(sfbxS_Subject, "");
				meta->createChild(sfbxS_Author, "");
				meta->createChild(sfbxS_Keywords, "");
				meta->createChild(sfbxS_Revision, "");
				meta->createChild(sfbxS_Comment, "");
			}
			{
				auto prop = scene_info->createChild(sfbxS_Properties70);
				prop->createChild(sfbxS_P, sfbxS_DocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
				prop->createChild(sfbxS_P, sfbxS_SrcDocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
				prop->createChild(sfbxS_P, sfbxS_Original, sfbxS_Compound, "", "");
				prop->createChild(sfbxS_P, sfbxS_OriginalApplicationVendor, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_OriginalApplicationName, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_OriginalApplicationVersion, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_OriginalDateTime_GMT, sfbxS_DateTime, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_OriginalFileName, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_LastSaved, sfbxS_Compound, "", "");
				prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationVendor, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationName, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationVersion, sfbxS_KString, "", "", "");
				prop->createChild(sfbxS_P, sfbxS_LastSavedDateTime_GMT, sfbxS_DateTime, "", "", "");
			}
		}
	}
	
	createNode(sfbxS_FileId)->addProperties(make_span(g_fbx_file_id));
	createNode(sfbxS_CreationTime)->addProperties(g_fbx_time_id);
	createNode(sfbxS_Creator)->addProperties("Power Engine Alpha 1.0");
	
	// Create the GlobalSettings node
	auto global_settings = createNode(sfbxS_GlobalSettings);
	{
		global_settings->createChild(sfbxS_Version, sfbxI_GlobalSettingsVersion);
		auto prop = global_settings->createChild(sfbxS_Properties70);
		
		// Set up global properties
		prop->createChild(sfbxS_P, sfbxS_UpAxis, sfbxS_int, sfbxS_Integer, "", this->global_settings.up_axis);
		prop->createChild(sfbxS_P, sfbxS_UpAxisSign, sfbxS_int, sfbxS_Integer, "", this->global_settings.up_axis_sign);
		prop->createChild(sfbxS_P, sfbxS_FrontAxis, sfbxS_int, sfbxS_Integer, "", 0);
		prop->createChild(sfbxS_P, sfbxS_FrontAxisSign, sfbxS_int, sfbxS_Integer, "", 1);
		prop->createChild(sfbxS_P, sfbxS_CoordAxis, sfbxS_int, sfbxS_Integer, "", 1);
		prop->createChild(sfbxS_P, sfbxS_CoordAxisSign, sfbxS_int, sfbxS_Integer, "", 1);
		prop->createChild(sfbxS_P, sfbxS_OriginalUpAxis, sfbxS_int, sfbxS_Integer, "", -1);
		prop->createChild(sfbxS_P, sfbxS_OriginalUpAxisSign, sfbxS_int, sfbxS_Integer, "", 1);
		prop->createChild(sfbxS_P, sfbxS_UnitScaleFactor, sfbxS_double, sfbxS_Number, "", this->global_settings.unit_scale);
		prop->createChild(sfbxS_P, sfbxS_OriginalUnitScaleFactor, sfbxS_double, sfbxS_Number, "", this->global_settings.original_unit_scale);
		prop->createChild(sfbxS_P, sfbxS_AmbientColor, sfbxS_ColorRGB, sfbxS_Color, "", 0.0, 0.0, 0.0);
		prop->createChild(sfbxS_P, sfbxS_DefaultCamera, sfbxS_KString, "", "", this->global_settings.camera);
		prop->createChild(sfbxS_P, sfbxS_TimeMode, sfbxS_enum, "", "", 0);
		prop->createChild(sfbxS_P, sfbxS_TimeProtocol, sfbxS_enum, "", "", 2);
		prop->createChild(sfbxS_P, sfbxS_SnapOnFrameMode, sfbxS_enum, "", "", 0);
		prop->createChild(sfbxS_P, sfbxS_TimeSpanStart, sfbxS_KTime, sfbxS_Time, "", (int64)0);
		prop->createChild(sfbxS_P, sfbxS_TimeSpanStop, sfbxS_KTime, sfbxS_Time, "", (int64)sfbxI_TicksPerSecond);
		prop->createChild(sfbxS_P, sfbxS_CustomFrameRate, sfbxS_double, sfbxS_Number, "", -1.0);
		prop->createChild(sfbxS_P, sfbxS_TimeMarker, sfbxS_Compound, "", "");
		prop->createChild(sfbxS_P, sfbxS_CurrentTimeMarker, sfbxS_int, sfbxS_Integer, "", -1);
	}
	
	// Create the Documents node
	auto documents = createNode(sfbxS_Documents);
	{
		documents->createChild(sfbxS_Count, (int32)1);
		auto doc = documents->createChild(sfbxS_Document, (int64)this, "My Scene", "Scene");
		{
			auto prop = doc->createChild(sfbxS_Properties70);
			prop->createChild(sfbxS_P, sfbxS_SourceObject, sfbxS_object, "", "");
			prop->createChild(sfbxS_P, sfbxS_ActiveAnimStackName, sfbxS_KString, "", "", take_name);
			
			doc->createChild(sfbxS_RootNode, 0);
		}
	}
	
	auto references = createNode(sfbxS_References);
	
	// Create the Definitions node
	auto definitions = createNode(sfbxS_Definitions);
	
	// Include PropertyTemplate blocks
	{
		auto add_object_type = [definitions](size_t n, const char* type, const char* templateName = nullptr, std::function<void(Node*)> addProperties = nullptr) -> Node* {
			if (n == 0)
				return nullptr;
			auto ot = definitions->createChild(sfbxS_ObjectType, type);
			ot->createChild(sfbxS_Count, (int32)n);
			
			// Add PropertyTemplate if templateName is provided
			if (templateName) {
				auto prop_template = ot->createChild("PropertyTemplate", templateName);
				auto props = prop_template->createChild(sfbxS_Properties70);
				if (addProperties)
					addProperties(props);
			}
			
			return ot;
		};
		
		// GlobalSettings do not need PropertyTemplate
		add_object_type(1, sfbxS_GlobalSettings);
		
		// NodeAttribute with FbxSkeleton PropertyTemplate
		add_object_type(countObjects<NodeAttribute>(), sfbxS_NodeAttribute, "FbxSkeleton", [](Node* props) {
			props->createChild(sfbxS_P, sfbxS_Color, sfbxS_ColorRGB, sfbxS_Color, "", 0.8, 0.8, 0.8);
			props->createChild(sfbxS_P, "Size", sfbxS_double, sfbxS_Number, "", 33.3333);
			props->createChild(sfbxS_P, "LimbLength", sfbxS_double, sfbxS_Number, "H", 1.0);
		});
		
		// Model with FbxNode PropertyTemplate
		add_object_type(countObjects<Model>(), sfbxS_Model, "FbxNode", [](Node* props) {
			props->createChild(sfbxS_P, "QuaternionInterpolate", sfbxS_enum, "", "", 0);
			props->createChild(sfbxS_P, "RotationOffset", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "RotationPivot", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "ScalingOffset", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "ScalingPivot", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "TranslationActive", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMin", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "TranslationMax", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "TranslationMinX", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMinY", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMinZ", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMaxX", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMaxY", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "TranslationMaxZ", sfbxS_bool, "", "", 0);
			// Continue adding properties as per the correct FBX file
			props->createChild(sfbxS_P, sfbxS_LclTranslation, sfbxS_LclTranslation, "", "A", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, sfbxS_LclRotation, sfbxS_LclRotation, "", "A", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, sfbxS_LclScale, sfbxS_LclScale, "", "A", 1.0, 1.0, 1.0);
			props->createChild(sfbxS_P, sfbxS_Visibility, sfbxS_Visibility, "", "A", 1.0);
			props->createChild(sfbxS_P, "Visibility Inheritance", "Visibility Inheritance", "", "", 1);
		});
		
		// Geometry with FbxMesh PropertyTemplate
		add_object_type(countObjects<Geometry>(), sfbxS_Geometry, "FbxMesh", [](Node* props) {
			props->createChild(sfbxS_P, sfbxS_Color, sfbxS_ColorRGB, sfbxS_Color, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "BBoxMin", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "BBoxMax", sfbxS_Vector3D, sfbxS_Vector, "", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "Primary Visibility", sfbxS_bool, "", "", 1);
			props->createChild(sfbxS_P, "Casts Shadows", sfbxS_bool, "", "", 1);
			props->createChild(sfbxS_P, "Receive Shadows", sfbxS_bool, "", "", 1);
		});
		
		// Material with FbxSurfacePhong PropertyTemplate
		add_object_type(countObjects<Material>(), sfbxS_Material, "FbxSurfacePhong", [](Node* props) {
			props->createChild(sfbxS_P, "ShadingModel", sfbxS_KString, "", "", "Phong");
			props->createChild(sfbxS_P, "MultiLayer", sfbxS_bool, "", "", 0);
			props->createChild(sfbxS_P, "EmissiveColor", sfbxS_Color, "", "A", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "EmissiveFactor", sfbxS_Number, "", "A", 1.0);
			props->createChild(sfbxS_P, "AmbientColor", sfbxS_Color, "", "A", 0.2, 0.2, 0.2);
			props->createChild(sfbxS_P, "AmbientFactor", sfbxS_Number, "", "A", 1.0);
			props->createChild(sfbxS_P, "DiffuseColor", sfbxS_Color, "", "A", 0.8, 0.8, 0.8);
			props->createChild(sfbxS_P, "DiffuseFactor", sfbxS_Number, "", "A", 1.0);
			// Continue adding properties as per the correct FBX file
			props->createChild(sfbxS_P, "SpecularColor", sfbxS_Color, "", "A", 0.2, 0.2, 0.2);
			props->createChild(sfbxS_P, "SpecularFactor", sfbxS_Number, "", "A", 1.0);
			props->createChild(sfbxS_P, "ShininessExponent", sfbxS_Number, "", "A", 20.0);
			props->createChild(sfbxS_P, "ReflectionColor", sfbxS_Color, "", "A", 0.0, 0.0, 0.0);
			props->createChild(sfbxS_P, "ReflectionFactor", sfbxS_Number, "", "A", 1.0);
		});
		
		// If you have Lights
		add_object_type(countObjects<Light>(), sfbxS_Light, "FbxLight", [](Node* props) {
			props->createChild(sfbxS_P, sfbxS_Color, sfbxS_ColorRGB, sfbxS_Color, "", 1.0, 1.0, 1.0);
			props->createChild(sfbxS_P, sfbxS_Intensity, sfbxS_Number, "", "A", 100.0);
			// Add other properties as needed
		});
		
		// Add other object types and their PropertyTemplates as needed
		// For example, for Textures, Cameras, etc.
	}
	
	createNode(sfbxS_Objects);
	createNode(sfbxS_Connections);
	
	// Export objects and connections
	for (size_t i = 0; i < m_objects.size(); ++i)
		m_objects[i]->exportFBXObjects();
	for (size_t i = 0; i < m_objects.size(); ++i)
		m_objects[i]->exportFBXConnections();
	
	// Create the Takes node
	auto takes = createNode(sfbxS_Takes);
	takes->createChild(sfbxS_Current, take_name);
	for (auto t : m_anim_stacks) {
		auto take = takes->createChild(sfbxS_Take, t->getName());
		take->createChild(sfbxS_FileName, std::string(t->getName()) + ".tak");
		
		float lstart = t->getLocalStart();
		float lstop = t->getLocalStop();
		float rstart = t->getReferenceStart();
		float rstop = t->getReferenceStop();
		if (lstart != 0.0f || lstop != 0.0f)
			take->createChild(sfbxS_LocalTime, ToTicks(lstart), ToTicks(lstop));
		if (rstart != 0.0f || rstop != 0.0f)
			take->createChild(sfbxS_ReferenceTime, ToTicks(rstart), ToTicks(rstop));
	}
}

template<class T>  std::shared_ptr<T>  Object::createChild(string_view name)
{
    auto ret = document()->createObject<T>(name);
    addChild(ret);
    return ret;
}
#define Body(T) template std::shared_ptr<T>  Object::createChild(string_view name);
sfbxEachObjectType(Body)
#undef Body

} // namespace sfbx
