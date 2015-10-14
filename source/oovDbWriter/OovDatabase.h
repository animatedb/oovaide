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
        /// Create all of the tables needed for the database.
        bool createTables();
//        bool initIntegrity()
//            { return exec("PRAGMA foreign_keys = ON"); }
        /// Query the Module table by name to find the module ID.
        /// @param name The module name to search for.
        /// @param failMissing Set true to set an error string and fail the return.
        /// @param moduleId The returned module ID.
        bool getModuleId(OovStringRef name, bool failMissing, int &moduleId);
        /// Add a module in the Module table.
        /// @param name The name to add.
        /// @param moduleId The returned module ID.
        /// @param codeLines The number of lines of code for the module.
        /// @param commentLines The number of lines of comments for the module.
        /// @param moduleLines The total number of lines for the module.
        bool addModule(OovStringRef name, int &moduleId, int codeLines,
            int commentLines, int moduleLines);

        /// Add a component to the Component table.
        /// @param name The component name to add.
        /// @param componentId The returned component ID.
        bool addComponent(OovStringRef name, int &componentId);
        /// Update the owning component in the Module table.
        /// @param name The module name.
        /// @param componentId The owning component.
        bool updateModuleWithComponent(OovStringRef name, int componentId);

        /// Query the Type table by name to find the type ID.
        /// @param name The type name.
        /// @param failMissing Set true to set an error string and fail the return.
        /// @param id The returned type ID.
        bool getTypeId(OovStringRef name, bool failMissing, int &id);
        /// Add a type to the Type table.
        /// @param name The name of the type to add.
        /// @param moduleId The module ID of the type's definition.
        /// @param lineNum The line number in the module where the type is defined.
        /// @param typeId The returned type ID.
        bool addType(OovStringRef name, int moduleId, int lineNum, int &typeId);
        enum eTypeRelations { TR_Inheritance, TR_Member };
        /// This must be done after both types already exist.
        /// Adds a type relation to the TypeRelation table.
        /// @param supplierName The supplier of the relation.
        /// @param consumerName The consumer of the relation. The consumer is
        ///     dependent on the supplier.
        /// @param visibility The Visibility values are defined in the
        ///     ModelObjects file.
        /// @param rt The type of relation (inheritance or member).
        bool addTypeRelation(OovStringRef supplierName, OovStringRef consumerName,
            int visibility, eTypeRelations rt);

        /// Query the Method table by name to find the method ID.
        /// @param idClass The ID of the class that defines the method.
        /// @param name The name of the method.
        /// @param failMissing Set true to set an error string and fail the return.
        /// @param id The returned method id.
        bool getMethodId(int idClass, OovStringRef name, bool failMissing, int &id);
        /// Add a method to the Method table.
        /// @param name The name of the method to add.
        /// @param lineNum The line number where the method is defined.
        /// @param visibility The Visibility values are defined in the
        ///     ModelObjects file.
        /// @parma isConst True if the method is const.
        /// @parma isConst True if the method is virtual.
        /// @param owningTypeId The ID of the class that defines the method.
        /// @param owningModuleId The ID of the module that contains the method.
        /// @param methodId The returned method ID.
        bool addMethod(OovStringRef name, int lineNum, int visibility,
            bool isConst, bool isVirt, int owningTypeId, int owningModuleId,
            int &methodId);
        enum eVarRelations { VR_Parameter, VR_BodyVariable };
        /// Adds a relation between a method and a type to the MethodTypeRef table.
        /// @param identName The instance name of the type.
        /// @param varRel The type of relation (parameter or body variable)
        /// @param idOwningMethod The ID of the method that refers to the type.
        /// @param idSupplierType The ID of the type of the variable.
        bool addMethodTypeRef(OovStringRef identName, eVarRelations varRel,
            int idOwningMethod, int idSupplierType);
        /// Add a statment for a method to the Statement table.
        /// @param statementType The eModelStatementTypes defined in ModelObjects.
        /// @param lineNum This is not the actual line number. Just a statement index.
        /// @param idOwningMethod The method that contains the statement.
        /// @param idSupplierClass The ID of the class that is referenced from the method.
        /// @param idSupplierMethod The ID of the method that is referenced.
        bool addStatement(int statementType, int lineNum, int idOwningMethod,
            int idSupplierClass, int idSupplierMethod);
/*
        bool removeExtraTypes();
        bool removeExtraMethods(int idType);
        bool removeExtraStatements(int idMethod);
*/
        /// Executes an SQL statement and sets an error if there is one.
        bool exec(OovStringRef sqlStr);
        /// Find an ID out of a table.
        /// Returns id of UNDEFINED_INT if no matching ids were returned.
        /// The search should be done on unique columns.
        /// @param idColName The name of the column that contains an ID.
        /// @param table The table to search.
        /// @param srchColName The name of the column to search by name.
        /// @param srchColVal The value used to search the column.
        /// @param failMissing Set true to set an error string and fail the return.
        /// @param id The returned ID.
        bool execSelectId(OovStringRef idColName, OovStringRef table,
                OovStringRef srchColName, OovStringRef srchColVal, bool failMissing, int &id);
        /// Run the last_insert_rowid() function from the database to get
        /// the last inserted row ID.
        /// @param id The ID of the last record that was inserted.
        bool execSelectLastInsertedRowId(int &id);
        /// Get the last error that was set when an error occurred.
        OovStringRef getLastError() const
            { return mLastError; }
        /// Set the last error.
        void setLastError(OovStringRef errStr)
            { mLastError = errStr; }
        /// Check if the last query returned any results.
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
