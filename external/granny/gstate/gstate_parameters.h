// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_parameters.h $
// $DateTime: 2011/12/06 12:28:06 $
// $Change: 35917 $
// $Revision: #1 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_PARAMETERS_H)

#ifndef GSTATE_NODE_H
#include "gstate_node.h"
#endif

BEGIN_GSTATE_NAMESPACE;

class parameters : public node
{
    typedef node parent;

public:
    virtual void AcceptNodeVisitor(node_visitor*);

    void  SetParameter(granny_int32x OutputIdx, granny_real32 NewVal);
    void  GetMinMax(granny_int32x OutputIdx, float* MinVal, float* MaxVal);
    void  SetMinMax(granny_int32x OutputIdx, float MinVal,  float MaxVal);

    virtual granny_int32x AddOutput(node_edge_type EdgeType, char const* EdgeName);
    virtual bool          DeleteOutput(granny_int32x OutputIndex);

    virtual granny_real32      SampleScalarOutput(granny_int32x OutputIdx,
                                                  granny_real32 AtT);

    // We track this to let any owning state machines know that we've changed a parameter
    // name...
    virtual bool SetOutputName(granny_int32x OutputIdx, char const* NewEdgeName);

    virtual void SetParameterDefault(granny_int32x OutputIdx, granny_real32 NewVal);

protected:
    DECL_CONCRETE_CLASS_TOKEN(parameters);
    DECL_SNAPPABLE();
    IMPL_CASTABLE_INTERFACE(parameters);

private:
    float* m_Parameters;
};

END_GSTATE_NAMESPACE;


#define GSTATE_PARAMETERS_H
#endif /* GSTATE_PARAMETERS_H */
