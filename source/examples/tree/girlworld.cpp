

#include "Girl.h"
#include "YoungMan.h"

int main( int argc, const char* argv[] )
    {
    Girl girl;
    YoungMan youngMan;
    Hand hand = girl.takeHand();
    youngMan.acceptHand(hand);
    }

