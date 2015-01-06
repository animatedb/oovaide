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
//	a class can inherit from std::string and the std::string will be output
//	as a Datatype.
//	Only referenced datatypes are output in the XMI file.
//
// - "Class" values are class, struct or union definitions (CXType_Record) or
//	class templates.
// 	The class is defined in this TU if it has a module.
//	A class is defined if it has operations or attributes.
//	(An example of a class not in this TU is from other header files)
//	Only defined classes or defined in this TU or referenced types will
//	be output in the XMI file,
//	except class templates will always be output and contain definitions?
//      Only defined classes in the parsed translation unit have a module name/id.
//
// - Inheritance relations are also only defined if they are defined in this TU.

bool ModelWriter::openFile(OovStringRef const filename)
    {
#if(DEBUG_WRITE)
    writeDebugStr(std::string("**** File ****") + filename);
#endif
    mFp = fopen(filename, "w");
    if(mFp)
        {
        fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", mFp);
        fputs("<XMI xmi.version=\"1.2\" xmlns:UML=\"http://schema.omg.org/spec/UML/1.3\" >\n", mFp);
        fputs(" <XMI.content>\n", mFp);
        }
    return(mFp != nullptr);
    }

static int sModelId = MIO_NoLookup;
int newModelId()
    { return sModelId++; }

int ModelWriter::getObjectModelId(const std::string &name)
    {
    int index = -1;
    for(unsigned int i=0; i<mModelData.mTypes.size(); i++)
        {
        if(name.compare(mModelData.mTypes[i]->getName()) == 0)
            {
            index = i + MIO_Object;
            break;
            }
        }
    return index;
    }


static char const * const boolStr(bool val)
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
	    case '>':	    	retStr += "&gt;";	break;
	    case '<':	    	retStr += "&lt;";	break;
	    case '&':		retStr += "&amp;";	break;
	    case '\'':		retStr += "&apos;";	break;
	    case '\"':		retStr += "&quot;";	break;
	    default:		retStr += str[i];	break;
	    }
	}
    return retStr;
    }


// # symbol is between statement types
// For each statement type, the @ symbol separates values defining the statement.
//
// There are the following types of statements:
//	Open nesting
//	Close nesting
//	Call
//	Variable reference
//
// Format of values in a open nesting:
//	{ optional conditional @ ADD? list of non-const identifiers or functions
// Format of values in a close nesting:
//	}
// Format of values in a call statement:
//	c= call name @ type ID
//	call name is membername.funcname, where dot is used for any member ref including "->".
// Format of values in a variable statement:
//	v= variable name @ class type ID @ type ID @ f=read/t=written
// Example:
//    <Parms list="{[ c1 ]#c=a@2#}#{[ c2 ]#c=b@2#}" />
//	{[ c1 ]		Open nesting with conditional on variable c1
//	c=a@2		Call to function a() with type id=2
//	}		Close nesting
void ModelWriter::writeStatements(const ModelStatements &stmts)
    {
    if(stmts.size() > 0)
	{
	fprintf(mFp, "%s", "   <Statements list=\"");
	bool firstTime = true;
	for(auto &stmt : stmts)
	    {
	    if(!firstTime)
		{
		fprintf(mFp, "#");	// # statement separator
		}
	    firstTime = false;
	    switch(stmt.getStatementType())
		{
		case ST_OpenNest:
		    fprintf(mFp, "{%s", translate(stmt.getName()).c_str());
		    break;

		case ST_CloseNest:
		    fprintf(mFp, "}");
		    break;

		case ST_Call:
		    {
		    std::string className;
		    if(stmt.getClassDecl().getDeclType())
			className = stmt.getClassDecl().getDeclType()->getName();
		    fprintf(mFp, "c=%s@%d", translate(stmt.getName()).c_str(),
			getObjectModelId(className));
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
		    fprintf(mFp, "v=%s@%d@%d@%s", translate(stmt.getName()).c_str(),
			getObjectModelId(className), getObjectModelId(varType),
			boolStr(stmt.getVarAccessWrite()));
		    }
		    break;
		}
	    }
	fprintf(mFp, "\"/>\n");
	}
    }

