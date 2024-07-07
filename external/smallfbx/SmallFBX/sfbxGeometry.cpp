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
    //ret->setName(getName()); // FBX SDK seems don't do this
    return ret;
}

template<> std::shared_ptr<BlendShape> Geometry::createDeformer()
{
    auto ret = createChild<BlendShape>();
    ret->setName(getName());
    return ret;
}


ObjectSubClass GeomMesh::getSubClass() const { return ObjectSubClass::Mesh; }

template<typename T>
void GeomMesh::checkModes(LayerElement<T>& layer)
{
    auto expected_ref_mode = layer.indices.empty() ? "Direct" : "IndexToDirect";
    if (layer.reference_mode.empty())
        layer.reference_mode = expected_ref_mode;
    else if (layer.reference_mode != expected_ref_mode) {
        sfbxPrint("unexpected reference mode\n");
//        layer.reference_mode = expected_ref_mode;
    }

    auto expected_map_mode = "None";
    auto mapping_size = (layer.reference_mode == "Direct" ? layer.data.size() : layer.indices.size());
    int match = 0;
    if (mapping_size == m_indices.size()) {
        expected_map_mode = "ByPolygonVertex";
        match++;
    }
    if (mapping_size == m_points.size()) {
        expected_map_mode = "ByControlPoint";
        match++;
    }
    if (mapping_size == m_counts.size()) {
        expected_map_mode = "ByPolygon";
        match++;
    }
    if (match == 0 && mapping_size == 1) {
        expected_map_mode = "AllSame";
        match++;
    }
    if (layer.mapping_mode.empty()) {
        layer.mapping_mode = expected_map_mode;
        if (match > 1)
            sfbxPrint("ambiguous mapping mode\n");
    } else if (match <= 1 && layer.mapping_mode != expected_map_mode) {
        sfbxPrint("unexpected mapping mode\n");
//        layer.mapping_mode = expected_map_mode;
    }
}
void GeomMesh::importFBXObjects()
{
	for (auto n : getNode()->getChildren()) {
		if (n->getName() == sfbxS_Vertices) {
			// points
			GetPropertyValue<double3>(m_points, n);
		} else if (n->getName() == sfbxS_Normals) {
			// normals
			GetPropertyValue<double3>(m_normals, n);
		} else if (n->getName() == sfbxS_PolygonVertexIndex) {
			// counts & indices
			GetPropertyValue<int>(m_indices, n);
			m_counts.resize(m_indices.size()); // reserve
			int* dst_counts = m_counts.data();
			size_t cfaces = 0;
			size_t cpoints = 0;
			for (int& i : m_indices) {
				++cpoints;
				if (i < 0) { // negative value indicates the last index in the face
					i = ~i;
					dst_counts[cfaces++] = cpoints;
					cpoints = 0;
				}
			}
			m_counts.resize(cfaces); // fit to actual size
			
			// Adjust the indices for decomposed polygons to triangles
			std::vector<int> adjusted_indices;
			adjusted_indices.reserve(m_indices.size() * 3); // Reserve space for the expanded indices
			
			size_t start = 0;
			for (size_t i = 0; i < m_counts.size(); ++i) {
				size_t face_size = m_counts[i];
				if (face_size < 3) continue; // Skip degenerate faces
				
				for (size_t j = 1; j < face_size - 1; ++j) {
					adjusted_indices.push_back(m_indices[start]);
					adjusted_indices.push_back(m_indices[start + j]);
					adjusted_indices.push_back(m_indices[start + j + 1]);
				}
				
				start += face_size;
			}
			
			m_indices.assign(adjusted_indices.begin(), adjusted_indices.end());
		} else if (n->getName() == sfbxS_LayerElementNormal) {
			// normals
			LayerElementF3 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double3>(tmp.data, n, sfbxS_Normals);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_NormalsIndex);
			GetChildPropertyValue<string_view>(tmp.mapping_mode, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(tmp.reference_mode, n, sfbxS_ReferenceInformationType);
			checkModes(tmp);
			m_normal_layers.push_back(std::move(tmp));
		} else if (n->getName() == sfbxS_LayerElementUV) {
			// uv
			LayerElementF2 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double2>(tmp.data, n, sfbxS_UV);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_UVIndex);
			GetChildPropertyValue<string_view>(tmp.mapping_mode, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(tmp.reference_mode, n, sfbxS_ReferenceInformationType);
			checkModes(tmp);
			
			// Adjust indices for decomposed vertex indices
			std::vector<int> adjusted_uv_indices;
			adjusted_uv_indices.reserve(m_indices.size());
			
			for (int i = 0; i < m_indices.size(); i++) {
				int uv_index = i;
				if (!tmp.indices.empty()) {
					uv_index = tmp.indices[i % tmp.indices.size()]; // Adjust uv_index for the current vertex
				}
				adjusted_uv_indices.push_back(uv_index);
			}
			
			tmp.indices.assign(adjusted_uv_indices.begin(), adjusted_uv_indices.end()); // Replace original indices with adjusted ones
			m_uv_layers.push_back(std::move(tmp));
		} else if (n->getName() == sfbxS_LayerElementColor) {
			// colors
			LayerElementF4 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<double4>(tmp.data, n, sfbxS_Colors);
			GetChildPropertyValue<int>(tmp.indices, n, sfbxS_ColorIndex);
			GetChildPropertyValue<string_view>(tmp.mapping_mode, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(tmp.reference_mode, n, sfbxS_ReferenceInformationType);
			checkModes(tmp);
			m_color_layers.push_back(std::move(tmp));
		} else if (n->getName() == sfbxS_LayerElementMaterial) {
			// materials
			LayerElementI1 tmp;
			tmp.name = GetChildPropertyString(n, sfbxS_Name);
			GetChildPropertyValue<int>(tmp.data, n, sfbxS_Materials);
			GetChildPropertyValue<string_view>(tmp.mapping_mode, n, sfbxS_MappingInformationType);
			GetChildPropertyValue<string_view>(tmp.reference_mode, n, sfbxS_ReferenceInformationType);
			m_material_layers.push_back(std::move(tmp));
		} else if (n->getName() == sfbxS_Layer) {
			std::vector<LayerElementDesc> layer;
			for (auto n : n->getChildren()) {
				LayerElementDesc tmp;
				if (n->getName() == sfbxS_LayerElement) {
					GetChildPropertyValue<string_view>(tmp.type, n, sfbxS_Type);
					GetChildPropertyValue<int>(tmp.index, n, sfbxS_TypedIndex);
					layer.push_back(std::move(tmp));
				}
			}
			m_layers.push_back(std::move(layer));
		}
	}
}

void GeomMesh::exportFBXObjects()
{
    super::exportFBXObjects();

    Node* n = getNode();

    n->createChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);

    // points
    n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_points));

    // indices
    {
        // check if counts and indices are valid
        size_t total_counts = 0;
        for (int c : m_counts)
            total_counts += c;

        if (total_counts != m_indices.size()) {
            sfbxPrint("sfbx::Mesh: *** indices mismatch with counts ***\n");
        }
        else {
            auto* src_counts = m_counts.data();
            auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
            auto dst_prop = dst_node->createProperty();
            auto dst = dst_prop->allocateArray<int>(m_indices.size()).data();

            size_t cpoints = 0;
            for (int i : m_indices) {
                if (++cpoints == *src_counts) {
                    i = ~i; // negative value indicates the last index in the face
                    cpoints = 0;
                    ++src_counts;
                }
                *dst++ = i;
            }
        }
    }

    auto add_mapping_and_reference_info = [](Node* node, const auto& layer) {
        node->createChild(sfbxS_MappingInformationType, layer.mapping_mode);
        node->createChild(sfbxS_ReferenceInformationType, layer.reference_mode);
        //TODO if empty run checkModes() or warning?
    };

    int clayers = 0;

    // normal layers
	Node* l = nullptr;
	if(!m_normal_layers.empty()){
		l = n->createChild(sfbxS_LayerElementNormal, 0);
	}

    for (auto& layer : m_normal_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
		l->createChild(sfbxS_Version, sfbxI_LayerElementNormalVersion);
		l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
		l->createChild(sfbxS_Normals, make_adaptor<double3>(layer.data));
        if (!layer.indices.empty())
			l->createChild(sfbxS_NormalsIndex, layer.indices);
    }

    // uv layers
	if(!m_uv_layers.empty()){
		l = n->createChild(sfbxS_LayerElementUV, 0);
	}

    for (auto& layer : m_uv_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;

		l->createChild(sfbxS_Version, sfbxI_LayerElementUVVersion);
		l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
		l->createChild(sfbxS_UV, make_adaptor<double2>(layer.data));
        if (!layer.indices.empty())
			l->createChild(sfbxS_UVIndex, layer.indices);
    }

    // color layers
	if(!m_color_layers.empty()){
		l = n->createChild(sfbxS_LayerElementColor, 0);
	}

    for (auto& layer : m_color_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
		l->createChild(sfbxS_Version, sfbxI_LayerElementColorVersion);
		l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
		l->createChild(sfbxS_Colors, make_adaptor<double4>(layer.data));
        if (!layer.indices.empty())
			l->createChild(sfbxS_ColorIndex, layer.indices);
    }

    // material layers
	
	if(!m_material_layers.empty()){
		l = n->createChild(sfbxS_LayerElementMaterial, 0);
	}

    for (auto& layer : m_material_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
		l->createChild(sfbxS_Version, sfbxI_LayerElementMaterialVersion);
		l->createChild(sfbxS_Name, layer.name);

		
        //TODO add_mapping_and_reference_info+checkModes?
		
		
		l->createChild(sfbxS_MappingInformationType, "AllSame");
		l->createChild(sfbxS_ReferenceInformationType, "IndexToDirect");

		l->createChild(sfbxS_Materials, layer.data);
    }

    if (clayers) {
		
		auto layerContainer = n->createChild(sfbxS_Layer, 0);

		layerContainer->createChild(sfbxS_Version, sfbxI_LayerVersion);

		for (auto& layer : m_layers) {

			for(auto& desc : layer){

				auto le = layerContainer->createChild(sfbxS_LayerElement);
				
				le->createChild(sfbxS_Type, desc.type);
				le->createChild(sfbxS_TypedIndex, desc.index);

			}
			
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
span<LayerElementI1> GeomMesh::getMatrialLayers() const { return make_span(m_material_layers); }
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
