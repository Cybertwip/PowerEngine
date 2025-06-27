// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_mask_invert.cpp $
// $DateTime: 2012/04/19 14:05:32 $
// $Change: 37093 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#include "gstate_mask_invert.h"

#include "gstate_node_visitor.h"
#include "gstate_character_instance.h"
#include "gstate_token_context.h"

#include <string.h>
#include <math.h>

#define GSTATE_INTERNAL_HEADER 1
#include "gstate_character_internal.h"

USING_GSTATE_NAMESPACE;

struct GSTATE mask_invert::mask_invertImpl
{
    granny_int32 Dummy;
};

granny_data_type_definition GSTATE
mask_invert::mask_invertImplType[] =
{
    { GrannyInt32Member, "Dummy" },
    { GrannyEndMember },
};

// mask_invert is a concrete class, so we must create a slotted container
struct mask_invert_token
{
    DECL_UID();
    DECL_OPAQUE_TOKEN_SLOT(node);
    DECL_TOKEN_SLOT(mask_invert);
};

granny_data_type_definition mask_invert::mask_invertTokenType[] =
{
    DECL_UID_MEMBER(mask_invert),
    DECL_TOKEN_MEMBER(node),
    DECL_TOKEN_MEMBER(mask_invert),

    { GrannyEndMember }
};

DEFAULT_TAKE_TOKENOWNERSHIP(mask_invert);
IMPL_CREATE_DEFAULT(mask_invert);


GSTATE
mask_invert::mask_invert(token_context*               Context,
                         granny_data_type_definition* TokenType,
                         void*                        TokenObject,
                         token_ownership              TokenOwnership)
  : parent(Context, TokenType, TokenObject, TokenOwnership),
    m_mask_invertToken(0)
{
    IMPL_INIT_FROM_TOKEN(mask_invert);

    if (EditorCreated())
    {
        // Add our default input/output
        AddInput(MaskEdge, "Mask");
        AddOutput(MaskEdge, "Mask");
    }
}


GSTATE
mask_invert::~mask_invert()
{
    DTOR_RELEASE_TOKEN(mask_invert);
}


bool GSTATE
mask_invert::FillDefaultToken(granny_data_type_definition* TokenType,
                              void* TokenObject)
{
    if (!parent::FillDefaultToken(TokenType, TokenObject))
        return false;

    // Declares mask_invertImpl*& Slot = // member
    GET_TOKEN_SLOT(mask_invert);

    // Our slot in this token should be empty.
    // Create a new mask invert Token
    GStateAssert(Slot == 0);
    GStateAllocZeroedStruct(Slot);

    return true;
}


void GSTATE
mask_invert::AcceptNodeVisitor(node_visitor* Visitor)
{
    Visitor->VisitNode(this);
}

granny_int32x GSTATE
mask_invert::AddOutput(node_edge_type EdgeType,
                       char const*    EdgeName)
{
    GStateAssert(EdgeType == MaskEdge);
    TakeTokenOwnership();

    int NewOutput = parent::AddOutput(EdgeType, EdgeName);
    GStateAssert(NewOutput == GetNumOutputs() - 1);

    return NewOutput;
}

bool GSTATE
mask_invert::DeleteOutput(granny_int32x OutputIndex)
{
    GS_NotYetImplemented;
    return false;
}


bool GSTATE
mask_invert::SampleMaskOutput(granny_int32x      OutputIdx,
                              granny_real32      AtT,
                              granny_track_mask* ModelMask)
{
    GStateAssert(OutputIdx == 0);
    GStateAssert(ModelMask != 0);

    node* MaskNode = 0;
    granny_int32x MaskEdge = -1;
    GetInputConnection(0, &MaskNode, &MaskEdge);

    if (!MaskNode)
        return false;

    if (MaskNode->SampleMaskOutput(MaskEdge, AtT, ModelMask) == false)
        return false;

    GrannyInvertTrackMask(ModelMask);
    return true;
}

granny_int32x GSTATE
mask_invert::GetOutputPassthrough(granny_int32x OutputIdx) const
{
    return OutputIdx;
}
