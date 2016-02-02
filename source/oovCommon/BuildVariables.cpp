// File: BuildVariables.cpp
// Created: 2016-01-20
// \copyright 2016 DCBlaha.  Distributed under the GPL.

#include "BuildVariables.h"
#include <algorithm>


void BuildVariable::initVarFromString(OovStringRef filterDef, OovStringRef varValue)
    {
    clearFilter();
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
            OovStringVec filters = filterString.split('&');
            for(auto const &filter : filters)
                {
                OovStringVec filterParts = filter.split(':');
                if(filterParts.size() == 2)
                    {
                    addFilter(filterParts[0], filterParts[1]);
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
    name += getFilterAsString();
    name += mFunction;
    return name;
    }

bool BuildVariable::isSubsetOf(VariableFilterList const &superset) const
    {
    bool subset = true;

    for(auto const &subVar : mVarFilterList)
        {
        auto it = std::find_if(superset.begin(), superset.end(),
            [subVar](VariableFilter const &superVar)
            { return subVar.mFilterName.compare(superVar.mFilterName) == 0;});
        if(it != superset.end())
            {
            if(subVar.mFilterValue != (*it).mFilterValue)
                {
                subset = false;
                }
            }
        else
            {
            // Warning - variable is not in environment
            }
        }
    return subset;
    }

OovString BuildVariable::getFilterAsString() const
    {
    OovString str = "[";
    for(auto const &vf : mVarFilterList)
        {
        if(str.length() != 1)
            {
            str += " & ";
            }
        str += vf.mFilterName;
        str += ':';
        str += vf.mFilterValue;
        }
    str += ']';
    return str;
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
                    val.insert(0, funcVal);
                    break;

                case BuildVariable::F_Insert:
                    val += funcVal;
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
