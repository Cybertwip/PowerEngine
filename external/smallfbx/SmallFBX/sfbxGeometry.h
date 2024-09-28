#pragma once
#include "sfbxObject.h"

namespace sfbx {

template<class T>
inline constexpr bool is_deformer = std::is_base_of_v<Deformer, T>;


// Geometry and its subclasses:
//  (Mesh, Shape)

class Geometry : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(ObjectPtr v) override;
    void eraseChild(ObjectPtr v) override;

	std::shared_ptr<Model> getModel() const;
    bool hasDeformer() const;
    bool hasSkinDeformer() const;
    span<std::shared_ptr<Deformer>> getDeformers() const;

    // T: Skin, BlendShape
    template<class T, sfbxRestrict(is_deformer<T>)>
	std::shared_ptr<T> createDeformer();

protected:
    std::vector<std::shared_ptr<Deformer>> m_deformers;
};

//for reference only, fbx stores as strings
enum class LayerMappingMode : int
{
    None,
    ByControlPoint,
    ByPolygonVertex,
    ByPolygon,
    ByEdge,
    AllSame,
};
enum class LayerReferenceMode : int
{
	None,
    Direct,
    Index,
    IndexToDirect,
};

// Helper functions to convert between enums and strings
inline LayerMappingMode toLayerMappingMode(const std::string& s)
{
	if (s == "ByControlPoint") return LayerMappingMode::ByControlPoint;
	else if (s == "ByPolygonVertex") return LayerMappingMode::ByPolygonVertex;
	else if (s == "ByPolygon") return LayerMappingMode::ByPolygon;
	else if (s == "ByEdge") return LayerMappingMode::ByEdge;
	else if (s == "AllSame") return LayerMappingMode::AllSame;
	else return LayerMappingMode::None;
}

inline std::string toString(LayerMappingMode mode)
{
	switch (mode) {
		case LayerMappingMode::ByControlPoint: return "ByControlPoint";
		case LayerMappingMode::ByPolygonVertex: return "ByPolygonVertex";
		case LayerMappingMode::ByPolygon: return "ByPolygon";
		case LayerMappingMode::ByEdge: return "ByEdge";
		case LayerMappingMode::AllSame: return "AllSame";
		default: return "None";
	}
}

inline LayerReferenceMode toLayerReferenceMode(const std::string& s)
{
	if (s == "Direct") return LayerReferenceMode::Direct;
	else if (s == "Index") return LayerReferenceMode::Index;
	else if (s == "IndexToDirect") return LayerReferenceMode::IndexToDirect;
	else return LayerReferenceMode::Direct; // Default to Direct if unknown
}

inline std::string toString(LayerReferenceMode mode)
{
	switch (mode) {
		case LayerReferenceMode::Direct: return "Direct";
		case LayerReferenceMode::Index: return "Index";
		case LayerReferenceMode::IndexToDirect: return "IndexToDirect";
		default: return "Direct";
	}
}

// **Implement GetMappingModeName and GetReferenceModeName**
inline const char* GetMappingModeName(LayerMappingMode mode)
{
	switch (mode) {
		case LayerMappingMode::ByControlPoint: return "ByControlPoint";
		case LayerMappingMode::ByPolygonVertex: return "ByPolygonVertex";
		case LayerMappingMode::ByPolygon: return "ByPolygon";
		case LayerMappingMode::ByEdge: return "ByEdge";
		case LayerMappingMode::AllSame: return "AllSame";
		default: return "None";
	}
}

inline const char* GetReferenceModeName(LayerReferenceMode mode)
{
	switch (mode) {
		case LayerReferenceMode::Direct: return "Direct";
		case LayerReferenceMode::Index: return "Index";
		case LayerReferenceMode::IndexToDirect: return "IndexToDirect";
		default: return "Direct";
	}
}

