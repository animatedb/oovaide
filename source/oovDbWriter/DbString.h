/*
 * DbString.h
 *
 *  Created on: Sept. 30, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef DB_STRING_H
#define DB_STRING_H

#include "OovString.h"

class DbValue:public OovString
    {
    public:
        /// This converts the integer into a string and appends to DbValue.
        /// @param val The value to convert into a string.
        DbValue(int val)
            { appendInt(val); }
        /// @param val Use nullptr to indicate NULL or pass in a string that
        ///            will be enclosed within quotes.
        DbValue(char const *val);
    };

typedef std::vector<OovString> DbNames;
typedef std::vector<DbValue> DbValues;

// This uses method chaining or the named parameter idiom.
// Examples:
//      dbStr.SELECT("idModule").FROM("Module").WHERE("name", "=", name);
//      dbStr.INSERT("Module").INTO({"idModule","name"}).VALUES({nullptr, name});
//      dbStr.UPDATE("Module").SET({"idModule","name"}, {nullptr, name});
class DbString:public OovString
    {
    public:
        DbString()
            {}
        DbString(char const * const str):
            OovString(str)
            {}
        // Appends "SELECT column"
        DbString &SELECT(OovStringRef column);

        // Appends "INSERT INTO table"
        DbString &INSERT(OovStringRef table);
        // Appends " FROM "
        DbString &FROM(OovStringRef table);
        // Appends "(columnName1, columnName2)"
        DbString &INTO(DbNames const &columnNames);
        // Appends "VALUES (columnValue1, columnValue2)"
        DbString &VALUES(DbValues const &columnValues);

        // Appends "UPDATE table"
        DbString &UPDATE(OovStringRef table);
        // Appends " SET column1, column2 = value1, value2"
        // The number of columns and values must match.
        DbString &SET(DbNames const &columns, DbValues const &values);

        // Appends " WHERE columnName operStr colVal"
        DbString &WHERE(OovStringRef columnName, OovStringRef operStr,
            DbValue colVal);
        // Appends " AND columnName operStr colVal"
        DbString &AND(OovStringRef columnName, OovStringRef operStr,
            DbValue colVal);
        // Appends a semicolon to the returned string.
        OovString getDbStr() const;

    private:
        // Prevent usage. Use getDbStr instead. This is undefined.
        char const *c_str();
    };

#endif
