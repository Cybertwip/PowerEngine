#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bone_operations.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct transform;

void BuildIdentityWorldPoseComposite(real32 const *ParentMatrix,
                                     real32 const *InverseWorld4x4,
                                     real32 *Result, real32 *ResultWorldMatrix);
void BuildPositionWorldPoseComposite(real32 const *Position,
                                     real32 const *ParentMatrix,
                                     real32 const *InverseWorld4x4,
                                     real32 *Result, real32 *ResultWorldMatrix);
void BuildPositionOrientationWorldPoseComposite(real32 const *Position, real32 const *Orientation,
                                                real32 const *ParentMatrix, real32 const *InverseWorld4x4,
                                                real32 *Result, real32 *ResultWorldMatrix);
void BuildFullWorldPoseComposite(transform const &Transform,
                                 real32 const *ParentMatrix, real32 const *InverseWorld4x4,
                                 real32 *Result, real32 *ResultWorldMatrix);
void BuildIdentityWorldPoseOnly(real32 const *ParentMatrix,
                                real32 *ResultWorldMatrix);
void BuildPositionWorldPoseOnly(real32 const *Position,
                                real32 const *ParentMatrix,
                                real32 *ResultWorldMatrix);
void BuildPositionOrientationWorldPoseOnly(real32 const *Position, real32 const *Orientation,
                                           real32 const *ParentMatrix,
                                           real32 *ResultWorldMatrix);
void BuildFullWorldPoseOnly(transform const &Transform,
                            real32 const *ParentMatrix,
                            real32 *ResultWorldMatrix);
void BuildSingleCompositeFromWorldPose(real32 const *InverseWorld4x4,
                                       real32 const *WorldMatrix,
                                       real32 *ResultComposite);
void BuildSingleCompositeFromWorldPoseTranspose(real32 const *InverseWorld4x4,
                                                real32 const *WorldMatrix,
                                                real32 *ResultComposite);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BONE_OPERATIONS_H
#endif
