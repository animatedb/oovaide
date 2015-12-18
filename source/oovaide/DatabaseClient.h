/*
 * DatabaseClient.h
 *
 *  Created on: Oct 9, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "OovLibrary.h"
#include "ModelObjects.h"


/// This is the C style interface to the Oov database writer run-time library.
extern "C"
{
struct DatabaseWriterInterface
    {
    /// This will open or create a database in the project directory.
    /// @param projectDir The directory where the database resides.
    /// @param modelData The model data that will be written to the database.
    bool (*OpenDb)(char const *projectDir, void const *modelData);
    /// Write type information from the modelData to the database.
    /// @param passIndex This can be 0 to 1. The first pass writes basic
    ///         type information and methods. The second pass looks at
    ///         each methods statements to find related types and methods.
    /// *param typeIndex The index of the type to save from ModelData::mTypes.
    bool (*WriteDb)(int passIndex, int &typeIndex, int maxTypesPerTransaction);
    /// This writes the component information that is used by the modules table.
    bool (*WriteDbComponentTypes)(void const *compTypesFile);
    /// This writes the module relations for include or import.
    bool (*WriteDbModuleRelations)(void const *incMapFile);
    /// Get the last error string.
    char const *(*GetLastDbError)();
    /// Close the database.
    void (*CloseDb)();
    };
};

/// This loads the database writer library (dll or so), and resolves
/// the symbols so that the writeDatabase function will call the
/// appropriate functions to save all of the database information.
/// The component types information is read from the component types
/// file, and written to the database.
class DatabaseWriter:public DatabaseWriterInterface, public OovLibrary
    {
    public:
        /// Write the model data and component information.
        void writeDatabase(ModelData *modelData);

    private:
        void loadSymbols();
    };

