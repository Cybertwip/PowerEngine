#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Geometry::getClass() const { return ObjectClass::Geometry; }

void Geometry::addChild(ObjectPtr v)
{
	super::addChild(v);
	if (auto deformer = as<Deformer>(v))
		m_deformers.push_back(deformer);
}

void Geometry::eraseChild(ObjectPtr v)
{
	super::eraseChild(v);
	if (auto deformer = as<Deformer>(v))
		erase(m_deformers, deformer);
}

std::shared_ptr<Model> Geometry::getModel() const
{
	for (auto p : m_parents)
		if (auto model = as<Model>(p))
			return model;
	return nullptr;
}

bool Geometry::hasDeformer() const
{
	return !m_deformers.empty();
}

bool Geometry::hasSkinDeformer() const
{
	for (auto d : m_deformers)
		if (auto skin = as<Skin>(d))
			return true;
	return false;
}

span<std::shared_ptr<Deformer>> Geometry::getDeformers() const
{
	return make_span(m_deformers);
}

template<> std::shared_ptr<Skin> Geometry::createDeformer()
{
	auto ret = createChild<Skin>();
	return ret;
}

template<> std::shared_ptr<BlendShape> Geometry::createDeformer()
{
	auto ret = createChild<BlendShape>();
	ret->setName(getName());
	return ret;
}

void GeomMesh::addControlPoint(float x, float y, float z) {
	m_points.push_back(float3{x, y, z});
}

void GeomMesh::addPolygon(int idx1, int idx2, int idx3) {
	// Add indices for the polygon
	m_indices.push_back(idx1);
	m_indices.push_back(idx2);
	m_indices.push_back(idx3);
	// Since it's a triangle, add count as 3
	m_counts.push_back(3);
}

void GeomMesh::addNormal(float x, float y, float z, size_t layer_index) {
	// Ensure the specified normal layer exists
	if (layer_index >= m_normal_layers.size()) {
		// Create new layers up to the required index
		while (m_normal_layers.size() <= layer_index) {
			LayerElementF3 new_layer;
			new_layer.name = "Normals";
			new_layer.mapping_mode = LayerMappingMode::ByControlPoint;
			new_layer.reference_mode = LayerReferenceMode::Direct;
			m_normal_layers.emplace_back(std::move(new_layer));
		}
	}
	// Add the normal to the specified layer
	m_normal_layers[layer_index].data.push_back(float3{x, y, z});
}

void GeomMesh::addUV(int set_index, float u, float v) {
	// Ensure the specified UV layer exists
	if (set_index >= m_uv_layers.size()) {
		// Create new layers up to the required index
		while (m_uv_layers.size() <= set_index) {
			LayerElementF2 new_layer;
			new_layer.name = "UVSet" + std::to_string(m_uv_layers.size() + 1);
			new_layer.mapping_mode = LayerMappingMode::ByControlPoint;
			new_layer.reference_mode = LayerReferenceMode::Direct;
			m_uv_layers.emplace_back(std::move(new_layer));
		}
	}
	// Add the UV to the specified layer
	m_uv_layers[set_index].data.push_back(float2{u, v});
}

void GeomMesh::addVertexColor(float r, float g, float b, float a, size_t layer_index) {
	// Ensure the specified color layer exists
	if (layer_index >= m_color_layers.size()) {
		// Create new layers up to the required index
		while (m_color_layers.size() <= layer_index) {
			LayerElementF4 new_layer;
			new_layer.name = "VertexColor";
			new_layer.mapping_mode = LayerMappingMode::ByControlPoint;
			new_layer.reference_mode = LayerReferenceMode::Direct;
			m_color_layers.emplace_back(std::move(new_layer));
		}
	}
	// Add the vertex color to the specified layer
	m_color_layers[layer_index].data.push_back(float4{r, g, b, a});
}


ObjectSubClass GeomMesh::getSubClass() const { return ObjectSubClass::Mesh; }

