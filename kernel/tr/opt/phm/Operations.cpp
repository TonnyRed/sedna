#include "Operations.h"
#include "tr/opt/cost/Statistics.h"
#include "tr/structures/producer.h"

#include "tr/executor/xpath/XPathExecution.h"
#include "tr/executor/algorithms/SequenceModel.h"
#include "tr/executor/algorithms/Scans.h"
#include "tr/executor/algorithms/Joins.h"
#include "tr/executor/xpath/XPathLookup.h"
#include "tr/executor/base/ITupleSerializer.h"

#include "tr/executor/algorithms/Comparison.h"

#define OPINFO(OP) static const prot_info_t OP##_info = {#OP, };
#define OPREF(OP) (&(OP##_info))
#define CDGQNAME(N) xsd::QName::getConstantQName(NULL_XMLNS, N)

OPINFO(AbsPathScanPrototype)
OPINFO(PathEvaluationPrototype)
OPINFO(SortMergeJoinPrototype)
OPINFO(StructuralJoinPrototype)
OPINFO(ValueScanPrototype)
OPINFO(ValidatePathPrototype)

IElementProducer* elementValue(IElementProducer* element, const char * name, tuple_cell value)
{
    element = element->addElement(CDGQNAME(name));
    element->addText(text_source_tuple_cell(value));
    element->close();

    return element;
}

AbsPathScanPrototype::AbsPathScanPrototype(PhysicalModel* model, const TupleRef& tref)
  : POProt(OPREF(AbsPathScanPrototype)), dataRoot(), path()
{
    U_ASSERT(tref->node->type == DataNode::dnDatabase);

    dataRoot = tref->node->root;
    path = tref->node->path;

    result = model->updateOne(tref.tupleDesc, POProtIn(this, tref.tid));
    resultSet.push_back(tref.tid);

    evaluateCost(publicCostModel);
}

IElementProducer* AbsPathScanPrototype::__toXML(IElementProducer* element) const
{
    POProt::__toXML(element);

    IElementProducer * child;

    child = element->addElement(CDGQNAME("root"));
    child->addText(text_source_cstr(dataRoot.toLRString().c_str()));
    child->close();

    child = element->addElement(CDGQNAME("path"));
    child->addText(text_source_cstr(path.toXPathString().c_str()));
    child->close();

    return element;
}


PathEvaluationPrototype::PathEvaluationPrototype(PhysicalModel* model, const POProtIn& _left, const TupleRef& _right, const pe::Path& _path)
  : POProt(OPREF(PathEvaluationPrototype)), path(_path)
{
    in.push_back(_left);
    result = model->updateOne(_left.op->result, POProtIn(this, _right.tid));
    resultSet.push_back(_right.tid);

    evaluateCost(publicCostModel);
}

IElementProducer* PathEvaluationPrototype::__toXML(IElementProducer* element) const
{
    POProt::__toXML(element);

    IElementProducer * child = element->addElement(CDGQNAME("path"));
    child->addText(text_source_cstr(path.toXPathString().c_str()));
    child->close();

    return element;
}

SortMergeJoinPrototype::SortMergeJoinPrototype(PhysicalModel* model, const POProtIn& _left, const POProtIn& _right, const Comparison& _cmp)
  : BinaryOpPrototype(OPREF(SortMergeJoinPrototype), _left, _right), cmp(_cmp)
{
    result = model->updateTwo(_left.op->result, _right.op->result, this, _left.index, _right.index);
    resultSet.push_back(_left.index);
    resultSet.push_back(_right.index);

    evaluateCost(publicCostModel);
}

StructuralJoinPrototype::StructuralJoinPrototype(PhysicalModel* model, const POProtIn& _left, const POProtIn& _right, const pe::Path& _path)
  : BinaryOpPrototype(OPREF(StructuralJoinPrototype), _left, _right), path(_path)
{
    result = model->updateTwo(_left.op->result, _right.op->result, this, _left.index, _right.index);
    resultSet.push_back(_left.index);
    resultSet.push_back(_right.index);

    evaluateCost(publicCostModel);
}

