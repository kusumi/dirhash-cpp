#include <sstream>
#include <iterator>
#include <algorithm>

#include "./hash.h"
#include "./squash1.h"

const std::string SQUASH_LABEL("squash");
const int SQUASH_VERSION = 1;

void Squash::update_buffer(const std::vector<char>& bx) {
	auto [b, _ignore] = get_byte_hash(bx, hash::MD5);
	_buffer.push_back(b);
}

std::vector<char> Squash::get_buffer(void) {
	// XXX directly sort vector<vector<char>>
	std::vector<std::string> s;
	for (const auto& v : _buffer)
		s.push_back(get_hex_sum(v));
	std::sort(s.begin(), s.end());
	std::ostringstream ss;
	std::copy(s.begin(), s.end(),
		std::ostream_iterator<std::string>(ss, ""));
	auto x = ss.str();
	return std::vector<char>(x.begin(), x.end());
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
