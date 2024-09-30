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
	m_points.push_back(double3{x, y, z});
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
	m_normal_layers[layer_index].data.push_back(double3{x, y, z});
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
	m_uv_layers[set_index].data.push_back(double2{u, v});
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
	m_color_layers[layer_index].data.push_back(double4{r, g, b, a});
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
	
	// Prefill the vertex-to-material map with all material layers
	if (!m_material_layers.empty()) {
		// Clear the map before populating
		m_vertex_to_material_map.clear();
		
		for (const auto& material_layer : m_material_layers) { // Iterate through all material layers
			switch (material_layer.mapping_mode) {
				case LayerMappingMode::ByControlPoint: {
					// One material per vertex
					if (material_layer.indices.size() != m_points.size()) {
						sfbxPrint("Warning: Material layer indices size (%zu) does not match number of vertices (%zu) in ByControlPoint mapping.\n",
								  material_layer.indices.size(), m_points.size());
					}
					size_t limit = std::min(material_layer.indices.size(), m_points.size());
					for (size_t i = 0; i < limit; ++i) {
						unsigned int material_index = material_layer.indices[i];
						// Insert or update the map entry for vertex i
						m_vertex_to_material_map[i].push_back(material_index);
					}
					break;
				}
					
				case LayerMappingMode::ByPolygon: {
					size_t current_start = 0; // Initialize the starting index
					for (size_t poly_idx = 0; poly_idx < m_counts.size(); ++poly_idx) {
						if (poly_idx < material_layer.indices.size()) {
							unsigned int material_index = material_layer.indices[poly_idx];
							size_t count = m_counts[poly_idx]; // Number of vertices in this polygon
							
							// Assign the material to all vertices of the polygon
							for (size_t i = 0; i < count; ++i) {
								size_t vertex_index_in_m_indices = current_start + i;
								if (vertex_index_in_m_indices < m_indices.size()) {
									unsigned int vertex_idx = m_indices[vertex_index_in_m_indices];
									if (vertex_idx >= 0 && vertex_idx < static_cast<int>(m_points.size())) {
										m_vertex_to_material_map[vertex_idx].push_back(material_index);
									} else {
										sfbxPrint("Warning: Vertex index %u out of range in ByPolygon mapping.\n", vertex_idx);
									}
								} else {
									sfbxPrint("Warning: Vertex index %zu out of range in ByPolygon mapping.\n", vertex_index_in_m_indices);
								}
							}
							
							current_start += count; // Update the starting index for the next polygon
						} else {
							sfbxPrint("Warning: Not enough material indices (%zu) for all polygons (%zu).\n",
									  material_layer.indices.size(), m_counts.size());
							break;
						}
					}
					break;
				}
					
				case LayerMappingMode::ByPolygonVertex: {
					// One material per polygon vertex
					size_t limit = std::min(material_layer.indices.size(), m_indices.size());
					for (size_t i = 0; i < limit; ++i) {
						unsigned int material_index = material_layer.indices[i];
						unsigned int vertex_idx = m_indices[i];
						if (vertex_idx >= 0 && vertex_idx < static_cast<int>(m_points.size())) {
							m_vertex_to_material_map[vertex_idx].push_back(material_index);
						} else {
							sfbxPrint("Warning: Vertex index %u out of range in ByPolygonVertex mapping.\n", vertex_idx);
						}
					}
					if (material_layer.indices.size() > m_indices.size()) {
						sfbxPrint("Warning: More material indices (%zu) than polygon vertices (%zu) in ByPolygonVertex mapping.\n",
								  material_layer.indices.size(), m_indices.size());
					}
					break;
				}
					
				case LayerMappingMode::AllSame: {
					// Apply the same material to all vertices
					if (!material_layer.indices.empty()) {
						unsigned int universal_material_index = material_layer.indices[0];
						
						if (material_layer.indices.size() > 1) {
							sfbxPrint("Warning: More than one material index provided for AllSame mapping mode. Only the first (%u) will be used.\n",
									  universal_material_index);
						}
						
						for (size_t index = 0; index < m_indices.size(); ++index) {
							unsigned int vertex_idx = m_indices[index];
							if (vertex_idx >= 0 && vertex_idx < static_cast<int>(m_points.size())) {
								m_vertex_to_material_map[vertex_idx].push_back(universal_material_index);
							} else {
								sfbxPrint("Warning: Vertex index %u out of range in AllSame mapping.\n", vertex_idx);
							}
						}
					} else {
						sfbxPrint("Warning: No material index provided for AllSame mapping mode.\n");
					}
					break;
				}
					
				default:
					sfbxPrint("Unsupported material mapping mode: %s\n", toString(material_layer.mapping_mode).c_str());
					break;
			}
		}
	}
}

