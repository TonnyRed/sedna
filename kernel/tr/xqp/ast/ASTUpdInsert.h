/*
 * File:  ASTUpdInsert.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_UPDATE_INSERT_H_
#define _AST_UPDATE_INSERT_H_

#include "ASTNode.h"
class ASTVisitor;

class ASTUpdInsert : public ASTNode
{
public:
    enum UpdType
    {
        PRECEDING,
        INTO,
        FOLLOWING
    };

    ASTNode *what, *where;
    UpdType type;

public:
    ASTUpdInsert(const ASTNodeCommonData &loc, ASTNode *what_, ASTNode *where_, UpdType type_) : ASTNode(loc), what(what_), where(where_), type(type_) {}

    ~ASTUpdInsert();

    void accept(ASTVisitor &v);
    ASTNode *dup();
    void modifyChild(const ASTNode *oldc, ASTNode *newc);

    static ASTNode *createNode(scheme_list &sl);
};

#endif
