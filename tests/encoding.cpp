#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestEncoding
#include <boost/test/unit_test.hpp>

#include <text-gl/utf8.h>


using namespace TextGL;

BOOST_AUTO_TEST_CASE(prev_test)
{
    const int8_t text[] = "БejЖbaba";
    UTF8Char characters[8];

    size_t i = 8;
    const int8_t *p = GetUTF8Position(text, i);

    BOOST_CHECK_EQUAL(*p, NULL);  // string termination

    for (; p > text; i--, p = PrevUTF8Char(p, characters[i]));

    BOOST_CHECK_EQUAL(characters[0], 'Б');
    BOOST_CHECK_EQUAL(characters[1], 'e');
    BOOST_CHECK_EQUAL(characters[3], 'Ж');
    BOOST_CHECK_EQUAL(characters[7], 'a');
}

BOOST_AUTO_TEST_CASE(next_test)
{
    const int8_t text[] = "БejЖbaba";
    UTF8Char characters[8];

    size_t i = 0;
    const int8_t *p;

    for (p = text; *p; p = NextUTF8Char(p, characters[i]), i++);

    BOOST_CHECK_EQUAL(characters[0], 'Б');
    BOOST_CHECK_EQUAL(characters[1], 'e');
    BOOST_CHECK_EQUAL(characters[3], 'Ж');
    BOOST_CHECK_EQUAL(characters[7], 'a');
}
