// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_transition_onsubloop.cpp $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#include "gstate_transition_onsubloop.h"

#include "gstate_container.h"
#include "gstate_node.h"
#include "gstate_token_context.h"

#include <string.h>

USING_GSTATE_NAMESPACE;

struct GSTATE tr_onsubloop::tr_onsubloopImpl
{
    granny_variant Subnode;        // type/token
    granny_int32   SubnodeOutput;  // The output to check (hmmm...)
};

granny_data_type_definition GSTATE
tr_onsubloop::tr_onsubloopImplType[] =
{
    { GrannyVariantReferenceMember, "Subnode" },
    { GrannyInt32Member,            "SubnodeOutput" },

    { GrannyEndMember },
};


// transition is a concrete class, so we must create a slotted container
struct tr_onsubloop_token
{
    DECL_UID();
    DECL_OPAQUE_TOKEN_SLOT(transition);
    DECL_TOKEN_SLOT(tr_onsubloop);
};

granny_data_type_definition tr_onsubloop::tr_onsubloopTokenType[] =
{
    DECL_UID_MEMBER(tr_onsubloop),
    DECL_TOKEN_MEMBER(transition),
    DECL_TOKEN_MEMBER(tr_onsubloop),

    { GrannyEndMember }
};

DEFAULT_TAKE_TOKENOWNERSHIP(tr_onsubloop);
IMPL_CREATE_DEFAULT(tr_onsubloop);


GSTATE
tr_onsubloop::tr_onsubloop(token_context*                Context,
                           granny_data_type_definition* TokenType,
                           void*                        TokenObject,
                           token_ownership              TokenOwnership)
  : parent(Context, TokenType, TokenObject, TokenOwnership),
    m_tr_onsubloopToken(0),
    m_Subnode(0)
{
    IMPL_INIT_FROM_TOKEN(tr_onsubloop);
}


GSTATE
tr_onsubloop::~tr_onsubloop()
{
    DTOR_RELEASE_TOKEN(tr_onsubloop);
}


transition_type GSTATE
tr_onsubloop::GetTransitionType() const
{
    return Transition_OnSubLoop;
}

bool GSTATE
tr_onsubloop::FillDefaultToken(granny_data_type_definition* TokenType,
                                   void*                        TokenObject)
{
    if (!parent::FillDefaultToken(TokenType, TokenObject))
        return false;

    // Declares tr_onsubloopImpl*& Slot = // member
    GET_TOKEN_SLOT(tr_onsubloop);

    // Our slot in this token should be empty.
    // Create a new tr_onsubloop token
    GStateAssert(Slot == 0);
    GStateAllocZeroedStruct(Slot);

    Slot->Subnode.Object = 0;
    Slot->Subnode.Type   = 0;
    Slot->SubnodeOutput  = -1;

    return true;
}

node* GSTATE
tr_onsubloop::GetSubnode()
{
    return m_Subnode;
}

void GSTATE
tr_onsubloop::SetSubnode(node* Node)
{
    TakeTokenOwnership();

    // We're changing nodes, reset the output, except in the very weird case here that we're resetting the same node...
    if (Node != m_Subnode)
    {
        m_tr_onsubloopToken->SubnodeOutput = -1;
    }
    
    m_Subnode = 0;
    m_tr_onsubloopToken->Subnode.Object = 0;
    m_tr_onsubloopToken->Subnode.Type   = 0;

    if (Node != 0)
    {
        if (!Node->GetTypeAndToken(&m_tr_onsubloopToken->Subnode))
        {
            GS_InvalidCodePath("Unable to obtain type and token for node");
        }
        else
        {
            m_Subnode = Node;
        }
    }
}


granny_int32 GSTATE
tr_onsubloop::GetSubnodeOutput() const
{
    return m_tr_onsubloopToken->SubnodeOutput;
}

void GSTATE
tr_onsubloop::SetSubnodeOutput(granny_int32 Output)
{
    GStateAssert(Output >= 0);

    // Don't check the output here.  We might be setting to -1, or doing some other
    // trickiness.

    TakeTokenOwnership();

    m_tr_onsubloopToken->SubnodeOutput = Output;
}

bool GSTATE
tr_onsubloop::ShouldActivate(int PassNumber,
                             activate_trigger Trigger,
                             granny_real32 AtT,
                             granny_real32 DeltaFromLastCheck)
{
    if (!m_Subnode)
        return false;

    if (PassNumber != 0 || Trigger_Requested)
        return false;

    // Not set yet?
    if (m_Subnode == 0 || m_tr_onsubloopToken->SubnodeOutput == -1)
        return false;

    container* Start = GSTATE_DYNCAST(GetStartNode(), container);
    if (Start == 0)
    {
        GS_PreconditionFailed;
        return false;
    }

    return Start->DidSubLoopOccur(m_Subnode,
                                  m_tr_onsubloopToken->SubnodeOutput,
                                  AtT, DeltaFromLastCheck);
}

bool GSTATE
tr_onsubloop::CaptureNodeLinks()
{
    if (!parent::CaptureNodeLinks())
        return false;

    if (m_tr_onsubloopToken->Subnode.Object)
    {
        tokenized* SubTokenized = GetTokenContext()->GetProductForToken(m_tr_onsubloopToken->Subnode.Object);

        // See transition::CaptureNodeLinks for reasons why this might not be present
        if (!SubTokenized)
            return false;

        m_Subnode = GSTATE_DYNCAST(SubTokenized, node);
        GStateAssert(m_Subnode);
    }
    else
    {
        m_Subnode = 0;
    }

    return true;
}

void GSTATE
tr_onsubloop::Note_NodeDelete(node* DeletedNode)
{
    // If this is the node that we're tracking, clear out the field.
    if (DeletedNode == m_Subnode)
        SetSubnode(0);
    
    // Send it up the chain for syncs and conditionals...
    parent::Note_NodeDelete(DeletedNode);
}



void GSTATE
tr_onsubloop::Note_OutputEdgeDelete(node* DeletedNode, granny_int32x EdgeIndex)
{
    // We're going to bail as soon as we discover this doesn't apply to us, so do the
    // parent work first
    parent::Note_OutputEdgeDelete(DeletedNode, EdgeIndex);

    if (DeletedNode != m_Subnode)
        return;

    if (m_tr_onsubloopToken->SubnodeOutput == -1)
        return;

    if (m_tr_onsubloopToken->SubnodeOutput == EdgeIndex)
    {
        SetSubnodeOutput(-1);
    }
    else if (m_tr_onsubloopToken->SubnodeOutput > EdgeIndex)
    {
        SetSubnodeOutput(m_tr_onsubloopToken->SubnodeOutput - 1);
    }
}
