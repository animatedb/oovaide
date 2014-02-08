/*
 * Options.h
 *
 *  Created on: Jun 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <vector>
#include "NameValueFile.h"

class BuildOptions:public NameValueFile
    {
    public:
	void setDefaultOptions();
    };


#define OptBuildConfigs "BuildConfigs"

#define OptGuiShowAttributes "ShowAttributes"
#define OptGuiShowOperations "ShowOperations"
#define OptGuiShowOperParams "ShowOperParams"
#define OptGuiShowAttrTypes "ShowAttrTypes"
#define OptGuiShowOperTypes "ShowOperTypes"
#define OptGuiShowPackageName "ShowPackageName"

#define OptGuiShowOovSymbols "ShowOovSymbols"
#define OptGuiShowOperParamRelations "ShowOperParamRelations"
#define OptGuiShowOperBodyVarRelations "ShowOperBodyVarRelations"

#define OptGuiEditorPath "EditorPath"
#define OptGuiEditorLineArg "EditorLineArg"

class GuiOptions:public NameValueFile
    {
    public:
	void setDefaultOptions();
    };

extern BuildOptions gBuildOptions;
extern GuiOptions gGuiOptions;


#endif /* OPTIONS_H_ */
