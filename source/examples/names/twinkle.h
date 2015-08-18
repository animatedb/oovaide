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

template<class T> class IdentityDisplayer:public T
    {
    public:
        void displayIdentity()
            { printf("I am a %s\n", this->whatAreYou()); }
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
        };
    };

    namespace Awards
    {
    class Star
        {
        public:
            char const *whatAreYou() const
                { return("award star"); }
        };
    };

class Star
    {
    public:
        char const *whatAreYou() const
            { return("little star"); }
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
    };


typedef int NotAStar;
typedef Star AlteredStar;
typedef DisplayStar NewNameDisplayStar;
typedef IdentityDisplayer<Star> NewTypeDisplayStar;
};
