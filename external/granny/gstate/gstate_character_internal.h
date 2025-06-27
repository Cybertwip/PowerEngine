#if !GSTATE_INTERNAL_HEADER
#error "Should not be included from user code"
#endif

// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_character_internal.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_CHARACTER_INTERNAL_H)

#ifndef GSTATE_BASE_H
#include "gstate_base.h"
#endif

struct gstate_character_instance;

BEGIN_GSTATE_NAMESPACE;

class token_context;

struct source_file_ref
{
    char*             SourceFilename;
    granny_int32      ExpectedAnimCount;
    granny_uint32     AnimCRC;

    granny_file_info* SourceInfo;
};

struct animation_spec
{
    source_file_ref* SourceReference;
    granny_int32     AnimationIndex;
    char*            ExpectedName;
};

struct animation_slot
{
    char*        Name;
    granny_int32 Index;  // This is redundant, but we need to refer to animation_slots by pointer...
};

struct animation_set
{
    char* Name;

    granny_int32      SourceFileReferenceCount;
    source_file_ref** SourceFileReferences;

    // AnimationSpecCount == AnimationSlotCount in the containing info...
    granny_int32    AnimationSpecCount;
    animation_spec* AnimationSpecs;
};


extern granny_data_type_definition AnimationSetType[];
extern granny_data_type_definition AnimationSlotType[];

// Ok, why am I hiding this here?  Because it's not for you.  So there.  These are for the
// editor.  Seriously.  Don't call these in your game.  :)
class state_machine;
gstate_character_instance* InstantiateEditCharacter(gstate_character_info* Info,
                                                    granny_int32x AnimationSet,
                                                    granny_model* Model);
void UnbindEditCharacter(gstate_character_instance*);
void ReplaceStateMachine(gstate_character_instance*, state_machine* NewMachine);
void SetCurrentAnimationSet(gstate_character_instance*, granny_int32x Set);

granny_uint32 ComputeAnimCRC(granny_file_info*);

END_GSTATE_NAMESPACE;

struct gstate_character_info
{
    // May be zero, which indicates that we can't properly presize the token contexts for
    // instances
    granny_int32 NumUniqueTokenized;

    // The root is always a state_machine, but it does go through the token system
    granny_variant StateMachine;

    granny_int32            AnimationSlotCount;
    GSTATE animation_slot** AnimationSlots;

    granny_int32           AnimationSetCount;
    GSTATE animation_set** AnimationSets;

    // Used by the editor to auto-load the last used model if possible.
    char*        ModelNameHint;
    granny_int32 ModelIndexHint;
};

struct gstate_character_instance
{
    granny_model* TargetModel;
    granny_model* SourceModel;

    // granny_retargeter* Retargeter;

    GSTATE token_context*         TokenContext;
    gstate_character_info* CharacterInfo;

    // 0 by default...
    granny_int32x AnimationSet;

    GSTATE state_machine* StateMachine;

    granny_real32 CurrentTime; // Starts at 0 (todo: random?)
    granny_real32 DeltaT;      // The parameter from the last AdvanceT call
};


#define GSTATE_CHARACTER_INTERNAL_H
#endif /* GSTATE_CHARACTER_INTERNAL_H */

