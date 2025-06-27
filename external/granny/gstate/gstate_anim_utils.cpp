// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_anim_utils.cpp $
// $DateTime: 2012/02/07 17:19:59 $
// $Change: 36502 $
// $Revision: #3 $
//
// $Notice: $
// ========================================================================
#include "gstate_anim_utils.h"
#include <math.h>

#include <string.h>
#include <stdlib.h>

USING_GSTATE_NAMESPACE;

granny_real32
ComputeRawTime(granny_real32 AtTime,
               granny_real32 Speed,
               granny_real32 Offset,
               granny_int32x LoopCount,
               granny_real32 AnimDuration)
{
    granny_real32 Unclamped = (AtTime * Speed) + Offset;
    if (LoopCount <= 0)
        return Unclamped;

    granny_real32 ClampStart = Offset;
    granny_real32 ClampEnd   = ((AnimDuration * LoopCount) * Speed) + Offset;

    return Clamp(ClampStart, Unclamped, ClampEnd);
}

granny_real32
ComputeLoopedTime(granny_real32 AtTime,
                  granny_real32 Speed,
                  granny_real32 Offset,
                  granny_int32x LoopCount,
                  granny_real32 AnimDuration)
{
    granny_real32 LocalUnclamped = (AtTime * Speed) + Offset;
    if (LoopCount <= 0)
        return LocalUnclamped;

    granny_real32 LocalClampEnd = (AnimDuration * LoopCount);

    if (LocalUnclamped <= 0)
        return 0;
    else if (LocalUnclamped >= LocalClampEnd)
        return LocalClampEnd;

    return LocalUnclamped;
}


// Note that because of the local clamp end branch, this cannot simply be a
// fmod(ComputeLooped)
granny_real32
ComputeModTime(granny_real32 AtTime,
               granny_real32 Speed,
               granny_real32 Offset,
               granny_int32x LoopCount,
               granny_real32 AnimDuration)
{
    granny_real32 LocalUnclamped = (AtTime * Speed) + Offset;
    if (LoopCount <= 0)
    {
        granny_real32 PossiblyNeg = fmod(LocalUnclamped, AnimDuration);
        if (PossiblyNeg < 0)
            return PossiblyNeg + AnimDuration;
        else
            return PossiblyNeg;
    }

    granny_real32 LocalClampEnd = (AnimDuration * LoopCount);

    if (LocalUnclamped <= 0)
        return 0;
    else if (LocalUnclamped >= LocalClampEnd)
        return AnimDuration;

    return fmod(LocalUnclamped, AnimDuration);
}

static int
CompareDuration(void const* One, void const* Two)
{
    granny_text_track_entry const* OneEntry = (granny_text_track_entry const*)One;
    granny_text_track_entry const* TwoEntry = (granny_text_track_entry const*)Two;

    if (OneEntry->TimeStamp < TwoEntry->TimeStamp)
        return -1;
    else if (OneEntry->TimeStamp > TwoEntry->TimeStamp)
        return 1;
    else
        return 0;
}

void
SortEventsByTimestamp(granny_text_track_entry* Events,
                      granny_int32x NumEvents)
{
    qsort(Events, NumEvents, sizeof(granny_text_track_entry), CompareDuration);
}

granny_int32x
FilterDuplicateEvents(granny_text_track_entry* Events,
                      granny_int32x KnownUnique,
                      granny_int32x NumEvents)
{
    GStateAssert(NumEvents >= KnownUnique);

    // Merge in the new events (maybedo: sort to avoid n^2?)...
    while (NumEvents != KnownUnique)
    {
        bool Found = false;
        {for (int Idx = 0; Idx < KnownUnique; ++Idx)
        {
            if (strcmp(Events[Idx].Text, Events[KnownUnique].Text) == 0)
            {
                Found = true;
                break;
            }
        }}

        if (Found)
        {
            // Do not advance
            // Move the last event down to this position.  (Might be a redundant copy a the end...)
            Events[KnownUnique] = Events[NumEvents - 1];
            --NumEvents;
        }
        else
        {
            // Advance, take the event
            ++KnownUnique;
        }
    }

    return KnownUnique;
}


