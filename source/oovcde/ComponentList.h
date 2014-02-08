/*
 * ComponentList.h
 *
 *  Created on: Oct 1, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTLIST_H_
#define COMPONENTLIST_H_

#include "Gui.h"
#include <vector>


struct ComponentListItem
    {
    ComponentListItem()
	{}
    ComponentListItem(char const * const compName, char const * const modName,
	    char const * const path):
	    mComponentName(compName), mModuleName(modName), mPathName(path)
	{}
    std::string mComponentName;
    std::string mModuleName;
    std::string mPathName;
    };

class ComponentList:public GuiTree
    {
    public:
	void init(Builder &builder)
	    {
	    GuiTree::init(builder, "ComponentTreeview", "Component List");
	    }
	void clear()
	    {
	    mListMap.clear();
	    GuiTree::clear();
	    }
	void updateComponentList();
	std::string getSelectedFileName() const;

    private:
	std::vector<ComponentListItem> mListMap;
    };



#endif /* CLASSLIST_H_ */
