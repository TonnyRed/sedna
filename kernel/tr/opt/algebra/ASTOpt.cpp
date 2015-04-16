#include "ASTOpt.h"

#include "tr/opt/algebra/AllOperations.h"
#include "tr/opt/functions/AllFunctions.h"
#include "tr/opt/functions/FnAxisStep.h"
#include "tr/executor/base/XPath.h"

#include "tr/opt/types/StaticContext.h"

using namespace sedna;
using namespace rqp;

RPBase* lr2opt::getPlan() const
{
    return context->resultStack.top().op;
}

lr2opt::lr2opt(XQueryDriver* drv_, XQueryModule* mod_, dynamic_context* dyn_cxt_, bool is_subquery_)
    : lr2por(drv_, mod_, dyn_cxt_, is_subquery_), context(NULL)
{
    context = new StaticContext;
    context->context = invalidContext;
    // TODO : remove this hack
    context->collation = static_context::collation_manager.get_default_collation_handler();
    planContext = optimizer->planContext();
}

lr2opt::~lr2opt()
{
    delete context;
}

void lr2opt::visit(ASTLoadFile& n)
{
    context->resultStack.push(ResultInfo(NULL));
}

/*
 * NOTE: Context item can be ONLY overwritten
 * during evaluation of path expressions and predicates!!!
 */

void lr2opt::visit(ASTMainModule &n)
{
//    n.prolog->accept(*this);
    // TODO : remove

    n.query->accept(*this);
}

void lr2opt::visit(ASTQuery &n)
{
    if (n.type == ASTQuery::QUERY) {
        n.query->accept(*this);
    } else {
        context->resultStack.push(ResultInfo(NULL));
    }
}

void lr2opt::visit(ASTAxisStep &n)
{
    context->stepStack.push(StepInfo());
    StepInfo & info = context->stepStack.top();

    info.tqname = xsd::QNameAny;

    switch (n.axis) {
      case ASTAxisStep::CHILD :
        info.axis = pe::axis_child;
        break;
      case ASTAxisStep::DESCENDANT :
        info.axis = pe::axis_descendant;
        break;
      case ASTAxisStep::DESCENDANT_OR_SELF :
        info.axis = pe::axis_descendant_or_self;
        break;
      case ASTAxisStep::ATTRIBUTE :
        info.axis = pe::axis_attribute;
        break;
      default:
        throw USER_EXCEPTION(2902);
    }

    RPBase * parentOp = null_op;

    if (n.cont != NULL) {
        n.cont->accept(*this);
        parentOp = context->popResult();
    } else {
        parentOp = context->getContextVariableOp();
    };

    if (n.test == NULL) {
        throw USER_EXCEPTION(2902);
    } else {
        n.test->accept(*this);
    }

    opt::TupleId param = planContext->generateTupleId();

    RPBase * resultOp = new FunCallParams(
        axisStepFunction,
        new AxisStepData(pe::Step(info.axis, info.nodeTest, info.tqname)),
        fParams(param));

    context->stepStack.pop();

    if (n.preds != NULL) {
        ContextInfo saveContextVariable = context->context;

        for (ASTNodesVector::iterator pred = n.preds->begin(); pred != n.preds->end(); pred++) {
            context->generateContext();
            (*pred)->accept(*this);

            resultOp =
              new MapConcat(
                new If(
                  /* Condition */ context->popResult(),
                  /* If holds */ new VarIn(context->context.item),
                  null_op),
                resultOp,
                context->context.item);
        }

        context->context = saveContextVariable;
    }

    U_ASSERT(parentOp != null_op);

    resultOp = new MapConcat(resultOp, parentOp, param);

    context->resultStack.push(ResultInfo(resultOp));
}

