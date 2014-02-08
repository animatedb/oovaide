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


// Writes the following format:
//  <DataType stereotype="datatype" type="typeid1" name="typeName" />
//  <Class ... name="cExampleClass" />
//  <Attribute type="typeid1" name="memberName" />
//  <Generalization child="subClassRef" parent="superClassRef" />
enum ModelIdOffsets { MIO_Module=1, MIO_Object=2, MIO_NoLookup=100000 };


// Some details about the XMI output file:
// - "Datatype" values are simple data types.
//
// - "Class" values are structs or classes (CXType_Record). These are always
// output as "Class", but they will be empty if they are not defined in this TU.
// (For example, classes included from other header files)
//
// - Inheritance relations are also only defined if they are defined in this TU.

bool ModelWriter::openFile(char const * const filename)
    {
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
        str = "true";
    else
        str = "false";
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

void ModelWriter::writeStatements(const ModelStatement *stmts, int level)
    {
    std::string ws;
    ws.append(level*2, ' ');
    level++;

    if(stmts->getObjectType() == otCondStatement)
	{
	const ModelCondStatements *condstmts = static_cast<const ModelCondStatements*>(stmts);
	if(condstmts->getStatements().size() != 0)
	    {
	    fprintf(mFp, "      %s<Condition name=\"%s\">\n",
		ws.c_str(), translate(condstmts->getName()).c_str());
	    for(const auto &childstmt : condstmts->getStatements())
		{
		// Recurse
		writeStatements(childstmt, level);
		}
	    fprintf(mFp, "      %s</Condition>\n", ws.c_str());
	    }
	}
    else
	{
	const ModelOperationCall *call = static_cast<const ModelOperationCall*>(stmts);
	std::string className;
	if(call->getDecl().getDeclType())
	    className = call->getDecl().getDeclType()->getName();
	fprintf(mFp, "      %s<Call name=\"%s\" type=\"%d\" />\n",
	    ws.c_str(), translate(call->getName()).c_str(),
	    getObjectModelId(className.c_str()));
	}
    }

void ModelWriter::writeType(const ModelType &mtype)
{
    if(mFp)
        {
        int classXmiId = getObjectModelId(mtype.getName());
        char const *typeName;
        char lineNumStr[50];
        if(mtype.getObjectType() == otClass)
            {
            typeName = "Class";
	    const ModelClassifier *cl = ModelObject::getClass(&mtype);
	    snprintf(lineNumStr, sizeof(lineNumStr), "line=\"%d\"", cl->getLineNum());
            }
        else
            {
            typeName = "DataType";
            lineNumStr[0] = '\0';
            }
        fprintf(mFp, "  <%s xmi.id=\"%d\" name=\"%s\" module=\"%d\" %s>\n",
            typeName, classXmiId, translate(mtype.getName()).c_str(), MIO_Module,
            lineNumStr);

        if(mtype.getObjectType() == otClass)
            {
            const ModelClassifier &classifier = static_cast<const ModelClassifier&>(mtype);
	    for(const auto &attr : classifier.getAttributes())
		{
		fprintf(mFp, "    <Attribute name=\"%s\" type=\"%d\" "
		    "const=\"%s\" ref=\"%s\" access=\"%s\" />\n",
		    attr->getName().c_str(),
		    getObjectModelId(attr->getDeclType()->getName()),
		    boolStr(attr->isConst()), boolStr(attr->isRefer()), attr->getAccess().asStr());
		}

	    for(const auto &oper : classifier.getOperations())
		{
		if(oper)
		    {
		    char locStr[50];
		    if(oper->getModule())
			{
			snprintf(locStr, sizeof(locStr), "module=\"%d\" line=\"%d\"", MIO_Module, oper->getLineNum());
			}
		    else
			{
			locStr[0] = '\0';
			}

		    fprintf(mFp, "    <Operation name=\"%s\" access=\"%s\" const=\"%s\" %s>\n",
			oper->getName().c_str(), oper->getAccess().asStr(),
			boolStr(oper->isConst()), locStr);

		    for(const auto &param : oper->getParams())
			{
			fprintf(mFp, "      <Parameter name=\"%s\" type=\"%d\" "
			    "const=\"%s\" ref=\"%s\" />\n",
			    param->getName().c_str(),
			    getObjectModelId(param->getDeclType()->getName()),
			    boolStr(param->isConst()), boolStr(param->isRefer()));
			}
    /*
		    for(const auto &decl : oper->getDefinitionDeclarators())
			{
			fprintf(mFp, "      <FuncDefDecl name=\"%s\" type=\"%d\" "
			    "const=\"%s\" ref=\"%s\" />\n",
			    decl->getName().c_str(),
			    getClassModelId(decl->getDeclType()->getName()),
			    boolStr(decl->isConst()), boolStr(decl->isRefer()));
			}
    */
		    for(const auto &decl : oper->getBodyVarDeclarators())
			{
			fprintf(mFp, "      <BodyVarDecl name=\"%s\" type=\"%d\" "
			    "const=\"%s\" ref=\"%s\" />\n",
			    decl->getName().c_str(),
			    getObjectModelId(decl->getDeclType()->getName()),
			    boolStr(decl->isConst()), boolStr(decl->isRefer()));
			}
		    writeStatements(&oper->getCondStatements(), 0);

		    fprintf(mFp, "    </Operation>\n");
		    }
		}
            }
        fprintf(mFp, "  </%s>\n", typeName);
        }
}

void ModelWriter::writeAssociation(const ModelAssociation &assoc)
{
    if(mFp)
        {
        fprintf(mFp, "  <Generalization xmi.id=\"%d\" child=\"%d\" parent=\"%d\" "
            "access=\"%s\" />\n",
            newModelId(), getObjectModelId(assoc.getChild()->getName()),
            getObjectModelId(assoc.getParent()->getName()), assoc.getAccess().asStr());
        }
}

bool ModelWriter::writeFile(char const * const filename)
{
    bool success = openFile(filename);
    if(success)
        {
        int moduleXmiId=MIO_Module;
        fprintf(mFp, "  <Module xmi.id=\"%d\" module=\"%s\" >\n",
            moduleXmiId, mModelData.mModules[0]->getModulePath().c_str());
        fprintf(mFp, "  </Module>\n");
        for(auto &type : mModelData.mTypes)
            {
	    ModelClassifier *cl = ModelObject::getClass(type);
	    if(cl)
		{
		if(!(cl->getOutput() & ModelClassifier::O_DefineOperations))
		    cl->clearOperations();
		if(!(cl->getOutput() & ModelClassifier::O_DefineAttributes))
		    cl->clearAttributes();
		}
	    writeType(*type);
            }
        for(auto &assoc : mModelData.mAssociations)
            writeAssociation(*assoc);
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

