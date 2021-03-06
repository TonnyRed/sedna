/*
 * File:  ASTDropUser.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_DROP_USER_H_
#define _AST_DROP_USER_H_

#include "ASTNode.h"
class ASTVisitor;

#include <string>

class ASTDropUser : public ASTNode
{
public:
    std::string *user;

public:
    ASTDropUser(const ASTNodeCommonData &loc, std::string *user_) : ASTNode(loc), user(user_) {}

    ~ASTDropUser();

    void accept(ASTVisitor &v);
    ASTNode *dup();
    void modifyChild(const ASTNode *oldc, ASTNode *newc);

    static ASTNode *createNode(scheme_list &sl);
};

#endif
