// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_node_visitor.cpp $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#include "gstate_node_visitor.h"

#include "gstate_node_header_list.h"


GSTATE node_visitor::~node_visitor()
{
}

void GSTATE
node_visitor::VisitNode(node*)
{
    // Nada
}

// Ignore the node, defined above.
#define TAKE_ACTION_NODE(type)
#define TAKE_ACTION(type)                       \
    void GSTATE                                 \
    node_visitor::VisitNode(type* Node)         \
    {                                           \
        VisitNode((type::parent*)Node);         \
    }

#include "gstate_node_type_list.h"

#undef TAKE_ACTION_NODE
#undef TAKE_ACTION

