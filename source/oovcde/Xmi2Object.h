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
            mModel(model), mCurrentClassifier(NULL)
            {}
    public:
	bool parse(char const * const buf);

    private:
        ModelData &mModel;
        std::vector<ModelObject*> mElementStack;
        ModelClassifier *mCurrentClassifier;
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
    };

bool loadXmiFile(FILE *fp, ModelData &model, char const * const fn);

#endif

