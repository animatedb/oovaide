/*
 * ComponentList.cpp
 *
 *  Created on: Oct 2, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentList.h"
#include "Project.h"
#include "NameValueFile.h"

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
	std::string compSrcTag = "Comp-src-";
	compSrcTag += compName;
	std::string compSrcStr = compFile.getValue(compSrcTag.c_str());
	CompoundValue srcVal;
	srcVal.parseString(compSrcStr.c_str());
	for(const auto &fn : srcVal)
	    {
	    FilePath modName(fn, FP_File);
	    modName.discardDirectory();
	    modName.discardExtension();
	    appendText(compParent, modName.c_str());
	    mListMap.push_back(ComponentListItem(compName.c_str(),
		    modName.c_str(), fn.c_str()));
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

