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
*  @file      xmiWriter.h
*
*  @date      2/1/2013
*
*/


#include <stdio.h>
#include "ModelObjects.h"
#include "File.h"


/**
* @xrefitem oovCppParse "CPP Parser" "CPP Parser Requirement List" R02.3
*/
class ModelWriter
{
public:
    ModelWriter(const ModelData &modelData):
        mModelData(modelData)
        {}
    bool writeFile(OovStringRef const filename);
    ~ModelWriter();

private:
    File mFile;
    const ModelData &mModelData;

    bool openFile(OovStringRef const filename);
    int getObjectModelId(const std::string &name);
    bool writeType(const ModelType &type);
    bool writeClassDefinition(const ModelClassifier &classifier, bool isClassDef);
    bool writeOperation(ModelClassifier const &classifier, ModelOperation const &oper);
    bool writeStatements(const ModelStatements &stmts);
    bool writeAssociation(const ModelAssociation &assoc);
};
