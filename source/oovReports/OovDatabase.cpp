/*
 * OovDatabase.cpp
 * Created on: September 25, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "SQLiteImport.h"
#include "stdio.h"
#include "OovString.h"
#include <limits>


class OovDatabase:public SQLite, public SQLiteListener
    {
    public:
        OovDatabase()
            {
            setListener(this);
            }
        void createTables();
        void initIntegrity()
            { exec("PRAGMA foreign_keys = ON"); }
        /// Return is the last inserted row ID (primary key ID).
        int insertClass(char const *name, int moduleId, int lineNum);
        int insertMethod(char const *name, int overloadKey, int lineNum,
            int visibility, bool isConst, bool isVirt, int owningClassId,
            int owningModuleId);

    private:
        std::vector<OovString> mLastResults;

        virtual void SQLError(int retCode, char const *errMsg) override
            {
            printf("%s\n", errMsg);
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

void OovDatabase::createTables()
    {
    // In SQLite, primary keys are not null.
    // In SQLite, primary keys must be INTEGER and not INT.
    // Referential integrity is not checked unless the pragma:
    //      PRAGMA foreign_keys = ON
    char const *createStrs[] = 
        {
        "CREATE TABLE IF NOT EXISTS Class("
            "idClass INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "idOwningModule INT NOT NULL,"
            "lineNumber INT NOT NULL"
//            FOREIGN KEY(idModule) REFERENCES Modules(idModule)
            ");",

        "CREATE TABLE IF NOT EXISTS Method("
            "idMethod INTEGER PRIMARY KEY NOT NULL,"
            "name VARCHAR NOT NULL,"
            "overloadKey INT NOT NULL,"
            "lineNumber INT NOT NULL,"
            "visibility INT NOT NULL,"
            "const BOOL NOT NULL,"
            "virtual BOOL NOT NULL,"
            "idOwningClass INT NOT NULL,"
            "idOwningModule INT NOT NULL,"
            "FOREIGN KEY(idOwningClass) REFERENCES Class(idClass)"
//            "FOREIGN KEY(idOwningClass) REFERENCES idOwningModule(idModule)"
            ");"
        };
    for(auto const *createStr : createStrs)
        {
        exec(createStr);
        }
    }

class DbString:public OovString
    {
    public:
        DbString()
            {}
        DbString(char const * const str):
            OovString(str)
            {}
        void appendString(OovStringRef str)
            {
            append("\"");
            append(str);
            append("\"");
            }
    };

int OovDatabase::insertClass(char const *name, int moduleId, int lineNum)
    {
    DbString ins = "INSERT INTO Class (idClass,name,idOwningModule,lineNumber) "
        " VALUES (";
    ins += "NULL,";
    ins.appendString(name);
    ins += ",";
    ins.appendInt(moduleId);
    ins += ",";
    ins.appendInt(lineNum);
    ins += ");";
    exec(ins.c_str());
    exec("SELECT last_insert_rowid()");
    int lastRowId = 0;
    mLastResults[0].getInt(0, std::numeric_limits<int>::max(), lastRowId);
    return lastRowId;
    }

int OovDatabase::insertMethod(char const *name, int overloadKey, int lineNum,
    int visibility, bool isConst, bool isVirt, int owningClassId,
    int owningModuleId)
    {
    DbString ins = "INSERT INTO Method (idMethod,name,overloadKey,lineNumber,"
        "visibility, const, virtual, idOwningClass, idOwningModule) VALUES (";
    ins += "NULL,";
    ins.appendString(name);
    ins += ",";
    ins.appendInt(overloadKey);
    ins += ",";
    ins.appendInt(lineNum);
    ins += ",";
    ins.appendInt(visibility);
    ins += ",";
    ins.appendInt(isConst);
    ins += ",";
    ins.appendInt(isVirt);
    ins += ",";
    ins.appendInt(owningClassId);
    ins += ",";
    ins.appendInt(owningModuleId);
    ins += ");";
    exec(ins.c_str());
    exec("SELECT last_insert_rowid()");
    int lastRowId = 0;
    mLastResults[0].getInt(0, std::numeric_limits<int>::max(), lastRowId);
    return lastRowId;
    }


int main(int argc, char const * const argv[])
    {
    OovDatabase db;
    if(db.open("OovReports.db"))
        {
        db.createTables();
        db.initIntegrity();
        int classId = db.insertClass("FooClass", 1, 1);
        db.insertMethod("FooMethod", 0, 1, 1, 0, 0, classId, 1);
        }
    }


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
