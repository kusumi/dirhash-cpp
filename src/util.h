#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <tuple>
#include <string>

enum class FileType {
	Dir,
	Reg,
	Device,
	Symlink,
	Unsupported,
	Invalid,
};

// XXX Unlike dirload-cpp, since all paths (except for broken symlink targets)
// are guaranteed to exist, lexical=true by default works better with dirhash
// as it doesn't resolve symlink.
std::string canonicalize_path(const std::string&, bool=true);
std::string get_abspath(const std::string&, bool=true);
std::string get_dirpath(const std::string&, bool=true);
std::string get_basename(const std::string&, bool=true);
bool is_abspath(const std::string&);
bool is_windows(void);
char get_path_separator(void);
FileType get_raw_file_type(const std::string&);
FileType get_file_type(const std::string&);
const std::string& get_file_type_string(const FileType&);
bool path_exists(const std::string&);
std::tuple<std::string, bool> is_valid_hexsum(const std::string&);
std::string get_xsum_format_string(const std::string&, const std::string&,
	bool);
std::string get_num_format_string(unsigned long, const std::string&);
void print_num_format_string(unsigned long, const std::string&);
void panic_file_type(const std::string&, const std::string&, const FileType&);

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

class UtilTest: public CPPUNIT_NS::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(UtilTest);
	CPPUNIT_TEST(test_canonicalize_path);
	CPPUNIT_TEST(test_get_abspath);
	CPPUNIT_TEST(test_get_dirpath);
	CPPUNIT_TEST(test_get_basename);
	CPPUNIT_TEST(test_is_abspath);
	CPPUNIT_TEST(test_is_windows);
	CPPUNIT_TEST(test_get_path_separator);
	CPPUNIT_TEST(test_get_raw_file_type);
	CPPUNIT_TEST(test_get_file_type);
	CPPUNIT_TEST(test_get_file_type_string);
	CPPUNIT_TEST(test_path_exists);
	CPPUNIT_TEST(test_is_valid_hexsum);
	CPPUNIT_TEST(test_get_num_format_string);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_canonicalize_path(void);
	void test_get_abspath(void);
	void test_get_dirpath(void);
	void test_get_basename(void);
	void test_is_abspath(void);
	void test_is_windows(void);
	void test_get_path_separator(void);
	void test_get_raw_file_type(void);
	void test_get_file_type(void);
	void test_get_file_type_string(void);
	void test_path_exists(void);
	void test_is_valid_hexsum(void);
	void test_get_num_format_string(void);
};
#endif
#endif // SRC_UTIL_H_