void lr2opt::visit(ASTFilterStep &n) {
    RPBase * resultOp = null_op;

    if (n.cont != NULL) {
        n.cont->accept(*this);
        resultOp = context->popResult();
    } else {
        resultOp = context->getContextVariableOp();
    };

    if (n.expr != NULL) {
        ContextInfo saveContextVariable = context->context;
        context->generateContext();
        n.expr->accept(*this);
        resultOp = new MapConcat(context->popResult(), resultOp, context->context.item);
        context->context = saveContextVariable;
    }

    if (n.preds != NULL) {
        ContextInfo saveContextVariable = context->context;

        for (ASTNodesVector::iterator pred = n.preds->begin(); pred != n.preds->end(); pred++) {
            context->generateContext();
            (*pred)->accept(*this);

            resultOp =
              new MapConcat(
                new If(
                  /* Condition */ context->popResult(),
                  /* If holds */ new VarIn(context->context.item),
                  null_op),
                resultOp,
                context->context.item);
        }

        context->context = saveContextVariable;
    }

    context->resultStack.push(ResultInfo(resultOp));
}

void lr2opt::visit(ASTPred &n) {
    U_ASSERT(n.conjuncts.size() == 0);

    RPBase * resultOp = null_op;

    for (ASTPred::ASTConjuncts::iterator conj = n.others.begin(); conj != n.others.end(); conj++) {
        conj->expr->accept(*this);
        resultOp = context->popResult();
//        resultOp = new And(resultOp, context->popResult());
    }

    context->resultStack.push(ResultInfo(resultOp));
}

void lr2opt::visit(ASTDocTest &n) {
//    n.elem_test->accept();
    context->stepStack.top().nodeTest = pe::nt_document;
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTAttribTest &n) {
    U_ASSERT(dynamic_cast<ASTAxisStep*>(*(vis_path.rbegin()+1)) != NULL);

    context->stepStack.top().nodeTest = pe::nt_attribute;
    n.name->accept(*this);
    // TODO! Type assert
}

void lr2opt::visit(ASTElementTest &n) {
    U_ASSERT(dynamic_cast<ASTAxisStep*>(*(vis_path.rbegin()+1)) != NULL);

    context->stepStack.top().nodeTest = pe::nt_element;
    
    n.name->accept(*this);
    // TODO! Type assert
}

void lr2opt::visit(ASTEmptyTest &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTTextTest &n) {
    U_ASSERT(dynamic_cast<ASTAxisStep*>(*(vis_path.rbegin()+1)) != NULL);
    context->stepStack.top().nodeTest = pe::nt_text;
}

void lr2opt::visit(ASTCommTest &n) {
    U_ASSERT(dynamic_cast<ASTAxisStep*>(*(vis_path.rbegin()+1)) != NULL);
    context->stepStack.top().nodeTest = pe::nt_comment;
}

void lr2opt::visit(ASTPiTest &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTSchemaAttrTest &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTSchemaElemTest &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTNameTest &n) {
    U_ASSERT(
       dynamic_cast<ASTAttribTest*>(*(vis_path.rbegin()+1)) != NULL ||
       dynamic_cast<ASTElementTest*>(*(vis_path.rbegin()+1)) != NULL
            );
  
    xmlns_ptr ns = NULL_XMLNS;
    const char * local = xsd::QNameWildcard;

    if (n.pref != NULL) {
        ns = skn->resolvePrefix(n.pref->c_str());
    };

    if (n.local != NULL) {
        local = n.local->c_str();
    };

    context->stepStack.top().tqname = xsd::TemplateQName(ns == NULL_XMLNS ? NULL : ns->get_uri(), local);
}

void lr2opt::visit(ASTNodeTest &n) {
    context->stepStack.top().nodeTest = pe::nt_any_kind;
}

/* XQuery language - Constructors */

