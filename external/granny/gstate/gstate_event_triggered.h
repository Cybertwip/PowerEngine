// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_event_triggered.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_EVENT_TRIGGERED_H)

#if !defined(GSTATE_CONDITIONAL_H)
#include "gstate_conditional.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class event_triggered : public conditional
{
    typedef conditional parent;

public:
    virtual conditional_type GetType();

    void SetTriggerEvent(granny_int32x OutputIndex, char const*   EventName);
    void GetTriggerEvent(granny_int32x* OutputIndex, char const**   EventName);

    virtual void Note_OutputEdgeDelete(node*         AffectedNode,
                                       granny_int32x RemovedOutput);

    DECL_CONCRETE_CLASS_TOKEN(event_triggered);
protected:
    virtual bool IsTrueImpl(granny_real32 AtT, granny_real32 DeltaT);
};

END_GSTATE_NAMESPACE;

#define GSTATE_EVENT_TRIGGERED_H
#endif /* GSTATE_EVENT_TRIGGERED_H */
