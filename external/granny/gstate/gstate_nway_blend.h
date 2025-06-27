// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_nway_blend.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_NWAY_BLEND_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class nway_blend : public node
{
    typedef node parent;

public:
    virtual void AcceptNodeVisitor(node_visitor*);
    virtual granny_local_pose* SamplePoseOutput(granny_int32x      OutputIdx,
                                                granny_real32      AtT,
                                                granny_real32      AllowedError,
                                                granny_pose_cache* PoseCache);
    virtual bool GetRootMotionVectors(granny_int32x  OutputIdx,
                                      granny_real32  AtT,
                                      granny_real32  DeltaT,
                                      granny_real32* Translation,
                                      granny_real32* Rotation,
                                      bool Inverse);


    virtual void Activate(granny_int32x OnOutput, granny_real32 AtT);

    // Todo: should phaselock?
    //virtual granny_real32 GetDuration(granny_int32x OnOutput);

protected:
    DECL_CONCRETE_CLASS_TOKEN(nway_blend);

private:
    bool ComputeBlend(granny_real32  AtT,
                      granny_real32& Factor,
                      granny_int32x& IndexFrom,
                      granny_int32x& IndexTo);
};

END_GSTATE_NAMESPACE;

#define GSTATE_NWAY_BLEND_H
#endif /* GSTATE_NWAY_BLEND_H */
