/**
*   Copyright (C) 2013 by dcblaha
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*  @file      ModelWriter.cpp
*
*  @date      2/1/2013
*
*/


#include "ModelWriter.h"
#include "OovString.h"
#include "OovError.h"

// Writes the following format:
//  <DataType stereotype="datatype" type="typeid1" name="typeName" />
//  <Class ... name="cExampleClass" />
//  <Attribute type="typeid1" name="memberName" />
//  <Generalization child="subClassRef" parent="superClassRef" />
enum ModelIdOffsets { MIO_Module=1, MIO_Object=2, MIO_NoLookup=100000 };

#define DEBUG_WRITE 0
#if(DEBUG_WRITE)
#include "File.h"
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>
static void writeDebugStr(std::string const &str)
    {
    SharedFile file;
    file.open("DebugCppParseWriter.txt", M_ReadWriteExclusiveAppend);
    if(file.isOpen())
        {
        OovString tempStr;
        tempStr.appendInt(getpid());
        tempStr += str + '\n';
        file.write(tempStr.c_str(), tempStr.length());
        }
    }
#endif


// Some details about the XMI output file:
// - "Datatype" values are simple data types or class references. For example,
//      a class can inherit from std::string and the std::string will be output
//      as a Datatype.
//      Only referenced datatypes are output in the XMI file.
//
// - "Class" values are class, struct or union definitions (CXType_Record) or
//      class templates.
//      The class is defined in this TU if it has a module.
//      A class is defined if it has operations or attributes.
//      (An example of a class not in this TU is from other header files)
//      Only defined classes or defined in this TU or referenced types will
//      be output in the XMI file,
//      except class templates will always be output and contain definitions?
//      Only defined classes in the parsed translation unit have a module name/id.
//
// - Inheritance relations are also only defined if they are defined in this TU.

OovStatusReturn ModelWriter::openFile(OovStringRef const filename)
    {
#if(DEBUG_WRITE)
    writeDebugStr(std::string("**** File ****") + filename);
#endif
    OovStatus status(true, SC_File);
    mFile.open(filename, "w");
    if(mFile.isOpen())
        {
        status = mFile.putString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        if(status.ok())
            {
            status = mFile.putString("<XMI xmi.version=\"1.2\""
                    " xmlns:UML=\"http://schema.omg.org/spec/UML/1.3\" >\n");
            }
        if(status.ok())
            {
            status = mFile.putString(" <XMI.content>\n");
            }
        }
    return(status);
    }

static int sModelId = MIO_NoLookup;
static int newModelId()
    { return sModelId++; }

int ModelWriter::getObjectModelId(const std::string &name)
    {
    int index = -1;
    for(size_t i=0; i<mModelData.mTypes.size(); i++)
        {
        if(name.compare(mModelData.mTypes[i]->getName()) == 0)
            {
            index = static_cast<int>(i) + MIO_Object;
            break;
            }
        }
    return index;
    }


static char const * boolStr(bool val)
{
    char const *str;
    if(val)
        str = "t";
    else
        str = "f";
    return str;
}

static std::string translate(const std::string &str)
    {
    std::string retStr;
    for(size_t i=0; i<str.length(); i++)
        {
        switch(str[i])
            {
            case '>':           retStr += "&gt;";       break;
            case '<':           retStr += "&lt;";       break;
            case '&':           retStr += "&amp;";      break;
            case '\'':          retStr += "&apos;";     break;
            case '\"':          retStr += "&quot;";     break;
            default:            retStr += str[i];       break;
            }
        }
    return retStr;
    }


