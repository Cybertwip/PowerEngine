// ========================================================================
// $Notice: $
// ========================================================================
#if !defined(GSTATE_NODE_VISITOR_H)

#ifndef GSTATE_BASE_H
#include "gstate_base.h"
#endif

BEGIN_GSTATE_NAMESPACE;

#define TAKE_ACTION(typename) class typename;
#include "gstate_node_type_list.h"
#undef TAKE_ACTION

class node_visitor
{
public:
    virtual ~node_visitor();

#define TAKE_ACTION(typename) virtual void VisitNode(typename*);
#include "gstate_node_type_list.h"
#undef TAKE_ACTION

};

END_GSTATE_NAMESPACE;

#define GSTATE_NODE_VISITOR_H
#endif /* GSTATE_NODE_VISITOR_H */
