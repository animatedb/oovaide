/*
 * EditOptions.h
 *
 *  Created on: Feb 21, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef EDITOPTIONS_H_
#define EDITOPTIONS_H_

#include "NameValueFile.h"


#define OptEditDebuggee "Debuggee"
#define OptEditDebuggeeArgs "DebuggeeArgs"
#define OptEditDebuggerWorkingDir "DebuggerWorkDir"

class EditOptions:public NameValueFile
    {
    public:
	void setProjectDir(std::string projDir);
	void setScreenCoord(char const * const tag, int val);
	void saveScreenSize(int width, int height);
	bool getScreenCoord(char const * const tag, int &val);
	bool getScreenSize(int &width, int &height);
    };



#endif /* EDITOPTIONS_H_ */
