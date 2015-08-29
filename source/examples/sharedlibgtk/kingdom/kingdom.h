// kingdom.h

#include <gmodule.h>
#include <string>
#include <algorithm>


/// This is the interfaces to resources that can be found within the kingdom.
/// Each resource will be in its own run-time loaded library.
class ResourceClientInterface
    {
    public:
        /// Get the resource name from the run-time loaded resource library.
        void (*getResourceName)(char *buf, int maxBytes);
        /// Provide a function that returns true if putTogether succeeds.
        bool (*putTogether)();
    };


/// This will hold the client side of one resource.
class ResourceClient
    {
    public:
        /// Load the resource by using the file name.
        void open(char const * const fn);
        /// Close the resource.
        void close();
        /// Check if the resource is open.
        bool isOpen() const
            { return(mModule != nullptr); }
        /// This returns the interface that can be used to communicate with
        /// the run-time loaded resource.
        ResourceClientInterface const &getInterface() const
            { return mInterface; }

    private:
        GModule *mModule;
        ResourceClientInterface mInterface;
        /// Load the symbols needed for the interface to the library.
        void loadSymbols();
        /// Load a single symbol from the run-time library.
        void loadModuleSymbol(const gchar *symbolName, gpointer *symbol) const;
    };

/// This program provides no faces, so there is no way to put egg on your face.
class Egg
    {
    public:
        /// Each egg can have its own name.
        Egg(char const * const name):
            mName(name)
            {}
        /// This will return the name that was previously given.
        std::string getName() const
            { return mName; }
        /// This will randomly shuffle the name.
        void fall()
            { random_shuffle(mName.begin(), mName.end()); }

    private:
        std::string mName;
    };

class Kingdom
    {
    public:
        /// The whole kingdom has just a single constructor.
        Kingdom();
    };
