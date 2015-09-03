// twinkle.cpp

#include <stdio.h>      // For printf
#include "twinkle.h"

using namespace Universe;
namespace Teaching = Awards;   // Really from Universe::Awards

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
        void displaySomeThoughts(Teaching::Star const &teachingStar,
            Imaginary::FakeStar fakeStar, char const * const noStar);

    private:
        Teaching::Star mTeachingStar;
        Teaching::Star const &mTeachingStarRef;
        Imaginary::PretendStar **mPretendStar;
        // Really Universe::Movie::Star
        Movies::Star mMovieStar;
        IdentityDisplayer<Movies::Star> mMovieStarDisplay;
    };

MyThoughts::MyThoughts():
    mTeachingStarRef(mTeachingStar)
    {
    Galaxy Galaxy;
    DisplayStar littleStar;
    Teaching::Star teachingStar;
    Imaginary::FakeStar fakeStar;
    IdentityDisplayer<Awards::Star> awardStar;

    printf("My thoughts are:\n");
    littleStar.displayIdentity();
    mMovieStarDisplay.displayIdentity();
    awardStar.displayIdentity();
    displaySomeThoughts(teachingStar, fakeStar, "noStar");
    }

void MyThoughts::displaySomeThoughts(Teaching::Star const &teachingStar,
    Imaginary::FakeStar fakeStar, char const * const noStar)
    {
    Awards::Star anotherStar;
    printf("More thoughts are:\n");
    printf("  I think I am a Teaching::Star, I am a %s\n", teachingStar.whatAreYou());
    printf("  I think I am a Award::Star, I am a %s\n", anotherStar.whatAreYou());
    // The whatAreYou method is not available for the Imaginary star.
    printf("  I think I am a Imaginary::FakeStar, I don't know what I am\n");
    }


// This shows aliased star names (typedefs)
class MyOtherThoughts
    {
    public:
        // Some of these thoughts are fleeting.
        void displayOtherThoughts(Movies::Star const &moviesStar)
            {
            printf("My other thoughts are:\n");
            printf("  I think I am a NewNameDisplayStar. ");
            mNewNameStar.displayIdentity();
            printf("  I think I am a NewTypeDisplayStar. ");
            mNewTypeStar.displayIdentity();
            printf("  I think I am a Movies::Star, I am a %s\n",
                moviesStar.whatAreYou());
            }
    private:
        NotAStar mNotAStar;
        NewNameDisplayStar mNewNameStar;
        NewTypeDisplayStar mNewTypeStar;
    };


int main(int argc, const char* argv[])
    {
    MyThoughts thoughts;
    // These thoughts are not revealed during construction.
    MyOtherThoughts myOtherThoughts;
    Movies::Star moviesStar;
    myOtherThoughts.displayOtherThoughts(moviesStar);
    }