IElementProducer* StructuralJoinPrototype::__toXML(IElementProducer* element) const
{
    POProt::__toXML(element);

    IElementProducer * child = element->addElement(CDGQNAME("path"));
    child->addText(text_source_cstr(path.toXPathString().c_str()));
    child->close();

    return element;
}


ValueScanPrototype::ValueScanPrototype(PhysicalModel* model, const POProtIn& _left, const TupleRef& _right, const Comparison& _cmp)
  : POProt(OPREF(ValueScanPrototype)), cmp(_cmp)
{
    in.push_back(_left);
  
    result = model->updateOne(_left.op->result, POProtIn(this, _left.index));
    resultSet.push_back(_left.index);

    value = _right->node->sequence;

    evaluateCost(publicCostModel);
}

ValidatePathPrototype::ValidatePathPrototype(PhysicalModel* model, const POProtIn& _tuple)
  : POProt(OPREF(ValidatePathPrototype))
{
    in.push_back(_tuple);
    
    result = model->updateOne(_tuple.op->result, POProtIn(this, _tuple.index));
    resultSet.push_back(_tuple.index);

    DataNode * dn = _tuple.op->result->get(_tuple.index)->node;

    if (dn != NULL) {
        path = dn->path;
        dataRoot = dn->root;
    }

    evaluateCost(publicCostModel);
}

IElementProducer* ValidatePathPrototype::__toXML(IElementProducer* element) const
{
    POProt::__toXML(element);

    IElementProducer * child;
    
    child = element->addElement(CDGQNAME("root"));
    child->addText(text_source_cstr(dataRoot.toLRString().c_str()));
    child->close();
    
    child = element->addElement(CDGQNAME("path"));
    child->addText(text_source_cstr(path.toXPathString().c_str()));
    child->close();

    return element;
}



/*

MagicJoinPrototype::MagicJoinPrototype(PhysicalModel* model, const POProtIn& _left, const POProtIn& _right, const pe::Path& _path)
  : BinaryOpPrototype(model, _left, _right), path(_path)
{
    U_ASSERT(_left.op == NULL || _right.op == NULL);
  
    result = model->updateTwo(_left.op->result, _right.op->result, this, _left.index, _right.index);
    resultSet.push_back(_left.index);
    resultSet.push_back(_right.index);

    evaluateCost(publicCostModel);
}

IElementProducer* MagicJoinPrototype::__toXML(IElementProducer* ) const
{
    POProt::__toXML(element);

    IElementProducer * child = element->addElement(CDGQNAME("path"));
    child->addText(text_source_cstr(path.toXPathString().c_str()));
    child->close();

    return element;
}
*/

struct XLogXOp { double operator() (double x) { return x*log(1+x); } };

void AbsPathScanPrototype::evaluateCost(CostModel* model)
{
    TupleStatistics * stats = new TupleStatistics();
    result->get(resultSet.at(0))->statistics = stats;
    PathCostModel * costInfo = model->getAbsPathCost(dataRoot, path, stats);

    result->rowCount = stats->distinctValues;
    result->rowSize = model->getNodeSize();

    cost = new OperationCost();

    cost->firstCost = costInfo->schemaTraverseCost;

    Range evalCost = costInfo->blockCount * model->getIOCost() + costInfo->card * model->getCPUCost();

    wantSort = true;
    Range heapSortCost = (wantSort ? (costInfo->card.map<XLogXOp>() * model->getCPUCost()) : Range(0.0));

    if (stats->distinctValues.upper == 0) {
        cost->nextCost = 0;
    } else if (stats->distinctValues.lower == 0) {
        cost->nextCost = (evalCost + heapSortCost) / stats->distinctValues.upper;
    } else {
        cost->nextCost = (evalCost + heapSortCost) / stats->distinctValues;
    }

    U_ASSERT(cost->nextCost.lower > 0);
    cost->fullCost = cost->firstCost + evalCost + heapSortCost;
}

