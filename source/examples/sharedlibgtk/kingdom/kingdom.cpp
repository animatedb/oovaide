// kingdom.cpp
#include "kingdom.h"
#include <stdio.h>

void ResourceClient::open(char const * const fn)
    {
    mModule = g_module_open(fn, G_MODULE_BIND_LAZY);
    if(mModule)
        loadSymbols();
    }

void ResourceClient::close()
    {
    if(mModule)
        {
        g_module_close(mModule);
        mModule = NULL;
        }
    }

void ResourceClient::loadSymbols()
    {
    loadModuleSymbol("getResourceName", (gpointer*)&mInterface.getResourceName);
    loadModuleSymbol("putTogether", (gpointer*)&mInterface.putTogether);
    }

void ResourceClient::loadModuleSymbol(const gchar *symbolName, gpointer *symbol) const
    {
    if(!g_module_symbol(mModule, symbolName, symbol))
        {
        *symbol = NULL;
        }
    }

//////////////

Kingdom::Kingdom()
    {
    Egg humpty("Humpty Dumpty");
    ResourceClient client;
    char buf[100];

    static char const * const resources[] =
        {
        "./resources/horses.so",
        "./resources/men.so",
        };

    printf("Eggs identity is %s\n", humpty.getName().c_str());
    printf("Running fall function\n");
    humpty.fall();
    printf("Eggs identity is %s\n", humpty.getName().c_str());
    for(auto const &resource : resources)
        {
        client.open(resource);
        if(!client.getInterface().putTogether())
            {
            client.getInterface().getResourceName(buf, sizeof(buf));
            printf("%s could not put together %s\n", buf, humpty.getName().c_str());
            }
        client.close();
        }
    }


int main( int argc, const char* argv[] )
    {
    Kingdom kingdom;
    }