// # symbol is between statement types
// For each statement type, the @ symbol separates values defining the statement.
//
// There are the following types of statements:
//      Open nesting
//      Close nesting
//      Call
//      Variable reference
//
// Format of values in a open nesting:
//      { optional conditional @ ADD? list of non-const identifiers or functions
// Format of values in a close nesting:
//      }
// Format of values in a call statement:
//      c= call name @ type ID
//      call name is membername.funcname, where dot is used for any member ref including "->".
// Format of values in a variable statement:
//      v= variable name @ class type ID @ type ID @ f=read/t=written
//      class type ID is the class that the variable is in.
//      type ID is the type of the variable.
// Example:
//    <Parms list="{[ c1 ]#c=a@2#}#{[ c2 ]#c=b@2#}" />
//      {[ c1 ]         Open nesting with conditional on variable c1
//      c=a@2           Call to function a() with type id=2
//      }               Close nesting
OovStatusReturn ModelWriter::writeStatements(const ModelStatements &stmts)
    {
    OovStatus status(true, SC_File);
    if(stmts.size() > 0)
        {
        OovString outStr = "   <Statements list=\"";
        bool firstTime = true;
        for(auto const &stmt : stmts)
            {
            if(!firstTime)
                {
                outStr += '#';      // # statement separator
                }
            firstTime = false;
            switch(stmt.getStatementType())
                {
                case ST_OpenNest:
                    outStr += '{';
                    outStr += translate(stmt.getCondName()).c_str();
                    break;

                case ST_CloseNest:
                    outStr += '}';
                    break;

                case ST_Call:
                    {
                    std::string className;
                    std::string funcName = stmt.getFullName();
                    ModelType const *type = stmt.getClassDecl().getDeclType();
                    if(type)
                        {
                        className = type->getName();
                        ModelClassifier const *classifier = ModelType::getClass(type);
                        if(classifier && !classifier->isOperOverloaded(
                                stmt.getFuncName()))
                            {
                            ModelStatement::eraseOverloadKey(funcName);
                            }
                        }
                    outStr += "c=";
                    outStr += translate(funcName);
                    outStr += '@';
                    outStr.appendInt(getObjectModelId(className));
                    }
                    break;

                case ST_VarRef:
                    {
                    std::string className;
                    std::string varType;
                    if(stmt.getClassDecl().getDeclType())
                        className = stmt.getClassDecl().getDeclType()->getName();
                    if(stmt.getVarDecl().getDeclType())
                        varType = stmt.getVarDecl().getDeclType()->getName();
                    outStr += "v=";
                    outStr += translate(stmt.getAttrName());
                    outStr += '@';
                    outStr.appendInt(getObjectModelId(className));
                    outStr += '@';
                    outStr.appendInt(getObjectModelId(varType));
                    outStr += '@';
                    outStr += boolStr(stmt.getVarAccessWrite());
                    }
                    break;
                }
            }
        outStr += "\"/>\n";
        status = mFile.putString(outStr);
        }
    return status;
    }

static void appendAttr(OovStringRef attrName, OovStringRef attrVal, OovString &outStr)
    {
    outStr += ' ';
    outStr += attrName;
    outStr += "=\"";
    outStr += attrVal;
    outStr += '\"';
    }

static void appendIntAttr(OovStringRef attrName, int val, OovString &outStr)
    {
    OovString numStr;
    numStr.appendInt(val);
    appendAttr(attrName, numStr, outStr);
    }