template<typename T>
void GeomMesh::checkModes(LayerElement<T>& layer)
{
	// Skip checking for materials
	if (layer.name == sfbxS_LayerElementMaterial) {
		return;
	}
	
	// Ensure the reference mode is correct
	const LayerReferenceMode expected_ref_mode = layer.indices.empty() ? LayerReferenceMode::Direct : LayerReferenceMode::IndexToDirect;
	if (layer.reference_mode == LayerReferenceMode::None) {
		layer.reference_mode = expected_ref_mode;
	} else if (layer.reference_mode != expected_ref_mode) {
		sfbxPrint("Unexpected reference mode: %s (expected %s)\n",
				  toString(layer.reference_mode).c_str(), toString(expected_ref_mode).c_str());
	}
	
	// Determine the expected mapping mode based on data size
	size_t mapping_size = (layer.reference_mode == LayerReferenceMode::Direct) ? layer.data.size() : layer.indices.size();
	int match = 0;
	LayerMappingMode expected_map_mode = LayerMappingMode::None;
	
	// Compare mapping size with different attributes to determine expected mapping mode
	if (mapping_size == m_indices.size()) {
		expected_map_mode = LayerMappingMode::ByPolygonVertex;
		match++;
	}
	if (mapping_size == m_points.size()) {
		expected_map_mode = LayerMappingMode::ByControlPoint;
		match++;
	}
	if (mapping_size == m_counts.size()) {
		expected_map_mode = LayerMappingMode::ByPolygon;
		match++;
	}
	if (mapping_size == 1) {
		expected_map_mode = LayerMappingMode::AllSame;
		match++;
	}
	
	// Set mapping mode or report ambiguity if multiple matches exist
	if (layer.mapping_mode == LayerMappingMode::None) {
		layer.mapping_mode = expected_map_mode;
		if (match > 1) {
			sfbxPrint("Ambiguous mapping mode for layer %s, setting to %s\n", layer.name.c_str(), toString(expected_map_mode).c_str());
		}
	} else if (match == 0 || layer.mapping_mode != expected_map_mode) {
		// If no match found or current mapping mode differs from expected, log a warning
		sfbxPrint("Unexpected mapping mode: %s (expected %s)\n", toString(layer.mapping_mode).c_str(), toString(expected_map_mode).c_str());
	}
}


