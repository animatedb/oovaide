/*
 * DatabaseClient.h
 *
 *  Created on: Oct 9, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovLibrary.h"
#include "ModelObjects.h"


// This is the C style interface to the Oov database writer run-time library.
extern "C"
{
struct DatabaseWriterInterface
    {
    bool (*OpenDb)(char const *projectDir, void const *modelData);
    bool (*WriteDb)(int passIndex, int &typeIndex, int maxTypesPerTransaction);
    bool (*WriteDbComponentTypes)(void const *compTypesFile);
    char const *(*GetLastError)();
    void (*CloseDb)();
    };
};


class DatabaseWriter:public DatabaseWriterInterface, public OovLibrary
    {
    public:
        void writeDatabase(ModelData *modelData);

    private:
        void loadSymbols();
    };

