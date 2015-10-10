/*
 * OovDatabase.h
 * Created on: September 30, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOV_DATABASE_H
#define OOV_DATABASE_H

#include "SQLiteImport.h"
#include "DbString.h"

#define UNDEFINED_INT -1

class OovDatabase:public SQLite, public SQLiteListener
    {
    public:
        OovDatabase()
            {
            setListener(this);
            }
        virtual ~OovDatabase()
            {}
        bool createTables();
//        bool initIntegrity()
//            { return exec("PRAGMA foreign_keys = ON"); }
        bool getModuleId(OovStringRef name, bool failMissing, int &typeId);
        bool addModule(OovStringRef name, int &moduleId, int codeLines,
            int commentLines, int moduleLines);

        bool addComponent(OovStringRef name, int &componentId);
        bool updateModuleWithComponent(OovStringRef name, int componentId);

        bool getTypeId(OovStringRef name, bool failMissing, int &id);
        bool addType(OovStringRef name, int moduleId, int lineNum, int &typeId);
        // This must be done after both types already exist.
        enum eTypeRelations { TR_Inheritance, TR_Member };
        bool addTypeRelation(OovStringRef supplierName, OovStringRef consumerName,
            int visibility, eTypeRelations rt);

        bool getMethodId(int idClass, OovStringRef name, bool failMissing, int &id);
        bool addMethod(OovStringRef name, int lineNum, int visibility,
            bool isConst, bool isVirt, int owningTypeId, int owningModuleId,
            int &methodId);
        enum eVarRelations { VR_Parameter, VR_BodyVariable };
        bool addMethodTypeRef(OovStringRef identName, eVarRelations varRel,
            int idOwningMethod, int idSupplierType);
        bool addStatement(int statementType, int lineNum, int idOwningMethod,
            int idSupplierClass, int idSupplierMethod);
/*
        bool removeExtraTypes();
        bool removeExtraMethods(int idType);
        bool removeExtraStatements(int idMethod);
*/
        bool exec(OovStringRef sqlStr);
        /// Returns id of UNDEFINED_INT if no matching ids were returned.
        /// The search should be done on unique columns.
        bool execSelectId(OovStringRef idColName, OovStringRef table,
                OovStringRef srchColName, OovStringRef srchColVal, bool failMissing, int &id);
        bool execSelectLastInsertedRowId(int &id);
        OovStringRef getLastError() const
            { return mLastError; }
        void setLastError(OovStringRef errStr)
            { mLastError = errStr; }
        bool haveResults() const
            { return(mLastResults.size() > 0); }

    private:
        std::vector<OovString> mLastResults;
        OovString mLastError;
        OovString mSqlError;

        bool addName(OovStringRef colIdName, OovStringRef tableName,
                OovStringRef nameColName, OovStringRef nameValName, int &retId);
        bool addRecord(OovStringRef colIdName, OovStringRef tableName,
            OovStringRef srchColName, OovStringRef srchColVal,
            DbNames insertColNames, DbValues insertColValues, int &retId);
        virtual void SQLError(int retCode, char const *errMsg) override
            {
            mSqlError = errMsg;
            }
        virtual void SQLResultCallback(int numColumns, char **colVal,
            char **colName) override
            {
            mLastResults.resize(numColumns);
            for(int i=0; i<numColumns; i++)
                {
                mLastResults[i] = colVal[i] ? colVal[i] : "NULL";
                }
            }
    };

#endif
