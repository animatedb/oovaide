// mary.h

#include <stdio.h>
#include <string>

class Location
    {
    public:
        Location(int x=0, int y=0):
            mX(x), mY(y)
            {}
        void setPosition(Location loc);
        std::string getLocationString() const;
    private:
        int mX;
        int mY;
    };

class MoveableThing
    {
    public:
        virtual void move(Location loc)
            {
            mLocation.setPosition(loc);
            }
        Location const &getLocation() const
            { return mLocation; }
        std::string getLocationString() const
            { return mLocation.getLocationString(); }
    private:
        Location mLocation;
    };

class Mammal:public MoveableThing
    {
    };

class Lamb:public Mammal
    {
    public:
        MoveableThing mFleece;
        void follows(Mammal const &leader)
            { move(leader.getLocation()); }
        virtual void move(Location loc);
        virtual void showLocation()
            {
            printf("Lamb %s, Fleece %s\n", getLocationString().c_str(),
                mFleece.getLocationString().c_str());
            }
    };

class Mary:public Mammal
    {
    public:
    	Lamb lamb;

        virtual void showLocation()
            { printf("Mary %s\n", getLocationString().c_str()); }
    };

class World
    {
    public:
	World();
    private:
    	Mary mary;
    };

