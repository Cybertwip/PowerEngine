// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_timeshift.cpp $
// $DateTime: 2012/04/19 14:05:32 $
// $Change: 37093 $
// $Revision: #5 $
//
// $Notice: $
// ========================================================================
#include "gstate_timeshift.h"

#include "gstate_node_visitor.h"
#include "gstate_token_context.h"

#include <string.h>

USING_GSTATE_NAMESPACE;

struct GSTATE timeshift::timeshiftImpl
{
    granny_int32 Dummy;
};

granny_data_type_definition GSTATE
timeshift::timeshiftImplType[] =
{
    { GrannyInt32Member, "Dummy" },
    { GrannyEndMember },
};

// timeshift is a concrete class, so we must create a slotted container
struct timeshift_token
{
    DECL_UID();
    DECL_OPAQUE_TOKEN_SLOT(node);
    DECL_TOKEN_SLOT(timeshift);
};

granny_data_type_definition timeshift::timeshiftTokenType[] =
{
    DECL_UID_MEMBER(timeshift),
    DECL_TOKEN_MEMBER(node),
    DECL_TOKEN_MEMBER(timeshift),

    { GrannyEndMember }
};

DEFAULT_TAKE_TOKENOWNERSHIP(timeshift);
IMPL_CREATE_DEFAULT(timeshift);


GSTATE
timeshift::timeshift(token_context*               Context,
                     granny_data_type_definition* TokenType,
                     void*                        TokenObject,
                     token_ownership              TokenOwnership)
  : parent(Context, TokenType, TokenObject, TokenOwnership),
    m_timeshiftToken(0)
{
    IMPL_INIT_FROM_TOKEN(timeshift);

    if (EditorCreated())
    {
        // Add our default input/output
        parent::AddInput(ScalarEdge, "TimeScale");
        parent::AddInput(ScalarEdge, "TimeOffset");
        parent::AddInput(PoseEdge,   "Pose");
        parent::AddOutput(PoseEdge,  "Pose");
    }
}


GSTATE
timeshift::~timeshift()
{
    DTOR_RELEASE_TOKEN(timeshift);
}


bool GSTATE
timeshift::FillDefaultToken(granny_data_type_definition* TokenType,
                                void* TokenObject)
{
    if (!parent::FillDefaultToken(TokenType, TokenObject))
        return false;

    // Declares timeshiftImpl*& Slot = // member
    GET_TOKEN_SLOT(timeshift);

    // Our slot in this token should be empty.
    // Create a new mask invert Token
    GStateAssert(Slot == 0);
    GStateAllocZeroedStruct(Slot);

    return true;
}


void GSTATE
timeshift::AcceptNodeVisitor(node_visitor* Visitor)
{
    Visitor->VisitNode(this);
}

void GSTATE
timeshift::GetScaleOffset(granny_real32 AtT,
                          granny_real32& Scale,
                          granny_real32& Offset)
{
    node* ScaleNode = 0;
    granny_int32x ScaleEdge = -1;
    GetInputConnection(0, &ScaleNode, &ScaleEdge);
    if (ScaleNode != 0)
    {
        Scale = ScaleNode->SampleScalarOutput(ScaleEdge, AtT);

        // Clamp the scale to 0, since we can't run backwards
        if (Scale < 0)
            Scale = 0.0f;
    }

    node* OffsetNode = 0;
    granny_int32x OffsetEdge = -1;
    GetInputConnection(1, &OffsetNode, &OffsetEdge);
    if (OffsetNode != 0)
    {
        Offset = OffsetNode->SampleScalarOutput(OffsetEdge, AtT);
    }
}

granny_local_pose* GSTATE
timeshift::SamplePoseOutput(granny_int32x      OutputIdx,
                            granny_real32      AtT,
                            granny_real32      AllowedError,
                            granny_pose_cache* PoseCache)
{
    GStateAssert(OutputIdx == 0);
    GStateAssert(PoseCache != 0);

    node* PoseNode = 0;
    granny_int32x PoseEdge = -1;
    GetInputConnection(OutputIdx+2, &PoseNode, &PoseEdge);

    if (!PoseNode)
        return 0;

    granny_real32 Scale  = 1.0f;
    granny_real32 Offset = 0.0f;
    GetScaleOffset(AtT, Scale, Offset);

    granny_real32 SampleT = AtT * Scale + Offset;
    return PoseNode->SamplePoseOutput(PoseEdge, SampleT, AllowedError, PoseCache);
}