void GeomMesh::exportFBXObjects()
{
	// If using Option 2 from the previous assistant's suggestion
	super::exportFBXObjects(); // Ensure the geometry node is created only once
	
	Node* n = getNode();
	if (!n)
		return;
	
	n->createChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);
	
	// Export points
	n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_points));
	
	// Export indices with correct negative syntax
	{
		size_t total_counts = 0;
		for (int c : m_counts)
			total_counts += c;
		
		if (total_counts != m_indices.size()) {
			sfbxPrint("sfbx::Mesh: *** indices mismatch with counts ***\n");
		}
		else {
			const int* src_counts = m_counts.data();
			auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
			auto dst_prop = dst_node->createProperty();
			int* dst = dst_prop->allocateArray<int>(m_indices.size()).data();
			
			size_t cpoints = 0;
			for (size_t i = 0, idx = 0; i < m_indices.size(); ++i) {
				int index = m_indices[i];
				cpoints++;
				if (cpoints == src_counts[idx]) {
					index = ~index; // Apply bitwise complement to the last index
					cpoints = 0;
					idx++;
				}
				*dst++ = index;
			}
		}
	}
	
	// Export other geometry data (normals, UVs, layers, etc.)
	exportLayers();
	
	// Export child models
	for (auto& deformer : getDeformers()) {
		deformer->exportFBXObjects();
	}
}

void GeomMesh::exportLayers()
{
	// Export normal layers
	for (const auto& layer : m_normal_layers)
		exportLayer(layer, sfbxS_LayerElementNormal);
	
	// Export UV layers
	for (const auto& layer : m_uv_layers)
		exportLayer(layer, sfbxS_LayerElementUV);
	
	// Export color layers
	for (const auto& layer : m_color_layers)
		exportLayer(layer, sfbxS_LayerElementColor);
	
	// Export material layers
	for (const auto& layer : m_material_layers)
		exportLayer(layer, sfbxS_LayerElementMaterial);
}


template<typename LayerType>
void GeomMesh::exportLayer(const LayerType& layer, const char* layerName)
{
	Node* layerNode = m_node->createChild(layerName);
	layerNode->createChild(sfbxS_Name, layer.name);
	layerNode->createChild(sfbxS_MappingInformationType, GetMappingModeName(layer.mapping_mode));
	layerNode->createChild(sfbxS_ReferenceInformationType, GetReferenceModeName(layer.reference_mode));
	
	// Determine the correct data and index property names
	const char* dataPropertyName = nullptr;
	const char* indexPropertyName = nullptr;
	
	if (layerName == sfbxS_LayerElementNormal) {
		dataPropertyName = sfbxS_Normals;
		indexPropertyName = sfbxS_NormalsIndex;
	}
	else if (layerName == sfbxS_LayerElementUV) {
		dataPropertyName = sfbxS_UV;
		indexPropertyName = sfbxS_UVIndex;
	}
	else if (layerName == sfbxS_LayerElementColor) {
		dataPropertyName = sfbxS_Colors;
		indexPropertyName = sfbxS_ColorIndex;
	}
	else if (layerName == sfbxS_LayerElementMaterial) {
		dataPropertyName = sfbxS_Materials;
		// Materials do not have an index property in the same way
		indexPropertyName = nullptr;
	}
	else {
		// For custom layers or unsupported types
		dataPropertyName = "Data";
		indexPropertyName = "Indices";
	}
	
	// Export data
	if (!layer.data.empty() && dataPropertyName)
		layerNode->createChild(dataPropertyName)->addProperties(layer.data);
	
	// Export indices
	if (!layer.indices.empty() && indexPropertyName)
		layerNode->createChild(indexPropertyName)->addProperties(layer.indices);
}

