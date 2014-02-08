// men.h

#include <gmodule.h>
#if defined(SHARED_LIBRARY)
#  define SHAREDSHARED_EXPORT G_MODULE_EXPORT
#else
#  define SHAREDSHARED_EXPORT G_MODULE_IMPORT
#endif


extern "C"
    {
    SHAREDSHARED_EXPORT void getResourceName(char *buf, int maxBytes);
    SHAREDSHARED_EXPORT bool putTogether();
    };

class Men
    {
    public:
        char const * const getName() const;
        bool putTogether();
    };
