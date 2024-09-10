#ifndef SRC_SQUASH1_H_
#define SRC_SQUASH1_H_

#include <vector>
#include <string>

extern const std::string SQUASH_LABEL;
extern const int SQUASH_VERSION;

class Squash {
	public:
	Squash(void):
		_buffer{} {
	}
	void init_buffer(void) {
		_buffer.clear();
	}
	void update_buffer(const std::vector<char>&);
	std::vector<char> get_buffer(void);

	private:
	std::vector<std::vector<char>> _buffer;
};

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

class SquashTest: public CPPUNIT_NS::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(SquashTest);
	CPPUNIT_TEST(test_init_buffer);
	CPPUNIT_TEST(test_update_buffer);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_init_buffer(void);
	void test_update_buffer(void);
};
#endif
#endif // SRC_SQUASH1_H_
