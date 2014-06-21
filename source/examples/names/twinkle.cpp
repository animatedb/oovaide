// twinkle.cpp

#include "twinkle.h"

using namespace Universe;
namespace InkStar = Teaching;   // Really from Universe::Teaching

namespace Imaginary
    {
    class FakeStar
        {
        };

    class PretendStar
        {
        };
    };

// MyThoughts are not in the Universe
class MyThoughts
    {
    public:
        MyThoughts();
        void displaySomeThoughts(InkStar::Star const &inkStar,
            Imaginary::FakeStar fakeStar, char *noStar);

    private:
        Hollywood::Star mHollywoodStar;
        Teaching::Star const &mTeachingStar;
        Imaginary::PretendStar **mPretendStar;
        // Really Universe::Hollywood::Star
        Displayer<Hollywood::Star> mHollywoodStarDisplay;
    };

#include <stdio.h>

MyThoughts::MyThoughts()
    {
    World();
    Star littleStar;
    InkStar::Star inkStar;
    Imaginary::FakeStar fakeStar;

    mHollywoodStarDisplay.displayThoughts(mHollywoodStarDisplay);
    littleStar.displayThoughts(littleStar));
    displaySomeThoughts(inkStar, fakeStar);
    }

void MyThoughts::displaySomeThoughts(InkStar::Star const &inkStar,
    FakeStar fakeStar, char *noStar)
    {
    InkStar::Star anotherStar;
    printf("I am a %s\n", inkStar.whatAreYou());
    printf("I am a %s\n", anotherStar.whatAreYou());
    }

int main(int argc, const char* argv[])
    {
    MyThoughts thoughts();
    }