void lr2opt::visit(ASTDocConst &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTAttr &n){
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTAttrConst &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTElem &n)
{
    xmlns_ptr ns = NULL_XMLNS;
    unsigned count = 0;

    U_ASSERT(n.local != NULL);

    int sknMark = skn->mark();

    if (n.attrs)
    {
        ASTVisitor::VisitNodesVector(n.attrs, *this);
        count += n.attrs->size();
    }

    if (n.cont)
    {
        ASTVisitor::VisitNodesVector(n.cont, *this);
        count += n.cont->size();
    }

    OperationList oplist;
    oplist.reserve(count);

    while (count--) {
        oplist.push_back(context->resultStack.top().op);
        context->resultStack.pop();
    }

    std::reverse(oplist.begin(), oplist.end());

    xsd::QName qname = xsd::QName::resolve(
        n.pref != NULL ? n.pref->c_str() : NULL,
        n.local->c_str(), skn);
    
    context->resultStack.push(ResultInfo(
        new Construct(element,
            new Const(tuple_cell::atomic(qname)),
            new Sequence(oplist)
        )));

    skn->rollbackToMark(sknMark);
}

void lr2opt::visit(ASTElemConst &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTXMLComm &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTCommentConst &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTPi &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTPIConst &n) {
    throw USER_EXCEPTION(2902);
}


void lr2opt::visit(ASTTextConst &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTCharCont &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTSpaceSeq &n) {
    n.expr->accept(*this);

    Sequence * op = new Sequence(context->resultStack.top().op);
    context->resultStack.top().op = op;

    if (n.atomize) {
        op->setSpaces(Sequence::all_spaces);
    } else {
        op->setSpaces(Sequence::atomic_spaces);
    };
}

void lr2opt::visit(ASTNsp &n) {
    throw USER_EXCEPTION(2902);
}


/* XQuery language - Functions and operators */

void lr2opt::visit(ASTFunCall &n) {
    OperationList oplist;
    ParamList params;

    for (ASTNodesVector::iterator it = n.params->begin(); it != n.params->end(); ++it) {
        (*it)->accept(*this);

        opt::TupleId param = planContext->generateTupleId();
        oplist.push_back(context->resultStack.top().op);
        params.push_back(planContext->generateTupleId());
        context->resultStack.pop();
    };

    xmlns_ptr ns;

    if (!n.pref->empty()) {
          ns = skn->resolvePrefix(n.pref->c_str());
    } else {
          ns = skn->resolvePrefix("fn");
    };

    phop::FunctionInfo * f = getFunctionLibrary()->findFunction(
          xsd::constQName(ns, n.local->c_str()), params.size());

    if (f == NULL) {
        throw XQUERY_EXCEPTION2(0017, "Function not found");
    };

    RPBase * op = new FunCallParams(f, NULL, params);

    ParamList::const_iterator jt = params.begin();
    for (OperationList::const_iterator it = oplist.begin(); it != oplist.end(); ++it) {
        op = new SequenceConcat(op, *it, *jt);
        ++jt;
    }

    context->resultStack.push(ResultInfo(op));
}

