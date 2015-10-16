/*
 * OovError.h
 *
 *  Created on: Sept. 18, 2015
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOV_ERROR
#define OOV_ERROR

#include "OovString.h"

#define OovStatusReturn OovStatus __attribute__((warn_unused_result))
enum OovErrorTypes { ET_Error, ET_Info, ET_Diagnostic };
// Layout of OovStatus codes in 32 bits:
//    ---------------   ---------------   ----------------  ---------------
//    F E D C B A 9 8   7 6 5 4 3 2 1 0   F E D C B A 9 8   7 6 5 4 3 2 1 0
//    ---------------   ---------------   ---------------   ---------------
//   |              Component            |R|            Class              |
//    ---------------------------------------------------------------------
//
//  R = report flag
//
enum OovStatusShift { SO_ComponentShift=16, SO_ReportShift=15 };
enum OovErrorComponent
    {
    EC_Oovaide = 1 << SO_ComponentShift,
    EC_OovBuilder = 2 << SO_ComponentShift,
    EC_OovCMaker = 4 << SO_ComponentShift,
    EC_OovCovInstr = 8 << SO_ComponentShift,
    EC_OovCppParser = 10 << SO_ComponentShift,
    EC_OovDbWriter = 20 << SO_ComponentShift,
    EC_OovEdit = 40 << SO_ComponentShift
    };
enum OovStatusReportStatus { RS_ReportNeeded=SO_ReportShift, RS_Reported };
enum OovStatusClass
    {
    SC_Ok,
    SC_File,            // use for file related errors.
    SC_Time,            // use for various types of timeouts.
    SC_User,            // user stopped the operation.
    SC_Logic,           // other
    };

class OovErrorListener
    {
    public:
        virtual void errorListener(OovStringRef str, OovErrorTypes et) = 0;
        virtual ~OovErrorListener();
    };

/*
* The purpose of this class is to send errors to somewhere that the user can
* view.  In a command line program, the output will go to either stdout or
* stderr. In a GUI program, the output may go to a scrolling window.
*/
class OovError
    {
    public:
        /// If a listener is not set, this defaults to using stdout and stderr.
        static void setListener(OovErrorListener *listener);
        static void setComponent(OovErrorComponent component);
        static void report(OovErrorTypes et, OovStringRef str);
        static char const *getComponentString();
    };


/*
At the lowest level, error status codes are returned in the OovStatus class.
If they are dropped without being reported, then an error will be output.
Failure reports are not on parts of files, only whole files.
OovError::report sends scrolling data to the user that they do not have to
acknowledge with a button press, for example printf or a scrolling window in
gtk apps. High level files such as component types are reported, but
no GUI/gtk message boxes until the GUI layer.  The GUI layer may use !ok()
instead of needReport because a message box may be displayed no matter whether
the error was sent to the scrolling output
*/
/*
The purpose of this class is to prevent dropping errors on the floor, and to
prevent reporting errors too many times.
This tries to prevent common errors such as:
    - Creating an OovStatus in a nested inner code block and dropping it.
    - Not returning a status
    - Not handling a returned status

Functions that ask a question such as isDir() will return a boolean, and
pass a status argument as reference.
*/
/*
OovStatus is used to catch unreported return codes.

Typical use:

OovStatusReturn func1();

OovStatusReturn func2()
    {
    OovStatus status = func1();
    if(status.needReport())
        {
        status.report(ET_Error, "something failed.");
        }
    return status;
    }
*/
class OovStatus
    {
    public:
        // There are three ways to initialize an OovStatus:
        // 1. initialize with a success status from a boolean function
        // 2. initialize as an assignment from another OovStatus variable
        // 3. To initialize an OovStatus where the error is set later,
        //    use something like : OovStatus status(true, SC_File);
        OovStatus(bool success, OovStatusClass sc):
            mStatus(SC_Ok)
            {
            statInScope();
            set(success, sc);
            }
        ~OovStatus()
            {
            statOutScope();
            }
        /// Call this with false to set an error status.
        void set(bool success, OovStatusClass sc);
        /// This may be different than the ok() function. The report() or
        /// reported() functions may indicate that an error is no longer
        /// needed to be reported, but the ok() may still indicate that
        /// a function failed.
        bool needReport() const
            { return(mStatus & RS_ReportNeeded); }
        /// This is used by the GUI or elsewhere to indicate that the
        /// error has been handled or discarded.
        void reported()
            { removeError(mStatus); }
        /// This should rarely be used. It clears the reported status, and
        /// clears the error.
        void clearError();
        /// This sends the string to OovError::report, which typically goes
        /// to stdout, stderr, or a GUI scrolling output.
        void report(OovErrorTypes et, OovStringRef str);
        /// See notes for needReport().
        bool ok() const
            { return(mStatus == SC_Ok); }
        /// This can be called to see if there are any errors that have
        /// not been cleaned up yet. This should typically be called
        /// at the highest level of error handling, meaning that the
        /// OovStatus object will not be passed any higher up the chain.
        /// This should only be used if the OovStatus object is not
        /// going to go out of scope, since if it does, this is called
        /// anyway.
        static void checkErrors();

    private:
        int mStatus;
        static int mScopeCounter;

        static void addError(int err);
        static void removeError(int err);
        static void statInScope()
            {
            mScopeCounter++;
            }
        static void statOutScope()
            {
            mScopeCounter--;
            if(mScopeCounter == 0)
                {
                checkErrors();
                }
            }
        static void reportStr(OovErrorTypes et, OovStringRef str);
    };

#endif
