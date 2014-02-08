#include "mary.h"

void Location::setPosition(Location loc)
    {
    mX = loc.mX;
    mY = loc.mY;
    }

std::string Location::getLocationString() const
    {
    char buf[40];
    sprintf(buf, "%d, %d", mX, mY);
    return buf;
    }

void Lamb::move(Location loc)
    {
    Mammal::move(loc);
    mFleece.move(loc);
    }

World::World()
    {
    mary.move(Location(5, 5));
    mary.lamb.follows(mary);

    mary.showLocation();
    mary.lamb.showLocation();
    }

int main( int argc, const char* argv[] )
    {
    World world;
    }
