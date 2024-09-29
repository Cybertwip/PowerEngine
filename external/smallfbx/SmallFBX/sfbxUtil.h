#pragma once

#define sfbxPrint(...) printf(__VA_ARGS__)

namespace sfbx {

// escape forbidden characters (e.g. " -> &quot;). return false if no escape is needed.
bool Escape(std::string& v);
std::string Base64Encode(span<char> src);

RawVector<int> Triangulate(span<int> counts, span<int> indices);

struct JointWeights;
struct JointMatrices;
bool DeformPoints(span<double3> dst, const JointWeights& jw, const JointMatrices& jm, span<double3> src);
bool DeformVectors(span<double3> dst, const JointWeights& jw, const JointMatrices& jm, span<double3> src);

} // namespace sfbx
