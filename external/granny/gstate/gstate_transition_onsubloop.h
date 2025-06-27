// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_transition_onsubloop.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_TRANSITION_ONSUBLOOP_H)

#ifndef GSTATE_TRANSITION_H
#include "gstate_transition.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class tr_onsubloop : public transition
{
    typedef transition parent;

public:
    virtual transition_type GetTransitionType() const;

    bool ShouldActivate(int PassNumber,
                        activate_trigger Trigger,
                        granny_real32 AtT,
                        granny_real32 DeltaT);

    DECL_CONCRETE_CLASS_TOKEN(tr_onsubloop);

    node* GetSubnode();
    void  SetSubnode(node*);

    granny_int32 GetSubnodeOutput() const;
    void SetSubnodeOutput(granny_int32 Output);

private:
    virtual bool CaptureNodeLinks();

    virtual void Note_NodeDelete(node* DeletedNode);
    virtual void Note_OutputEdgeDelete(node* DeletedNode, granny_int32x EdgeIndex);

    node* m_Subnode;
};

END_GSTATE_NAMESPACE;

#define GSTATE_TRANSITION_ONSUBLOOP_H
#endif /* GSTATE_TRANSITION_ONSUBLOOP_H */
