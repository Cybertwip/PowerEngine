// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_selection.h $
// $DateTime: 2012/03/16 15:41:10 $
// $Change: 36794 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_SELECTION_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

struct animation_spec;

class selection : public node
{
    typedef node parent;

public:
    virtual void AcceptNodeVisitor(node_visitor* Visitor);
    virtual bool DeleteInput(granny_int32x InputIndex);

    virtual granny_local_pose* SamplePoseOutput(granny_int32x      OutputIdx,
                                                granny_real32      AtT,
                                                granny_real32      AllowedError,
                                                granny_pose_cache* PoseCache);
    virtual bool GetRootMotionVectors(granny_int32x  OutputIdx,
                                      granny_real32  AtT,
                                      granny_real32  DeltaT,
                                      granny_real32* ResultTranslation,
                                      granny_real32* ResultRotation,
                                      bool           Inverse);

    virtual granny_real32 GetDuration(granny_int32x OnOutput);

    DECL_CONCRETE_CLASS_TOKEN(selection);

public:
    virtual void Activate(granny_int32x OnOutput, granny_real32 AtT);
    virtual bool DidLoopOccur(granny_int32x OnOutput,
                              granny_real32 AtT,
                              granny_real32 DeltaT);

    bool ObtainSampleIndex(granny_real32 AtT, int* Index);
};


END_GSTATE_NAMESPACE;

#define GSTATE_SELECTION_H
#endif /* GSTATE_SELECTION_H */
