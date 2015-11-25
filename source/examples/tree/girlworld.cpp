

#include "Girl.h"
#include "YoungMan.h"

class GirlWorld
    {
    public:
        void run()
            {
            Hand hand = mGirl.takeHand();
            mYoungMan.acceptHand(hand);
            }

    private:
        Girl mGirl;

        YoungMan mYoungMan;

    };

int main( int argc, const char* argv[] )
    {
    GirlWorld gw;
    gw.run();
    }

