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
        DbValue(int val)
            { appendInt(val); }
        /// Use nullptr to indicate NULL.
        DbValue(char const *val)
            {
            if(val)
                {
                append("\"");
                append(val);
                append("\"");
                }
            else
                {
                append("NULL");
                }
            }
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
        DbString &SELECT(OovStringRef columns);

        DbString &INSERT(OovStringRef table);
        DbString &FROM(OovStringRef table);
        DbString &INTO(DbNames const &columns);
        DbString &VALUES(DbValues const &columns);

        DbString &UPDATE(OovStringRef table);
        DbString &SET(DbNames const &columns, DbValues const &values);

        DbString &WHERE(OovStringRef columnName, OovStringRef operStr,
            DbValue colVal);
        DbString &AND(OovStringRef columnName, OovStringRef operStr,
            DbValue colVal);
        OovString getDbStr() const;

    private:
        // Prevent usage. Use getDbStr instead.
        char const *c_str();
    };

#endif
