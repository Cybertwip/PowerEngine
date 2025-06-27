// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_mask_invert.h $
// $DateTime: 2012/04/19 14:05:32 $
// $Change: 37093 $
// $Revision: #2 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_MASK_INVERT_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class mask_invert : public node
{
    typedef node parent;

public:
    virtual void AcceptNodeVisitor(node_visitor* Visitor);

    virtual granny_int32x AddOutput(node_edge_type EdgeType, char const* EdgeName);
    virtual bool          DeleteOutput(granny_int32x OutputIndex);

    virtual bool SampleMaskOutput(granny_int32  OutputIdx,
                                  granny_real32 AtT,
                                  granny_track_mask*);

    virtual granny_int32x GetOutputPassthrough(granny_int32x OutputIdx) const;

    DECL_CONCRETE_CLASS_TOKEN(mask_invert);
};


END_GSTATE_NAMESPACE;

#define GSTATE_MASK_INVERT_H
#endif /* GSTATE_MASK_INVERT_H */
