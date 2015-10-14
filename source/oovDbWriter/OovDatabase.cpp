/*
 * OovDatabase.cpp
 * Created on: September 30, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovDatabase.h"
#include "DbString.h"
#include <limits>


bool OovDatabase::exec(OovStringRef sqlStr)
    {
    mLastResults.clear();
    bool success = SQLite::execDb(sqlStr);
    if(!success)
        {
        OovString errStr = sqlStr;
        errStr += " : ";
        errStr += mSqlError;
        setLastError(errStr);
        }
    return success;
    }

bool OovDatabase::execSelectId(OovStringRef colName, OovStringRef table,
        OovStringRef srchColName, OovStringRef srchColVal, bool failMissing, int &id)
    {
    DbString query;
    id = UNDEFINED_INT;
    query.SELECT(colName).FROM(table).WHERE(srchColName, "=", DbValue(srchColVal));
    bool success = exec(query.getDbStr());
    if(success && haveResults())
        {
        mLastResults[0].getInt(0, std::numeric_limits<int>::max(), id);
        }
    else if(failMissing)
        {
        OovString str = "Missing id for exec, table ";
        str += table;
        str += " column ";
        str += srchColName;
        str += " value ";
        str += srchColVal;
        setLastError(str);
        success = false;
        }
    return success;
    }

bool OovDatabase::execSelectLastInsertedRowId(int &id)
    {
    DbString rowQuery;
    id = UNDEFINED_INT;
    rowQuery.SELECT("last_insert_rowid()");
    bool success = exec(rowQuery.getDbStr());
    if(success && haveResults())
        {
        mLastResults[0].getInt(0, std::numeric_limits<int>::max(), id);
        }
    return success;
    }


bool OovDatabase::createTables()
    {
    // In SQLite, primary keys are not null.
    // In SQLite, primary keys must be INTEGER and not INT.
    // Referential integrity is not checked unless the pragma:
    //      PRAGMA foreign_keys = ON
    char const *createStrs[] =
        {
        // Modules are files that contain code.
        "CREATE TABLE IF NOT EXISTS Module("
            "idModule INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "codeLines INT NOT NULL,"
            "commentlines INT NOT NULL,"
            "moduleLines INT NOT NULL,"
            "idOwningComponent INTEGER"         // This is null because it is updated later.
            ");",

        // Types are either simple types or classes.
        "CREATE TABLE IF NOT EXISTS Type("
            "idType INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "idOwningModule INT NOT NULL,"
            "lineNumber INT NOT NULL"
//            FOREIGN KEY(idModule) REFERENCES Modules(idModule)
            ");",

        // Methods are either methods or functions. Functions are treated as
        // methods that belong to the class with an empty name.
        "CREATE TABLE IF NOT EXISTS Method("
            "idMethod INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "lineNumber INT NOT NULL,"
            "visibility INT NOT NULL,"
            "const BOOL NOT NULL,"
            "virtual BOOL NOT NULL,"
            "idOwningType INT NOT NULL,"
            "idOwningModule INT NOT NULL"
// This cannot be done since a file can have a type removed, then the
// dependent file has to report that the type is missing.
//            "FOREIGN KEY(idOwningType) REFERENCES Type(idType)"
//            "FOREIGN KEY(idOwningModule) REFERENCES idOwningModule(idModule)"
            ");",

        // Only statements needed for sequence diagrams are saved along with
        // references to types.  This includes conditional (and loop)
        // statements, and begin end blocks for conditionals.
        "CREATE TABLE IF NOT EXISTS Statement("
            "idStatement INTEGER PRIMARY KEY NOT NULL,"
            "statementDescription INT NOT NULL,"
            "lineNumber INT NOT NULL,"
            "idOwningMethod INT NOT NULL,"
            "idSupplierClass INT NOT NULL,"
            "idSupplierMethod INT NOT NULL"
            ");",

        // This is for inheritance and aggregation.
        "CREATE TABLE IF NOT EXISTS TypeRelation("
            "idTypeRelation INTEGER PRIMARY KEY NOT NULL,"
            "typeRelationDescription INT NOT NULL,"
            "visibility INT NOT NULL,"
            "idSupplierType INT NOT NULL,"
            "idConsumerType INT NOT NULL"
            ");",

        // This is for method parameters and returns.
        "CREATE TABLE IF NOT EXISTS MethodTypeRef("
            "idMethodTypeRef INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "varRelationDescription INT NOT NULL,"
            "idOwningMethod INT NOT NULL,"
            "idSupplierType INT NOT NULL"
            ");",

        // Components are directories that contain modules.
        "CREATE TABLE IF NOT EXISTS Component("
            "idComponent INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL"
            ");",
        };
    bool success = exec("begin");
    if(success)
        {
        for(auto const *createStr : createStrs)
            {
            success = exec(createStr);
            if(!success)
                {
                break;
                }
            }
        if(success)
            {
            success = exec("end");
            }
        }
    return success;
    }

bool OovDatabase::addRecord(OovStringRef colIdName, OovStringRef tableName,
    OovStringRef srchColName, OovStringRef srchColVal,
    DbNames insertColNames, DbValues insertColValues, int &retId)
    {
    bool success = execSelectId(colIdName, tableName, srchColName, srchColVal,
        false, retId);
    if(success && retId == UNDEFINED_INT)
        {
        DbString ins;
        ins.INSERT(tableName).INTO(insertColNames).VALUES(insertColValues);
        success = exec(ins.getDbStr());
        if(success)
            {
            success = execSelectLastInsertedRowId(retId);
            }
        }
    return success;
    }

bool OovDatabase::addName(OovStringRef colIdName, OovStringRef tableName,
    OovStringRef nameColName, OovStringRef nameValName, int &retId)
    {
    return addRecord(colIdName, tableName, nameColName, nameValName,
        {colIdName, nameColName}, {nullptr, nameValName.getStr()}, retId);
    }

bool OovDatabase::addComponent(OovStringRef name, int &componentId)
    {
    return addName("idComponent", "Component", "name", name, componentId);
    }

bool OovDatabase::updateModuleWithComponent(OovStringRef name, int componentId)
    {
    int moduleId;
    bool success = getModuleId(name, false, moduleId);
    if(success)
        {
        DbString dbStr;
        dbStr.UPDATE("Module").SET({"idOwningComponent"},
                {componentId}).WHERE("idModule", "=", moduleId);
        success = exec(dbStr.getDbStr());
        }
    return success;
    }

bool OovDatabase::getModuleId(OovStringRef name, bool failMissing, int &moduleId)
    {
    return execSelectId("idModule", "Module", "name", name, failMissing, moduleId);
    }

bool OovDatabase::addModule(OovStringRef name, int &moduleId, int codeLines,
    int commentLines, int moduleLines)
    {
    return addRecord("idModule", "Module", "name", name,
        {"idModule", "name", "codeLines", "commentLines", "moduleLines"},
        { nullptr, name.getStr(), codeLines, commentLines, moduleLines },
        moduleId);
    }

bool OovDatabase::getTypeId(OovStringRef name, bool failMissing, int &typeId)
    {
    return execSelectId("idType", "Type", "name", name, failMissing, typeId);
    }

bool OovDatabase::addType(OovStringRef name, int moduleId, int lineNum,
    int &typeId)
    {
    bool success = getTypeId(name, false, typeId);
    if(success && typeId == UNDEFINED_INT)
        {
        DbString ins;
        ins.INSERT("Type").INTO({"idType","name","idOwningModule","lineNumber"}).
                VALUES({nullptr, name.getStr(), moduleId, lineNum});
        success = exec(ins.getDbStr());
        if(success)
            {
            success = execSelectLastInsertedRowId(typeId);
            }
        }
    return success;
    }

bool OovDatabase::addTypeRelation(OovStringRef supplierName, OovStringRef consumerName,
    int visibility, eTypeRelations tr)
    {
    int idSupplier = UNDEFINED_INT;
    int idConsumer = UNDEFINED_INT;
    bool success = execSelectId("idType", "Type", "name", supplierName, true, idSupplier);
    if(success && idSupplier != UNDEFINED_INT)
        {
        success = execSelectId("idType", "Type", "name", consumerName, true, idConsumer);
        }
    if(success)
        {
        DbString ins;
        ins.INSERT("TypeRelation").INTO({"idTypeRelation",
                "typeRelationDescription","visibility","idSupplierType","idConsumerType"}).
                VALUES({nullptr, tr, visibility, idSupplier, idConsumer});
        success = exec(ins.getDbStr());
        }
    return success;
    }

bool OovDatabase::getMethodId(int idClass, OovStringRef name, bool failMissing, int &methodId)
    {
    return execSelectId("idMethod", "Method", "name", name, failMissing, methodId);
    }

bool OovDatabase::addMethod(OovStringRef name, int lineNum, int visibility,
    bool isConst, bool isVirt, int owningTypeId, int owningModuleId, int &methodId)
    {
    DbString ins;
    methodId = UNDEFINED_INT;
    bool success = getMethodId(owningTypeId, name, false, methodId);
    if(success && methodId == UNDEFINED_INT)
        {
        ins.INSERT("Method").INTO({"idMethod","name","lineNumber",
            "visibility", "const", "virtual", "idOwningType", "idOwningModule"}).
            VALUES({nullptr, name.getStr(), lineNum, visibility, isConst, isVirt,
            owningTypeId, owningModuleId});
        success = exec(ins.getDbStr());
        if(success)
            {
            success = execSelectLastInsertedRowId(methodId);
            }
        }
    return success;
    }

bool OovDatabase::addMethodTypeRef(OovStringRef identName, eVarRelations varRel,
    int idOwningMethod, int idSupplierType)
    {
    DbString ins;
    ins.INSERT("MethodTypeRef").INTO({"idMethodTypeRef","name",
        "varRelationDescription","idOwningMethod", "idSupplierType"}).
        VALUES({nullptr, identName.getStr(), varRel, idOwningMethod, idSupplierType});
    return exec(ins.getDbStr());
    }

bool OovDatabase::addStatement(int statementType, int lineNum, int idOwningMethod,
    int idSupplierClass, int idSupplierMethod)
    {
    DbString ins;
    ins.INSERT("Statement").INTO({"idStatement", "statementDescription",
        "lineNumber","idOwningMethod","idSupplierClass", "idSupplierMethod"}).
        VALUES({nullptr,statementType, lineNum, idOwningMethod,
        idSupplierClass, idSupplierMethod});
    return exec(ins.getDbStr());
    }



/// Old interface code for ODBC and MySql
/*
#if(0)
#define OTL_ODBC
#include "otlv4.h"
#include "OovString.h"
#ifdef __linux__
#else
#include <winsock2.h>
#endif
#include <mysql.h>

// otl?
// odbc driver for sqlite?
// Deploy mysql app without password?

// MySql Workbench
//  DataBase/Connect To Database
//  File/Open SQL Script

// mysql in bin
//  login:              myswl -u root -proot
//  create database:    mysql -u root -proot oovReports < oovReports.sql
// mysql prompt
//  show databases

class ReportConnection
    {
    public:
        ReportConnection():
            mConnection(nullptr)
            {}
        ~ReportConnection()
            { mysql_close(mConnection); }
        bool connect();
        MYSQL *getConnection()
            { return mConnection; }

    private:
        MYSQL *mConnection;
    };

bool ReportConnection::connect()
    {
    mConnection = mysql_init(nullptr);
    if(mConnection)
        {
        mysql_query(mConnection, "CREATE DATABASE oovReports");
        }
    return(mConnection != nullptr);
    }
#endif



#if(0)
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>


class ReportConnection
    {
    public:
        ReportConnection():
            mConnection(nullptr)
            {}
        ~ReportConnection()
            { delete mConnection; }
        bool connect(oovStringRef user, OovStringRef password,
            OovStringRef dbName);
        sql::Connection *getConnection()
            { return mConnection; }

    private:
        sql::Connection *mConnection;
    };


bool ReportConnection::connect(oovStringRef user, OovStringRef password,
    OovStringRef dbName)
    {
    bool success = false;
    try
        {
        sql::Driver *driver = get_driver_instance();
        if(driver)
            {
            mConnection = driver->connect("tcp://127.0.0.1:3306", user, password);
            if(mConnection)
                {
                mConnection->setSchema(dbName);
                success = true;
                }
        }
    catch (sql::SQLException &e)
        {
        // MySQL error code: " << e.getErrorCode()
        }
    return success;
    }

int main(int argc, char const * const argv[])
    {
    ReportConnection conn;
    conn.connect("root", "root", "mydb");
    }


/////////////

class ReportDb
    {
    public:
        bool connect();

    private:
        sql::Driver *mDriver;
        sql::Connection *mConnection;
    };

ReportDb::connect()
    {
    ReportConnection connection;
    if(connection.connect())
        {
        }
    }
#endif
*/
