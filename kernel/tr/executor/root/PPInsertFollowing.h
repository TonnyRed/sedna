/*
 * File:  PPInsertFollowing.h
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */


#ifndef _PPINSERTFOLLOWING_H
#define _PPINSERTFOLLOWING_H

#include "common/sedna.h"
#include "tr/executor/base/PPBase.h"

class PPInsertFollowing : public PPUpdate
{
private:
    PPOpIn child1, child2;
    dynamic_context *cxt;

    virtual void do_open();
    virtual void do_close();
    virtual void do_execute();
    virtual void do_accept(PPVisitor& v);

public:
    PPInsertFollowing(PPOpIn _child1_, 
                      PPOpIn _child2_,
                      dynamic_context *_cxt_);
    ~PPInsertFollowing();
};


#endif

