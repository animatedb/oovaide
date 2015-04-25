// twinkle.cpp

#include <stdio.h>      // For printf
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
            Imaginary::FakeStar fakeStar, char const * const noStar);

    private:
        Teaching::Star mTeachingStar;
        Teaching::Star const &mTeachingStarRef;
        Imaginary::PretendStar **mPretendStar;
        // Really Universe::Hollywood::Star
        Hollywood::Star mHollywoodStar;
        Displayer<Hollywood::Star> mHollywoodStarDisplay;
    };

// This is for testing aliased start names (templates)
class MyOtherThoughts
	{
	private:
		NotAStar mNotAStar;
		RenamedStar mRenamedStar;
		RenamedDisplayStar mRenamedDisplayStar;
	};

#include <stdio.h>

MyThoughts::MyThoughts():
    mTeachingStarRef(mTeachingStar)
    {
    World();
    DisplayStar littleStar;
    InkStar::Star inkStar;
    Imaginary::FakeStar fakeStar;

    mHollywoodStarDisplay.displayThoughts(mHollywoodStar);
    littleStar.displayThoughts(littleStar);
    displaySomeThoughts(inkStar, fakeStar, "");
    }

void MyThoughts::displaySomeThoughts(InkStar::Star const &inkStar,
    Imaginary::FakeStar fakeStar, char const * const noStar)
    {
    InkStar::Star anotherStar;
    printf("I think I am a inkStar, I am a %s\n", inkStar.whatAreYou());
    printf("I think I am a anotherStar, I am a %s\n", anotherStar.whatAreYou());
    }

int main(int argc, const char* argv[])
    {
    MyThoughts thoughts;
	MyOtherThoughts myOtherThoughts;	// These thoughts are not revealed.
    InkStar::Star inkStar;
    Imaginary::FakeStar fakeStar;
    thoughts.displaySomeThoughts(inkStar, fakeStar, "noStar");
    }
