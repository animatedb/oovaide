/*
 * BuildVariablesDialog.h
 *
 *  Created on: Feb 9, 2016
 *  \copyright 2016 DCBlaha.  Distributed under the GPL.
 */

#ifndef BUILDVARIABLESDIALOG_H_
#define BUILDVARIABLESDIALOG_H_

#include "Gui.h"
#include "Project.h"
#include <vector>


struct StringIdItem
    {
    StringIdItem(char const *userString, int id):
        mUserString(userString), mId(id)
        {}
    char const *mUserString;
    int mId;
    };

class StringIdVector:public std::vector<StringIdItem>
    {
    public:
        int getIndex(OovStringRef val) const;
        int getIndex(int id) const;
        int getId(OovStringRef val) const;
    };

class BuildVarSettingsDialog:public Dialog
    {
    public:
        BuildVarSettingsDialog(ProjectReader &project, GtkWindow *parentWnd);
        bool editVariable(OovString &varStr);

        // Called from extern functions
        void updateFilterValueList();
        void addFilterList();
        void removeFilterList();
        void updateSettingText();

    private:
        StringIdVector mVarTypes;
        StringIdVector mFilterNames;
        StringIdVector mFunctions;
        GuiTree mFilterValuesTreeView;
        GuiTree mFiltersTreeView;
        OovString mVarStr;
        CompoundValue mBuildConfigs;

        virtual void beforeRun();
    };

class BuildVariablesDialog:public Dialog
    {
    public:
        BuildVariablesDialog(ProjectReader &project, GtkWindow *parentWnd);
        static bool runAdvancedDialog();

        // Called from extern functions
        void updateVarList();
        void editVariable();
        void removeVariableSetting();
        void addVariableSetting();

    private:
        ProjectReader &mProject;
        GuiTree mVarListTreeView;
        StringIdVector mVarTypes;
        BuildVarSettingsDialog mBuildVarSettingsDialog;

        virtual void beforeRun();
    };


#endif /* BUILDVARIABLESDIALOG_H_ */
