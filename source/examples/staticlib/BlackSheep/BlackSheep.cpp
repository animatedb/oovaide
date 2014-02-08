// BlackSheep.cpp
#include "BlackSheep.h"


char const * const WoolBag::quantity() const
    { return("Full"); }

BlackSheep::BlackSheep()
    {
    for(int i=0; i<3; i++)
        mBags.push_back(WoolBag());
    }

WoolBag BlackSheep::getBag()
    {
    WoolBag bag = mBags[mBags.size()-1];
    mBags.pop_back();
    return bag;
    }
