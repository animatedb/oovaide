/*
 * OovDatabase.h
 * Created on: September 30, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOV_DATABASE_H
#define OOV_DATABASE_H

#include "SQLiteImport.h"
#include "OovString.h"

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
        bool addModule(char const *name, int &moduleId);

        bool getTypeId(char const *name, bool failMissing, int &id);
        bool addType(char const *name, int moduleId, int lineNum, int &typeId);
        // This must be done after both types already exist.
        enum eTypeRelations { TR_Inheritance, TR_Member };
        bool addTypeRelation(char const *supplierName, char const *consumerName,
            int visibility, eTypeRelations rt);

        bool getMethodId(int idClass, char const *name, bool failMissing, int &id);
        bool addMethod(char const *name, int lineNum, int visibility,
            bool isConst, bool isVirt, int owningTypeId, int owningModuleId,
            int &methodId);
        enum eVarRelations { VR_Parameter, VR_BodyVariable };
        bool addMethodTypeRef(char const *identName, eVarRelations varRel,
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
