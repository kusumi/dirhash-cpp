#ifndef SRC_STAT_H_
#define SRC_STAT_H_

#include <vector>
#include <string>

#include "./util.h"

class Stat {
	public:
	Stat(void);
	void init_stat(void);

	// num stat
	unsigned long num_stat_total(void) const {
		return num_stat_directory() + num_stat_regular() +
			num_stat_device() + num_stat_symlink();
	}
	unsigned long num_stat_directory(void) const {
		return static_cast<unsigned long>(_stat_directory.size());
	}
	unsigned long num_stat_regular(void) const {
		return static_cast<unsigned long>(_stat_regular.size());
	}
	unsigned long num_stat_device(void) const {
		return static_cast<unsigned long>(_stat_device.size());
	}
	unsigned long num_stat_symlink(void) const {
		return static_cast<unsigned long>(_stat_symlink.size());
	}
	unsigned long num_stat_unsupported(void) const {
		return static_cast<unsigned long>(_stat_unsupported.size());
	}
	unsigned long num_stat_invalid(void) const {
		return static_cast<unsigned long>(_stat_invalid.size());
	}
	unsigned long num_stat_ignored(void) const {
		return static_cast<unsigned long>(_stat_ignored.size());
	}

	// append stat
	void append_stat_total(void) {
	}
	void append_stat_directory(const std::string& f) {
		_stat_directory.push_back(f);
	}
	void append_stat_regular(const std::string& f) {
		_stat_regular.push_back(f);
	}
	void append_stat_device(const std::string& f) {
		_stat_device.push_back(f);
	}
	void append_stat_symlink(const std::string& f) {
		_stat_symlink.push_back(f);
	}
	void append_stat_unsupported(const std::string& f) {
		_stat_unsupported.push_back(f);
	}
	void append_stat_invalid(const std::string& f) {
		_stat_invalid.push_back(f);
	}
	void append_stat_ignored(const std::string& f) {
		_stat_ignored.push_back(f);
	}

	// print stat
	void print_stat_directory(const std::string& inp) const {
		print_stat(_stat_directory,
			get_file_type_string(FileType::Dir), inp);
	}
	void print_stat_regular(const std::string& inp) const {
		print_stat(_stat_regular, get_file_type_string(FileType::Reg),
			inp);
	}
	void print_stat_device(const std::string& inp) const {
		print_stat(_stat_device, get_file_type_string(FileType::Device),
			inp);
	}
	void print_stat_symlink(const std::string& inp) const {
		print_stat(_stat_symlink,
			get_file_type_string(FileType::Symlink), inp);
	}
	void print_stat_unsupported(const std::string& inp) const {
		print_stat(_stat_unsupported,
			get_file_type_string(FileType::Unsupported), inp);
	}
	void print_stat_invalid(const std::string& inp) const {
		print_stat(_stat_invalid,
			get_file_type_string(FileType::Invalid), inp);
	}
	void print_stat_ignored(const std::string& inp) const {
		print_stat(_stat_ignored, "ignored file", inp);
	}
	void print_stat(const std::vector<std::string>&, const std::string&,
		const std::string&) const;

	// num written
	unsigned long num_written_total(void) const {
		return num_written_directory() + num_written_regular() +
			num_written_device() + num_written_symlink();
	}
	unsigned long num_written_directory(void) const {
		return _written_directory;
	}
	unsigned long num_written_regular(void) const {
		return _written_regular;
	}
	unsigned long num_written_device(void) const {
		return _written_device;
	}
	unsigned long num_written_symlink(void) const {
		return _written_symlink;
	}

	// append written
	void append_written_total([[maybe_unused]]unsigned long written) {
	}
	void append_written_directory(unsigned long written) {
		_written_directory += written;
	}
	void append_written_regular(unsigned long written) {
		_written_regular += written;
	}
	void append_written_device(unsigned long written) {
		_written_device += written;
	}
	void append_written_symlink(unsigned long written) {
		_written_symlink += written;
	}

	private:
	std::vector<std::string> _stat_directory; // hashed
	std::vector<std::string> _stat_regular; // hashed
	std::vector<std::string> _stat_device; // hashed
	std::vector<std::string> _stat_symlink; // hashed
	std::vector<std::string> _stat_unsupported;
	std::vector<std::string> _stat_invalid;
	std::vector<std::string> _stat_ignored;
	unsigned long _written_directory; // hashed
	unsigned long _written_regular; // hashed
	unsigned long _written_device; // hashed
	unsigned long _written_symlink; // hashed
};

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

class StatTest: public CPPUNIT_NS::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(StatTest);
	CPPUNIT_TEST(test_num_stat_regular);
	CPPUNIT_TEST(test_append_stat_regular);
	CPPUNIT_TEST(test_num_written_regular);
	CPPUNIT_TEST(test_append_written_regular);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_num_stat_regular(void);
	void test_append_stat_regular(void);
	void test_num_written_regular(void);
	void test_append_written_regular(void);
};
#endif
#endif // SRC_STAT_H_
