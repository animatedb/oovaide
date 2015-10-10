/*
 * DatabaseClient.cpp
 *
 *  Created on: Oct 9, 2015
 */

#include "DatabaseClient.h"
#include "Project.h"
#include "Gui.h"
#include "Components.h"

void DatabaseWriter::writeDatabase(ModelData *modelData)
    {
    // Linux is interesting here.  If the project directory (a static
    // string) is read after the library is opened, then the string
    // is empty in this calling executable code.  So this must be
    // read before the library is opened, then passed to the library.
    OovString projDir = Project::getProjectDirectory();
#ifdef __linux__
    FilePath reportLibName(Project::getLibDirectory(), FP_Dir);
    reportLibName.appendFile("liboovDbWriter.so");
#else
    OovStringRef reportLibName = "oovDbWriter.dll";
#endif
    bool success = OovLibrary::open(reportLibName.getStr());
    if(success)
        {
        loadSymbols();
        bool success = OpenDb(projDir.getStr(), modelData);
        if(success)
            {
            TaskBusyDialog progressDlg;
            progressDlg.setParentWindow(Gui::getMainWindow());
            bool keepGoing = true;
            size_t totalTypes = modelData->mTypes.size();
            for(int pass=0; pass<2 && success && keepGoing; pass++)
                {
                int typeIndex = 0;
                OovString str = "Adding Data - Pass ";
                str.appendInt(pass+1);
                str += " of 2";
                progressDlg.startTask(str.getStr(), totalTypes);
                while(typeIndex < static_cast<int>(totalTypes) && success && keepGoing)
                    {
                    success = WriteDb(pass, typeIndex, 40);
                    if(success)
                        {
                        keepGoing = progressDlg.updateProgressIteration(typeIndex, nullptr, true);
                        typeIndex++;
                        }
                    }
                progressDlg.endTask();
                }
            if(success)
                {
                ComponentTypesFile compFile;
                OovStatus status = compFile.read();
                if(status.ok())
                    {
                    success = WriteDbComponentTypes(&compFile);
                    }
                if(status.needReport())
                    {
                    status.reported();
                    OovError::report(ET_Error, "Unable to open component types file.");
                    }
                }
            CloseDb();
            }
        if(!success)
            {
            Gui::messageBox(GetLastError());
            }
        }
    else
        {
        OovString errStr = "Unable to find ";
        errStr += reportLibName;
        Gui::messageBox(errStr.getStr());
        }
    }

void DatabaseWriter::loadSymbols()
    {
    loadModuleSymbol("OpenDb", (OovProcPtr*)&OpenDb);
    loadModuleSymbol("WriteDb", (OovProcPtr*)&WriteDb);
    loadModuleSymbol("WriteDbComponentTypes", (OovProcPtr*)&WriteDbComponentTypes);
    loadModuleSymbol("GetLastError", (OovProcPtr*)&GetLastError);
    loadModuleSymbol("CloseDb", (OovProcPtr*)&CloseDb);
    }
