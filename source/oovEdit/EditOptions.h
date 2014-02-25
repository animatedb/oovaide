/*
 * EditOptions.h
 *
 *  Created on: Feb 23, 2014
 *      Author: dave
 */

#ifndef EDITOPTIONS_H_
#define EDITOPTIONS_H_

#include "NameValueFile.h"
#include "Project.h"

#define OptEditDebuggee "Debuggee"

class EditOptions:public NameValueFile
    {
    public:
	EditOptions()
	    {
	    setNameValue(OptEditDebuggee, "./oovEdit");
	    }
	void saveScreenSize(int x, int y);
	bool getScreenSize(int &x, int &y);
	void setProjectDir(std::string const &dir)
	    { mProjectDir = dir; }
    private:
	std::string mProjectDir;
    };

#endif /* EDITOPTIONS_H_ */
