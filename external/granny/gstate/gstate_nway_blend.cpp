// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_nway_blend.cpp $
// $DateTime: 2012/03/16 15:41:10 $
// $Change: 36794 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#include "gstate_nway_blend.h"

#include "gstate_node_visitor.h"
#include "gstate_parameters.h"
#include "gstate_token_context.h"

#include <string.h>

USING_GSTATE_NAMESPACE;


struct GSTATE nway_blend::nway_blendImpl
{
    granny_int32 DummyMember;
};

granny_data_type_definition GSTATE
nway_blend::nway_blendImplType[] =
{
    { GrannyInt32Member, "DummyMember" },
    { GrannyEndMember },
};

// nway_blend is a concrete class, so we must create a slotted container
struct nway_blend_token
{
    DECL_UID();
    DECL_OPAQUE_TOKEN_SLOT(node);
    DECL_TOKEN_SLOT(nway_blend);
};

granny_data_type_definition nway_blend::nway_blendTokenType[] =
{
    DECL_UID_MEMBER(nway_blend),
    DECL_TOKEN_MEMBER(node),
    DECL_TOKEN_MEMBER(nway_blend),

    { GrannyEndMember }
};

DEFAULT_TAKE_TOKENOWNERSHIP(nway_blend);
IMPL_CREATE_DEFAULT(nway_blend);


GSTATE
nway_blend::nway_blend(token_context*               Context,
                       granny_data_type_definition* TokenType,
                       void*                        TokenObject,
                       token_ownership              TokenOwnership)
  : parent(Context, TokenType, TokenObject, TokenOwnership),
    m_nway_blendToken(0)
{
    IMPL_INIT_FROM_TOKEN(nway_blend);

    if (EditorCreated())
    {
        // Default is one pose output and one input
        granny_int32x ParamInput = AddInput(ScalarEdge, "Parameter");
        GStateAssert(ParamInput == 0);

        AddOutput(PoseEdge,  "Pose");

        // By default, there are 3 inputs.  (Otherwise, you'd be using blend, right?)
        AddInput(PoseEdge, "Pose");
        AddInput(PoseEdge, "Pose");
        AddInput(PoseEdge, "Pose");
    }
}


GSTATE
nway_blend::~nway_blend()
{
    DTOR_RELEASE_TOKEN(nway_blend);
}


bool GSTATE
nway_blend::FillDefaultToken(granny_data_type_definition* TokenType,
                             void*                        TokenObject)
{
    if (!parent::FillDefaultToken(TokenType, TokenObject))
        return false;

    // Declares nway_blendImpl*& Slot = // member
    GET_TOKEN_SLOT(nway_blend);

    // Our slot in this token should be empty.
    // Create a new nway_blend Token
    GStateAssert(Slot == 0);
    GStateAllocZeroedStruct(Slot);

    // Nothing really here...
    Slot->DummyMember = 0;

    return true;
}

void GSTATE
nway_blend::AcceptNodeVisitor(node_visitor* Visitor)
{
    Visitor->VisitNode(this);
}