void GeomMesh::importFBXObjects()
{
	super::importFBXObjects();
	
	for (auto n : getNode()->getChildren()) {
		auto name = n->getName();
		if (name == sfbxS_Vertices) {
			// Points
			GetPropertyValue<double3>(m_points, n);
		}
		else if (name == sfbxS_PolygonVertexIndex) {
			// Indices and Counts
			GetPropertyValue<int>(m_indices, n);
			m_counts.clear();
			int count = 0;
			for (auto& idx : m_indices) {
				++count;
				if (idx < 0) {
					idx = ~idx;
					m_counts.push_back(count);
					count = 0;
				}
			}
		}
		else if (name == sfbxS_LayerElementNormal) {
			// Normals
			LayerElementF3 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double3>(tmp.data, n, sfbxS_Normals);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_NormalsIndex);
			std::string mapping_mode_str, reference_mode_str;
			GetChildPropertyValue<string_view>(mapping_mode_str, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(reference_mode_str, n, sfbxS_ReferenceInformationType);
			tmp.mapping_mode = toLayerMappingMode(mapping_mode_str);
			tmp.reference_mode = toLayerReferenceMode(reference_mode_str);
			checkModes(tmp);
			m_normal_layers.push_back(std::move(tmp));
		}
		else if (name == sfbxS_LayerElementUV) {
			// UVs
			LayerElementF2 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double2>(tmp.data, n, sfbxS_UV);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_UVIndex);
			std::string mapping_mode_str, reference_mode_str;
			GetChildPropertyValue<string_view>(mapping_mode_str, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(reference_mode_str, n, sfbxS_ReferenceInformationType);
			tmp.mapping_mode = toLayerMappingMode(mapping_mode_str);
			tmp.reference_mode = toLayerReferenceMode(reference_mode_str);
			checkModes(tmp);
			m_uv_layers.push_back(std::move(tmp));
		}
		else if (name == sfbxS_LayerElementColor) {
			// Colors
			LayerElementF4 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double4>(tmp.data, n, sfbxS_Colors);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_ColorIndex);
			std::string mapping_mode_str, reference_mode_str;
			GetChildPropertyValue<string_view>(mapping_mode_str, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(reference_mode_str, n, sfbxS_ReferenceInformationType);
			tmp.mapping_mode = toLayerMappingMode(mapping_mode_str);
			tmp.reference_mode = toLayerReferenceMode(reference_mode_str);
			checkModes(tmp);
			m_color_layers.push_back(std::move(tmp));
		}
		else if (name == sfbxS_LayerElementMaterial) {
			// Materials
			LayerElementI1 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_Materials);
			std::string mapping_mode_str, reference_mode_str;
			GetChildPropertyValue<string_view>(mapping_mode_str, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(reference_mode_str, n, sfbxS_ReferenceInformationType);
			tmp.mapping_mode = toLayerMappingMode(mapping_mode_str);
			tmp.reference_mode = toLayerReferenceMode(reference_mode_str);
			checkModes(tmp);
			m_material_layers.push_back(std::move(tmp));
		}
		else if (name == sfbxS_Layer) {
			// Layer descriptions
			int layer_index = GetChildPropertyValue<int>(n, sfbxS_Layer);
			std::vector<LayerElementDesc> layer_descs;
			for (auto layer_node : n->getChildren()) {
				if (layer_node->getName() == sfbxS_LayerElement) {
					LayerElementDesc desc;
					desc.type = GetChildPropertyString(layer_node, sfbxS_Type);
					desc.index = GetChildPropertyValue<int>(layer_node, sfbxS_TypedIndex);
					layer_descs.push_back(std::move(desc));
				}
			}
			m_layers.push_back(std::move(layer_descs));
		}
	}
	
	// Prefill the vertex-to-material map
	if (!m_material_layers.empty()) {
		const auto& material_layer = m_material_layers[0]; // Assuming the first material layer
		LayerMappingMode mapping_mode = material_layer.mapping_mode;
		
		size_t vertex_count = 0;
		for (size_t i = 0; i < m_counts.size(); ++i) {
			size_t polygon_size = m_counts[i];
			
			// Get the material index for this polygon
			int material_index = -1;
			if (mapping_mode == LayerMappingMode::AllSame) {
				material_index = 0; // All faces have the same material
			}
			else if (mapping_mode == LayerMappingMode::ByPolygon) {
				if (i < material_layer.indices.size()) {
					material_index = material_layer.indices[i];
				} else {
					material_index = -1; // Out of bounds
				}
			}
			
			// Prefill the map for each vertex in this polygon
			for (size_t j = 0; j < polygon_size; ++j) {
				m_vertex_to_material_map[vertex_count + j] = material_index;
			}
			
			// Increment the vertex count by the number of vertices in this polygon
			vertex_count += polygon_size;
		}
	}
	m_point_to_vertex_map.clear(); // Clear the map to ensure no stale data
	size_t vertex_counter = 0;
	
	// Loop through polygons to populate the point-to-vertex map
	for (size_t polygon_index = 0; polygon_index < m_counts.size(); ++polygon_index) {
		int polygon_size = m_counts[polygon_index];
		
		for (int i = 0; i < polygon_size; ++i) {
			int point_index = m_indices[vertex_counter];
			
			// Append the vertex_counter to the vector for this point_index
			m_point_to_vertex_map[point_index].push_back(static_cast<int>(vertex_counter));
			
			vertex_counter++;
		}
	}

}

