/*
 * ComponentList.cpp
 *
 *  Created on: Oct 2, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentList.h"
#include "Project.h"
#include "NameValueFile.h"
#include <algorithm>	// For find_if


static void addNames(char const * const compName, NameValueFile &compFile,
	char const * const compSrcTagName, std::vector<ComponentListItem> &names)
    {
    std::string compSrcTag = compSrcTagName;
    compSrcTag += compName;
    std::string compSrcStr = compFile.getValue(compSrcTag.c_str());
    CompoundValue srcVal;
    srcVal.parseString(compSrcStr.c_str());
    for(const auto &fn : srcVal)
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
	    names.push_back(ComponentListItem(compName, modName.c_str(),
		    fn.c_str()));
	    }
	}
    }

void ComponentList::updateComponentList()
    {
    clear();
    NameValueFile compFile;
    compFile.setFilename(Project::getComponentTypesFilePath().c_str());
    compFile.readFile();
    std::string compStr = compFile.getValue("Components");
    CompoundValue compVal;
    compVal.parseString(compStr.c_str());
    GuiTreeItem root;
    for(const auto &compName:compVal)
	{
	GuiTreeItem compParent = appendText(root, compName.c_str());
	addNames(compName.c_str(), compFile, "Comp-src-", mListMap);
	addNames(compName.c_str(), compFile, "Comp-inc-", mListMap);
	for(const auto &item : mListMap)
	    {
	    if(item.mComponentName == compName)
		{
		appendText(compParent, item.mModuleName.c_str());
		}
	    }
	}
    }

std::string ComponentList::getSelectedFileName() const
    {
    std::vector<std::string> names;
    getSelected(names);

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

