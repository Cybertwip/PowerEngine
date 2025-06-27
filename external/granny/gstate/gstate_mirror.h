// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_mirror.h $
// $DateTime: 2012/04/19 14:05:32 $
// $Change: 37093 $
// $Revision: #3 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_MIRROR_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

struct granny_mirror_specifcation;

BEGIN_GSTATE_NAMESPACE;

class mirror : public node
{
    typedef node parent;

public:
    virtual bool BindToCharacter(gstate_character_instance* Instance);
    virtual void UnbindFromCharacter();

    granny_mirror_axis GetMirrorAxis();
    void SetMirrorAxis(granny_mirror_axis);

    int  GetNameSwapCount();
    bool SetNameSwaps(int NamePairCount,
                      char const** NamePairs);

    char const* GetNameSwap(int Idx);
    bool SetNameSwap(int Idx, char const* NewStr);

    virtual void AcceptNodeVisitor(node_visitor* Visitor);
    virtual granny_real32 GetDuration(granny_int32x OnOutput);

    virtual granny_local_pose* SamplePoseOutput(granny_int32x  OutputIdx,
                                                granny_real32 AtT,
                                                granny_real32 AllowedError,
                                                granny_pose_cache* PoseCache);

    virtual bool GetRootMotionVectors(granny_int32x  OutputIdx,
                                      granny_real32  AtT,
                                      granny_real32  DeltaT,
                                      granny_real32* ResultTranslation,
                                      granny_real32* ResultRotation,
                                      bool           Inverse);

    DECL_CONCRETE_CLASS_TOKEN(mirror);

public:
    virtual void Activate(granny_int32x OnOutput, granny_real32 AtT);
    virtual bool DidLoopOccur(granny_int32x OnOutput, granny_real32 AtT, granny_real32 DeltaT);
    virtual granny_int32x GetOutputPassthrough(granny_int32x OutputIdx) const;

private:
    granny_mirror_specification* m_MirrorSpec;
    bool CreateMirrorSpec();
};

END_GSTATE_NAMESPACE;

#define GSTATE_MIRROR_H
#endif /* GSTATE_MIRROR_H */
