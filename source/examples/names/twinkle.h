// twinkle.h

// The namespace Hierarchy is:
//  Universe
//      {
//      Hollywood
//      Teaching
//      }
//  Imaginary
//
// Classes:
//      Universe::Star
//      Universe::World
//      Universe::Hollywood::Star
//      Universe::Teaching::Star
//      Imaginary::FakeStar
//      Imaginary::PretendStar

template<class T> class Displayer
    {
    void displayThoughts(T type)
        { printf("I am a %s\n", type.whatAreYou()); }
    };


namespace Universe
{

class Star:public Displayer<Star>
    {
    public:
        void twinkle()
            {}
        char const *whatAreYou()
            { return("little star"); }
    };

class World
    {
    public:
        World()
            {}
    };


namespace Hollywood
{

class Star
    {
    public:
        char const *whatAreYou()
            { return("Hollywood star"); }
    };

};

namespace Teaching
{
class Star
    {
    public:
        char const *whatAreYou()
            { return("ink star"); }
    };
};

};
