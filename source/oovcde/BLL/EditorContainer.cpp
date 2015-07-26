// EditorContainer.cpp
//  Created on: July 15, 2013
//  \copyright 2015 DCBlaha.  Distributed under the GPL.


#include "EditorContainer.h"
#include "OovIpc.h"
#include "Gui.h"

#if(USE_IPC)


static EditorContainer *sEditorContainer;


// Returns an empty string if no editor is found.
static OovString getEditor(GuiOptions const &guiOptions)
    {
    FilePath proc(guiOptions.getEditorPath(), FP_File);
    if(!FileIsFileOnDisk(proc))
        {
        // If the path to the user specified editor is not found, then
        // search the standard locations for the executable directory.
        FilePath testProcPath(Project::getBinDirectory(), FP_Dir);
        testProcPath.append(proc.getNameExt());
        if(FileIsFileOnDisk(testProcPath))
            {
            proc = testProcPath;
            }
        }
    // If the editor is not present, return an empty string.
    if(!FileIsFileOnDisk(proc))
        {
        proc.clear();
        Gui::messageBox("Use Analysis/Settings to set up an editor to view source");
        }
    return proc;
    }

void viewSource(GuiOptions const &guiOptions, OovStringRef const module,
        unsigned int lineNum)
    {
    if(sEditorContainer)
        {
        OovString proc = getEditor(guiOptions);
        if(proc.length())
            {
            OovProcessChildArgs args;
            args.addArg(proc);
            OovString lineArg = guiOptions.getValue(OptGuiEditorLineArg);
            if(lineArg.length() != 0)
                {
                lineArg.appendInt(static_cast<int>(lineNum));
                args.addArg(lineArg);
                }
            FilePath file(module, FP_File);
            FilePathQuoteCommandLinePath(file);
            args.addArg(file);

            std::string projArg;
            bool useOovEditor = (proc.find("oovEdit") != std::string::npos);
            if(useOovEditor)
                {
                projArg += "-p";
                projArg += Project::getProjectDirectory();
                args.addArg(projArg);
                }
            if(useOovEditor)
                {
                sEditorContainer->viewFile(proc, args.getArgv(), module, lineNum);
                }
            else
                {
                spawnNoWait(proc, args.getArgv());
                }
            }
        }
    }

EditorContainer::EditorContainer(ModelData const &modelData):
    mModelData(modelData)
    {
    sEditorContainer = this;
    mBackgroundProcess.setListener(this);
    }

bool EditorContainer::okToExit()
    {
    return(!mBackgroundProcess.isRunning());
    }

/// @todo - args are duplicate and used based on whether the
/// editor is already running.
void EditorContainer::viewFile(OovStringRef const procPath, char const * const *argv,
    OovStringRef const fn, int lineNum)
    {
    if(!mBackgroundProcess.isRunning())
        {
        mBackgroundProcess.startProcess(procPath, argv, true);
        }
    else
        {
        OovString line;
        line.appendInt(lineNum);
        mBackgroundProcess.childProcessSend(OovMsg::buildSendMsg(EC_ViewFile,
            fn, line));
        }
    }

void EditorContainer::handleCommand(OovStringRef cmdStr)
    {
    OovString retStr;
    switch(cmdStr[0])
        {
        case ECC_GetMethodDef:
            {
            ModelClassifier const *classifier = findClass(OovMsg::getArg(
                cmdStr, 1));
            if(classifier)
                {
                ModelOperation const *operation = classifier->
                    getOperation(OovMsg::getArg(cmdStr, 2), false);
                if(operation)
                    {
                    ModelModule const *module = operation->getModule();
                    OovString line;
                    line.appendInt(operation->getLineNum());
                    retStr = OovMsg::buildSendMsg(EC_ViewFile, module->getName(),
                        line);
                    }
                }
            }
            break;

        case ECC_GetClassDef:
            {
            ModelClassifier const *classifier = findClass(OovMsg::getArg(
                cmdStr, 1));
            if(classifier)
                {
                ModelModule const *module = classifier->getModule();
                OovString line;
                line.appendInt(classifier->getLineNum());
                retStr = OovMsg::buildSendMsg(EC_ViewFile, module->getName(),
                    line);
                }
            }
            break;

        case ECC_RunAnalysis:
            break;

        case ECC_StopAnalysis:
            break;
        }
    if(retStr.length() > 0)
        {
        mBackgroundProcess.childProcessSend(retStr);
        }
    }

ModelClassifier const *EditorContainer::findClass(OovStringRef name)
    {
    ModelType const *type = mModelData.findType(name);
    ModelClassifier const *classifier = nullptr;
    if(type)
        {
        classifier = ModelType::getClass(type);
        }
    return classifier;
    }

void EditorContainer::onStdOut(OovStringRef const out, size_t len)
    {
    mReceivedData.append(OovString(out, len));
    size_t pos = mReceivedData.find('\n');
    if(pos != std::string::npos)
        {
        handleCommand(OovString(mReceivedData, pos));
        mReceivedData.erase(0, pos);
        }
    }

void EditorContainer::onStdErr(OovStringRef const out, size_t len)
    {
    }

void EditorContainer::processComplete()
    {
    }

#endif
