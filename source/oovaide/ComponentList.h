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
    ComponentListItem(OovStringRef const compName, OovStringRef const modName,
            OovStringRef const path):
            mComponentName(compName), mModuleName(modName), mPathName(path)
        {}
    std::string mComponentName;
    std::string mModuleName;
    std::string mPathName;
    };

// This is a component list that includes modules.
class ComponentList:public GuiTree
    {
    public:
        void init(Builder &builder, OovStringRef const widgetName,
                OovStringRef const title)
            {
            GuiTree::init(builder, widgetName, title);
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



#endif /* COMPONENTLIST_H_ */
