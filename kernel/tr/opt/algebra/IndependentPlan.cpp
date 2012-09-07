#include "IndependentPlan.h"

#include "tr/models/XmlConstructor.h"
#include "tr/opt/functions/Functions.h"
#include "tr/opt/graphs/DataGraphs.h"

#include <set>

using namespace rqp;
using namespace opt;

RTTI_DEF_BASE(RPBase)

RTTI_DEF(ConstantOperation)
RTTI_DEF(ListOperation)
RTTI_DEF(ManyChildren)

int RPBase::opids = 0;
const opt::TupleScheme empty_tuple_set;

void RPBase::replace(RPBase* op, RPBase* with)
{
    for (OperationList::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it == op) {
            *it = with;
            return;
        }
    };

    U_ASSERT(false);
}

XmlConstructor& RPBase::toXML(XmlConstructor& element) const
{
    element.openElement(SE_EL_NAME(info()->name));
    element.addAttributeValue(SE_EL_NAME("id"), tuple_cell::atomic_int(opuid));
    __toXML(element);
/*
    for (TupleScheme::const_iterator it = dependantVariables.begin(); it != dependantVariables.end(); ++it) {
        element.openElement(SE_EL_NAME("uses"));
        element.addAttributeValue(SE_EL_NAME("varid"), tuple_cell::atomic_int(*it));
        element.closeElement();
    };
*/
    element.closeElement();

    return element;
}

XmlConstructor& ConstantOperation::__toXML(XmlConstructor& element) const
{
    return element;
}

XmlConstructor& ListOperation::__toXML(XmlConstructor& element) const
{
    if (getList() != null_obj) {
        getList()->toXML(element);
    }

    return element;
}

XmlConstructor& ManyChildren::__toXML(XmlConstructor& element) const
{
    for (OperationList::const_iterator it = children.begin(); it != children.end(); ++it) {
        if (*it != null_obj) {
            (*it)->toXML(element);
        }
    };

    return element;
}

PlanContext::PlanContext()
  : lastScopeMarker(0), currentTupleId(1)
{
}


PlanContext::~PlanContext()
{
}

void PlanContext::newScope() {
    scopeStack.push(invalidTupleId);
    ++lastScopeMarker;
}

void PlanContext::clearScope()
{
    while (scopeStack.top() != invalidTupleId) {
        scope.erase(varGraph.getVariable(scopeStack.top()).name);
        scopeStack.pop();
    }
    --lastScopeMarker;
}

ScopeMarker PlanContext::setScopeMarker()
{
    return lastScopeMarker;
}

void PlanContext::clearScopesToMarker(ScopeMarker marker)
{
    while (lastScopeMarker > marker) {
        clearScope();
    }
}

TupleId PlanContext::generateTupleIdVarScoped(const std::string & varName)
{
    TupleId tid = generateTupleId();

    varGraph.addVariableName(tid, varName);
    scope.insert(VarNameMap::value_type(varName, tid));
    scopeStack.push(tid);
    
    return tid;
}

TupleId PlanContext::getVarTupleInScope(const std::string& canonicalName)
{
    return scope.at(canonicalName);
}


/*
TupleScheme * PlanContext::newMapExtend(TupleScheme * in, const TupleScheme * with) {
    TupleScheme * ts;
    
    if (in != NULL) {
        ts = new TupleScheme(*in);
    } else {
        ts = new TupleScheme();
    }
    
    if (with != NULL) {
        ts->insert(with->begin(), with->end());
    }
    
    tupleSchemeStorage.push_back(ts);
    return ts;
}

TupleScheme * PlanContext::newMapExtendSingle(TupleScheme * in, TupleId with) {
    TupleScheme * ts;
    
    if (in != NULL) {
        ts = new TupleScheme(*in);
    } else {
        ts = new TupleScheme();
    }
    
    ts->insert(with);
    tupleSchemeStorage.push_back(ts);
    return ts;
}
*/

