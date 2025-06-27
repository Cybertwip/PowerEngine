// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_conditional.h $
// $DateTime: 2012/01/25 13:33:09 $
// $Change: 36369 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_CONDITIONAL_H)

#ifndef GSTATE_TOKENIZED_H
#include "gstate_tokenized.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class state_machine;

// These may *never* change
enum conditional_type
{
    Conditional_ScalarCompare  = 0,
    Conditional_EventTriggered = 1
};

// todo: holy shit.
char const*  NameForConditionalType(conditional_type);

class conditional : public tokenized
{
    typedef tokenized parent;

public:
    virtual conditional_type GetType() = 0;

    virtual char const* GetName() const;
    virtual bool        SetName(char const* NewName);

    bool GetInitialState() const;
    void SetInitialState(bool InitialState);


    // Each conditional belongs to a state_machine
    state_machine* GetOwner();

    void SetOwner(state_machine* Owner);

    void Activate(granny_real32 AtT);
    bool IsTrue(granny_real32 AtT);

    DECL_CLASS_TOKEN(conditional);
    DECL_SNAPPABLE();
    IMPL_CASTABLE_INTERFACE(conditional);

protected:
    state_machine* m_Owner;

    granny_real32  m_LastValid;
    bool           m_LastObservation;

    virtual bool IsTrueImpl(granny_real32 AtT, granny_real32 DeltaT) = 0;

public:
    virtual bool CaptureNodeLinks();
    virtual void Note_OutputEdgeDelete(node*         AffectedNode,
                                       granny_int32x RemovedOutput);
};

END_GSTATE_NAMESPACE;

#define GSTATE_CONDITIONAL_H
#endif /* GSTATE_CONDITIONAL_H */
