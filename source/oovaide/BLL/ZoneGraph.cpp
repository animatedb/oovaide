/*
 * ZoneGraph.cpp
 * Created on: Feb 6, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ZoneGraph.h"
#include <algorithm>
#include "Project.h"

static bool isFiltered(ModelModule const *module,
        OovStringSet const &filteredModules)
    {
    auto const &it = filteredModules.find(ZoneGraph::getComponentText(module));
    return(it != filteredModules.end());
    }

static bool mapPath(ZonePathMap const &map, OovString const &origName,
        OovString &newName)
    {
    bool replaced = false;
    size_t starti = 0;
    newName = origName;
    do
        {
        size_t firstFoundi = OovString::npos;
        ZonePathReplaceItem const *repItem = nullptr;
        for(auto const &mapItem : map)
            {
            size_t foundi = newName.find(mapItem.mSearchPath, starti);
            if(foundi < firstFoundi)
                {
                repItem = &mapItem;
                firstFoundi = foundi;
                }
            }
        if(firstFoundi != OovString::npos)
            {
            newName.replace(firstFoundi, repItem->mSearchPath.length(),
                    repItem->mReplacePath);
            starti = firstFoundi + repItem->mReplacePath.length();
            replaced = true;
            }
        else
            starti = OovString::npos;
        } while(starti != OovString::npos);
    return replaced;
    }

OovString ZoneNode::getMappedComponentName(ZonePathMap const &map) const
    {
    OovString compName = ZoneGraph::getComponentText(mType->getClass()->getModule());
    OovString newName;
    if(mapPath(map, compName, newName))
        {
        compName = newName;
        }
    return compName;
    }

static const size_t NO_INDEX = static_cast<size_t>(-1);

class ReverseIndexLookup
    {
    public:
        ReverseIndexLookup(std::vector<ZoneNode> const &nodes):
            mClasses(nodes.size())
            {
            for(size_t i=0; i<nodes.size(); i++)
                {
                mClasses[i] = nodes[i].mType->getClass();
                }
            }

        // Returns NO_INDEX for class not found;
        size_t getClassIndex(const ModelClassifier *classifier) const
            {
            size_t index = NO_INDEX;
            for(size_t i=0; i<mClasses.size(); i++)
                {
                if(mClasses[i] == classifier)
                    {
                    index = i;
                    break;
                    }
                }
            return index;
            }

    private:
        /// @todo - could use a map or something here.
        std::vector<const ModelClassifier*> mClasses;
    };

void ZoneConnections::insertConnection(size_t nodeIndex1, size_t nodeIndex2,
        eZoneDependencyDirections zdd)
    {
    if(nodeIndex1 != nodeIndex2)
        {
        size_t firstIndex = nodeIndex1;
        size_t secondIndex = nodeIndex2;
        /// Make it so the first index is always highest.
        if(nodeIndex1 < nodeIndex2)
            {
            firstIndex = nodeIndex2;
            secondIndex = nodeIndex1;
            if(zdd == ZDD_FirstIsClient)
                zdd = ZDD_SecondIsClient;
            else if(zdd == ZDD_SecondIsClient)
                zdd = ZDD_FirstIsClient;
            }
        insert(ZoneConnection(ZoneConnectIndices(firstIndex, secondIndex), zdd));
        }
    }

void ZoneConnections::insertConnection(size_t nodeIndex,
    const ModelClassifier *classifier,
    ReverseIndexLookup const &indexLookup, eZoneDependencyDirections zdd)
    {
    size_t secondIndex = indexLookup.getClassIndex(classifier);
    if(secondIndex != NO_INDEX) // NO_INDEX is not a class, or is not in the list of indexed classes.
        {
        if(nodeIndex != secondIndex)
            {
            insertConnection(nodeIndex, secondIndex, zdd);
            }
        }
    }



std::string ZoneGraph::getComponentText(ModelModule const *modelModule)
    {
    OovString relFile = Project::getSrcRootDirRelativeSrcFileName(
            modelModule->getName(), Project::getSrcRootDirectory());
    FilePath moduleName(relFile, FP_File);
    moduleName.discardFilename();
    /// @todo - FilePath doesn't have a good way to remove a trailing slash
    /// since it wants to reserve the indication of directory
    std::string name = moduleName;
    int pos = name.length()-1;
    if(name[pos] == '/')
        name.erase(pos);
    return name;
    }

void ZoneGraph::clearAndAddWorldNodes(const ModelData &model)
    {
    mModel = &model;
    setDiagramChange(ZDC_Graph);
    }

void ZoneGraph::setFilter(std::string moduleName, bool set)
    {
    setDiagramChange(ZDC_Graph);
    auto const &it = mFilteredModules.find(moduleName);
    if(set)
        {
        if(it == mFilteredModules.end())
            {
            mFilteredModules.insert(moduleName);
            }
        }
    else
        {
        if(it != mFilteredModules.end())
            mFilteredModules.erase(it);
        }
    }

void ZoneGraph::setDrawOptions(ZoneDrawOptions const &options)
    {
    if(options.mDrawFunctionRelations != mDrawOptions.mDrawFunctionRelations)
        {
        setDiagramChange(ZDC_Connections);
        }
    if(options.mDrawAllClasses != mDrawOptions.mDrawAllClasses)
        {
        setDiagramChange(ZDC_Graph);
        }
    mDrawOptions = options;
    }

void ZoneGraph::updateGraph()
    {
    if(mDiagramChanged & ZDC_Nodes)
        {
        clearGraph();
        for(auto const &type : mModel->mTypes)
            {
            ModelClassifier const *cls = type.get()->getClass();
            if(cls && cls->getModule() != nullptr)
                {
                if(!isFiltered(cls->getModule(), mFilteredModules))
                    {
                    mNodes.push_back(ZoneNode(type.get()));
                    }
                }
            }
        mDrawOptions.initDrawOptions(mNodes.size() < 500);
        sortNodes();
        }
    if(mDiagramChanged & ZDC_Connections)
        {
        updateConnections();
        }
    mDiagramChanged = ZDC_None;
    }


void ZoneGraph::sortNodes()
    {
    for(auto &node : mNodes)
        {
        OovString mappedName;
        if(mapPath(mPathMap, node.mType->getClass()->getModule()->getModulePath(),
                mappedName))
            {
            node.setMappedName(mappedName);
            }
        else
            {
            node.clearMappedName();
            }
        }
    std::sort(mNodes.begin(), mNodes.end(), [](ZoneNode const &n1, ZoneNode const &n2)
                { return(n1.getMappedNameForSorting() < n2.getMappedNameForSorting()); }
        );
    }


void ZoneGraph::updateConnections()
    {
    bool drawFuncRels = mDrawOptions.mDrawFunctionRelations;
    ReverseIndexLookup indexLookup(getNodes());
    mConnections.clear();
    for(size_t nodeIndex=0; nodeIndex<getNodes().size(); nodeIndex++)
        {
        const ModelClassifier *classifier = getNodes()[nodeIndex].mType->getClass();
        if(classifier)
            {
            // Get attributes of classifier, and get the decl type
            for(const auto &attr : classifier->getAttributes())
                {
                mConnections.insertConnection(nodeIndex, attr->getDeclType()->getClass(),
                        indexLookup, ZDD_FirstIsClient);
                }
            if(drawFuncRels)
                {
                ConstModelClassifierVector relatedClasses;
                mModel->getRelatedFuncInterfaceClasses(*classifier, relatedClasses);
                for(auto const &item : relatedClasses)
                    {
                    mConnections.insertConnection(nodeIndex, item,
                            indexLookup, ZDD_FirstIsClient);
                    }

                ConstModelDeclClasses relatedDeclClasses;
                mModel->getRelatedBodyVarClasses(*classifier, relatedDeclClasses);
                for(auto const &item : relatedDeclClasses)
                    {
                    mConnections.insertConnection(nodeIndex, item.getClass(),
                            indexLookup, ZDD_FirstIsClient);
                    }
                }

            // Go through associations, and get related classes.
            for(const auto &assoc : mModel->mAssociations)
                {
                size_t n1Index = NO_INDEX;
                size_t n2Index = NO_INDEX;
                if(assoc->getChild() == classifier)
                    {
                    n1Index = indexLookup.getClassIndex(assoc->getParent());
                    n2Index = indexLookup.getClassIndex(classifier);
                    }
                else if(assoc->getParent() == classifier)
                    {
                    n2Index = indexLookup.getClassIndex(assoc->getChild());
                    n1Index = indexLookup.getClassIndex(classifier);
                    }
                if(n1Index != NO_INDEX && n2Index != NO_INDEX)
                    {
                    mConnections.insertConnection(n1Index, n2Index, ZDD_SecondIsClient);
                    }
                }
            }
        }
    }