span<int> GeomMesh::getCounts() const { return make_span(m_counts); }
span<int> GeomMesh::getIndices() const { return make_span(m_indices); }
span<double3> GeomMesh::getPoints() const { return make_span(m_points); }
span<double3> GeomMesh::getNormals() const { return make_span(m_normals); }
span<LayerElementF3> GeomMesh::getNormalLayers() const  { return make_span(m_normal_layers); }
span<LayerElementF2> GeomMesh::getUVLayers() const      { return make_span(m_uv_layers); }
span<LayerElementF4> GeomMesh::getColorLayers() const   { return make_span(m_color_layers); }
span<LayerElementI1> GeomMesh::getMaterialLayers() const { return make_span(m_material_layers); }
span<std::vector<LayerElementDesc>> GeomMesh::getLayers() const { return make_span(m_layers); }

void GeomMesh::setCounts(span<int> v) { m_counts = v; }
void GeomMesh::setIndices(span<int> v) { m_indices = v; }
void GeomMesh::setPoints(span<double3> v) { m_points = v; }
//TODO update layers list
void GeomMesh::addNormalLayer(LayerElementF3&& v)   { m_normal_layers.push_back(std::move(v)); }
void GeomMesh::addUVLayer(LayerElementF2&& v)       { m_uv_layers.push_back(std::move(v)); }
void GeomMesh::addColorLayer(LayerElementF4&& v)    { m_color_layers.push_back(std::move(v)); }
void GeomMesh::addMaterialLayer(LayerElementI1&& v) { m_material_layers.push_back(std::move(v)); }

span<double3> GeomMesh::getPointsDeformed(bool apply_transform)
{
	if (getDeformers().empty() && !apply_transform)
		return make_span(m_points);
	
	m_points_deformed = m_points;
	auto dst = make_span(m_points_deformed);
	bool is_skinned = false;
	for (auto deformer : getDeformers()) {
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

span<double3> GeomMesh::getNormalsDeformed(size_t layer_index, bool apply_transform)
{
	if (layer_index >= m_normal_layers.size())
		return {};
	
	auto& l = m_normal_layers[layer_index];
	if (getDeformers().empty() && !apply_transform)
		return make_span(l.data);
	
	l.data_deformed = l.data;
	auto dst = make_span(l.data_deformed);
	bool is_skinned = false;
	for (auto deformer : getDeformers()) {
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
int GeomMesh::getMaterialForVertexIndex(unsigned int vertex_index) const
{
	auto it = m_vertex_to_material_map.find(vertex_index);
	if (it != m_vertex_to_material_map.end() && !it->second.empty()) {
		return static_cast<int>(it->second[0]); // Assign the first material
	}
	return -1; // Material not found for this vertex
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
span<double3> Shape::getDeltaPoints() const { return make_span(m_delta_points); }
span<double3> Shape::getDeltaNormals() const { return make_span(m_delta_normals); }

void Shape::setIndices(span<int> v) { m_indices = v; }
void Shape::setDeltaPoints(span<double3> v) { m_delta_points = v; }
void Shape::setDeltaNormals(span<double3> v) { m_delta_normals = v; }

} // namespace sfbx
