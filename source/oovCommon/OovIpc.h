// OovIpc.h
//  Created on: July 16, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#ifndef OOV_IPC
#define OOV_IPC

#include "OovString.h"

/// The commands that the editor can perform.
enum EditorCommands
    {
    EC_ViewFile='v',            // arg1 = fileName, arg2 = lineNum
    };

/// The commands that the editor's container can perform.
enum EditorContainerCommands
    {
    ECC_GotoMethodDef = 'm',            // arg1=className, arg2=methodName
    ECC_ViewClassDiagram = 'c',         // arg1=className
    ECC_ViewPortionDiagram = 'p',       // arg1=className
    ECC_RunAnalysis = 'a',              // no args
    ECC_Build = 'b',                    // no args
    ECC_StopAnalysis = 's',             // no args
    };

/// Defines messages for interprocess communication.  This is merely a string
/// with command and arguments separated by delimiters.  The delimiter is a
/// comma, meaning that the arguments cannot have commas.
class OovIpcMsg:public OovString
    {
    public:
        OovIpcMsg()
            {}
        /// Simply copies the source message
        /// @param msg The source message
        OovIpcMsg(OovString const &msg):
            OovString(msg)
            {}
        /// Constructs a message from a command and arguments. The built
        /// message is just the bits put together with some delimiters.
        OovIpcMsg(int cmd, OovStringRef arg1=nullptr, OovStringRef arg2=nullptr);
        /// Get the command from the message string
        int getCommand() const;
        /// Get an argument from the message string
        /// @param argNum Zero is the command, so one and above are arguments.
        OovString getArg(size_t argNum) const;
    };

/// The interprocess communication used at this time is the parent/child pipes
/// between the editor and the editor's container, which is the Oovcde program.
class OovIpc
    {
    public:
        /// Sends a message by using printf and fflush.
        /// @param msg The message is generally of type OovIpcMsg.
        static void sendMessage(OovStringRef msg);
    };

#endif