// # symbol is between parameters.
// For each parameter, the @ symbol separates values defining a parameter.
// Format of values in a parameter:
//	parameter name @ type ID @ is const @ is reference
// Example:
//    <Parms list="symbolName@120@true@true#symbol@121@false@true" />
void ModelWriter::writeOperation(ModelOperation const &oper)
    {
    char locStr[50];
//    snprintf(locStr, sizeof(locStr), "module=\"%d\" line=\"%d\"", MIO_Module, oper.getLineNum());
    if(oper.getLineNum() > 0)
	snprintf(locStr, sizeof(locStr), "line=\"%d\"", oper.getLineNum());
    else
	locStr[0] = '\0';
#if(OPER_RET_TYPE)
    ModelTypeRef const &retType = oper.getReturnType();
    fprintf(mFp, "  <Oper name=\"%s\" access=\"%s\" const=\"%s\" %s "
	    "ret=\"%d\" retconst=\"%s\" retref=\"%s\">\n",
#else
    fprintf(mFp, "  <Oper name=\"%s\" access=\"%s\" const=\"%s\" %s>\n",
#endif
	oper.getName().getStr(), oper.getAccess().asUmlStr().getStr(),
	boolStr(oper.isConst()), locStr
#if(OPER_RET_TYPE)
	,
	getObjectModelId(retType.getDeclType()->getName()),
	boolStr(retType.isConst()), boolStr(retType.isRefer())
#endif
	);

    if(oper.getParams().size() > 0)
	{
	OovString parmStr="";
	for(const auto &param : oper.getParams())
	    {
	    /*
	    fprintf(mFp, "   <Parm name=\"%s\" type=\"%d\" "
		"const=\"%s\" ref=\"%s\" />\n",
		param->getName().c_str(),
		getObjectModelId(param->getDeclType()->getName()),
		boolStr(param->isConst()), boolStr(param->isRefer()));
		*/
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
	fprintf(mFp, "   <Parms list=\"%s\" />\n", parmStr.c_str());
	}

    for(const auto &decl : oper.getBodyVarDeclarators())
	{
	fprintf(mFp, "   <BodyVarDecl name=\"%s\" type=\"%d\" "
	    "const=\"%s\" ref=\"%s\" />\n",
	    decl->getName().c_str(),
	    getObjectModelId(decl->getDeclType()->getName()),
	    boolStr(decl->isConst()), boolStr(decl->isRefer()));
	}
    writeStatements(oper.getStatements());
    fprintf(mFp, "  </Oper>\n");
    }

void ModelWriter::writeClassDefinition(const ModelClassifier &classifier, bool isClassDef)
    {
    if(isClassDef)
	{
	for(const auto &attr : classifier.getAttributes())
	    {
	    fprintf(mFp, "  <Attr name=\"%s\" type=\"%d\" "
		"const=\"%s\" ref=\"%s\" access=\"%s\" />\n",
		attr->getName().c_str(),
		getObjectModelId(attr->getDeclType()->getName()),
		boolStr(attr->isConst()), boolStr(attr->isRefer()),
		attr->getAccess().asUmlStr().getStr());
	    }
	}

    for(const auto &oper : classifier.getOperations())
	{
	if(oper->getModule())
	    {
	    writeOperation(*oper);
	    }
	}
    }

void ModelWriter::writeType(const ModelType &mtype)
    {
    if(mFp)
        {
        int classXmiId = getObjectModelId(mtype.getName());
        bool isDefinedClass = false;
        bool isDefinedOpers = false;
	const ModelClassifier *cl = mtype.getClass();
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
        if(isDefinedClass || isDefinedOpers || mModelData.isTypeReferencedByDefinedObjects(mtype))
            {
	    char const *typeName;
	    char lineNumStr[50];
	    if(mtype.getDataType() == DT_Class)
		{
		typeName = "Class";
		const ModelClassifier *cl = mtype.getClass();
		snprintf(lineNumStr, sizeof(lineNumStr), "line=\"%d\"", cl->getLineNum());
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
		const ModelClassifier *cl = mtype.getClass();
		if(cl->getModule())
		    {
		    moduleStr = "module=\"";
		    moduleStr.appendInt(MIO_Module);
		    moduleStr += "\" ";
		    }
		if(cl->getAttributes().size() == 0 && cl->getOperations().size() == 0)
		    {
		    earlyTermStr = "/";
		    }
		}
	    else
		{
		earlyTermStr = "/";
		}
	    fprintf(mFp, "  <%s id=\"%d\" name=\"%s\" %s%s%s>\n",
		typeName, classXmiId, translate(mtype.getName()).c_str(),
		moduleStr.c_str(), lineNumStr, earlyTermStr.c_str());

	    if(mtype.getDataType() == DT_Class)
		{
		const ModelClassifier &classifier = static_cast<const ModelClassifier&>(mtype);
		writeClassDefinition(classifier, isDefinedClass);
		}
	    if(earlyTermStr.length() == 0)
		{
		fprintf(mFp, "  </%s>\n", typeName);
		}
	    }
        }
    }

void ModelWriter::writeAssociation(const ModelAssociation &assoc)
    {
    if(mFp)
        {
        fprintf(mFp, "  <Genrl id=\"%d\" child=\"%d\" parent=\"%d\" "
            "access=\"%s\" />\n",
            newModelId(), getObjectModelId(assoc.getChild()->getName()),
            getObjectModelId(assoc.getParent()->getName()),
            assoc.getAccess().asUmlStr().getStr());
        }
    }

bool ModelWriter::writeFile(OovStringRef const filename)
{
    bool success = openFile(filename);
    if(success)
        {
        int moduleXmiId=MIO_Module;
        fprintf(mFp, "  <Module id=\"%d\" module=\"%s\" >\n",
            moduleXmiId, mModelData.mModules[0]->getModulePath().c_str());
        fprintf(mFp, "  </Module>\n");
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Types"));
#endif
        for(auto &type : mModelData.mTypes)
            {
            /*
	    ModelClassifier *cl = ModelObject::getClass(type.get());
	    if(cl)
		{
		if(!(cl->getOutput() & ModelClassifier::O_DefineOperations))
		    cl->clearOperations();
		if(!(cl->getOutput() & ModelClassifier::O_DefineAttributes))
		    cl->clearAttributes();
		}
		*/
	    writeType(*type);
            }
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Assocs"));
#endif
        for(auto &assoc : mModelData.mAssociations)
            writeAssociation(*assoc);
#if(DEBUG_WRITE)
        writeDebugStr(std::string("     Done"));
#endif
        }
    return success;
}

ModelWriter::~ModelWriter()
{
    if(mFp)
        {
        fputs(" </XMI.content>\n", mFp);
        fputs("</XMI>\n", mFp);
        fclose(mFp);
        }
}