void SortMergeJoinPrototype::evaluateCost(CostModel* model)
{
    TupleRef leftIn(in[0], NULL), rightIn(in[1], NULL);
    TupleRef leftResult(result, resultSet[0]), rightResult(result, resultSet[1]);

    if (leftIn->statistics == NULL || rightIn->statistics == NULL) {
        U_ASSERT(false);
        return;
    };

    cost = new OperationCost();

    model->getValueCost(leftIn->statistics->pathInfo, leftIn->statistics);
    model->getValueCost(rightIn->statistics->pathInfo, rightIn->statistics);

    leftResult->statistics = new TupleStatistics(leftIn->statistics);
    rightResult->statistics = new TupleStatistics(rightIn->statistics);

    SequenceInfo * leftSeq = model->getValueSequenceCost(leftIn);
    SequenceInfo * rightSeq = model->getValueSequenceCost(rightIn);

    ComparisonInfo * cmpInfo = model->getCmpInfo(leftIn->statistics, rightIn->statistics, cmp);

    leftResult->statistics->distinctValues *= std::max(1.0, cmpInfo->selectivity.avg());
    rightResult->statistics->distinctValues *= std::max(1.0, cmpInfo->selectivity.avg());

    result->rowCount = std::min(leftIn.tupleDesc->rowCount, rightIn.tupleDesc->rowCount) * cmpInfo->selectivity;

    cost->firstCost =
        in.at(0).op->getCost()->fullCost +
        in.at(1).op->getCost()->fullCost;

    // TODO if sorted, update sorted information
    cost->firstCost +=
        leftSeq->sortCost + rightSeq->sortCost;

    Range mergeCost =
        leftSeq->card * cmpInfo->opCost +
        rightSeq->card * cmpInfo->opCost +
        leftSeq->blockCount * model->getIOCost() +
        rightSeq->blockCount * model->getIOCost();

    cost->nextCost = mergeCost / result->rowCount;
    cost->fullCost = cost->firstCost + mergeCost;

    // Update sort information
}

void ValueScanPrototype::evaluateCost(CostModel* model)
{
    TupleRef inRef(in[0], NULL);
    TupleRef outRef(result, resultSet[0]);

    U_ASSERT(inRef->statistics);

    cost = new OperationCost();

    model->getValueCost(inRef->statistics->pathInfo, inRef->statistics);
    outRef->statistics = new TupleStatistics(inRef->statistics);

    ComparisonInfo * cmpInfo = model->getCmpInfo(inRef->statistics, model->getConstInfo(value.get()), cmp);

    outRef->statistics->distinctValues *= cmpInfo->selectivity;
    result->rowCount = outRef->statistics->distinctValues;

    OperationCost * inCost = in.at(0).op->getCost();

    cost->firstCost = inCost->firstCost;
    cost->fullCost  = inCost->fullCost + inRef.tupleDesc->rowCount * cmpInfo->opCost;
    cost->nextCost  = (cost->fullCost - cost->firstCost) / result->rowCount;
}

void PathEvaluationPrototype::evaluateCost(CostModel* model)
{
    TupleRef inRef(in[0], NULL);
    TupleRef outRef(result, resultSet[0]);
    const OperationCost * inCost = in.at(0).op->getCost();

    U_ASSERT(inRef->statistics);

    cost = new OperationCost();

    outRef->statistics = new TupleStatistics();
    model->getPathCost(inRef, path, outRef->statistics);

    result->rowCount = outRef->statistics->distinctValues;
    result->rowSize = inRef.tupleDesc->rowSize + model->getNodeSize();

    cost->firstCost = inCost->firstCost;
    cost->fullCost  = inCost->fullCost + inRef->statistics->distinctValues * outRef->statistics->pathInfo->iterationCost;
    cost->nextCost  = (cost->fullCost - cost->firstCost) / result->rowCount;
}