bool GSTATE
timeshift::GetRootMotionVectors(granny_int32x  OutputIdx,
                                granny_real32  AtT,
                                granny_real32  DeltaT,
                                granny_real32* Translation,
                                granny_real32* Rotation,
                                bool Inverse)
{
    if (DeltaT < 0)
    {
        GS_PreconditionFailed;
        return false;
    }
    if (!Translation || !Rotation)
    {
        GS_PreconditionFailed;
        return false;
    }

    node* FromNode = 0;
    granny_int32x FromEdge = -1;

    GetInputConnection(OutputIdx+2, &FromNode, &FromEdge);
    if (!(FromNode))
        return false;

    granny_real32 Scale  = 1.0f;
    granny_real32 Offset = 0.0f;
    GetScaleOffset(AtT, Scale, Offset);

    granny_real32 SampleT  = AtT * Scale + Offset;
    granny_real32 NewDelta = DeltaT * Scale;

    return FromNode->GetRootMotionVectors(FromEdge, SampleT, NewDelta, Translation, Rotation, Inverse);
}

granny_int32x GSTATE
timeshift::AddOutput(node_edge_type EdgeType, char const* EdgeName)
{
    GStateAssert(EdgeType == PoseEdge);
    GStateAssert(EdgeName);

    granny_int32x NewInput = AddInput(EdgeType, EdgeName);
    granny_int32x NewOutput = parent::AddOutput(EdgeType, EdgeName);
    GStateAssert(NewInput == NewOutput + 2);

    return NewOutput;
}

bool GSTATE
timeshift::DeleteOutput(granny_int32x OutputIndex)
{
    GStateAssert(GS_InRange(OutputIndex, GetNumOutputs()));
    GStateAssert(GS_InRange(OutputIndex+2, GetNumInputs()));

    if (DeleteInput(OutputIndex+2) == false)
    {
        GS_InvalidCodePath("catastrohpic");
    }

    return parent::DeleteOutput(OutputIndex);
}

granny_int32x GSTATE
timeshift::GetOutputPassthrough(granny_int32x OutputIdx) const
{
    GStateAssert(GS_InRange(OutputIdx, GetNumOutputs()));

    return OutputIdx + 2;
}


void GSTATE
timeshift::Activate(granny_int32x OnOutput,
                    granny_real32 AtT)
{
    parent::Activate(OnOutput, AtT);

    GStateAssert(OnOutput == 0);

    node* PoseNode = 0;
    granny_int32x PoseEdge = -1;
    GetInputConnection(OnOutput+2, &PoseNode, &PoseEdge);

    if (!PoseNode)
        return;

    granny_real32 Scale  = 1.0f;
    granny_real32 Offset = 0.0f;
    GetScaleOffset(AtT, Scale, Offset);

    granny_real32 NewAtT  = AtT * Scale + Offset;

    PoseNode->Activate(PoseEdge, NewAtT);
}

bool GSTATE
timeshift::DidLoopOccur(granny_int32x OnOutput,
                        granny_real32 AtT,
                        granny_real32 DeltaT)
{
    GStateAssert(OnOutput == 0);

    node* FromNode = 0;
    granny_int32x FromEdge = -1;

    GetInputConnection(OnOutput+2, &FromNode, &FromEdge);
    if (!(FromNode))
        return false;

    granny_real32 Scale  = 1.0f;
    granny_real32 Offset = 0.0f;
    GetScaleOffset(AtT, Scale, Offset);

    granny_real32 NewAtT   = AtT * Scale + Offset;
    granny_real32 NewDelta = DeltaT * Scale;

    return FromNode->DidLoopOccur(FromEdge, NewAtT, NewDelta);
}
