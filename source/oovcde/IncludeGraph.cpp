/*
 * IncludeGraph.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeGraph.h"
#include "Debug.h"
#include <algorithm>


IncludeNode const *IncludeGraph::getNode(OovStringRef name, IncludeNodeTypes nt) const
    {
    IncludeNode const *retNode = nullptr;
    for(auto const &node : mNodes)
        {
        if(node.getNodeType() == nt && OovString(node.getName()) == name.getStr())
            {
            retNode = &node;
            break;
            }
        }
    return retNode;
    }

void IncludeGraph::clearAndAddInclude(OovStringRef incName)
    {
    clearGraph();
    addSuppliers(incName);
    }

void IncludeGraph::addSuppliers(OovStringRef incName)
    {
    mNodes.push_back(IncludeNode(incName, INT_Project));
    std::set<IncludedPath> incFiles;
    mIncludeMap->getImmediateIncludeFilesUsedBySourceFile(incName, incFiles);
    for(auto const &incFile : incFiles)
        {
        auto const &iter = std::find_if(mNodes.begin(), mNodes.end(),
                [incFile](IncludeNode const &node)
                    {
                    bool val = incFile.getFullPath() == node.getName().getStr();
                    return(val);
                    }
                );
        if(iter == mNodes.end())
            {
#define RECURSE 0
#if(RECURSE)
            addSuppliers(incFile.getFullPath());
#else
            mNodes.push_back(IncludeNode(incFile.getFullPath(), INT_Project));
#endif
            }
        IncludeConnection conn(getNodeIndex(getNode(incFile.getFullPath(),
                INT_Project)), getNodeIndex(getNode(incName, INT_Project)));
        mConnections.push_back(conn);
        }
    }