void StructuralJoinPrototype::evaluateCost(CostModel* model)
{
    TupleRef leftIn(in[0], NULL), rightIn(in[1], NULL);
    TupleRef leftResult(result, resultSet[0]), rightResult(result, resultSet[1]);

    if (leftIn->statistics == NULL || rightIn->statistics == NULL) {
        U_ASSERT(false);
        return;
    };

    cost = new OperationCost();

    leftResult->statistics = new TupleStatistics(leftIn->statistics);
    rightResult->statistics = new TupleStatistics(rightIn->statistics);

    SequenceInfo * leftSeq = model->getDocOrderSequenceCost(leftIn);
    SequenceInfo * rightSeq = model->getDocOrderSequenceCost(rightIn);

    ComparisonInfo * cmpInfo = model->getDocOrderInfo(leftIn->statistics->pathInfo, rightIn->statistics->pathInfo, path);

    leftResult->statistics->distinctValues *= std::max(1.0, cmpInfo->selectivity.avg());
    rightResult->statistics->distinctValues *= std::max(1.0, cmpInfo->selectivity.avg());

    result->rowCount = std::min(leftIn.tupleDesc->rowCount, rightIn.tupleDesc->rowCount) * cmpInfo->selectivity;

    // This operation may be implemented in two different ways with completely different costs
    
    Range bestFirstCost = in.at(0).op->getCost()->firstCost + in.at(1).op->getCost()->firstCost;
    Range worseFirstCost = in.at(0).op->getCost()->fullCost + in.at(1).op->getCost()->fullCost +
        leftSeq->sortCost + rightSeq->sortCost;

    Range bestMergeCost = cmpInfo->opCost * std::max(leftIn.tupleDesc->rowCount, rightIn.tupleDesc->rowCount);

    Range worseMergeCost =
        leftSeq->card * cmpInfo->opCost +
        rightSeq->card * cmpInfo->opCost +
        leftSeq->blockCount * model->getIOCost() +
        rightSeq->blockCount * model->getIOCost();

    Range mergeCost = Range(bestMergeCost.lower, worseMergeCost.upper).normalize();
        
    cost->firstCost = Range(bestFirstCost.lower, worseFirstCost.upper).normalize();
    cost->nextCost = mergeCost / result->rowCount;
    cost->fullCost = cost->firstCost + mergeCost;
}

void ValidatePathPrototype::evaluateCost(CostModel* model)
{
    POProt * opIn = in.at(0).op;

    cost = new OperationCost();
    *cost = *opIn->getCost();

    if (!path.getBody().isnull()) {
        cost->nextCost += model->getAbsPathCost(dataRoot, path, NULL)->iterationCost;
    }
}










using namespace phop;
using namespace pe;

struct ExecutionBlockWarden {
    ExecutionBlockWarden(POProt * opin)
    {
        phop::ExecutionBlock::current()->sourceStack.push(opin);
    };

    ~ExecutionBlockWarden()
    {
        phop::ExecutionBlock::current()->sourceStack.pop();
    };
};

phop::IOperator * AbsPathScanPrototype::compile()
{
    ExecutionBlockWarden(this);

    SchemaNodePtrSet schemaNodes;
    phop::TupleList inTuples;

    executeSchemaPathTest(dataRoot.getSchemaNode(), pe::AtomizedPath(path.getBody()->begin(), path.getBody()->end()), &schemaNodes, false);

    for (SchemaNodePtrSet::const_iterator it = schemaNodes.begin(); it != schemaNodes.end(); ++it) {
        inTuples.push_back(
            phop::MappedTupleIn(
                new phop::TupleFromItemOperator(
                    new phop::SchemaScan(schema_node_cptr(*it))), 0, 0));
    };

    ExecutionBlock::current()->resultMap[resultSet.at(0)] = 0;

    return new phop::DocOrderMerge(1, inTuples);
}

