/*
 * DbString.cpp
 *
 *  Created on: Sept. 30, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "DbString.h"
#include <ctype.h>

DbValue::DbValue(char const *val)
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

DbString &DbString::SELECT(OovStringRef column)
    {
    append("SELECT ");
    append(column);
    return *this;
    }

DbString &DbString::INSERT(OovStringRef table)
    {
    append("INSERT INTO ");
    append(table);
    return *this;
    }

DbString &DbString::UPDATE(OovStringRef table)
    {
    append("UPDATE ");
    append(table);
    return *this;
    }

DbString &DbString::FROM(OovStringRef table)
    {
    append(" FROM ");
    append(table);
    return *this;
    }

DbString &DbString::SET(DbNames const &columns, DbValues const &values)
    {
    append(" SET ");
    size_t numColumns = std::min(columns.size(), values.size());
    for(size_t i=0; i<numColumns; i++)
        {
        if(i != 0)
            {
            append(",");
            }
        append(columns[i]);
        append("=");
        append(values[i]);
        }
    return *this;
    }

DbString &DbString::WHERE(OovStringRef columnName, OovStringRef operStr, DbValue colVal)
    {
    append(" WHERE ");
    append(columnName);
    append(operStr);
    append(colVal);
    return *this;
    }

DbString &DbString::AND(OovStringRef columnName, OovStringRef operStr, DbValue colVal)
    {
    append(" AND ");
    append(columnName);
    append(operStr);
    append(colVal);
    return *this;
    }

DbString &DbString::INTO(DbNames const &columns)
    {
    append("(");
    for(size_t i=0; i<columns.size(); i++)
        {
        if(i != 0)
            {
            append(",");
            }
        append(columns[i]);
        }
    append(")");
    return *this;
    }

DbString &DbString::VALUES(DbValues const &columns)
    {
    append("VALUES (");
    bool first = true;
    for(auto item : columns)
        {
        if(first)
            {
            first = !first;
            }
        else
            {
            append(",");
            }
        append(item);
        }
    append(")");
    return *this;
    }

OovString DbString::getDbStr() const
    {
    OovString str = *this;
    str += ';';
    return str;
    }
