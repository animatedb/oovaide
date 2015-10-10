/*
 * OovDatabaseWriter.cpp
 * Created on: September 25, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovDatabase.h"
#include "Components.h"
#include "Project.h"
#include "stdio.h"
#include "ModelObjects.h"
#include "FilePath.h"   // For FileDelete.
#include "OovDatabaseWriter.h"

// SQL possible order of evaluation:
//    FROM
//    WHERE
//    GROUP BY
//    HAVING
//    SELECT
//    ORDER BY

// EXAMPLE QUERIES:
//      For SQLite command line utility, everything must terminate with a semicolon.
//
// Find module for class "Builder".
//      select * from Module where idModule=(select idOwningModule from Type
//      where name="Builder")
//
// Find all methods that use class "Builder".
//      select * from Method where idOwningType=(select idType from Type
//      where name = "Builder")
//
// Find all methods that use class "Builder", returns data from both tables
//      select * from Method inner join Type on Method.idOwningType=Type.idType
//      where Type.name="Builder"
//
// Find all methods of a particular type.
//      Destructors:            select * from Method where name like '~%'
//      Overloaded:             select * from Method where name like '%+;%'
//      Undefined/implicit:     select * from Method where name like '%#'
//
// Get the summed module lines for each component.
//      select Component.name, sum(Module.moduleLines) from Module inner join Component on
//      Component.idComponent=Module.idOwningComponent group by Component.idComponent
//      order by sum(Module.moduleLines)

// Get the external calls needed by some component.
//      select * from Method where idOwningModule==0 and idOwningType==1

/////////////////

class DbWriter
    {
    public:
        DbWriter():
            mModelData(nullptr)
            {}
        // The project directory is used to find the place to store the output.
        bool openDatabase(char const *projectDir, ModelData const *modelData);
        bool writeTypes(int passIndex, int &typeIndex, int maxTypesPerTransaction);
        bool writeComponentsInfo(ComponentTypesFile const* compTypesFile);
        void closeDatabase();
        OovStringRef getLastError() const
            { return mDb.getLastError(); }

    private:
        OovDatabase mDb;
        ModelData const *mModelData;

        bool writeTypesAndMethods(int &typeIndex, int maxTypesPerTransaction);
        // Write all type refs for the types specified by the index. Except associations.
        bool writeTypeRefs(int &typeIndex, int maxTypesPerTransaction);
        // Writes inheritance relations.
        bool writeAssociations();
        // Write aggregation and composition.
        bool writeDataMemberTypeRefs(ModelClassifier const *cls);
        // Write params and body variables.
        bool writeMethodTypeRefs(int idMethod, ModelOperation const *oper);
        bool writeTypeRefs(int methodId,
            std::vector<std::unique_ptr<ModelDeclarator>> const &decls,
            OovDatabase::eVarRelations varRel);
        bool writeStatement(ModelStatement const &stmt, int idMethod,
            int statementIndex);
    };

static DbWriter sDbWriter;


bool DbWriter::openDatabase(char const *projectDir, ModelData const *modelData)
    {
    Project::setProjectDirectory(projectDir);
    mModelData = modelData;
    mDb.close();
    FilePath dbFn = Project::getOutputDir();
    dbFn.appendFile("OovReports.db");
    OovStatus status = FileDelete(dbFn);
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to clear database file");
        }
#ifdef __linux__
    char const *libName="libsqlite3.so.0";
#else
    char const *libName="sqlite3.dll";
#endif
    bool success = mDb.loadDbLib(libName);
    if(success)
        {
        success = mDb.openDb(dbFn.getStr());
        if(success)
            {
            success = mDb.createTables();
            }
        else
            {
            OovString errStr = "Unable to open ";
            errStr += libName;
            mDb.setLastError(errStr);
            }
        }
    else
        {
        OovString errStr = "Unable to load ";
        errStr += dbFn;
        mDb.setLastError(errStr);
        }
    return success;
    }

bool DbWriter::writeComponentsInfo(ComponentTypesFile const* compTypesFile)
    {
    bool success = true;
    if(compTypesFile)
        {
        success = mDb.exec("begin");
        OovStringVec compNames = compTypesFile->getComponentNames();
        for(auto const &compName : compNames)
            {
            OovStringVec sources = compTypesFile->getComponentFiles(
                ComponentTypesFile::CFT_CppSource, compName);
            OovStringVec includes = compTypesFile->getComponentFiles(
                ComponentTypesFile::CFT_CppInclude, compName);
            sources.insert(sources.end(), includes.begin(), includes.end());
            int compId = 0;
            success = mDb.addComponent(compName, compId);
            if(success)
                {
                for(auto const &module : sources)
                    {
                    success = mDb.updateModuleWithComponent(module, compId);
                    if(!success)
                        {
                        break;
                        }
                    }
                }
            if(!success)
                {
                break;
                }
            }
        if(success)
            {
            success = mDb.exec("end");
            }
        }
    return success;
    }

bool DbWriter::writeTypes(int passIndex, int &typeIndex, int maxTypesPerTransaction)
    {
    bool success = true;
    if(passIndex == 0)
        {
        success = writeTypesAndMethods(typeIndex, maxTypesPerTransaction);
        }
    else if(passIndex == 1)
        {
        success = writeTypeRefs(typeIndex, maxTypesPerTransaction);
        }
    else
        {
        typeIndex++;
        }
    return success;
    }

bool DbWriter::writeTypesAndMethods(int &typeIndex, int maxTypesPerTransaction)
    {
    bool success = mDb.exec("begin");
    size_t highestIndex = 0;
    for(size_t i=typeIndex; i<mModelData->mTypes.size() && success; i++)
        {
        highestIndex = i;
        ModelType *typePtr = mModelData->mTypes[i].get();
        ModelClassifier *cls = ModelType::getClass(typePtr);
        int lineNum = UNDEFINED_INT;
        int classModuleId = UNDEFINED_INT;
        if(cls)
            {
            ModelModule const *module = cls->getModule();
            if(module)
                {
                ModelModuleLineStats const &stats = module->mLineStats;
                success = mDb.addModule(module->getName(), classModuleId,
                    stats.mNumCodeLines, stats.mNumCommentLines, stats.mNumModuleLines);
                }
            cls->getLineNum();
            }
        if(success)
            {
            int typeId = 0;
            success = mDb.addType(typePtr->getName(), classModuleId, lineNum, typeId);
            if(success && cls)
                {
                for(auto const &oper : cls->getOperations())
                    {
                    ModelOperation *operPtr = oper.get();

                    int methodModuleId = UNDEFINED_INT;
                    ModelModule const *module = operPtr->getModule();
                    if(module)
                        {
                        ModelModuleLineStats const &stats = module->mLineStats;
                        success = mDb.addModule(module->getName(), methodModuleId,
                            stats.mNumCodeLines, stats.mNumCommentLines, stats.mNumModuleLines);
                        }
                    if(success)
                        {
                        int unusedMethodId = UNDEFINED_INT;
                        success = mDb.addMethod(
                            operPtr->getOverloadFuncName(), operPtr->getLineNum(),
                            operPtr->getAccess().getVis(), operPtr->isConst(),
                            operPtr->isVirtual(), typeId, methodModuleId, unusedMethodId);
                        }
                    if(!success)
                        {
                        break;
                        }
                    }
                }
            }
        if(--maxTypesPerTransaction <= 0)
            {
            break;
            }
        }
    if(success)
        {
        success = mDb.exec("end");
        }
    if(success)
        {
        typeIndex = highestIndex;
        }
    return success;
    }

bool DbWriter::writeDataMemberTypeRefs(ModelClassifier const *cls)
    {
    bool success = true;
    for(size_t attri=0; attri<cls->getAttributes().size() && success; attri++)
        {
        auto const &attr = cls->getAttributes()[attri].get();
        OovStringRef memberName = attr->getDeclType()->getName();
        success = mDb.addTypeRelation(memberName, cls->getName(),
            attr->getAccess().getVis(), OovDatabase::TR_Member);
        }
    return success;
    }

bool DbWriter::writeTypeRefs(int idMethod,
        std::vector<std::unique_ptr<ModelDeclarator>> const &decls,
        OovDatabase::eVarRelations varRel)
    {
    bool success = true;
    for(auto const &var : decls)
        {
        int idSupplierType = UNDEFINED_INT;
        success = mDb.getTypeId(var->getDeclType()->getName(), true,
            idSupplierType);
        if(success)
            {
            success = mDb.addMethodTypeRef(var->getName(),
                varRel, idMethod, idSupplierType);
            }
        if(!success)
            {
            break;
            }
        }
    return success;
    }

bool DbWriter::writeStatement(ModelStatement const &stmt, int idMethod, int statementIndex)
    {
    bool success = true;
    eModelStatementTypes stType = stmt.getStatementType();
    int idSupplierType = UNDEFINED_INT;
    int idSupplierMethod = UNDEFINED_INT;
    switch(stType)
        {
        case ST_Call:
            {
            ModelType const *type = stmt.getClassDecl().getDeclType();
            if(type)
                {
                // Hide the supplier type since the method will
                // uniquely identify the class.
                int idSupplierClassTemp;
                success = mDb.getTypeId(type->getName(), true,
                    idSupplierClassTemp);
                success = mDb.getMethodId(idSupplierClassTemp,
                    stmt.getOverloadFuncName(), false, idSupplierMethod);
                // Some methods cannot be found in the class. For now, define
                // them with an ending '#' to indicate they were created.
                if(success && idSupplierMethod == UNDEFINED_INT)
                    {
                    idSupplierMethod = 0;
                    OovString name = stmt.getOverloadFuncName();
                    name += '#';
                    success = mDb.addMethod(name, 0, 0, 0, 0,
                        idSupplierClassTemp, 0, idSupplierMethod);
                    }
                }
            }
            break;

        case ST_VarRef:
            {
            ModelType const *type = stmt.getVarDecl().getDeclType();
            if(type)
                {
                success = mDb.getTypeId(type->getName(), true, idSupplierType);
                }
            }
            break;

        default:
            break;
        }
    if(success)
        {
        success = mDb.addStatement(stType, statementIndex, idMethod, idSupplierType,
            idSupplierMethod);
        }
    return success;
    }

bool DbWriter::writeMethodTypeRefs(int idMethod, ModelOperation const *oper)
    {
    bool success = true;
    success = writeTypeRefs(idMethod, oper->getParams(), OovDatabase::VR_Parameter);
    if(success)
        {
        success = writeTypeRefs(idMethod, oper->getBodyVarDeclarators(),
            OovDatabase::VR_BodyVariable);
        }
    if(success)
        {
        for(size_t i=0; i<oper->getStatements().size() && success; i++)
            {
            auto const &stmt = oper->getStatements()[i];
            success = writeStatement(stmt, idMethod, i);
            if(!success)
                {
                break;
                }
            }
        }
    return success;
    }

bool DbWriter::writeTypeRefs(int &typeIndex, int maxTypesPerTransaction)
    {
    bool success = mDb.exec("begin");
    size_t highestIndex = 0;
    if(success && typeIndex == 0)
        {
        success = writeAssociations();
        }
    if(success)
        {
        for(size_t typei=typeIndex; typei<mModelData->mTypes.size() && success; typei++)
            {
            highestIndex = typei;
            ModelType const *typePtr = mModelData->mTypes[typei].get();
            ModelClassifier const *cls = ModelType::getClass(typePtr);
            if(cls)
                {
                int idClass = UNDEFINED_INT;
                success = writeDataMemberTypeRefs(cls);
                if(success)
                    {
                    success = mDb.getTypeId(cls->getName(), true, idClass);
                    }
                if(success)
                    {
                    for(auto const &oper : cls->getOperations())
                        {
                        int idMethod = UNDEFINED_INT;
                        success = mDb.getMethodId(idClass,
                            oper->getOverloadFuncName(), true, idMethod);
                        if(success)
                            {
                            success = writeMethodTypeRefs(idMethod, oper.get());
                            }
                        if(!success)
                            {
                            break;
                            }
                        }
                    }
                }
            if(--maxTypesPerTransaction <= 0)
                {
                break;
                }
            }
        }
    if(success)
        {
        success = mDb.exec("end");
        }
    if(success)
        {
        typeIndex = highestIndex;
        }
    return success;
    }

bool DbWriter::writeAssociations()
    {
    bool success = true;
// The outer calling method already does a begin/end
//    bool success = mDb.exec("begin");
    for(size_t i=0; i<mModelData->mAssociations.size() && success; i++)
        {
        ModelAssociation *assoc = mModelData->mAssociations[i].get();
        const ModelClassifier *child = assoc->getChild();
        const ModelClassifier *parent = assoc->getParent();
        success = mDb.addTypeRelation(parent->getName(), child->getName(),
            assoc->getAccess().getVis(), OovDatabase::TR_Inheritance);
        }
    if(success)
        {
//        success = mDb.exec("end");
        }
    return success;
    }

void DbWriter::closeDatabase()
    {
    mDb.closeDb();
    }

extern "C"
{
SHAREDSHARED_EXPORT bool OpenDb(char const *projectDir, void const *modelData)
    { return sDbWriter.openDatabase(projectDir, static_cast<ModelData const*>(modelData)); }
SHAREDSHARED_EXPORT bool WriteDb(int passIndex, int &typeIndex, int maxTypesPerTransaction)
    { return sDbWriter.writeTypes(passIndex, typeIndex, maxTypesPerTransaction); }
SHAREDSHARED_EXPORT bool WriteDbComponentTypes(void const *compTypesFile)
    { return sDbWriter.writeComponentsInfo(static_cast<ComponentTypesFile const*>(compTypesFile)); }
SHAREDSHARED_EXPORT char const *GetLastError()
    { return sDbWriter.getLastError().getStr(); }
SHAREDSHARED_EXPORT void CloseDb()
    { sDbWriter.closeDatabase(); }
}

