#include <iostream>

#include <cassert>

#include "./dir.h"
#include "./global.h"
#include "./stat.h"
#include "./util.h"

Stat::Stat(void):
	_stat_directory{},
	_stat_regular{},
	_stat_device{},
	_stat_symlink{},
	_stat_unsupported{},
	_stat_invalid{},
	_stat_ignored{},
	_written_directory(0),
	_written_regular(0),
	_written_device(0),
	_written_symlink(0) {
}

void Stat::init_stat(void) {
	_stat_directory.clear();
	_stat_regular.clear();
	_stat_device.clear();
	_stat_symlink.clear();
	_stat_unsupported.clear();
	_stat_invalid.clear();
	_stat_ignored.clear();

	_written_directory = 0;
	_written_regular = 0;
	_written_device = 0;
	_written_symlink = 0;
}

void Stat::print_stat(const std::vector<std::string>& l, const std::string& msg,
	const std::string& inp) const {
	if (l.empty())
		return;
	print_num_format_string(l.size(), msg);

	for (const auto& v : l) {
		auto f = get_real_path(v, inp);
		auto t1 = get_raw_file_type(v);
		auto t2 = get_file_type(v);
		assert(t2 != FileType::Symlink); // symlink chains resolved
		if (t1 == FileType::Symlink) {
			assert(opt::ignore_symlink || t2 == FileType::Dir ||
				t2 == FileType::Invalid);
			std::cout << f << " (" << get_file_type_string(t1)
				<< " -> " << get_file_type_string(t2) << ")"
				<< std::endl;
		} else {
			assert(t2 != FileType::Dir);
			std::cout << f << " (" << get_file_type_string(t1)
				<< ")" << std::endl;
		}
	}
}

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestAssert.h>

#include "./cppunit.h"

void StatTest::test_num_stat_regular(void) {
	// 0
	Stat stat;
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 0lu);

	// 0
	stat.init_stat();
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 0lu);
}

void StatTest::test_append_stat_regular(void) {
	// 1
	Stat stat;
	stat.append_stat_regular("a");
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 1lu);

	// 2
	stat.append_stat_regular("b");
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 2lu);

	// 3
	stat.append_stat_regular("c");
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 3lu);

	// 1
	stat.init_stat();
	stat.append_stat_regular("d");
	CPPUNIT_ASSERT_EQUAL(stat.num_stat_regular(), 1lu);
}

void StatTest::test_num_written_regular(void) {
	Stat stat;
	CPPUNIT_ASSERT_EQUAL(stat.num_written_regular(), 0lu);

	stat.init_stat();
	CPPUNIT_ASSERT_EQUAL(stat.num_written_regular(), 0lu);
}

void StatTest::test_append_written_regular(void) {
	Stat stat;
	stat.append_written_regular(9999999999);
	CPPUNIT_ASSERT_EQUAL(stat.num_written_regular(), 9999999999lu);

	stat.append_written_regular(1);
	CPPUNIT_ASSERT_EQUAL(stat.num_written_regular(), 10000000000lu);

	stat.init_stat();
	CPPUNIT_ASSERT_EQUAL(stat.num_written_regular(), 0lu);
}

CPPUNIT_TEST_SUITE_REGISTRATION(StatTest);
#endif
