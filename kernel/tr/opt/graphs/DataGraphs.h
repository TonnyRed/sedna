#ifndef _DATA_GRAPHS_H_
#define _DATA_GRAPHS_H_

#include "tr/opt/OptTypes.h"
#include "tr/opt/path/DataSources.h"
#include "tr/models/XmlConstructor.h"

namespace opt {

struct Predicate : public IPlanDisposable {
    int index;
    PlanDesc indexBit;

    rqp::RPBase * opInfo;

    PredicateList neighbours;
    PredicateList evaluateAfter;
    PredicateList implies;
    DataNodeList dataNodeList;

//    bool createContext;
//    TupleId contextTuple;

    void setIndex(int _index) {
        index = _index;
        indexBit = 1ULL << _index;
    };

    Predicate() : index(0), indexBit(0) {};

    virtual void * compile(PhysicalModel * model) = 0;

//    virtual std::string toLRString() const = 0;
    virtual XmlConstructor & toXML(XmlConstructor & ) const = 0;
};

struct DataNode : public IPlanDisposable, public IXMLSerializable
{
    DataGraph * parent;

    enum data_node_type_t {
        dnConst = 1, dnExternal, dnDatabase, dnFreeNode, dnAlias, dnReplaced
    } type;

    DataNode * replacedWith; // If node is replaced, tells, what is it replaced with

    int index; // Index in graph 
    PlanDesc indexBit; // Shifted index in graph
    int absoluteIndex; // Node index used while building execution schema

    // Data root information and path information
    DataRoot root;
    pe::Path path;

    MemoryTupleSequencePtr constValue; // Value of constant node
    DataNode * aliasFor;

    TupleId varTupleId; // Variable node came from

    tuple_info_t properties;

    explicit DataNode(data_node_type_t _type, TupleId _varTupleId = opt::invalidTupleId)
        : parent(NULL), type(_type),
          replacedWith(NULL), index(0), indexBit(0),
          absoluteIndex(0), aliasFor(NULL),
          varTupleId(_varTupleId)
    { };

    void setIndex(int _index) {
        index = _index;
        indexBit = 1ULL << _index;
    };

    virtual XmlConstructor & toXML(XmlConstructor & ) const;
};

struct PredicateIndex {
    Predicate * p;
    PlanDesc neighbours;
    PlanDesc dataNodeMask;
    PlanDesc evaluateAfter;
    PlanDesc implies;
};

struct DataNodeIndex {
    DataNode * p;
    PlanDesc predicates;
    bool input;
    bool output;
};

class VariableUsageGraph;

struct DataGraph : public IPlanDisposable, public IXMLSerializable {
    VariableUsageGraph * owner;
    rqp::RPBase * operation;

    Predicate * predicates[MAX_GRAPH_SIZE];
    DataNode * dataNodes[MAX_GRAPH_SIZE];

    PlanDesc inputNodes;
    PlanDesc outputNodes;

    explicit DataGraph(VariableUsageGraph* _owner);
    virtual XmlConstructor & toXML(XmlConstructor & constructor) const;
};

struct DataGraphIndex {
    DataGraph * dg;

    DataNodeList nodes;
    DataNodeList in;
    DataNodeList out;

    void tuplesInOut(TupleScheme * in, TupleScheme * out);

    PredicateList predicates;

    PredicateIndex predicateIndex[MAX_GRAPH_SIZE];
    DataNodeIndex nodeIndex[MAX_GRAPH_SIZE];

    PlanDesc predicateMask;

    explicit DataGraphIndex(DataGraph * _dg);

    PlanDesc getNeighbours(PlanDesc x) {
        PlanDesc result = 0;
        PlanDescIterator iter(x);

        int i;
        while (-1 != (i = iter.next())) {
            result |= predicateIndex[i].neighbours;
        }

        return (result & ~x);
    };

    void update();
    void rebuild();

    void addOutNode(DataNode * node) { nodes.push_back(node); out.push_back(node); };

    inline 
    DataNode * getNode(TupleId tid) {
        for (DataNodeList::const_iterator it = out.begin(); it != out.end(); ++it) {
            if ((*it)->varTupleId == tid) {
                return (*it);
            };
        };

        return NULL;
    };
private:
    DataGraph * make(VariableUsageGraph * master, DataGraph * graph);
};

};

#define FOR_ALL_GRAPH_ELEMENTS(EL, IV) for (unsigned IV = 0; IV < MAX_GRAPH_SIZE; ++IV) if ((EL)[IV] != NULL)
#define FOR_ALL_INDEXES(EL, IV) for (unsigned IV = 0; IV < MAX_GRAPH_SIZE; ++IV) if ((EL)[IV].p != NULL)

#endif /* _DATA_GRAPHS_H_ */