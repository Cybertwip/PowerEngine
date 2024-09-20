#pragma once

class MeshBatch;
class SkinnedMeshBatch;

struct BatchUnit {
	BatchUnit(MeshBatch& meshBatch, SkinnedMeshBatch& skinnedMeshBatch)
	: mMeshBatch(meshBatch)
	, mSkinnedMeshBatch(skinnedMeshBatch) {
		
	}
	
	MeshBatch& mMeshBatch;
	SkinnedMeshBatch& mSkinnedMeshBatch;
};
