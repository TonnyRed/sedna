/*
 * File:  PPBulkLoad.h
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */


#ifndef _PPBULKLOAD_H
#define _PPBULKLOAD_H

#include "common/sedna.h"
#include "tr/executor/base/PPBase.h"


class PPBulkLoad : public PPUpdate
{
private:
    dynamic_context *cxt;
    PPOpIn filename, document, collection;

    virtual void do_open();
    virtual void do_close();
    virtual void do_execute();
    virtual void do_accept(PPVisitor& v);

public:
    PPBulkLoad(PPOpIn _filename_,
               PPOpIn _document_,
               PPOpIn _collection_,
               dynamic_context *_cxt_);
    ~PPBulkLoad();
};


#endif /* _PPBULKLOAD_H */
