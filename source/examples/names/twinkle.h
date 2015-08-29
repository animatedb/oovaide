// twinkle.h

// Sorry, this may be a bit nebulous.
//
// The Oovcde program must show all relationships between the correct
// objects even if a particular namespace is not specified at some point in
// this source code.
//
// This example shows nested namespaces, using namespace, namespace assignment,
// multiple uses of templates, typedefs of templates, etc.
// It also shows inheritance, aggregation, composition, class member methods
// and variables, method variables, method parameters, etc.
//
// The namespace Hierarchy is:
//  Universe
//      {
//      Movies
//      Awards
//      }
//  Imaginary
//
// Classes:
//      Universe::Galaxy
//      Universe::Star
//      Universe::DisplayStar
//      Universe::Movies::Star
//      Universe::Awards::Star
//      Imaginary::FakeStar
//      Imaginary::PretendStar


// @param T This requires a class that has a whatAreYou() method.
template<class T> class IdentityDisplayer:public T
    {
    public:
        void displayIdentity()
            {
            printf("I am a %s and I%s twinkle\n", this->whatAreYou(),
                this->haveTwinkle() ? "" : " don't");
            }
    };


namespace Universe
{
    namespace Movies
    {
    class Star
        {
        public:
            char const *whatAreYou() const
                { return("movie star"); }
            bool haveTwinkle() const
                { return true; }
        };
    };

    namespace Awards
    {
    class Star
        {
        public:
            char const *whatAreYou() const
                { return("award star"); }
            bool haveTwinkle() const
                { return false; }
        };
    };

class Star
    {
    public:
        char const *whatAreYou() const
            { return("little star"); }
        bool haveTwinkle() const
            { return true; }
    };

class DisplayStar:public Star, public IdentityDisplayer<Star>
    {
    };

class Galaxy
    {
    public:
        Galaxy()
            {}

    private:
        DisplayStar mStar;
        Awards::Star mAwardStar;
        Movies::Star mMovieStar;
        IdentityDisplayer<Awards::Star> mOtherAwardStar;
    };


typedef int NotAStar;
typedef Star AlteredStar;
typedef DisplayStar NewNameDisplayStar;
typedef IdentityDisplayer<Star> NewTypeDisplayStar;
};
