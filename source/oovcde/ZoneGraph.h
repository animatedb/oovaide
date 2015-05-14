/*
 * ZoneGraph.h
 * Created on: Feb 6, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef ZONE_GRAPH_H
#define ZONE_GRAPH_H

// A zone is a set of classes.  Plato didn't really have a grouping
// mechanism for forms (classes), but that is what a zone is.  The
// World zone is all of the classes in a project, and will be the
// starting diagram.

// The zones will be drawn as a circle graph (See Wikipedia), where
// the nodes are classes that reside on the circle. The chords between
// the any two classes are drawn with a heavier weight when there are
// more connections between the two classes.

// The goal is to have an area selection where the user can select nodes
// to define a new zone by using multiple rectangle selections. Then
// they can drill into the new zone and display a new circle graph for
// the zone.
#include "ModelObjects.h"
#include "OovString.h"
#include <algorithm>
#include <map>

struct ZoneDrawOptions
    {
    ZoneDrawOptions():
	mInitialized(false), mDrawFunctionRelations(false), mDrawAllClasses(false),
	mDrawDependencies(false), mDrawChildCircles(false)
	{}
    void initDrawOptions(bool drawAllClasses)
	{
	if(!mInitialized)
	    {
	    mDrawAllClasses = drawAllClasses;
	    mInitialized = true;
	    }
	}
    bool mInitialized;
    bool mDrawFunctionRelations;
    bool mDrawAllClasses;
    bool mDrawDependencies;
    bool mDrawChildCircles;
    };

class ZoneNode
    {
    public:
        ZoneNode(ModelType const *type):
            mType(type)
            {}

        ModelType const *mType;
        int getChordWeight(ModelType const *mType);
        void setMappedName(OovStringRef name)
            { mMappedName = name; }
        void clearMappedName()
            { mMappedName.clear(); }
        OovString getMappedComponentName(class ZonePathMap const &map) const;
        OovString getMappedName() const
            {
            if(mMappedName.length() == 0)
        	return mType->getClass()->getModule()->getName();
            else
        	return mMappedName;
            }
        // Make sure that /CodeGen/PBQP/Graph.h isn't between
        // /CodeGen/Optimize.h and /CodeGen/Passes.h
        OovString getMappedNameForSorting() const
            {
            OovString str = getMappedName();
            size_t pos = str.rfind('/');
            if(pos != std::string::npos)
        	{
        	// Insert a low character so shorter paths go first
        	str.insert(pos+1, 1, (char)0x1);
        	}
            return str;
            }

    private:
        OovString mMappedName;
    };

enum eZoneDependencyDirections { ZDD_FirstIsClient=0x1, ZDD_SecondIsClient=0x2,
    ZDD_Bidirectional=0x3 };

struct ZoneConnectIndices
    {
    ZoneConnectIndices(int index1=0, int index2=0):
	mFirstIndex(index1), mSecondIndex(index2)
	{}
    uint32_t getAsInt() const
	{ return (static_cast<uint32_t>(mFirstIndex<<16)) + mSecondIndex; }
    bool operator<(ZoneConnectIndices const &item) const
	{
	return(getAsInt() < item.getAsInt());
	}
    int mFirstIndex;
    int mSecondIndex;
    };

typedef std::pair<ZoneConnectIndices, eZoneDependencyDirections> ZoneConnection;
class ZoneConnections:public std::map<ZoneConnectIndices, eZoneDependencyDirections>
    {
    public:
	/// This inserts the indices sorted so that a connection is never listed
	/// twice, and only a single lookup is required.
	void insertConnection(int nodeIndex1, int nodeIndex2,
		eZoneDependencyDirections zdd);
	void insertConnection(int nodeIndex, const ModelClassifier *classifier,
		class ReverseIndexLookup const &indexLookup,
		eZoneDependencyDirections zdd);
    };

class ZonePathReplaceItem
    {
    public:
	ZonePathReplaceItem(OovStringRef srchPath, OovStringRef repPath):
	    mSearchPath(srchPath), mReplacePath(repPath)
	    {}
	bool operator==(ZonePathReplaceItem const &item) const
	    {
	    return(mSearchPath == item.mSearchPath && mReplacePath == item.mReplacePath);
	    }
    OovString mSearchPath;
    OovString mReplacePath;
    };

class ZonePathMap:public std::vector<ZonePathReplaceItem>
{
public:
    void remove(ZonePathReplaceItem const &item)
	{
//	auto it = std::find_if(begin(), end(), item);

	auto it = std::find_if(begin(), end(),
		[item](ZonePathReplaceItem const &item2)
		    { return item == item2; });
	if(it != end())
	    {
	    erase(it);
	    }
	}
};

class ZoneGraph
    {
    public:
	ZoneGraph():
	    mModel(nullptr), mDiagramChanged(ZDC_None)
	    {}
	void clearGraph()
	    {
	    mNodes.clear();
	    mConnections.clear();
	    }
	// The model must exist for the lifetime of the graph.
        void clearAndAddWorldNodes(const ModelData &model);
        /// Call updateGraph after calling this.
	void setFilter(std::string moduleName, bool set);
	void updateGraph();
        std::vector<ZoneNode> const &getNodes() const
            { return mNodes; }
        size_t getNodeIndex(ZoneNode const *node) const
            { return(static_cast<size_t>(node - &mNodes[0])); }
        ZoneConnections const &getConnections() const
            { return mConnections; }
	bool isModified() const
	    { return(mFilteredModules.size() != 0); }
	ZoneDrawOptions const &getDrawOptions() const
	    { return mDrawOptions; }
	void setDrawOptions(ZoneDrawOptions const &options);
	ZonePathMap const &getPathMap() const
	    { return mPathMap; }
	ZonePathMap &getPathMap()
	    { return mPathMap; }
	static std::string getComponentText(ModelModule const *modelModule);

    private:
	std::vector<ZoneNode> mNodes;
	ZoneConnections mConnections;
	const ModelData *mModel;
	ZoneDrawOptions mDrawOptions;
	OovStringSet mFilteredModules;
	ZonePathMap mPathMap;
	enum eZoneDiagChanges
	    {
	    ZDC_None, ZDC_Nodes=0x01, ZDC_Connections=0x2,
	    ZDC_Graph=ZDC_Nodes + ZDC_Connections
	    };
	eZoneDiagChanges mDiagramChanged;

	void setDiagramChange(eZoneDiagChanges dc)
	    { mDiagramChanged = static_cast<eZoneDiagChanges>(mDiagramChanged | dc); }
	void sortNodes();
	void updateConnections();
    };

#endif
