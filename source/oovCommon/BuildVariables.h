// File: BuildVariables.h
// Created: 2016-01-20
// \copyright 2016 DCBlaha.  Distributed under the GPL.

#ifndef BUILD_VARIABLES_H
#define BUILD_VARIABLES_H

#include "NameValueFile.h"

// The following classes are used to store values into a NameValueRecord
class VariableFilter
    {
    public:
        VariableFilter(OovStringRef filterName, OovStringRef filterValue):
            mFilterName(filterName), mFilterValue(filterValue)
            {}
        VariableFilter()
            {}
        void initFilterFromString(OovStringRef filter);
        OovString getFilterAsString() const;

    public:
        OovString mFilterName;
        OovString mFilterValue;
    };

class VariableFilterList:public std::vector<VariableFilter>
    {
    public:
        void addFilter(VariableFilter const &filter)
            { push_back(filter); }
        void addFilter(OovStringRef filterName, OovStringRef filterValue)
            { push_back(VariableFilter(filterName, filterValue)); }
    };

class BuildVariable
    {
    public:
        enum eFunctions { F_Assign='=', F_Append='+', F_Insert='^', F_Remove='-' };
        BuildVariable():
            mFunction(F_Assign)
            {}
        /// filterDef is something like var[fvar1:fval1 & fvar2:fval2]+
        void initVarFromString(OovStringRef filterDef, OovStringRef varValue);
        void initVarFromString(OovStringRef filter);
        void setVar(OovStringRef varName, OovStringRef varValue)
            {
            mVarName = varName;
            mVarValue = varValue;
            }
        void setVarName(OovStringRef varName)
            { mVarName = varName; }
        void setVarValue(OovStringRef varValue)
            { mVarValue = varValue; }
        void setFunction(eFunctions func)
            { mFunction = func; }
        void clearFilters()
            { mVarFilterList.clear(); }
        VariableFilterList const &getVarFilterList() const
            { return mVarFilterList; }
        void addFilter(VariableFilter const &filter)
            { mVarFilterList.addFilter(filter); }
        void addFilter(OovStringRef filterName, OovStringRef filterValue)
            { mVarFilterList.addFilter(filterName, filterValue); }

        bool isSubsetOf(VariableFilterList const &superset) const;

        OovString getFilterValue(OovStringRef filterName);
        eFunctions getFunction() const
             { return mFunction; }
        OovString const &getVarName() const
            { return mVarName; }
        OovString const &getVarValue() const
            { return mVarValue; }
        // This returns something like var[fvar1:fval1 & fvar2:fval2]|=value
        OovString getVarDefinition(char delimChar = '|') const;
        /// This returns the variable name, plus the filter values and function.
        OovString getVarFilterName();
        OovString getFiltersAsString() const;

    private:
        OovString mVarName;
        OovString mVarValue;
        VariableFilterList mVarFilterList;
        eFunctions mFunction;
    };

class BuildVariableEnvironment
    {
    public:
        BuildVariableEnvironment(NameValueRecord const &rec):
            mNameValues(rec)
            {}
        void clearCurrentFilterValues()
            { mCurrentFilterValues.clear(); }
        void addCurrentFilterValue(OovStringRef filterName, OovStringRef filterValue)
            { mCurrentFilterValues.addFilter(filterName, filterValue); }
        OovString getValue(OovStringRef varName) const;

    private:
        NameValueRecord const &mNameValues;
        VariableFilterList mCurrentFilterValues;
    };

#endif
