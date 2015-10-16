/*
 * GlobalSettings.h
 *
 *  Created on: Oct 9, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

// This is for saving things that don't belong to a project.
//
// On Linux, these are saved in ~/.local/share/data/Oovaide or
// ~/.config/Oovaide
//
// On Windows, the proper location is
//      <drive>:\Users\<username>\AppData\Roaming\Oovaide
// Or:
//      <drive>:\Users\<username>\AppData\Local\Oovaide
// Since I am not interested in Windows specific code, I will save in the
// bin directory, and if not possible, we simply won't store it, so the
// feature will not be available.
#include "OovString.h"

// There can be only one of these in a program.
class GlobalSettingsListener
    {
    public:
        GlobalSettingsListener();
        virtual void loadProject(OovStringRef projDir) = 0;
    };

class GlobalSettings
    {
    public:
        // This assumes that the projDir was properly created on disk.
        static void saveOpenProject(OovStringRef projDir);
        // While reading the projects, check if they exist on disk. If not,
        // remove them.
        static void updateRecentFilesMenu();
    };
