/*
 * Options.h
 *
 *  Created on: Jun 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <vector>
#include "Project.h"


// These options are saved in the oovcde-gui.txt file
#define OptGuiShowAttributes "ShowAttributes"
#define OptGuiShowOperations "ShowOperations"
#define OptGuiShowOperParams "ShowOperParams"
#define OptGuiShowOperReturn "ShowOperReturn"
#define OptGuiShowAttrTypes "ShowAttrTypes"
#define OptGuiShowOperTypes "ShowOperTypes"
#define OptGuiShowPackageName "ShowPackageName"

#define OptGuiShowOovSymbols "ShowOovSymbols"
#define OptGuiShowOperParamRelations "ShowOperParamRelations"
#define OptGuiShowOperBodyVarRelations "ShowOperBodyVarRelations"
#define OptGuiShowRelationKey "ShowRelationKey"

#define OptGuiShowCompImplicitRelations "ShowCompImplicitRelations"

#define OptGuiEditorPath "EditorPath"
#define OptGuiEditorLineArg "EditorLineArg"


// This is for the oovcde.txt file. Typically use ProjectReader directly for options.
class OptionsDefaults
    {
    public:
        OptionsDefaults(ProjectReader &project):
            mProject(project)
            {}
        void setDefaultOptions();

    private:
        ProjectReader &mProject;
    };

// This is for the oovcde-gui.txt file
class GuiOptions:public NameValueFile
    {
    public:
        OovString getEditorPath() const
            { return getValue(OptGuiEditorPath); }
        void setDefaultOptions();
        void read();

    private:
        void readFile();        // Not defined to prevent use.
    };

#endif /* OPTIONS_H_ */