bool GSTATE
nway_blend::ComputeBlend(granny_real32  AtT,
                         granny_real32& Factor,
                         granny_int32x& IndexFrom,
                         granny_int32x& IndexTo)
{
    // We absolutely require a parameter input to do anything sensible here
    node* ParamNode = 0;
    granny_int32x ParamEdge = -1;
    GetInputConnection(0, &ParamNode, &ParamEdge);

    if (ParamNode == 0)
    {
        return false;
    }

    // By default, this goes from [0,1].  If the scalar connection is a parameters node,
    // we can grab the min and max from it directly
    granny_real32 MinVal = 0;
    granny_real32 MaxVal = 1;
    if (GSTATE_DYNCAST(ParamNode, parameters) != 0)
    {
        parameters* Parameters = static_cast<parameters*>(ParamNode);
        Parameters->GetMinMax(ParamEdge, &MinVal, &MaxVal);
        GStateAssert(MinVal <= MaxVal);
    }

    // Sample the parameters, and clamp
    granny_real32 Value = ParamNode->SampleScalarOutput(ParamEdge, AtT);
    {
        if (Value < MinVal)
            Value = MinVal;
        else if (Value > MaxVal)
            Value = MaxVal;
    }

    granny_real32 ZeroToOne;
    if (MinVal != MaxVal)
        ZeroToOne = (Value - MinVal) / (MaxVal - MinVal);
    else
        ZeroToOne = 0;

    // Handle two cases specially, 0 && 1.
    if (ZeroToOne == 0)
    {
        Factor    = 0;
        IndexFrom = 1;
        IndexTo   = 2;
        return true;
    }
    else if (ZeroToOne == 1)
    {
        Factor    = 1;
        IndexFrom = GetNumInputs() - 2;
        IndexTo   = GetNumInputs() - 1;
        return true;
    }

    // Ok, we have legitimate work to do here...

    // The 0th is the parameter, of course
    int NumPoseInputs = GetNumInputs() - 1;
    GStateAssert(NumPoseInputs >= 2);

    // Because we handled the == 1 case above, we know these should be safe indices
    granny_int32x const PoseIndex = int(ZeroToOne * (NumPoseInputs-1));
    IndexFrom = PoseIndex + 1;
    IndexTo   = PoseIndex + 2;
    GStateAssert(IndexFrom < GetNumInputs());
    GStateAssert(IndexTo < GetNumInputs());

    Factor = (ZeroToOne - (PoseIndex / float(NumPoseInputs-1))) / (1.0f / (NumPoseInputs-1));
    return true;
}

granny_local_pose* GSTATE
nway_blend::SamplePoseOutput(granny_int32x      OutputIdx,
                             granny_real32      AtT,
                             granny_real32      AllowedError,
                             granny_pose_cache* PoseCache)
{
    GStateAssert(OutputIdx >= 0 && OutputIdx < GetNumOutputs());
    GStateAssert(PoseCache);

    granny_real32 ZeroToOne;
    granny_int32x IndexFrom;
    granny_int32x IndexTo;
    if (!ComputeBlend(AtT, ZeroToOne, IndexFrom, IndexTo))
        return 0;

    // Handle two cases specially, 0 && 1.
    if (ZeroToOne == 0)
    {
        node* PoseNode = 0;
        granny_int32x PoseEdge = -1;
        GetInputConnection(IndexFrom, &PoseNode, &PoseEdge);
        if (PoseNode)
            return PoseNode->SamplePoseOutput(PoseEdge, AtT, AllowedError, PoseCache);
        else
            return 0;
    }
    else if (ZeroToOne == 1)
    {
        node* PoseNode = 0;
        granny_int32x PoseEdge = -1;
        GetInputConnection(IndexTo, &PoseNode, &PoseEdge);
        if (PoseNode)
            return PoseNode->SamplePoseOutput(PoseEdge, AtT, AllowedError, PoseCache);
        else
            return 0;
    }

    // Ok, we have legitimate work to do here...

    // From here, it's more or less exactly like the blend node, with the exception that
    // the parameter is pulled from the calculation above...
    {
        node* FromNode  = 0;
        node* ToNode    = 0;
        granny_int32x FromEdge  = -1;
        granny_int32x ToEdge    = -1;

        GetInputConnection(IndexFrom, &FromNode, &FromEdge);
        GetInputConnection(IndexTo, &ToNode, &ToEdge);

        if (!FromNode || !ToNode)
        {
            // Is one of these NULL?  That simplifies things...
            if (FromNode)
                return FromNode->SamplePoseOutput(FromEdge, AtT, AllowedError, PoseCache);
            else if (ToNode)
                return ToNode->SamplePoseOutput(ToEdge, AtT, AllowedError, PoseCache);
            else
                return 0;  // nothing connected...
        }

        // Ok, this is the full meal deal.
        granny_local_pose* FromPose = FromNode->SamplePoseOutput(FromEdge, AtT, AllowedError, PoseCache);
        granny_local_pose* ToPose   = ToNode->SamplePoseOutput(ToEdge, AtT, AllowedError, PoseCache);

        // These can still be NULL, if they are connected to a nway_blend graph that's not
        // internally wired.  Check for that here...
        if (!FromPose || !ToPose)
        {
            if (FromPose)
                return FromPose;
            else if (ToPose)
                return ToPose;
            else
                return 0;
        }

        // Nway_Blend, and release endpose before we return
        GrannyLinearBlend(FromPose, FromPose, ToPose, ZeroToOne);
        GrannyReleaseCachePose(PoseCache, ToPose);
        return FromPose;
    }
}

