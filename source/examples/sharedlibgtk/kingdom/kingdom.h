// kingdom.h

#include <gmodule.h>
#include <string>
#include <algorithm>

class ResourceClientInterface
    {
    public:
        void (*getResourceName)(char *buf, int maxBytes);;
        bool (*putTogether)();
    };

class ResourceClient
    {
    public:
        void open(char const * const fn);
        void close();
        bool isOpen() const
            { return(mModule != nullptr); }
        ResourceClientInterface const &getInterface() const
            { return mInterface; }
    private:
        GModule *mModule;
        ResourceClientInterface mInterface;
        void loadSymbols();
        void loadModuleSymbol(const gchar *symbolName, gpointer *symbol) const;
    };

class Egg
    {
    public:
        Egg(char const * const name):
            mName(name)
            {}
        std::string getName() const
            { return mName; }
        void fall()
            { random_shuffle(mName.begin(), mName.end()); }
    private:
        std::string mName;
    };

class Kingdom
    {
    public:
        Kingdom();
    };
