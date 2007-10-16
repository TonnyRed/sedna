/*
 * File:  PPDocInCol.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include "common/sedna.h"

#include "tr/executor/xqops/PPDocInCol.h"
#include "tr/executor/fo/casting_operations.h"
#include "tr/structures/metadata.h"
#include "tr/executor/base/xs_uri.h"
#include "tr/executor/base/PPUtils.h"

PPDocInCol::PPDocInCol(dynamic_context *_cxt_, 
                       PPOpIn _col_name_op_,
                       PPOpIn _doc_name_op_) : PPIterator(_cxt_),
                                               col_name_op(_col_name_op_),
                                               doc_name_op(_doc_name_op_)
{
}

PPDocInCol::~PPDocInCol()
{
    delete col_name_op.op;
    col_name_op.op = NULL;
    delete doc_name_op.op;
    doc_name_op.op = NULL;
}

void PPDocInCol::open ()
{
    col_name_op.op->open();
    doc_name_op.op->open();

    first_time = true;
}


void PPDocInCol::reopen()
{
    col_name_op.op->reopen();
    doc_name_op.op->reopen();

    first_time = true;
}


void PPDocInCol::close ()
{
    col_name_op.op->close();
    doc_name_op.op->close();
}

void PPDocInCol::next(tuple &t)
{
    SET_CURRENT_PP(this);

    if (first_time)
    {
        col_name_op.op->next(t);
        if (t.is_eos()) {RESTORE_CURRENT_PP; return;}
        
        tuple_cell tc_col= atomize(col_name_op.get(t));
        if(!is_string_type(tc_col.get_atomic_type())) throw XQUERY_EXCEPTION2(XPTY0004, "Invalid type of the first argument in fn:doc (xs_string/derived/promotable is expected).");
        col_name_op.op->next(t);
        if (!t.is_eos()) throw XQUERY_EXCEPTION2(XPTY0004, "Invalid arity of the first argument in fn:doc. Argument contains more than one item.");
        tc_col = tuple_cell::make_sure_light_atomic(tc_col);

        doc_name_op.op->next(t);
        if (t.is_eos()) {RESTORE_CURRENT_PP; return;}

        tuple_cell tc_doc= atomize(doc_name_op.get(t));
        if(!is_string_type(tc_doc.get_atomic_type())) throw XQUERY_EXCEPTION2(XPTY0004, "Invalid type of the second argument in fn:doc (xs_string/derived/promotable is expected).");
        doc_name_op.op->next(t);
        if (!t.is_eos()) throw XQUERY_EXCEPTION2(XPTY0004, "Invalid arity of the second argument in fn:doc. Argument contains more than one item.");
        tc_doc = tuple_cell::make_sure_light_atomic(tc_doc);

        first_time = false;

        // Put lock on collection and check security
		counted_ptr<db_entity> db_ent(se_new db_entity);
        db_ent->name = se_new char[tc_col.get_strlen_mem() + 1];
        strcpy(db_ent->name, tc_col.get_str_mem());
		db_ent->type = dbe_collection;
        schema_node *root = get_schema_node(db_ent, "Unknown entity passed to PPDocInCol");

        bool valid;
        Uri::check_constraints(&tc_doc, &valid, NULL);
        if(!valid) throw XQUERY_EXCEPTION2(FODC0005, "Invalid uri in the first argument of fn:doc.");
        
        xptr res = find_document((const char*)tc_col.get_str_mem(), (const char*)tc_doc.get_str_mem());
        if (res == NULL)
        {
            throw XQUERY_EXCEPTION2(SE1006, (std::string("Document '") + 
                                           tc_doc.get_str_mem() + 
                                           "' in collection '" + 
                                           tc_col.get_str_mem() + "'").c_str());
        }

        t.copy(tuple_cell::node(res));
    }
    else 
    {
        first_time = true;
        t.set_eos();
    }

    RESTORE_CURRENT_PP;
}

PPIterator* PPDocInCol::copy(dynamic_context *_cxt_)
{
    PPDocInCol *res = se_new PPDocInCol(_cxt_, col_name_op, doc_name_op);
    res->col_name_op.op = col_name_op.op->copy(_cxt_);
    res->doc_name_op.op = doc_name_op.op->copy(_cxt_);
    res->set_xquery_line(__xquery_line);
    return res;
}

bool PPDocInCol::result(PPIterator* cur, dynamic_context *cxt, void*& r)
{
    return true;
}