bool GSTATE
nway_blend::GetRootMotionVectors(granny_int32x  OutputIdx,
                                 granny_real32  AtT,
                                 granny_real32  DeltaT,
                                 granny_real32* Translation,
                                 granny_real32* Rotation,
                                 bool Inverse)
{
    GStateAssert(GetBoundCharacter());
    GStateAssert(DeltaT >= 0);
    if (!Translation || !Rotation)
    {
        GS_PreconditionFailed;
        return false;
    }

    granny_real32 ZeroToOne;
    granny_int32x IndexFrom;
    granny_int32x IndexTo;
    if (!ComputeBlend(AtT, ZeroToOne, IndexFrom, IndexTo))
        return 0;

    // Clear these out by default...
    memset(Translation, 0, sizeof(granny_real32)*3);
    memset(Rotation,    0, sizeof(granny_real32)*3);

    // Handle two cases specially, 0 && 1.
    if (ZeroToOne == 0 || ZeroToOne == 1)
    {
        node* PoseNode = 0;
        granny_int32x PoseEdge = -1;
        if (ZeroToOne == 0)
            GetInputConnection(IndexFrom, &PoseNode, &PoseEdge);
        else
            GetInputConnection(IndexTo, &PoseNode, &PoseEdge);

        if (PoseNode)
            return PoseNode->GetRootMotionVectors(PoseEdge, AtT, DeltaT, Translation, Rotation, Inverse);
        else
            return false;
    }

    // From here, it's more or less exactly like the blend node, with the exception that
    // the parameter is pulled from the calculation above...
    node* FromNode  = 0;
    node* ToNode    = 0;
    granny_int32x FromEdge  = -1;
    granny_int32x ToEdge    = -1;

    GetInputConnection(IndexFrom, &FromNode, &FromEdge);
    GetInputConnection(IndexTo, &ToNode, &ToEdge);
    if (!FromNode || !ToNode)
    {
        // Is one of these NULL?  That simplifies things...
        if (FromNode)
            return FromNode->GetRootMotionVectors(FromEdge, AtT, DeltaT, Translation, Rotation, Inverse);
        else if (ToNode)
            return ToNode->GetRootMotionVectors(ToEdge, AtT, DeltaT, Translation, Rotation, Inverse);
        else
            return false;  // nothing connected...
    }

    // Ok, this is the full meal deal.
    granny_real32 FromTranslation[3] = { 0 };
    granny_real32 FromRotation[3]    = { 0 };
    granny_real32 ToTranslation[3]   = { 0 };
    granny_real32 ToRotation[3]      = { 0 };

    bool FromSuccess = FromNode->GetRootMotionVectors(FromEdge, AtT, DeltaT, FromTranslation, FromRotation, Inverse);
    bool ToSuccess = ToNode->GetRootMotionVectors(ToEdge, AtT, DeltaT, ToTranslation, ToRotation, Inverse);

    // These can still be NULL, if they are connected to a nway_blend graph that's not
    // internally wired.  Check for that here...
    if (!FromSuccess || !ToSuccess)
    {
        if (FromSuccess)
        {
            memcpy(Translation, FromTranslation, sizeof(FromTranslation));
            memcpy(Rotation, FromRotation, sizeof(FromRotation));
            return true;
        }
        else if (ToSuccess)
        {
            memcpy(Translation, ToTranslation, sizeof(ToTranslation));
            memcpy(Rotation, ToRotation, sizeof(ToRotation));
            return true;
        }

        return false;
    }

    {for (int Idx = 0; Idx < 3; ++Idx)
    {
        Translation[Idx] = FromTranslation[Idx] * (1 - ZeroToOne) + ToTranslation[Idx] * ZeroToOne;
        Rotation[Idx]    = FromRotation[Idx]    * (1 - ZeroToOne) + ToRotation[Idx]    * ZeroToOne;
    }}

    return true;
}


void GSTATE
nway_blend::Activate(granny_int32x OnOutput, granny_real32 AtT)
{
    {for (int Idx = 0; Idx < GetNumInputs(); ++Idx)
    {
        INPUT_CONNECTION(Idx, Input);
        if (InputNode != 0)
            InputNode->Activate(InputEdge, AtT);
    }}
}