phop::IOperator * PathEvaluationPrototype::compile()
{
    ExecutionBlockWarden(this);
    // TODO : make effective evaluation

    ITupleOperator * opin = dynamic_cast<ITupleOperator *>(in.at(0).op->compile());
    TupleIn aopin(opin, ExecutionBlock::current()->resultMap[in.at(0).index]);

    U_ASSERT(opin != NULL);

    IValueOperator * ain = new phop::ReduceToItemOperator(aopin, true);

    pe::PathVectorPtr pathBody = path.getBody();
    pe::PathVector::const_iterator it = pathBody->begin();

    while (it != pathBody->end()) {
        pe::PathVector::const_iterator pstart = it;

        while (it != pathBody->end() && it->satisfies(pe::StepPredicate(pe::ParentAxisTest))) {
            ++it;
        };

        if (pstart != it) {
            ain = new pe::PathEvaluateTraverse(ain, pe::AtomizedPath(pstart, it));
        }

        pe::PathVector::const_iterator cstart = it;

        while (it != pathBody->end() &&
                it->satisfies(pe::StepPredicate(pe::ChildAttrAxisTest))) {
            ++it;
        };

        if (cstart != it) {
            ain = new pe::PathSchemaResolve(ain, pe::AtomizedPath(cstart, it));
        }

        if (pstart == it) {
            U_ASSERT(false);
            break;
        };
    };

    if (it != pathBody->end()) {
        ain = new pe::PathEvaluateTraverse(ain, pe::AtomizedPath(it, pathBody->end()));
    }

    ExecutionBlock::current()->resultMap[resultSet.at(0)] = aopin->_tsize();

    return new phop::NestedEvaluation(aopin, ain, aopin->_tsize() + 1, aopin->_tsize());
}

inline
TupleCellComparison tccFromCmp(const Comparison & cmp) {
    switch (cmp.op) {
      case Comparison::g_eq :
        return TupleCellComparison(op_lt, op_eq, true);
      default :
        U_ASSERT(false);
        return TupleCellComparison(NULL, NULL, false);
    };
};

phop::IOperator * SortMergeJoinPrototype::compile()
{
    ExecutionBlockWarden(this);

    POProtIn left(in[0]), right(in[1]);

    ITupleOperator * leftPtr = dynamic_cast<ITupleOperator *>(left.op->compile());
    unsigned leftIdx = ExecutionBlock::current()->resultMap[left.index];

    ITupleOperator * rightPtr = dynamic_cast<ITupleOperator *>(right.op->compile());
    unsigned rightIdx = ExecutionBlock::current()->resultMap[right.index];

    TupleIn leftOp(leftPtr, leftIdx);
    TupleIn rightOp(rightPtr, rightIdx);

    if (needLeftSort) {
        leftOp.op = new TupleSort(leftOp->_tsize(), MappedTupleIn(leftOp), new GeneralCollationSerializer(leftOp.offs));
    }

    if (needRightSort) {
        rightOp.op = new TupleSort(rightOp->_tsize(), MappedTupleIn(rightOp), new GeneralCollationSerializer(rightOp.offs));
    }

    TupleChrysalis * rightScheme = right.op->result;

    for (unsigned i = 0; i < rightScheme->width(); ++i) {
        if (rightScheme->tuples[i].status == TupleValueInfo::evaluated) {
            ExecutionBlock::current()->resultMap[i] += leftOp->_tsize();
        };
    };
    
    return new TupleJoinFilter(
        leftOp->_tsize() + rightOp->_tsize(),
        MappedTupleIn(leftOp),
        MappedTupleIn(rightOp.op, rightOp.offs, leftOp->_tsize()),
        tccFromCmp(cmp));
}