void GeomMesh::exportFBXObjects()
{
	super::exportFBXObjects();
	
	Node* n = getNode();
	
	n->createChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);
	
	// Points
	n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_points));
	
	// Indices
	{
		auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
		auto dst_prop = dst_node->createProperty();
		auto dst = dst_prop->allocateArray<int>(m_indices.size()).data();
		
		size_t index = 0;
		for (size_t i = 0; i < m_counts.size(); ++i) {
			int count = m_counts[i];
			for (int j = 0; j < count; ++j) {
				int idx = m_indices[index++];
				if (j == count - 1)
					idx = ~idx; // Negate the last index in the polygon
				*dst++ = idx;
			}
		}
	}
	
	auto add_layer_element = [&](const std::string& element_name, auto& layers) {
		for (size_t i = 0; i < layers.size(); ++i) {
			auto& layer = layers[i];
			if (layer.data.empty())
				continue;
			
			Node* layer_node = n->createChild(element_name, (int)i);
			layer_node->createChild(sfbxS_Version, sfbxI_LayerVersion);
			layer_node->createChild(sfbxS_Name, layer.name);
			layer_node->createChild(sfbxS_MappingInformationType, toString(layer.mapping_mode));
			layer_node->createChild(sfbxS_ReferenceInformationType, toString(layer.reference_mode));

			if constexpr (std::is_same_v<decltype(layer), LayerElementF3&>) {
				// Normals
				layer_node->createChild(sfbxS_Normals, make_adaptor<double3>(layer.data));
				if (!layer.indices.empty())
					layer_node->createChild(sfbxS_NormalsIndex, layer.indices);
			}
			else if constexpr (std::is_same_v<decltype(layer), LayerElementF2&>) {
				// UVs
				layer_node->createChild(sfbxS_UV, make_adaptor<double2>(layer.data));
				if (!layer.indices.empty())
					layer_node->createChild(sfbxS_UVIndex, layer.indices);
			}
			else if constexpr (std::is_same_v<decltype(layer), LayerElementF4&>) {
				// Colors
				layer_node->createChild(sfbxS_Colors, make_adaptor<double4>(layer.data));
				if (!layer.indices.empty())
					layer_node->createChild(sfbxS_ColorIndex, layer.indices);
			}
			else if constexpr (std::is_same_v<decltype(layer), LayerElementI1&>) {
				// Correct: Writing materials from layer.indices
				layer_node->createChild(sfbxS_Materials, layer.indices);
			}
		}
	};
	
	// Export layer elements
	add_layer_element(sfbxS_LayerElementNormal, m_normal_layers);
	add_layer_element(sfbxS_LayerElementUV, m_uv_layers);
	add_layer_element(sfbxS_LayerElementColor, m_color_layers);
	add_layer_element(sfbxS_LayerElementMaterial, m_material_layers);
	
	// Export layer descriptions
	for (size_t i = 0; i < m_layers.size(); ++i) {
		Node* layer_node = n->createChild(sfbxS_Layer, (int)i);
		layer_node->createChild(sfbxS_Version, sfbxI_LayerVersion);
		for (auto& desc : m_layers[i]) {
			Node* le_node = layer_node->createChild(sfbxS_LayerElement);
			le_node->createChild(sfbxS_Type, desc.type);
			le_node->createChild(sfbxS_TypedIndex, desc.index);
		}
	}
}

span<int> GeomMesh::getCounts() const { return make_span(m_counts); }
span<int> GeomMesh::getIndices() const { return make_span(m_indices); }
span<float3> GeomMesh::getPoints() const { return make_span(m_points); }
span<float3> GeomMesh::getNormals() const { return make_span(m_normals); }
span<LayerElementF3> GeomMesh::getNormalLayers() const  { return make_span(m_normal_layers); }
span<LayerElementF2> GeomMesh::getUVLayers() const      { return make_span(m_uv_layers); }
span<LayerElementF4> GeomMesh::getColorLayers() const   { return make_span(m_color_layers); }
span<LayerElementI1> GeomMesh::getMaterialLayers() const { return make_span(m_material_layers); }
span<std::vector<LayerElementDesc>> GeomMesh::getLayers() const { return make_span(m_layers); }

