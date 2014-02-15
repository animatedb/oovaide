/**
*   Copyright (C) 2008 by dcblaha
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
*  @file      xmi2object.h
*
*  @author    dcblaha
*
*  @date      2/1/2009
*
*/

#ifndef XMI2OBJECT_H
#define XMI2OBJECT_H

#include "XmlParser.h"
#include "ModelObjects.h"
#include <map>
#include <vector>
#include <string>

typedef std::string tString;


class XmiParser:private XmlParser
    {
    public:
        XmiParser(ModelData &model):
            mModel(model), mCurrentClassifier(NULL), mStartingModuleTypeIndex(0),
            mNumModuleTypeIndices(0)
            {}
    public:
	bool parse(char const * const buf);
	// Since each file only has indices relative to the file, they
	// must be remapped to a global indices so that the references can be
	// resolved later.  It is the responsibility of the caller to start the
	// first file at zero, then get the next type index when the file is
	// completed parsing and use that for the starting index of the next module.
	void setStartingTypeIndex(int index)
	    { mStartingModuleTypeIndex = index; }
	int getNextTypeIndex() const
	    { return mNumModuleTypeIndices+1; }

    private:
        ModelData &mModel;
        std::vector<ModelObject*> mElementStack;
        ModelClassifier *mCurrentClassifier;
        int mStartingModuleTypeIndex;
        int mNumModuleTypeIndices;
        // First is index from current module or the original index. Second is
        // index from previous module or the new index that it will be changed to
        std::map<int, int> mFileTypeIndexMap;
        // These are the types that were loaded from the current module that
        // may need to have indices remapped.
        std::vector<ModelType*> mPotentialRemapIndicesTypes;
        void updateDeclTypeIndices(ModelDeclarator &decl);
        void updateStatementTypeIndices(ModelStatement *stmt);
        void updateTypeIndices();
        virtual void onOpenElem(char const * const name, int len);
        virtual void onCloseElem(char const * const /*name*/, int /*len*/);
        virtual void onAttr(char const * const name, int &nameLen,
        	char const * const val, int &valLen);
        void addClass(const ModelClassifier *obj);
        void addAttrs(const ModelClassifier *obj);
        void addOpers(const ModelClassifier *obj);
        void dumpTypeMap(char const * const str1, char const * const str2);
        ModelObject *findParentInStack(ObjectType type);
        ModelCondStatements *findStatementsParentInStack();
        void setDeclAttr(const std::string &attrName,
        	const std::string &attrVal, ModelDeclarator &decl);
    };

bool loadXmiFile(FILE *fp, ModelData &model, char const * const fn, int &typeIndex);

#endif