phop::IOperator * StructuralJoinPrototype::compile()
{
    ExecutionBlockWarden(this);

    POProtIn left(in[0]), right(in[1]);
    
    ITupleOperator * leftPtr = dynamic_cast<ITupleOperator *>(left.op->compile());
    unsigned leftIdx = ExecutionBlock::current()->resultMap[left.index];
    
    ITupleOperator * rightPtr = dynamic_cast<ITupleOperator *>(right.op->compile());
    unsigned rightIdx = ExecutionBlock::current()->resultMap[right.index];

    TupleIn leftOp(leftPtr, leftIdx);
    TupleIn rightOp(rightPtr, rightIdx);

    if (needLeftSort) {
        leftOp.op = new TupleSort(leftOp->_tsize(), MappedTupleIn(leftOp), new DocOrderSerializer(leftOp.offs));
    }

    if (needRightSort) {
        rightOp.op = new TupleSort(rightOp->_tsize(), MappedTupleIn(rightOp), new DocOrderSerializer(rightOp.offs));
    }

    pe::Step step = path.getBody()->at(0);

    TupleCellComparison tcc(op_doc_order_lt, op_doc_order_lt, false);

    // TODO : implement all possible axes

    switch (step.getAxis()) {
      case pe::axis_preceding :
        tcc = TupleCellComparison(op_doc_order_lt, op_doc_order_lt, false);
        break;
      case pe::axis_following :
        tcc = TupleCellComparison(op_doc_order_lt, op_doc_order_gt, false);
        break;
      case pe::axis_child :
      case pe::axis_descendant :
      case pe::axis_attribute :
        tcc = TupleCellComparison(op_doc_order_lt, op_doc_order_descendant, false);
        break;
      case pe::axis_ancestor :
      case pe::axis_parent :
        tcc = TupleCellComparison(op_doc_order_lt, op_doc_order_ancestor, false);
        break;
      default:
        U_ASSERT(false);
        break;
    }

    TupleChrysalis * rightScheme = right.op->result;
    
    for (unsigned i = 0; i < rightScheme->width(); ++i) {
        if (rightScheme->tuples[i].status == TupleValueInfo::evaluated) {
            ExecutionBlock::current()->resultMap[i] += leftOp->_tsize();
        };
    };
    
    return new TupleJoinFilter(
        leftOp->_tsize() + rightOp->_tsize(),
        MappedTupleIn(leftOp),
        MappedTupleIn(rightOp.op, rightOp.offs, leftOp->_tsize()),
        tcc);
}

phop::IOperator * ValueScanPrototype::compile()
{
    ExecutionBlockWarden(this);

    if (in.at(0).op->getProtInfo() == OPREF(AbsPathScanPrototype)) {
        AbsPathScanPrototype * pathScan = dynamic_cast<AbsPathScanPrototype *>(in.at(0).op);

        SchemaNodePtrSet schemaNodes;
        phop::TupleList inTuples;
        pe::AtomizedPath path = pe::AtomizedPath(pathScan->getPath().getBody()->begin(), pathScan->getPath().getBody()->end());

        executeSchemaPathTest(pathScan->getRoot().getSchemaNode(), path, &schemaNodes, false);

        for (SchemaNodePtrSet::const_iterator it = schemaNodes.begin(); it != schemaNodes.end(); ++it) {
            inTuples.push_back(
                MappedTupleIn(
                    new phop::SchemaValueScan(schema_node_cptr(*it),
                        tccFromCmp(cmp), value, 1, 0, 1), 0, 0));
        };

        ExecutionBlock::current()->resultMap[pathScan->resultSet.at(0)] = 0;
        
        return new phop::DocOrderMerge(1, inTuples);
    } else {
        ITupleOperator * leftOpPtr = dynamic_cast<ITupleOperator *>(in.at(0).op->compile());
        
        // WARNING: Result is mapped after compile()
        unsigned leftIdx = ExecutionBlock::current()->resultMap[in.at(0).index] = 0;
        MappedTupleIn leftOp(leftOpPtr, leftIdx, 0);

        return new CachedNestedLoop(leftOp->_tsize(), leftOp,
            MappedTupleIn(
                new TupleFromItemOperator(
                    new BogusConstSequence(value)), 0, TupleMap()),
            tccFromCmp(cmp),
            CachedNestedLoop::strict_output);
    }
}

phop::IOperator * ValidatePathPrototype::compile()
{
    ExecutionBlockWarden(this);

    return in.at(0).op->compile();
}