//
// This file tests what happens when GlobalFixture::setUp() fails
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>
#include <stdio.h>

class Fixture : public CxxTest::GlobalFixture
{
public:
    bool setUp() { return false; }
};

//
// We can rely on this file being included exactly once
// and declare this global variable in the header file.
//
static Fixture fixture;
 
class Suite : public CxxTest::TestSuite
{
public:
    void testOne()
    {
        TS_FAIL( "Shouldn't get here at all" );
    }
};

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//