void lr2opt::visit(ASTUop &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTBop &n) {
    n.lop->accept(*this);
    RPBase * leftSequence = context->resultStack.top().op;
    context->resultStack.pop();

    n.rop->accept(*this);
    RPBase * rightSequence = context->resultStack.top().op;
    context->resultStack.pop();

    opt::TupleId leftSequenceId = planContext->generateTupleId();
    opt::TupleId rightSequenceId = planContext->generateTupleId();

    if (n.op >= ASTBop::IS && n.op <= ASTBop::GE_G) {
        opt::Comparison cmp(
          opt::Comparison::invalid,
          context->collation);

        switch (n.op) {
          case ASTBop::PREC :
            cmp.op = opt::Comparison::do_before;
            break;
          case ASTBop::FOLLOW :
            cmp.op = opt::Comparison::do_after;
            break;
          case ASTBop::GT_G :
            cmp.op = opt::Comparison::g_gt;
            break;
          case ASTBop::EQ_G :
            cmp.op = opt::Comparison::g_eq;
            break;
/*
          case ASTBop::NE_G :
            cmp.op = opt::Comparison::g_ne;
            break;
          case ASTBop::LT_G :
            cmp.op = opt::Comparison::g_lt;
            break;
          case ASTBop::LE_G :
            cmp.op = opt::Comparison::g_le;
            break;
          case ASTBop::GE_G :
            cmp.op = opt::Comparison::g_ge;
            break;
*/
          default:
           throw USER_EXCEPTION(2902);
        };

        context->resultStack.push(ResultInfo(
            new SequenceConcat(
              new SequenceConcat(
                new FunCallParams(generalComparisonFunction,
                    new ComparisonData(cmp),
                    fParams(leftSequenceId, rightSequenceId)),
                leftSequence, leftSequenceId),
            rightSequence, rightSequenceId)));
    } else if (n.op >= ASTBop::OR && n.op <= ASTBop::MOD) {
        BinaryTupleCellOpData * fnData;

        switch (n.op) {
          case ASTBop::PLUS :
            fnData = new BinaryTupleCellOpData(xqbop_add);
            break;
          case ASTBop::MINUS :
            fnData = new BinaryTupleCellOpData(xqbop_sub);
            break;
          case ASTBop::MULT :
            fnData = new BinaryTupleCellOpData(xqbop_mul);
            break;
          case ASTBop::DIV :
            fnData = new BinaryTupleCellOpData(xqbop_div);
            break;
          case ASTBop::IDIV :
            fnData = new BinaryTupleCellOpData(xqbop_idiv);
            break;
          case ASTBop::MOD :
            fnData = new BinaryTupleCellOpData(xqbop_mod);
            break;
          default:
            U_ASSERT(false);
        };

        fnData->init(xs_anyType, xs_anyType);

        context->resultStack.push(ResultInfo(
            new SequenceConcat(
              new SequenceConcat(
                new FunCallParams(binaryOperationFunction,
                    fnData, fParams(leftSequenceId, rightSequenceId)),
                leftSequence, leftSequenceId),
            rightSequence, rightSequenceId)));
    } else {
        U_ASSERT(false);
    };
}

void lr2opt::visit(ASTCast &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTCastable &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTInstOf &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTTreat &n) {
    throw USER_EXCEPTION(2902);
}


/* XQuery language - Statements */

void lr2opt::visit(ASTCase &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTIf &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTQuantExpr &n)
{
    ScopeMarker scopeMarker = optimizer->planContext()->setScopeMarker();

    n.expr->accept(*this);

    rqp::RPBase * expression = context->resultStack.top().op;

    U_ASSERT(dynamic_cast<ASTTypeVar *>(n.var) != NULL);
    n.var->accept(*this);

    ContextInfo varBinding = context->resultStack.top().variable;
    context->resultStack.pop();

    n.sat->accept(*this);

    rqp::RPBase * predicate = context->resultStack.top().op;
    context->resultStack.pop();

    xsd::QName aggrFunction;

    // TODO: not fair! should be an aggr function
    if (n.type == ASTQuantExpr::SOME)
    {
        context->resultStack.push(ResultInfo(
          new If(
            new MapConcat(
              new If(predicate,
                new VarIn(varBinding.item), null_op),
              expression, varBinding.item),
            new Const(planContext->varGraph.alwaysTrueSequence),
            null_op)));
    }
    else /* EVERY */
    {
/*      
        aggrFunction = xsd::constQName(skn->resolvePrefix("fn"), "empty");
        predicate = new rqp::If(predicate, new rqp::Const(EmptySequenceConst()), new rqp::Const(fn_true()));
        
        context->resultStack.push(ResultInfo(
            new rqp::FunCall(aggrFunction,
                new rqp::MapConcat(predicate, expression, varBinding))));
*/                
    }

    optimizer->planContext()->clearScopesToMarker(scopeMarker);
}

void lr2opt::visit(ASTTypeSwitch &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTSeq &n) {
    
    throw USER_EXCEPTION(2902);
}


/* XQuery language - FLWOR and its parts */

