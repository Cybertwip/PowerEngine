// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_timeshift.h $
// $DateTime: 2012/04/19 14:05:32 $
// $Change: 37093 $
// $Revision: #4 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_TIMESHIFT_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class timeshift : public node
{
    typedef node parent;

public:
    virtual void AcceptNodeVisitor(node_visitor* Visitor);

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

    virtual granny_int32x AddOutput(node_edge_type EdgeType, char const* EdgeName);
    virtual bool          DeleteOutput(granny_int32x OutputIndex);

    DECL_CONCRETE_CLASS_TOKEN(timeshift);

public:
    virtual void Activate(granny_int32x OnOutput, granny_real32 AtT);
    virtual bool DidLoopOccur(granny_int32x OnOutput,
                              granny_real32 AtT,
                              granny_real32 DeltaT);

    virtual granny_int32x GetOutputPassthrough(granny_int32x OutputIdx) const;

private:
    void GetScaleOffset(granny_real32 AtT,
                        granny_real32& Scale,
                        granny_real32& Offset);
};


END_GSTATE_NAMESPACE;

#define GSTATE_TIMESHIFT_H
#endif /* GSTATE_TIMESHIFT_H */
