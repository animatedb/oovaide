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

class BuildOptions:public NameValueFile
    {
    public:
	// Warning: project must be set up before calling this.
	bool read()
	    {
	    setFilename(Project::getProjectFilePath());
	    return NameValueFile::readFile();
	    }
	void setDefaultOptions();
    private:
	bool readFile();	// Prevent calling NameValueFile - Not defined
    };


#define OptBuildConfigs "BuildConfigs"

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

class GuiOptions:public NameValueFile
    {
    public:
	void setDefaultOptions();
	// Warning: project must be set up before calling this.
	void read();

    private:
	bool readFile();	// Prevent calling NameValueFile - Not defined
    };

extern BuildOptions gBuildOptions;
extern GuiOptions gGuiOptions;


#endif /* OPTIONS_H_ */
