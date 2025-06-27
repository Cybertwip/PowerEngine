// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_blend.h $
// $DateTime: 2012/03/16 15:41:10 $
// $Change: 36794 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_BLEND_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class blend : public node
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

    virtual granny_real32 GetDuration(granny_int32x OnOutput);

    virtual void Activate(granny_int32x OnOutput, granny_real32 AtT);
    // todo: DidLoopOccur ? Only for phaselocked?

    bool CanPhaseLock(bool RefreshCache);
    bool GetPhaseLocked() const;
    void SetPhaseLocked(bool Lock);

protected:
    virtual void NoteInputChange(granny_int32x InputIndex);
    virtual void CaptureSiblingData();

    granny_real32 m_DurationFrom;
    granny_real32 m_DurationTo;

    granny_real32 m_LocalOffset;
    granny_real32 m_LastObservedParam;

    DECL_CONCRETE_CLASS_TOKEN(blend);
    DECL_SNAPPABLE();
};

END_GSTATE_NAMESPACE;

#define GSTATE_BLEND_H
#endif /* GSTATE_BLEND_H */
