/*
 * ComponentList.cpp
 *
 *  Created on: Oct 2, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentList.h"
#include "Project.h"
#include "Components.h"
#include <algorithm>    // For find_if


static bool addNames(OovStringRef const compName,
    ComponentTypesFile const &compFile,
    ScannedComponentInfo const &scannedCompInfoFile,
    std::vector<ComponentListItem> &names)
    {
    bool added = false;

    OovStringVec sources = scannedCompInfoFile.getComponentFiles(compFile,
        ScannedComponentInfo::CFT_CppSource, compName, false);
    OovStringVec includes = scannedCompInfoFile.getComponentFiles(compFile,
        ScannedComponentInfo::CFT_CppInclude, compName, false);
    sources.insert(sources.end(), includes.begin(), includes.end());
    OovStringVec javaSource = scannedCompInfoFile.getComponentFiles(compFile,
        ScannedComponentInfo::CFT_JavaSource, compName, false);
    sources.insert(sources.end(), javaSource.begin(), javaSource.end());
    for(const auto &fn : sources)
        {
        FilePath modName(fn, FP_File);
        modName.discardDirectory();
        modName.discardExtension();

        auto iter = std::find_if(names.begin(), names.end(),
                [modName, compName](ComponentListItem const &item) -> bool
                { return(item.mModuleName.compare(modName) == 0 &&
                        item.mComponentName.compare(compName) == 0); });
        if(iter == names.end())
            {
            added = true;
            names.push_back(ComponentListItem(compName, modName, fn));
            }
        }
    return added;
    }

void ComponentList::updateComponentList()
    {
    clear();
    ComponentTypesFile compFile(mProject);
    ScannedComponentInfo scannedCompInfo;
    OovStatus status = scannedCompInfo.readScannedInfo();
    if(status.ok())
        {
        GuiTreeItem root;
        for(const auto &compName : scannedCompInfo.getComponentNames())
            {
            bool addedSrc = addNames(compName, compFile, scannedCompInfo, mListMap);
//            bool addedInc = addNames(compName, compFile, mListMap);
            if(addedSrc /*|| addedInc*/)
                {
                GuiTreeItem compParent = appendText(root, compName);
                for(const auto &item : mListMap)
                    {
                    if(item.mComponentName == compName)
                        {
                        appendText(compParent, item.mModuleName);
                        }
                    }
                }
            }
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to read scanned component info for component list");
        }
    }

std::string ComponentList::getSelectedFileName() const
    {
    std::vector<OovString> names = getSelected();

    std::string path;
    if(names.size() >= 2)
        {
        for(const auto &item : mListMap)
            {
            if(item.mComponentName.compare(names[0]) == 0 &&
                    item.mModuleName.compare(names[1]) == 0)
                {
                path = item.mPathName;
                break;
                }
            }
        }
    return path;
    }