// FBX can store multiple normal / UV / vertex color channels ("layer" in FBX term).
// LayerElement store these data.
template<class T>
struct LayerElement
{
    std::string name;
    RawVector<int> indices; // can be empty. in that case, size of data must equal with vertex count or index count.
    RawVector<T> data;
    RawVector<T> data_deformed; // relevant only for normal layers for now.
	LayerMappingMode mapping_mode = LayerMappingMode::None;
	LayerReferenceMode reference_mode = LayerReferenceMode::Direct;
};
using LayerElementF2 = LayerElement<float2>;
using LayerElementF3 = LayerElement<float3>;
using LayerElementF4 = LayerElement<float4>;
using LayerElementI1 = LayerElement<int>;

struct LayerElementDesc
{
    std::string type;
    int index = 0;
};

// GeomMesh represents polygon mesh data. parent of GeomMesh seems to be always Mesh.
class GeomMesh : public Geometry
{
	using super = Geometry;
public:
	ObjectSubClass getSubClass() const override;
	
	// Existing getter and setter methods
	span<int> getCounts() const;
	span<int> getIndices() const;
	span<float3> getPoints() const;
	span<float3> getNormals() const;
	span<LayerElementF3> getNormalLayers() const; // can be zero or multiple layers
	span<LayerElementF2> getUVLayers() const;     // can be zero or multiple layers
	span<LayerElementF4> getColorLayers() const;  // can be zero or multiple layers
	span<LayerElementI1> getMaterialLayers() const;// can be zero or multiple layers
	span<std::vector<LayerElementDesc>> getLayers() const;     //
	
	void setCounts(span<int> v);
	void setIndices(span<int> v);
	void setPoints(span<float3> v);
	void addNormalLayer(LayerElementF3&& v);
	void addUVLayer(LayerElementF2&& v);
	void addColorLayer(LayerElementF4&& v);
	void addMaterialLayer(LayerElementI1&& v);
	
	// **New Methods**
	void addControlPoint(float x, float y, float z);
	void addPolygon(int idx1, int idx2, int idx3);
	void addNormal(float x, float y, float z, size_t layer_index = 0);
	void addUV(int set_index, float u, float v);
	void addVertexColor(float r, float g, float b, float a, size_t layer_index = 0);
	
	template<typename T>
	void checkModes(LayerElement<T>& layer); // Check & update to default/known-correct modes to data/indices
	
	span<float3> getPointsDeformed(bool apply_transform = false);
	span<float3> getNormalsDeformed(size_t layer_index = 0, bool apply_transform = false);
	
	int getMaterialForVertexIndex(size_t vertex_index) const;
	std::vector<int> getVertexIndicesForPointIndex(int point_index) const;
	
	void exportFBXObjects() override;

protected:
	void importFBXObjects() override;
	
	void exportLayers();
	
	template<typename LayerType>
	void exportLayer(const LayerType& layer, const char* layerName);
	
	// Existing member variables
	RawVector<int> m_counts;
	RawVector<int> m_uv_counts;
	RawVector<int> m_indices;
	RawVector<float3> m_points;
	RawVector<float3> m_normals;
	RawVector<float3> m_points_deformed;
	std::vector<LayerElementF3> m_normal_layers;
	std::vector<LayerElementF2> m_uv_layers;
	std::vector<LayerElementF4> m_color_layers;
	std::vector<LayerElementI1> m_material_layers;
	std::vector<std::vector<LayerElementDesc>> m_layers;
	
private:
	std::unordered_map<size_t, int> m_vertex_to_material_map;
	std::unordered_map<int, std::vector<int>> m_point_to_vertex_map;
};



// a Shape is a target of blend shape. see BlendShape and BlendShapeChannel in sfbxDeformer.h.
class Shape : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getIndices() const;
    span<float3> getDeltaPoints() const;
    span<float3> getDeltaNormals() const;

    void setIndices(span<int> v);
    void setDeltaPoints(span<float3> v);
    void setDeltaNormals(span<float3> v);

public:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    RawVector<int> m_indices;
    RawVector<float3> m_delta_points;
    RawVector<float3> m_delta_normals;
};


} // namespace sfbx
