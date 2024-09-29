#pragma once
#include "sfbxObject.h"

namespace sfbx {

enum class FileVersion : int
{
	Unknown = 0,
	
	Fbx2014 = 7400,
	Fbx2015 = Fbx2014,
	
	Fbx2016 = 7500,
	Fbx2017 = Fbx2016,
	Fbx2018 = Fbx2016,
	
	Fbx2019 = 7700,
	Fbx2020 = Fbx2019,
	
	Default = Fbx2020,
};

struct GlobalSettings
{
	double unit_scale = 1;
	double original_unit_scale = 1;
	double frame_rate = 60;
	std::string camera = "Camera";
	std::string path = "";
	int64_t time_stop = 0; // sfbxI_TicksPerSecond;
	
	int up_axis = 1;
	int up_axis_sign = 1;
	
	void exportFBXNodes(Document* doc);
	void importFBXObjects(Document* doc);
};

// **Add the Connection struct**
struct Connection
{
	enum class Type { OO, OP };
	
	ObjectPtr src;
	ObjectPtr dest;
	Type type;
	std::string property_name;
	
	Connection(ObjectPtr s, ObjectPtr d, Type t, std::string prop = "")
	: src(s), dest(d), type(t), property_name(std::move(prop)) {}
};

class Document
{
public:
	Document();
	explicit Document(std::istream& is);
	explicit Document(const std::string& path);
	bool valid() const;
	
	bool read(std::istream& is);
	bool read(const std::string& path);
	bool writeBinary(std::ostream& os);
	bool writeBinary(const std::string& path);
	bool writeAscii(std::ostream& os) const;
	bool writeAscii(const std::string& path) const;
	
	FileVersion getFileVersion() const;
	void setFileVersion(FileVersion v);
	
	// T: Mesh, Camera, Light, LimbNode, Skin, etc.
	template<class T>
	std::shared_ptr<T> createObject(string_view name = {});
	
	ObjectPtr findObject(int64 id) const;
	ObjectPtr findObject(string_view name) const;
	span<ObjectPtr> getAllObjects() const;
	span<Object*> getRootObjects() const;
	std::shared_ptr<Model> getRootModel() const;
	
	span<std::shared_ptr<AnimationStack>> getAnimationStacks() const;
	std::shared_ptr<AnimationStack> findAnimationStack(string_view name) const;
	std::shared_ptr<AnimationStack> getCurrentTake() const;
	void setCurrentTake(std::shared_ptr<AnimationStack> v);
	
	bool mergeAnimations(Document* doc);
	bool mergeAnimations(DocumentPtr doc) { return mergeAnimations(doc.get()); }
	bool mergeAnimations(std::istream& input);
	bool mergeAnimations(const std::string& path);
	
	void exportFBXNodes();
	
	// **Add the getConnections method**
	const std::vector<Connection>& getConnections() const;
	
	// utils
	template<class T>
	size_t countObjects() const
	{
		// Recursive helper function
		auto recursiveCount = [](const auto& object, auto&& recursiveCount) -> size_t {
			size_t count = 0;
			if (as<T>(object) && object->getID() != 0) {
				count++;
			}
			// Assuming object has a getChildren() method that returns a container of children
			for (const auto& child : object->getChildren()) {
				count += recursiveCount(child, recursiveCount);
			}
			return count;
		};
		
		size_t totalCount = 0;
		for (const auto& p : m_objects) {
			totalCount += recursiveCount(p, recursiveCount);
		}
		return totalCount;
	}
	GlobalSettings global_settings;
	
public:
	// internal
	
	void unload();
	bool readAscii(std::istream& is);
	bool readBinary(std::istream& is);
	
	Node* createNode(string_view name = {});
	Node* createChildNode(string_view name = {});
	void eraseNode(Node* n);
	Node* findNode(string_view name) const;
	span<NodePtr> getAllNodes() const;
	span<Node*> getRootNodes() const;
	
	ObjectPtr createObject(ObjectClass t, ObjectSubClass s);
	void addObject(ObjectPtr obj, bool check = false);
	
	void eraseObject(ObjectPtr obj);
	
	void createLinkOO(ObjectPtr child, ObjectPtr parent, string_view type = {});
	void createLinkOP(ObjectPtr child, ObjectPtr parent, string_view target);
	
private:
	void initialize();
	void importFBXObjects();
	
	FileVersion m_version = FileVersion::Default;
	
	std::vector<NodePtr> m_nodes;
	std::vector<Node*> m_root_nodes;
	
	std::vector<ObjectPtr> m_objects;
	std::vector<std::shared_ptr<AnimationStack>> m_anim_stacks;
	std::shared_ptr<Model> m_root_model{};
	std::shared_ptr<AnimationStack> m_current_take{};
	
	// **Add the connections member variable**
	std::vector<Connection> m_connections;
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
	return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx
