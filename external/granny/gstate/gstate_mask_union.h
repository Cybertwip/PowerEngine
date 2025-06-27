// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_mask_union.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_MASK_UNION_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class mask_union : public node
{
    typedef node parent;

public:
    virtual bool BindToCharacter(gstate_character_instance* Instance);
    virtual void UnbindFromCharacter();

    virtual void AcceptNodeVisitor(node_visitor* Visitor);

    virtual granny_int32x AddOutput(node_edge_type EdgeType, char const* EdgeName);
    virtual bool          DeleteOutput(granny_int32x OutputIndex);

    virtual bool SampleMaskOutput(granny_int32x OutputIdx,
                                  granny_real32 AtT,
                                  granny_track_mask*);

    DECL_CONCRETE_CLASS_TOKEN(mask_union);

private:
    // Set with BindToCharacter/Unbind
    granny_int32x      m_CachedBoneCount;
    granny_track_mask* m_CachedMask;
};


END_GSTATE_NAMESPACE;

#define GSTATE_MASK_UNION_H
#endif /* GSTATE_MASK_UNION_H */
