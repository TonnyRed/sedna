#ifndef _ELEMENTARY_OPERATIONS_H_
#define _ELEMENTARY_OPERATIONS_H_

#include "IndependentPlan.h"
#include "tr/opt/graphs/DataGraphs.h"

namespace rqp {

struct EmptySequenceConst { explicit EmptySequenceConst(int x = 0) {}; };

class Const : public ConstantOperation {
    RTTI_DECL(plan_operation_Const, ConstantOperation)
protected:
    virtual XmlConstructor& __toXML ( XmlConstructor& constructor ) const;
private:
    opt::MemoryTupleSequencePtr sequence;
public:
    explicit Const(opt::MemoryTupleSequencePtr _sequence)
       : ConstantOperation(SELF_RTTI_REF), sequence(_sequence)
    {};

    explicit Const(tuple_cell _value)
      : ConstantOperation(SELF_RTTI_REF), sequence(new opt::MemoryTupleSequence())
    { sequence->push_back(_value); };

    explicit Const(const EmptySequenceConst &)
      : ConstantOperation(SELF_RTTI_REF), sequence(new opt::MemoryTupleSequence())
    { };

    virtual void evaluateTo(executor::DynamicContext* dynamicContext);

    PROPERTY_RO(Sequence, opt::MemoryTupleSequencePtr, sequence)
};

class VarIn : public ConstantOperation {
    RTTI_DECL(plan_operation_VarIn, ConstantOperation)
protected:
    virtual XmlConstructor& __toXML ( XmlConstructor& constructor ) const;
public:
    opt::TupleId variable;

    explicit VarIn(opt::TupleId _tid)
      : ConstantOperation(SELF_RTTI_REF), variable(_tid)
    {
        context->varGraph.addVariableUsage(variable, this, NULL);
        dependantVariables.insert(_tid);
    };

    virtual void evaluateTo(executor::DynamicContext* dynamicContext);
    
    PROPERTY_RO(Tuple, opt::TupleId, variable)
};

/**
 * @brief Special logical operation for Exists function
 */
class Exists : public ListOperation {
    RTTI_DECL(plan_operation_Exists, ListOperation)
public:
    explicit Exists(RPBase * list)
      : ListOperation(SELF_RTTI_REF, list) {};
};

/**
 * @brief Special logical operation for EBV function
 */
class EffectiveBooleanValue : public ListOperation {
    RTTI_DECL(plan_operation_EffectiveBooleanValue, ListOperation)
public:
    explicit EffectiveBooleanValue(RPBase * list)
      : ListOperation(SELF_RTTI_REF, list) {};
};

class Sequence : public ManyChildren {
    RTTI_DECL(plan_operation_Sequence, ManyChildren)
protected:
    virtual XmlConstructor& __toXML ( XmlConstructor& constructor ) const;
public:
    enum space_t {
        none,
        atomic_spaces,
        all_spaces
    };

private:
    space_t spaces;
public:
    explicit Sequence(const OperationList & _oplist)
      : ManyChildren(SELF_RTTI_REF, _oplist), spaces(none) { };

    explicit Sequence(RPBase* _in)
      : ManyChildren(SELF_RTTI_REF, _in), spaces(none) { };

    PROPERTY(Spaces, space_t, spaces)
};

}

#endif /* _ELEMENTARY_OPERATIONS_H_ */