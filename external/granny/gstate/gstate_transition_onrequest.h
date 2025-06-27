// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_transition_onrequest.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_TRANSITION_ONREQUEST_H)

#ifndef GSTATE_TRANSITION_H
#include "gstate_transition.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class tr_onrequest : public transition
{
    typedef transition parent;

public:
    virtual transition_type GetTransitionType() const;

    bool ShouldActivate(int PassNumber,
                        activate_trigger Trigger,
                        granny_real32 AtT,
                        granny_real32 DeltaT);

    DECL_CONCRETE_CLASS_TOKEN(tr_onrequest);
};

END_GSTATE_NAMESPACE;

#define GSTATE_TRANSITION_ONREQUEST_H
#endif /* GSTATE_TRANSITION_ONREQUEST_H */