void GeomMesh::setCounts(span<int> v) { m_counts = v; }
void GeomMesh::setIndices(span<int> v) { m_indices = v; }
void GeomMesh::setPoints(span<float3> v) { m_points = v; }
//TODO update layers list
void GeomMesh::addNormalLayer(LayerElementF3&& v)   { m_normal_layers.push_back(std::move(v)); }
void GeomMesh::addUVLayer(LayerElementF2&& v)       { m_uv_layers.push_back(std::move(v)); }
void GeomMesh::addColorLayer(LayerElementF4&& v)    { m_color_layers.push_back(std::move(v)); }
void GeomMesh::addMaterialLayer(LayerElementI1&& v) { m_material_layers.push_back(std::move(v)); }

span<float3> GeomMesh::getPointsDeformed(bool apply_transform)
{
	if (m_deformers.empty() && !apply_transform)
		return make_span(m_points);
	
	m_points_deformed = m_points;
	auto dst = make_span(m_points_deformed);
	bool is_skinned = false;
	for (auto deformer : m_deformers) {
		deformer->deformPoints(dst);
		if (as<Skin>(deformer))
			is_skinned = true;
	}
	if (!is_skinned && apply_transform) {
		if (auto model = getModel()) {
			auto mat = model->getGlobalMatrix();
			for (auto& v : dst)
				v = mul_p(mat, v);
		}
	}
	return dst;
}

span<float3> GeomMesh::getNormalsDeformed(size_t layer_index, bool apply_transform)
{
	if (layer_index >= m_normal_layers.size())
		return {};
	
	auto& l = m_normal_layers[layer_index];
	if (m_deformers.empty() && !apply_transform)
		return make_span(l.data);
	
	l.data_deformed = l.data;
	auto dst = make_span(l.data_deformed);
	bool is_skinned = false;
	for (auto deformer : m_deformers) {
		deformer->deformNormals(dst);
		if (as<Skin>(deformer))
			is_skinned = true;
	}
	if (!is_skinned && apply_transform) {
		if (auto model = getModel()) {
			auto mat = model->getGlobalMatrix();
			for (auto& v : dst)
				v = normalize(mul_v(mat, v));
		}
	}
	return dst;
}
int GeomMesh::getMaterialForVertexIndex(size_t vertex_index) const
{
	auto it = m_vertex_to_material_map.find(vertex_index);
	if (it != m_vertex_to_material_map.end()) {
		return it->second; // Return the material index for this vertex
	}
	return -1; // Material not found for this vertex
}
std::vector<int> GeomMesh::getVertexIndicesForPointIndex(int point_index) const {
	auto it = m_point_to_vertex_map.find(point_index);
	if (it != m_point_to_vertex_map.end()) {
		return it->second; // Return all corresponding vertex indices
	}
	return {}; // Return empty vector if the point index is not found
}

ObjectSubClass Shape::getSubClass() const { return ObjectSubClass::Shape; }

void Shape::importFBXObjects()
{
	for (auto n : getNode()->getChildren()) {
		auto name = n->getName();
		if (name == sfbxS_Indexes)
			GetPropertyValue<int>(m_indices, n);
		else if (name == sfbxS_Vertices)
			GetPropertyValue<double3>(m_delta_points, n);
		else if (name == sfbxS_Normals)
			GetPropertyValue<double3>(m_delta_normals, n);
	}
}

void Shape::exportFBXObjects()
{
	super::exportFBXObjects();
	
	Node* n = getNode();
	n->createChild(sfbxS_Version, sfbxI_ShapeVersion);
	if (!m_indices.empty())
		n->createChild(sfbxS_Indexes, m_indices);
	if (!m_delta_points.empty())
		n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_delta_points));
	if (!m_delta_normals.empty())
		n->createChild(sfbxS_Normals, make_adaptor<double3>(m_delta_normals));
}

span<int> Shape::getIndices() const { return make_span(m_indices); }
span<float3> Shape::getDeltaPoints() const { return make_span(m_delta_points); }
span<float3> Shape::getDeltaNormals() const { return make_span(m_delta_normals); }

void Shape::setIndices(span<int> v) { m_indices = v; }
void Shape::setDeltaPoints(span<float3> v) { m_delta_points = v; }
void Shape::setDeltaNormals(span<float3> v) { m_delta_normals = v; }

} // namespace sfbx
