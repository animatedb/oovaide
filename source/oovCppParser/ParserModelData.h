/*
 * ParserModelData.h
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef PARSERMODELDATA_H_
#define PARSERMODELDATA_H_


#include "ModelObjects.h"
#include "ParseBase.h"
#include "OovString.h"


class ParserModelData
    {
    public:
	void addParsedModule(OovStringRef fileName);
	ModelModule const *getParsedModule() const;
	ModelClassifier *createOrGetClassRef(OovStringRef const name);
	ModelType *createOrGetBaseTypeRef(CXCursor cursor, RefType &rt);
	ModelType *createOrGetDataTypeRef(CXType type, RefType &rt);
	ModelType *createOrGetDataTypeRef(CXCursor cursor);
	ModelType *createOrGetTypedef(CXCursor cursor);
	bool isBaseClass(ModelClassifier const *child, OovStringRef const name) const;
	void addAssociation(ModelClassifier const *parent,
		ModelClassifier const *child, Visibility access);
	void setLineStats(ModelModuleLineStats const &lineStats);
	void writeModel(OovStringRef fileName);
	ModelData const &DebugGetModelData() const
	    { return mModelData; }

    private:
	 ModelData mModelData;
    };


#endif /* PARSERMODELDATA_H_ */
