#include <utility>

#include "./hash.h"
#include "./squash2.h"

const std::string SQUASH_LABEL("squash");
const int SQUASH_VERSION = 2;

void Squash::update_buffer(const std::vector<char>& bx) {
	// result depends on append order
	_buffer.insert(_buffer.end(), bx.begin(), bx.end());
	auto [b, _ignore] = get_byte_hash(_buffer, hash::SHA1);
	_buffer = std::move(b);
}

std::vector<char> Squash::get_buffer(void) {
	return _buffer;
}

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestAssert.h>

#include "./cppunit.h"

void SquashTest::test_init_buffer(void) {
	Squash squash;
	CPPUNIT_ASSERT(squash.get_buffer().empty());
}

void SquashTest::test_update_buffer(void) {
	Squash squash;
	squash.update_buffer(std::vector<char>{});
	CPPUNIT_ASSERT(!squash.get_buffer().empty());

	squash.update_buffer(std::vector<char>{});
	CPPUNIT_ASSERT(!squash.get_buffer().empty());

	std::string s1("xxx");
	squash.update_buffer(std::vector<char>(s1.begin(), s1.end()));
	CPPUNIT_ASSERT(!squash.get_buffer().empty());

	std::string s2(123456, 'x');
	squash.update_buffer(std::vector<char>(s2.begin(), s2.end()));
	CPPUNIT_ASSERT(!squash.get_buffer().empty());
}

CPPUNIT_TEST_SUITE_REGISTRATION(SquashTest);
#endif
