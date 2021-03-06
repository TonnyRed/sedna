/*
 * File:  ASTXMLComm.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_XML_COMM_H_
#define _AST_XML_COMM_H_

#include "ASTNode.h"
class ASTVisitor;

#include <string>

class ASTXMLComm : public ASTNode
{
public:
    std::string *cont; // character content

    bool deep_copy; // comment will be attached to virtual_root and copied on demand

public:
    ASTXMLComm(const ASTNodeCommonData &loc, std::string *cont_) : ASTNode(loc), cont(cont_) {}

    ~ASTXMLComm();

    void accept(ASTVisitor &v);
    ASTNode *dup();
    void modifyChild(const ASTNode *oldc, ASTNode *newc);

    static ASTNode *createNode(scheme_list &sl);
};

#endif