// # symbol is between parameters.
// For each parameter, the @ symbol separates values defining a parameter.
// Format of values in a parameter:
//      parameter name @ type ID @ is const @ is reference
// Example:
//    <Parms list="symbolName@120@true@true#symbol@121@false@true" />
OovStatusReturn ModelWriter::writeOperation(ModelClassifier const &classifier, ModelOperation const &oper)
    {
    ModelTypeRef const &retType = oper.getReturnType();
    OovString outStr = "  <Oper";
    appendAttr("name", oper.getName(), outStr);
    if(classifier.isOperOverloaded(oper.getName()))
        {
        appendAttr("sym", oper.getOverloadKey(), outStr);
        }
    appendAttr("access", oper.getAccess().asUmlStr(), outStr);
    appendAttr("const", boolStr(oper.isConst()), outStr);
    appendAttr("virt", boolStr(oper.isVirtual()), outStr);
    if(oper.getLineNum() > 0)
        {
        appendIntAttr("line", oper.getLineNum(), outStr);
        }
    appendIntAttr("ret", getObjectModelId(retType.getDeclType()->getName()), outStr);
    appendAttr("retconst", boolStr(retType.isConst()), outStr);
    appendAttr("retref", boolStr(retType.isRefer()), outStr);
    outStr += ">\n";

    OovStatus status = mFile.putString(outStr);
    if(status.ok() && oper.getParams().size() > 0)
        {
        OovString parmStr="";
        for(const auto &param : oper.getParams())
            {
            if(parmStr.length() != 0)
                {
                parmStr += '#';
                }
            parmStr += param->getName();
            parmStr += '@';
            parmStr.appendInt(getObjectModelId(param->getDeclType()->getName()));
            parmStr += '@';
            parmStr += boolStr(param->isConst());
            parmStr += '@';
            parmStr += boolStr(param->isRefer());
            }
        OovString parmOutStr;
        parmOutStr += "   <Parms";
        appendAttr("list", parmStr, parmOutStr);
        parmOutStr += " />\n";
        status = mFile.putString(parmOutStr);
        }
    if(status.ok())
        {
        for(const auto &decl : oper.getBodyVarDeclarators())
            {
            OovString declOutStr = "   <BodyVarDecl";
            appendAttr("name", decl->getName(), declOutStr);
            appendIntAttr("type", getObjectModelId(decl->getDeclType()->getName()), declOutStr);
            appendAttr("const", boolStr(decl->isConst()), declOutStr);
            appendAttr("ref", boolStr(decl->isRefer()), declOutStr);
            declOutStr +=  " />\n";
            status = mFile.putString(declOutStr);
            }
        }
    if(status.ok())
        {
        status = writeStatements(oper.getStatements());
        }
    if(status.ok())
        {
        status = mFile.putString("  </Oper>\n");
        }
    return status;
    }

OovStatusReturn ModelWriter::writeClassDefinition(const ModelClassifier &classifier, bool isClassDef)
    {
    OovStatus status(true, SC_File);
    if(isClassDef)
        {
        for(const auto &attr : classifier.getAttributes())
            {
            OovString outStr = "  <Attr";
            appendAttr("name", attr->getName(), outStr);
            appendIntAttr("type", getObjectModelId(attr->getDeclType()->getName()), outStr);
            appendAttr("const", boolStr(attr->isConst()), outStr);
            appendAttr("ref", boolStr(attr->isRefer()), outStr);
            appendAttr("access", attr->getAccess().asUmlStr().getStr(), outStr);
            outStr += " />\n";
            status = mFile.putString(outStr);
            }
        }

    if(status.ok())
        {
        for(const auto &oper : classifier.getOperations())
            {
            if(oper->getModule())
                {
                status = writeOperation(classifier, *oper);
                if(!status.ok())
                    {
                    break;
                    }
                }
            }
        }
    return status;
    }

