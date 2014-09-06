/*
 * ComponentGraph.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTGRAPH_H_
#define COMPONENTGRAPH_H_

#include <vector>
#include <set>
#include "Graph.h"	// For GraphRect
#include "Components.h"
#include "Packages.h"

class ComponentNode
    {
    public:
	enum ComponentNodeTypes { CNT_Component, CNT_ExternalPackage };
	ComponentNode():
	    mComponentType(ComponentTypesFile::CT_Unknown),
	    mComponentNodeType(CNT_Component)
	    {}
	ComponentNode(char const * const name, ComponentNodeTypes cnt,
		ComponentTypesFile::eCompTypes type=
			ComponentTypesFile::CT_Unknown):
	    mCompName(name), mComponentType(type), mComponentNodeType(cnt)
	    {
	    }
	void setPos(GraphPoint p)
	    { mRect.start = p;}
	void setSize(GraphSize s)
	    { mRect.size = s; }
	const GraphRect &getRect() const
	    { return mRect; }
	char const * const getName() const
	    { return mCompName.c_str(); }
	ComponentNodeTypes getComponentNodeType() const
	    { return mComponentNodeType; }
	ComponentTypesFile::eCompTypes getComponentType() const
	    { return mComponentType; }

    private:
	std::string mCompName;
	GraphRect mRect;
	ComponentTypesFile::eCompTypes mComponentType;
	ComponentNodeTypes mComponentNodeType;
    };

struct ComponentConnection
    {
    ComponentConnection(int nodeConsumer, int nodeSupplier):
	mNodeConsumer(nodeConsumer), mNodeSupplier(nodeSupplier)
	{}
    uint16_t mNodeConsumer;
    uint16_t mNodeSupplier;
    uint32_t getAsInt() const
	{ return (mNodeConsumer<<16) + mNodeSupplier; }
    bool operator<(const ComponentConnection &np) const
	{ return getAsInt() < np.getAsInt(); }
    };

class ComponentGraph
    {
    public:
	ComponentGraph():
	    mModified(false)
	    {}
	void updateGraph();
	GraphSize getGraphSize() const;
	const std::vector<ComponentNode> &getNodes() const
	    { return mNodes; }
	std::vector<ComponentNode> &getNodes()
	    { return mNodes; }
	const std::set<ComponentConnection> &getConnections() const
	    { return mConnections; }
	ComponentNode *getNode(int x, int y);
	bool isModified() const
	    { return mModified; }
	void setModified()
	    { mModified = true; }

    private:
	std::vector<ComponentNode> mNodes;
	std::set<ComponentConnection> mConnections;
	bool mModified;

	// This finds include paths for all source files of each component.
	void updateConnections(const class IncDirDependencyMapReader &incMap,
		const ComponentTypesFile &compFile, std::vector<Package> const &packages);
	int getComponentIndex(std::vector<std::string> const &compPaths,
		char const * const dir);
    };


#endif /* COMPONENTGRAPH_H_ */
