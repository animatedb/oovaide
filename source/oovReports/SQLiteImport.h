/*
 * SQLiteImport.h
 *
 *  Created on: Sept. 28, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

// This file allows using an SQLite database without requiring any
// source files from the SQLite project.  This performs a run time load
// of the sqlite3.dll.

// The OovLibrary contains a minimal wrapper for loading run time libraries
// on Linux or Windows.
#include "OovLibrary.h"

// This is the C style interface to the run-time library.
extern "C"
{
typedef struct sqlite3 sqlite3;
typedef int (*SQLite_callback)(void*,int,char**,char**);

struct SQLiteInterface
    {
    int (*sqlite3_open)(const char *filename, sqlite3 **ppDb);
    int (*sqlite3_close)(sqlite3 *pDb);
    int (*sqlite3_exec)(sqlite3 *pDb, const char *sql,
        SQLite_callback callback, void *callback_data,
        char **errmsg);
    void (*sqlite3_free)(void*);
    };
};

// This is normally defined in sqlite3.h, so if more error codes are needed,
// get them from there.
#define SQLITE_OK 0

/// This loads the symbols from the DLL into the interface.
class SQLiteImporter:public SQLiteInterface, public OovLibrary
    {
    public:
        void loadSymbols()
            {
            loadModuleSymbol("sqlite3_open", (OovProcPtr*)&sqlite3_open);
            loadModuleSymbol("sqlite3_close", (OovProcPtr*)&sqlite3_close);
            loadModuleSymbol("sqlite3_exec", (OovProcPtr*)&sqlite3_exec);
            // This must be called for returned error strings.
            loadModuleSymbol("sqlite3_free", (OovProcPtr*)&sqlite3_free);
            }
    };

/// Functions that return errors and results will call functions in this listener.
class SQLiteListener
    {
    public:
        virtual void SQLError(int retCode, char const *errMsg) = 0;
        /// This is called for each row returned from a query, so it can be
        /// called many times after a single exec call.
        virtual void SQLResultCallback(int numColumns, char **colVal,
            char **colName) = 0;
    };

/// This is a wrapper class for the SQLite functions.
class SQLite:public SQLiteImporter
    {
    public:
        SQLite():
            mDb(nullptr), mListener(nullptr)
            {}
        ~SQLite()
            {
            close();
            }
        void setListener(SQLiteListener *listener)
            { mListener = listener; }
#ifdef __linux__
        bool open(char const *dbName, char const *dllName="libsqlite3.so")
#else
        bool open(char const *dbName, char const *dllName="sqlite3.dll")
#endif
            {
            close();
            bool success = OovLibrary::open(dllName);
            if(success)
                {
                loadSymbols();
                int retCode = sqlite3_open(dbName, &mDb);
                success = handleRetCode(retCode, "Unable to open database");
                }
            return success;
            }
        bool exec(const char *sql)
            {
            char *errMsg = nullptr;
            int retCode = sqlite3_exec(mDb, sql, &resultsCallback, this, &errMsg);
            bool success = handleRetCode(retCode, errMsg);
            if(errMsg)
                {
                sqlite3_free(errMsg);
                }
            return success;
            }
        /// This is called from the destructor, so does not need an additional
        /// call unless it must be closed early.
        void close()
            {
            if(mDb)
                {
                sqlite3_close(mDb);
                mDb = nullptr;
                }
            }

    private:
        sqlite3 *mDb;
        SQLiteListener *mListener;

        static int resultsCallback(void *customData, int argc, char **argv,
            char **azColName)
            {
            SQLite *sqlite = static_cast<SQLite*>(customData);
            if(sqlite->mListener)
                {
                sqlite->mListener->SQLResultCallback(argc, argv, azColName);
                }
            return 0;
            }
        bool handleRetCode(int retCode, char const *errStr)
            {
            if(retCode != SQLITE_OK && mListener)
                {
                mListener->SQLError(retCode, errStr);
                }
            return(retCode == SQLITE_OK);
            }
    };