void lr2opt::visit(ASTFLWOR &n) {
    std::stack<ResultInfo> operationStack;

    ScopeMarker scopeMarker = optimizer->planContext()->setScopeMarker();
    
    for (ASTNodesVector::const_iterator fl = n.fls->begin(); fl != n.fls->end(); fl++) {
        optimizer->planContext()->newScope();
        (*fl)->accept(*this);

        operationStack.push(context->resultStack.top());
        context->resultStack.pop();
    }

    if (n.where) {
        n.where->accept(*this);

        operationStack.push(context->resultStack.top());
        context->resultStack.pop();
        operationStack.top().opid = If::clsid;
    }

    if (n.order_by) {
        throw USER_EXCEPTION(2902);
    }

    n.ret->accept(*this);

    while (!operationStack.empty()) {
        switch (operationStack.top().opid) {
          case MapConcat::clsid :
            context->resultStack.top().op =
              new MapConcat(context->resultStack.top().op, operationStack.top().op, operationStack.top().variable.item);
            break;
          case SequenceConcat::clsid :
            context->resultStack.top().op =
              new SequenceConcat(context->resultStack.top().op, operationStack.top().op, operationStack.top().variable.item);
            break;
          case If::clsid :
            context->resultStack.top().op =
              new If(operationStack.top().op, context->resultStack.top().op, null_op);
            break;
          default:
            U_ASSERT(false);
            throw USER_EXCEPTION(2902);
        };
        
        operationStack.pop();
    };

    optimizer->planContext()->clearScopesToMarker(scopeMarker);
}

void lr2opt::visit(ASTFor &n) {
    n.expr->accept(*this);

    n.tv->accept(*this);

    // TODO : process type info, important step, make type assert
    // TODO : process pos var

    context->resultStack.top().opid = MapConcat::clsid;

    if (n.usesPosVar()) {
        U_ASSERT(false);
    }
}

void lr2opt::visit(ASTLet &n) {
    n.expr->accept(*this);

    n.tv->accept(*this);

    // TODO : process type info, important step, make type assert
    
    context->resultStack.top().opid = SequenceConcat::clsid;
}

void lr2opt::visit(ASTOrder &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTOrderBy &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTOrderMod &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTOrderModInt &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTOrderSpec &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTPosVar &n) {
    throw USER_EXCEPTION(2902);
//    n.var->accept(*this);
}


/* XQuery language - Atoms */

void lr2opt::visit(ASTTypeSeq &n) {
    lr2por::visit(n);
}


void lr2opt::visit(ASTTypeVar &n) {
    std::string varName = static_cast<ASTVar *>(n.var)->getStandardName();
    opt::TupleId varBinding = optimizer->planContext()->generateTupleIdVarScoped(varName);
    context->resultStack.top().variable.item = varBinding;
}

void lr2opt::visit(ASTType &n) {
    lr2por::visit(n);
}

void lr2opt::visit(ASTVar &n) {
    context->resultStack.push(ResultInfo(
          new VarIn(optimizer->planContext()->getVarTupleInScope(n.getStandardName()))));
}

void lr2opt::visit(ASTLit &n) {
    opt::MemoryTupleSequence * mts = new opt::MemoryTupleSequence;
    context->resultStack.push(ResultInfo(new Const(mts)));

    try {
        switch (n.type) {
          case ASTLit::STRING:
            mts->push_back(string2tuple_cell(*n.lit, xs_string));
          break;
          case ASTLit::DOUBLE :
            mts->push_back(string2tuple_cell(*n.lit, xs_double));
          break;
          case ASTLit::INTEGER:
            mts->push_back(string2tuple_cell(*n.lit, xs_integer));
          break;
          case ASTLit::DECIMAL:
            mts->push_back(string2tuple_cell(*n.lit, xs_decimal));
          break;
          default:
            U_ASSERT(false);
            drv->error(SE4001, "unexpected literal node type");
        };
    }
    catch (SednaUserException &e)
    {
        drv->error(e.getCode(), e.getDescription());
    }
}

void lr2opt::visit(ASTQName &n) {
    throw USER_EXCEPTION(2902);
}

void lr2opt::visit(ASTOrdExpr &n) {
    throw USER_EXCEPTION(2902);
}


/* Xquery PP helpers */

void lr2opt::visit(ASTDDO &n) {
    throw USER_EXCEPTION(2902);
}
