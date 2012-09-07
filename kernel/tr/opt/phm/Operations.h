#ifndef _OPERATIONS_H
#define _OPERATIONS_H

#include "tr/opt/OptTypes.h"
#include "tr/opt/phm/PhysicalModel.h"
#include "tr/opt/graphs/Predicates.h"
#include "tr/opt/evaluation/VariableMap.h"

namespace opt {

class BinaryOpPrototype : public POProt {
    RTTI_DECL(physical_model_BinaryOpPrototype, POProt)
public:
    BinaryOpPrototype(clsinfo_t pinfo, const POProtIn & _left, const POProtIn & _right)
      : POProt(pinfo) { in.push_back(_left); in.push_back(_right); };
};

class MergeJoinPrototype : public BinaryOpPrototype {
    RTTI_DECL(physical_model_MergeJoinPrototype, BinaryOpPrototype)
protected:
    ComparisonPrototype * comparison;

    bool pathComparison;
    bool equalityComparison;

    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
    virtual phop::IOperator * compile();
public:
    MergeJoinPrototype(PhysicalModel * model, const POProtIn & _left, const POProtIn & _right, ComparisonPrototype * _comparison);

    virtual void evaluateCost(CostModel* model);
};

class FilterTuplePrototype : public BinaryOpPrototype {
    RTTI_DECL(physical_model_FilterTuplePrototype, BinaryOpPrototype)
protected:
    ComparisonPrototype * comparison;

    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
    virtual phop::IOperator * compile();
public:
    FilterTuplePrototype(PhysicalModel * model, const POProtIn & _left, const POProtIn & _right, ComparisonPrototype * _comparison);

    virtual void evaluateCost(CostModel* model);
};

class AbsPathScanPrototype : public POProt {
    RTTI_DECL(physical_model_AbsPathScanPrototype, POProt)

    DataRoot dataRoot;
    pe::Path path;
protected:
    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
    virtual phop::IOperator * compile();
public:
    bool wantSort;

    const DataRoot & getRoot() const { return dataRoot; };
    const pe::Path & getPath() const { return path; };
    
    AbsPathScanPrototype(PhysicalModel * model, const TupleRef & tref);

    virtual void evaluateCost(CostModel* model);
};

class PathEvaluationPrototype : public POProt {
    RTTI_DECL(physical_model_PathEvaluationPrototype, POProt)
    
    pe::Path path;
protected:
    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
    virtual phop::IOperator * compile();
public:
    PathEvaluationPrototype(PhysicalModel * model, const POProtIn & _left, const TupleRef & _right, const pe::Path& _path);

    virtual void evaluateCost(CostModel* model);
};

class ValueScanPrototype : public POProt {
    RTTI_DECL(physical_model_ValueScanPrototype, POProt)

    const Comparison cmp;
    MemoryTupleSequencePtr value;
protected:
    virtual phop::IOperator * compile();
public:
    ValueScanPrototype(PhysicalModel * model, const POProtIn & _left, const TupleRef & _right, const Comparison& _cmp);

    virtual void evaluateCost(CostModel* model);
};

class EvaluatePrototype : public POProt {
    RTTI_DECL(physical_model_EvaluatePrototype, POProt)

    phop::IFunction * func;
protected:
    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
    virtual phop::IOperator * compile();
public:
    EvaluatePrototype(PhysicalModel * model, const POProtIn & _left, const TupleRef & _right, phop::IFunction * _func);

    virtual void evaluateCost(CostModel* model);
};

class ExternalVarPrototype : public POProt {
    RTTI_DECL(physical_model_ExternalVarPrototype, POProt)

    TupleId varTupleId;
protected:
    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
public:
    ExternalVarPrototype(opt::PhysicalModel* model, const opt::TupleRef& tref);

    virtual void evaluateCost(CostModel* model);
    virtual phop::IOperator * compile();
};

class ValidatePathPrototype : public POProt {
    RTTI_DECL(physical_model_ValidatePathPrototype, POProt)

    DataRoot dataRoot;
    pe::Path path;
protected:
    virtual XmlConstructor & __toXML(XmlConstructor & ) const;
public:
    ValidatePathPrototype(PhysicalModel * model, const POProtIn & _tuple);

    virtual void evaluateCost(CostModel* model);
    virtual phop::IOperator * compile();
};

};


#endif /* _OPERATIONS_H */