OovStatusReturn ModelWriter::writeType(const ModelType &mtype)
    {
    int classXmiId = getObjectModelId(mtype.getName());
    bool isDefinedClass = false;
    bool isDefinedOpers = false;
    const ModelClassifier *cl = ModelClassifier::getClass(&mtype);
    if(cl)
        {
        if(cl->getModule())
            isDefinedClass = true;
        for(const auto &oper : cl->getOperations())
            {
            if(oper->getModule())
                isDefinedOpers = true;
            }
        }
    OovStatus status(true, SC_File);
    if(isDefinedClass || isDefinedOpers ||
        mModelData.isTypeReferencedByDefinedObjects(mtype))
        {
        char const *typeName;
        char lineNumStr[50];
        if(mtype.getDataType() == DT_Class)
            {
            typeName = "Class";
            const ModelClassifier *typeCl = ModelClassifier::getClass(&mtype);
            snprintf(lineNumStr, sizeof(lineNumStr), "line=\"%d\"",
                typeCl->getLineNum());
            }
        else
            {
            typeName = "DataType";
            lineNumStr[0] = '\0';
            }
        // Only defined classes in the parsed translation unit have a module.
        OovString moduleStr;
        std::string earlyTermStr;
        if(mtype.getDataType() == DT_Class)
            {
            const ModelClassifier *typeCl = ModelClassifier::getClass(&mtype);
            if(typeCl->getModule())
                {
                moduleStr = "module=\"";
                moduleStr.appendInt(MIO_Module);
                moduleStr += "\" ";
                }
            if(typeCl->getAttributes().size() == 0 &&
                typeCl->getOperations().size() == 0)
                {
                earlyTermStr = "/";
                }
            }
        else
            {
            earlyTermStr = "/";
            }
        OovString outStr = "  <";
        outStr += typeName;
        appendIntAttr("id", classXmiId, outStr);
        appendAttr("name", translate(mtype.getName()), outStr);
        outStr += ' ';
        outStr += moduleStr;
        outStr += lineNumStr;
        outStr += earlyTermStr;
        outStr += ">\n";
        status = mFile.putString(outStr);
        if(status.ok())
            {
            if(mtype.getDataType() == DT_Class)
                {
                const ModelClassifier &classifier = static_cast<const ModelClassifier&>(mtype);
                status = writeClassDefinition(classifier, isDefinedClass);
                }
            }
        if(status.ok())
            {
            if(earlyTermStr.length() == 0)
                {
                OovString outStr = "  </";
                outStr += typeName;
                outStr += ">\n";
                status = mFile.putString(outStr);
                }
            }
        }
    return status;
    }

OovStatusReturn ModelWriter::writeAssociation(const ModelAssociation &assoc)
    {
    OovString outStr = "  <Genrl";
    appendIntAttr("id", newModelId(), outStr);
    appendIntAttr("child", getObjectModelId(assoc.getChild()->getName()), outStr);
    appendIntAttr("parent", getObjectModelId(assoc.getParent()->getName()), outStr);
    appendAttr("access", assoc.getAccess().asUmlStr(), outStr);
    outStr += " />\n";
    return mFile.putString(outStr);
    }

OovStatusReturn ModelWriter::writeFile(OovStringRef const filename)
    {
    OovStatus status = openFile(filename);
    if(status.ok())
        {
        int moduleXmiId=MIO_Module;
        ModelModule const *module = mModelData.mModules[0].get();
        OovString outStr = "  <Module";
        appendIntAttr("id", moduleXmiId, outStr);
        appendAttr("module", module->getModulePath(), outStr);
        appendIntAttr("codeLines", module->mLineStats.mNumCodeLines, outStr);
        appendIntAttr("commentLines", module->mLineStats.mNumCommentLines, outStr);
        appendIntAttr("moduleLines", module->mLineStats.mNumModuleLines, outStr);
        outStr += " >\n";
        outStr += "  </Module>\n";
        status = mFile.putString(outStr);
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Types"));
#endif
        }
    if(status.ok())
        {
        for(auto &type : mModelData.mTypes)
            {
            status = writeType(*type);
            if(!status.ok())
                {
                break;
                }
            }
        }
    if(status.ok())
        {
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Assocs"));
#endif
        for(auto &assoc : mModelData.mAssociations)
            {
            status = writeAssociation(*assoc);
            if(!status.ok())
                {
                break;
                }
            }
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Done"));
#endif
        }
    if(!status.ok())
        {
        OovString str = "Unable to save model data file: ";
        str += filename;
        status.report(ET_Error, str);
        }
    return status;
    }

ModelWriter::~ModelWriter()
    {
    if(mFile.isOpen())
        {
        OovStatus status = mFile.putString(" </XMI.content>\n</XMI>\n");
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to finish model file");
            }
        }
    }

