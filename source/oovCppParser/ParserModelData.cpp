/*
 * ParserModelData.cpp
 *
 *  Created on: Aug 15, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ParserModelData.h"
#include "ModelWriter.h"

void ParserModelData::addParsedModule(OovStringRef fileName)
    {
    ModelModule *module = new ModelModule();;
    module->setModulePath(fileName);
    /// @todo - use make_unique when supported.
    mModelData.mModules.push_back(std::unique_ptr<ModelModule>(module));
    }

ModelModule const *ParserModelData::getParsedModule() const
    {
    return(mModelData.mModules[0].get());
    }

ModelType *ParserModelData::createOrGetBaseTypeRef(CXCursor cursor, RefType &rt)
    {
    CXType cursorType = clang_getCursorType(cursor);
    eModelDataTypes dType;
    switch(cursorType.kind)
        {
        case CXType_Record:
            dType = DT_Class;
            break;

        case CXType_Typedef:
            {
            // Normally this should already be defined, so won't create it.
            ModelType const *typeRef = createOrGetTypedef(cursor);
            if(typeRef)
                dType = typeRef->getDataType();
            else
                dType = DT_DataType;
            }
            break;

        /// For members that are base on templates, the cursorType.kind is
        /// CXType_Unexposed, so get the type declaration, and the type is
        /// CXCursor_ClassDecl.
        /// Just treat any other unexposed data as a simple datatype.
        case CXType_Unexposed:
            {
            CXCursor tc = clang_getTypeDeclaration(cursorType);
            if(tc.kind == CXCursor_ClassDecl)
                {
                dType = DT_Class;
                }
            else
                {
                dType = DT_DataType;
                }
            }
            break;

        default:
            dType = DT_DataType;
            break;
        }
    rt.isConst = isConstType(cursor);
    switch(cursorType.kind)
        {
        case CXType_LValueReference:
        case CXType_RValueReference:
        case CXType_Pointer:
            rt.isRef = true;
            break;

        default:
            break;
        }
    std::string typeName = getFullBaseTypeName(cursor);

#if(DEBUG_PARSE)
    if(sLog.mFp && !mModelData.getTypeRef(typeName))
        fprintf(sLog.mFp, "    Create Type Ref: %s\n", typeName.c_str());
#endif
    return mModelData.createOrGetTypeRef(typeName, dType);
    }

ModelType *ParserModelData::createOrGetDataTypeRef(CXType cursType, RefType &rt)
    {
    ModelType *type = nullptr;
    rt.isConst = isConstType(cursType);
    switch(cursType.kind)
        {
        case CXType_LValueReference:
        case CXType_RValueReference:
        case CXType_Pointer:
            rt.isRef = true;
            break;

        default:
            break;
        }
//    CXCursor retTypeDeclCursor = clang_getTypeDeclaration(cursType);
    CXStringDisposer retTypeStr(clang_getTypeSpelling(cursType));
//    if(retTypeDeclCursor.kind != CXCursor_NoDeclFound)
//      {
//      type = mModelData.createOrGetTypeRef(retTypeStr, DT_Class);
//      }
//    else
        {
        type = mModelData.createOrGetTypeRef(retTypeStr, DT_DataType);
        }
    return type;
    }

// This upgrades an otDatatype to an otClass in order to handle forward references
ModelClassifier *ParserModelData::createOrGetClassRef(OovStringRef const name)
    {
    ModelClassifier *classifier = nullptr;
#if(DEBUG_PARSE)
    if(sLog.mFp && !mModelData.getTypeRef(name))
        fprintf(sLog.mFp, "    Create Type Class: %s\n", name.getStr());
#endif
    ModelType *type = mModelData.createOrGetTypeRef(name, DT_Class);
    if(type->getDataType() == DT_Class)
        {
        classifier = static_cast<ModelClassifier*>(type);
        }
    else
        {
        classifier = static_cast<ModelClassifier*>(mModelData.createTypeRef(name, DT_Class));
        mModelData.replaceType(type, classifier);
        }
    return classifier;
    }

ModelType *ParserModelData::createOrGetDataTypeRef(CXCursor cursor)
    {
    std::string typeName = getFullBaseTypeName(cursor);
    return mModelData.createOrGetTypeRef(typeName, DT_DataType);
    }

ModelType *ParserModelData::createOrGetTypedef(CXCursor cursor)
    {
    std::string typedefName = getFullBaseTypeName(cursor);
    ModelType *typeRef = nullptr;
    CXType cursorType = clang_getTypedefDeclUnderlyingType(cursor);
    CXType baseType = getBaseType(cursorType);
    if(baseType.kind == CXType_Record)
        {
        typeRef = createOrGetClassRef(typedefName);
        }
    else
        {
        typeRef = mModelData.findType(typedefName);
        }
    return typeRef;
    }

void ParserModelData::addAssociation(ModelClassifier const *parent,
        ModelClassifier const *child, Visibility access)
    {
    if(child && parent)
        {
        ModelAssociation *assoc = new ModelAssociation(child, parent,
            access);
        /// @todo - use make_unique when supported.
        mModelData.mAssociations.push_back(std::unique_ptr<ModelAssociation>(assoc));
        }
    }

bool ParserModelData::isBaseClass(ModelClassifier const *child,
        OovStringRef const name) const
    {
    bool isBase = false;
    ConstModelClassifierVector classes;
    mModelData.addBaseClasses(*child, classes);
    for(auto const &cls : classes)
        {
        OovString const &baseName = cls->getName();
        // The caller class may have a namespace, so discard and compare.
        size_t pos = baseName.rfind(':');
        if(pos != std::string::npos)
            {
            pos++;
            }
        else
            {
            pos = 0;
            }
        if(baseName.compare(pos, std::string::npos, name) == 0)
            {
            isBase = true;
            break;
            }
        }
    return isBase;
    }

void ParserModelData::setLineStats(ModelModuleLineStats const &lineStats)
    {
    mModelData.mModules[0].get()->mLineStats = lineStats;
    }

void ParserModelData::writeModel(OovStringRef fileName)
    {
    ModelWriter writer(mModelData);
    writer.writeFile(fileName);
    }
