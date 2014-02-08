// BlackSheep.h
#include <vector>

class WoolBag
    {
    public:
        char const * const quantity() const;
    };

class BlackSheep
    {
    public:
        BlackSheep();
        bool haveWool() const
            { return(mBags.size() > 0); }
        WoolBag getBag();

    private:
        std::vector<WoolBag> mBags;
    };
