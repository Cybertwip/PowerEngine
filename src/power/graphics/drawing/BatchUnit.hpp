#pragma once

class IMeshBatch;
class ISkinnedMeshBatch;

struct BatchUnit {
	BatchUnit(IMeshBatch& meshBatch, ISkinnedMeshBatch& skinnedMeshBatch)
	: mMeshBatch(meshBatch)
	, mSkinnedMeshBatch(skinnedMeshBatch) {
		
	}
	
	IMeshBatch& mMeshBatch;
	ISkinnedMeshBatch& mSkinnedMeshBatch;
};
