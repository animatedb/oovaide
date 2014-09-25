/*
 * ComponentGraph.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "ComponentGraph.h"
#include "Project.h"
#include "BuildConfigReader.h"
#include "IncludeMap.h"
#include <algorithm>

void ComponentGraph::updateGraph(const ComponentDrawOptions &options)
    {
    ComponentTypesFile compFile;
    compFile.read();

    mNodes.clear();
    for(auto const &name : compFile.getComponentNames())
	{
	ComponentTypesFile::eCompTypes ct = compFile.getComponentType(name.c_str());
	if(ct != ComponentTypesFile::CT_Unknown)
	    {
	    mNodes.push_back(ComponentNode(name.c_str(),
		    ComponentNode::CNT_Component, ct));
	    }
	}
    BuildPackages buildPkgs(true);
    std::vector<Package> packages = buildPkgs.getPackages();
    for(auto const &pkg : packages)
	{
	mNodes.push_back(ComponentNode(pkg.getPkgName().c_str(),
		ComponentNode::CNT_ExternalPackage));
	}
    updateConnections(compFile, options);
    }

void ComponentGraph::updateConnections(const ComponentTypesFile &compFile,
	const ComponentDrawOptions &options)
    {
    IncDirDependencyMapReader incMap;
    BuildConfigReader buildConfig;
    std::string incDepsFilePath = buildConfig.getIncDepsFilePath(BuildConfigAnalysis);
    incMap.read(incDepsFilePath.c_str());
    BuildPackages buildPkgs(true);
    std::vector<Package> packages = buildPkgs.getPackages();
    mConnections.clear();

    std::vector<std::string> compPaths;
    for(size_t i=0; i<mNodes.size(); i++)
	{
	if(mNodes[i].getComponentNodeType() == ComponentNode::CNT_Component)
	    {
	    std::string incPath;
	    incPath = compFile.getComponentAbsolutePath(mNodes[i].getName());
	    compPaths.push_back(incPath);
	    }
	}

    for(size_t consumerIndex=0; consumerIndex<mNodes.size(); consumerIndex++)
	{
	if(mNodes[consumerIndex].getComponentNodeType() == ComponentNode::CNT_Component)
	    {
	    std::vector <std::string> srcFiles =
		    compFile.getComponentSources(mNodes[consumerIndex].getName());
	    for(auto const &srcFile : srcFiles)
		{
		FilePath fp;
		fp.getAbsolutePath(srcFile.c_str(), FP_File);
		std::vector<std::string> incDirs =
			incMap.getNestedIncludeDirsUsedBySourceFile(fp.c_str());
		for(auto const &incDir : incDirs)
		    {
		    int supplierIndex = getComponentIndex(compPaths, incDir.c_str());
		    if(supplierIndex != -1 && consumerIndex != (size_t)supplierIndex)
			mConnections.insert(ComponentConnection(consumerIndex, supplierIndex));
		    }
		}
	    }
	}
    for(size_t supplierIndex=0; supplierIndex<mNodes.size(); supplierIndex++)
	{
	if(mNodes[supplierIndex].getComponentNodeType() == ComponentNode::CNT_ExternalPackage)
	    {
	    std::string const &nodeName = mNodes[supplierIndex].getName();
	    auto const &supplierIt = std::find_if(packages.begin(), packages.end(),
		    [nodeName](Package const &pkg) -> bool
		    { return(pkg.getPkgName().compare(nodeName) == 0); });
	    if(supplierIt != packages.end())
		{
		for(size_t consumerIndex=0; consumerIndex<mNodes.size(); consumerIndex++)
		    {
		    if(mNodes[consumerIndex].getComponentNodeType() == ComponentNode::CNT_Component)
			{
			std::vector<std::string> incRoots = (*supplierIt).getIncludeDirs();
			if(incMap.anyRootDirsMatch(incRoots, compPaths[consumerIndex].c_str()))
			    {
			    mConnections.insert(ComponentConnection(consumerIndex, supplierIndex));
			    }
			}
		    }
		}
	    }
	}
    if(!options.drawImplicitRelations)
	pruneConnections();
    }

void ComponentGraph::findNumPaths(int consumerIndex, int supplierIndex, int &numPaths)
    {
    for(auto const &connection : mConnections)
	{
	if(connection.mNodeConsumer == consumerIndex)
	    {
	    if(connection.mNodeSupplier == supplierIndex)
		{
		numPaths++;
		}
	    else
		findNumPaths(connection.mNodeSupplier, supplierIndex, numPaths);
	    }
	}
    }

void ComponentGraph::pruneConnections()
    {
    for(auto & constConn : mConnections)
	{
	// The begin() iterator is const only in the <set> header file. Since
	// the set sorting is not dependent on the mImpliedDependency, this code is ok.
	ComponentConnection &connection = const_cast<ComponentConnection &>(constConn);
	int numPaths = 0;
	findNumPaths(connection.mNodeConsumer, connection.mNodeSupplier, numPaths);
	if(numPaths > 1)
	    {
	    connection.setImpliedDependency(true);
	    }
	}
    /*
    std::set<ComponentConnection>::iterator iter = mConnections.begin();
    while(iter != mConnections.end())
	{
	ComponentConnection const &connection = *iter;
	int numPaths = 0;
	findNumPaths(connection.mNodeConsumer, connection.mNodeSupplier, numPaths);
	if(numPaths > 1)
	    {
	    mConnections.erase(iter++);
	    }
	else
	    {
	    iter++;
	    }
	}
*/
    }

int ComponentGraph::getComponentIndex(std::vector<std::string> const &compPaths,
	char const * const dir)
    {
    int compIndex = -1;
    for(size_t i=0; i<compPaths.size(); i++)
	{
	std::string const &compPath = compPaths[i];
	if(compPath.compare(dir) == 0)
	    {
	    compIndex = i;
	    break;
	    }
	}
    return compIndex;
    }

ComponentNode *ComponentGraph::getNode(int x, int y)
    {
    ComponentNode *node = nullptr;
    for(size_t i=0; i<mNodes.size(); i++)
	{
	if(mNodes[i].getRect().isPointIn(GraphPoint(x, y)))
	    node = &mNodes[i];
	}
    return node;
    }

void ComponentGraph::removeNode(const ComponentNode &node,
	const ComponentDrawOptions &options)
    {
    for(size_t i=0; i<mNodes.size(); i++)
	{
	if(&node == &mNodes[i])
	    {
	    mNodes.erase(mNodes.begin() + i);
	    break;
	    }
	}
    ComponentTypesFile compFile;
    compFile.read();
    updateConnections(compFile, options);
    mModified = true;
    }

GraphSize ComponentGraph::getGraphSize() const
    {
    GraphSize size;
    for(const auto &node : mNodes)
	{
	GraphRect rect = node.getRect();
	if(rect.endx() > size.x)
	    size.x = rect.endx();
	if(rect.endy() > size.y)
	    size.y = rect.endy();
	}
    size.x++;
    size.y++;
    return size;
    }
