// File: BuildVariables.cpp
// Created: 2016-01-20
// \copyright 2016 DCBlaha.  Distributed under the GPL.

#include "BuildVariables.h"
#include "Project.h"
#include <algorithm>


void VariableFilter::initFilterFromString(OovStringRef filter)
    {
    OovStringVec filterParts = filter.split(':');
    if(filterParts.size() == 2)
        {
        mFilterName = filterParts[0];
        mFilterValue = filterParts[1];
        }
    }

OovString VariableFilter::getFilterAsString() const
    {
    OovString str;
    str += mFilterName;
    str += ':';
    str += mFilterValue;
    return str;
    }

void BuildVariable::initVarFromString(OovStringRef filter)
    {
    OovString filterStr = filter;
    OovStringVec varParts = filterStr.split('|');
    if(varParts.size() == 2)
        {
        initVarFromString(varParts[0], varParts[1]);
        }
    }

void BuildVariable::initVarFromString(OovStringRef filterDef, OovStringRef varValue)
    {
    clearFilters();
    OovString filterDefStr = filterDef;
    size_t filterStart=filterDefStr.find('[');
    eFunctions func = F_Assign;
    mVarName.clear();
    if(filterStart != std::string::npos)
        {
        mVarName = filterDefStr.substr(0, filterStart);
        size_t filterEnd=filterDefStr.find(']');
        if(filterEnd != std::string::npos)
            {
            char funcC = filterDefStr[filterEnd+1];
            func = static_cast<eFunctions>(funcC);
            filterStart++;
            OovString filterString = filterDefStr.substr(filterStart, filterEnd-filterStart);
            if(filterString.length() > 0)
                {
                OovStringVec filters = filterString.split('&');
                for(auto const &filterStr : filters)
                    {
                    VariableFilter filter;
                    filter.initFilterFromString(filterStr.getTrimmed());
                    mVarFilterList.addFilter(filter);
                    }
                }
            }
        }
    mFunction = func;
    mVarValue = varValue;
    }

OovString BuildVariable::getFilterValue(OovStringRef filterName)
    {
    OovString val;
    auto const &it = std::find_if(mVarFilterList.begin(), mVarFilterList.end(),
        [&filterName](VariableFilter const &fv)
        { return(fv.mFilterName.compare(filterName) == 0); });
    if(it != mVarFilterList.end())
        {
        val = (*it).mFilterValue;
        }
    return val;
    }

OovString BuildVariable::getVarFilterName()
    {
    OovString name = mVarName;
    name += getFiltersAsString();
    name += mFunction;
    return name;
    }

OovString BuildVariable::getVarDefinition(char delimChar) const
    {
    OovString name = mVarName;
    name += getFiltersAsString();
    name += mFunction;
    name += delimChar;
    name += mVarValue;
    return name;
    }

bool BuildVariable::isSubsetOf(VariableFilterList const &superset,
    const char *ignoreFilterName) const
    {
    bool subset = true;

    for(auto const &subVar : mVarFilterList)
        {
        auto it = std::find_if(superset.begin(), superset.end(),
            [subVar](VariableFilter const &superVar)
            { return subVar.mFilterName.compare(superVar.mFilterName) == 0;});
        if(it != superset.end())
            {
            if(ignoreFilterName == nullptr || subVar.mFilterName != ignoreFilterName)
                {
                if(subVar.mFilterValue != (*it).mFilterValue)
                    {
#if(0)
printf("Not subset %s: %s != %s\n", subVar.mFilterName.getStr(),
    subVar.mFilterValue.getStr(), (*it).mFilterValue.getStr());
fflush(stdout);
#endif
                    subset = false;
                    break;
                    }
                }
            }
        else
            {
            // Warning - variable is not in environment
            }
        }
    return subset;
    }

OovString BuildVariable::getFiltersAsString() const
    {
    OovString str = "[";
    for(auto const &vf : mVarFilterList)
        {
        if(str.length() != 1)
            {
            str += " & ";
            }
        str += vf.getFilterAsString();
        }
    str += ']';
    return str;
    }

OovStringVec BuildVariableEnvironment::getMatchingVariables(OovStringRef varName) const
    {
    OovStringVec varNames = mNameValues.getMatchingNames(varName);
    OovStringVec matchingEnvNames;
/// @TODO - base arg[] or assignments should be done first!
/// Simple solution that may be ok is to sort varNames by length.
    std::sort(varNames.begin(), varNames.end(),
        [](const OovString &l, const OovString &r)
            { return l.numBytes()<r.numBytes(); });
    for(auto const &varName : varNames)
        {
        BuildVariable fileVar;
        fileVar.initVarFromString(varName, "");
        char const *ignore = mIgnoreFilterName.length() ? mIgnoreFilterName.getStr() : nullptr;
        if(fileVar.isSubsetOf(mCurrentFilterValues, ignore))
            {
            matchingEnvNames.push_back(varName);
            }
        }
    return matchingEnvNames;
    }

OovStringVec BuildVariableEnvironment::getMatchingVariablesIgnoreComp(OovStringRef varName) const
    {
    mIgnoreFilterName = OptFilterNameComponent;
    OovStringVec matchingEnvNames = getMatchingVariables(varName);
    mIgnoreFilterName.clear();
    return matchingEnvNames;
    }

OovString BuildVariableEnvironment::getValue(OovStringRef varName) const
    {
    OovStringVec varNames = mNameValues.getMatchingNames(varName);
    OovString val;
/// @TODO - base arg[] must be done first!
/// Simple solution that may be ok is to sort varNames by length.
    std::sort(varNames.begin(), varNames.end(),
        [](const OovString &l, const OovString &r)
            { return l.numBytes()<r.numBytes(); });
    for(auto const &varName : varNames)
        {
        BuildVariable fileVar;
        fileVar.initVarFromString(varName, "");
        if(fileVar.isSubsetOf(mCurrentFilterValues))
            {
            OovString funcVal = mNameValues.getValue(varName);
            switch(fileVar.getFunction())
                {
                case BuildVariable::F_Assign:
                    val = funcVal;
                    break;

                case BuildVariable::F_Append:
                    val += funcVal;
                    break;

                case BuildVariable::F_Insert:
                    val.insert(0, funcVal);
                    break;

                case  BuildVariable::F_Remove:
                    {
                    size_t pos = val.find(funcVal);
                    if(pos != std::string::npos)
                        {
                        val.erase(pos, funcVal.size());
                        }
                    }
                    break;
                }
            }
        }
    return val;
    }
