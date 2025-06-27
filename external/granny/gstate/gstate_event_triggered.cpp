// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_event_triggered.cpp $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#include "gstate_event_triggered.h"

#include "gstate_node.h"
#include "gstate_state_machine.h"
#include "gstate_token_context.h"
#include <string.h>

USING_GSTATE_NAMESPACE;

struct GSTATE event_triggered::event_triggeredImpl
{
    granny_int32  OutputIndex;
    char*         EventName;
};


granny_data_type_definition GSTATE
event_triggered::event_triggeredImplType[] =
{
    { GrannyInt32Member,  "OutputIndex"    },
    { GrannyStringMember, "EventName"  },
    { GrannyEndMember },
};


// event_triggered is a concrete class, so we must create a slotted container
struct event_triggered_token
{
    DECL_UID();
    DECL_OPAQUE_TOKEN_SLOT(conditional);
    DECL_TOKEN_SLOT(event_triggered);
};

granny_data_type_definition event_triggered::event_triggeredTokenType[] =
{
    DECL_UID_MEMBER(event_triggered),
    DECL_TOKEN_MEMBER(conditional),
    DECL_TOKEN_MEMBER(event_triggered),

    { GrannyEndMember }
};

void GSTATE
event_triggered::TakeTokenOwnership()
{
    TAKE_TOKEN_OWNERSHIP(event_triggered);

    // When we take token ownership, future code will assume that it needs to free our
    // owned pointers when they change.  Don't disappoint them.
    GStateCloneString(m_event_triggeredToken->EventName, OldToken->EventName);
}

void GSTATE
event_triggered::ReleaseOwnedToken_event_triggered()
{
    GStateDeallocate(m_event_triggeredToken->EventName);
}


IMPL_CREATE_DEFAULT(event_triggered);

GSTATE
event_triggered::event_triggered(token_context*               Context,
                                 granny_data_type_definition* TokenType,
                                 void*                        TokenObject,
                                 token_ownership              TokenOwnership)
  : parent(Context, TokenType, TokenObject, TokenOwnership),
    m_event_triggeredToken(0)
{
    IMPL_INIT_FROM_TOKEN(event_triggered);
}


GSTATE
event_triggered::~event_triggered()
{
    DTOR_RELEASE_TOKEN(event_triggered);
}


conditional_type GSTATE
event_triggered::GetType()
{
    return Conditional_EventTriggered;
}

bool GSTATE
event_triggered::FillDefaultToken(granny_data_type_definition* TokenType,
                                  void*                        TokenObject)
{
    if (!parent::FillDefaultToken(TokenType, TokenObject))
        return false;

    // Declares event_triggeredImpl*& Slot = // member
    GET_TOKEN_SLOT(event_triggered);

    // Our slot in this token should be empty.
    // Create a new event_triggered token
    GStateAssert(Slot == 0);
    GStateAllocZeroedStruct(Slot);

    Slot->OutputIndex  = -1;
    Slot->EventName = 0;

    return true;
}

bool GSTATE
event_triggered::IsTrueImpl(granny_real32 AtT, granny_real32 DeltaT)
{
    // If we're not set, we can't activate
    if (m_event_triggeredToken->OutputIndex == -1 ||
        m_event_triggeredToken->EventName == 0)
    {
        return false;
    }

    state_machine* Machine = GetOwner();

    granny_text_track_entry Entries[16];
    granny_int32x NumUsed = 0;
    if (!Machine->SampleEventOutput(m_event_triggeredToken->OutputIndex,
                                    AtT, DeltaT,
                                    Entries, GStateArrayLen(Entries),
                                    &NumUsed))
    {
        // warn
        return false;
    }

    {for (int Idx = 0; Idx < NumUsed; ++Idx)
    {
        if (strcmp(Entries[Idx].Text, m_event_triggeredToken->EventName) == 0)
            return true;
    }}

    return false;
}


void GSTATE
event_triggered::SetTriggerEvent(granny_int32x OutputIndex,
                                 char const*   EventName)
{
    TakeTokenOwnership();

    m_event_triggeredToken->OutputIndex = OutputIndex;
    GStateReplaceString(m_event_triggeredToken->EventName, EventName);
}

void GSTATE
event_triggered::GetTriggerEvent(granny_int32x* OutputIndex,
                                 char const**   EventName)
{
    *OutputIndex = m_event_triggeredToken->OutputIndex;
    *EventName   = m_event_triggeredToken->EventName;
}

void GSTATE
event_triggered::Note_OutputEdgeDelete(node*         AffectedNode,
                                       granny_int32x RemovedOutput)
{
    TakeTokenOwnership();

    // We only care if this is our owner, that's the only node we can refer to for now...
    if (AffectedNode == GetOwner())
    {
        if (m_event_triggeredToken->OutputIndex == RemovedOutput)
            SetTriggerEvent(-1, 0);
        else if (m_event_triggeredToken->OutputIndex > RemovedOutput)
            --m_event_triggeredToken->OutputIndex;
    }
    
    parent::Note_OutputEdgeDelete(AffectedNode, RemovedOutput);
}
