#include <iostream>
#include <array>
#include <filesystem>

#include <cctype>
#include <cassert>

#include "./util.h"

namespace {
// This function
// * does not resolve symlink by default
// * resolves symlink when lexical=false
// * works with non existent path
std::filesystem::path canonicalize_path_impl(const std::string& f,
	bool lexical) {
	if (lexical)
		return std::filesystem::path(f).lexically_normal();
	else
		return std::filesystem::weakly_canonical(f);
}

std::string trim_end(const std::filesystem::path& p) {
	auto f = std::string(p);
	while (f.size() > 1 && f.ends_with("/"))
		f = f.substr(0, f.size()-1);
	return f;
}
} // namespace

std::string canonicalize_path(const std::string& f, bool lexical) {
	return trim_end(canonicalize_path_impl(f, lexical));
}

std::string get_abspath(const std::string& f, bool lexical) {
	return trim_end(std::filesystem::absolute(canonicalize_path_impl(f,
		lexical)));
}

std::string get_dirpath(const std::string& f, bool lexical) {
	return trim_end(canonicalize_path_impl(f, lexical).parent_path());
}

std::string get_basename(const std::string& f, bool lexical) {
	return trim_end(canonicalize_path_impl(f, lexical).filename());
}

bool is_abspath(const std::string& f) {
	return canonicalize_path_impl(f, true).is_absolute();
}

bool is_windows(void) {
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

char get_path_separator(void) {
	return std::filesystem::path::preferred_separator;
}

namespace {
FileType get_mode_type(const std::filesystem::file_type& t) {
	switch (t) {
	case std::filesystem::file_type::directory:
		return FileType::Dir;
	case std::filesystem::file_type::regular:
		return FileType::Reg;
	case std::filesystem::file_type::block:
		[[fallthrough]];
	case std::filesystem::file_type::character:
		return FileType::Device;
	case std::filesystem::file_type::symlink:
		return FileType::Symlink;
	default:
		return FileType::Unsupported;
	}
}
} // namespace

FileType get_raw_file_type(const std::string& f) {
	try {
		return get_mode_type(std::filesystem::symlink_status(f).type());
	} catch (std::filesystem::filesystem_error& e) {
		return FileType::Invalid;
	}
}

FileType get_file_type(const std::string& f) {
	try {
		return get_mode_type(std::filesystem::status(f).type());
	} catch (std::filesystem::filesystem_error& e) {
		return FileType::Invalid;
	}
}

const std::string& get_file_type_string(const FileType& t) {
	static const std::array<std::string, 6> x{
		"directory",
		"regular file",
		"device",
		"symlink",
		"unsupported file",
		"invalid file",
	};
	switch (t) {
	case FileType::Dir:
		return x[0];
	case FileType::Reg:
		return x[1];
	case FileType::Device:
		return x[2];
	case FileType::Symlink:
		return x[3];
	case FileType::Unsupported:
		return x[4];
	case FileType::Invalid:
		return x[5];
	default:
		assert(false);
	}
}

// std::filesystem::exists can't be used as it resolves symlink
bool path_exists(const std::string& f) {
	try {
		switch (std::filesystem::symlink_status(f).type()) {
		case std::filesystem::file_type::none:
			[[fallthrough]];
		case std::filesystem::file_type::not_found:
			return false;
		default:
			return true;
		}
	} catch (std::filesystem::filesystem_error& e) {
		return false;
	}
}

std::tuple<std::string, bool> is_valid_hexsum(const std::string& input) {
	auto s = input;
	auto orig = s;
	if (s.starts_with("0x"))
		s.erase(0, 2);
	if (s.size() < 32)
		return {orig, false};
	for (const auto& r : s) {
		auto x = tolower(r);
		if (!isdigit(x) && !(x >= 'a' && x <= 'f'))
			return {orig, false};
	}
	return {s, true};
}

std::string get_xsum_format_string(const std::string& f, const std::string& h,
	bool swap) {
	std::ostringstream ss;
	if (!swap) {
		// compatible with shaXsum commands
		ss << h << "  " << f;
	} else {
		ss << f << "  " << h;
	}
	return ss.str();
}

std::string get_num_format_string(unsigned long n, const std::string& msg) {
	if (msg.empty())
		return "???";
	std::ostringstream ss;
	ss << n << " " << msg;
	auto s = ss.str();
	if (n > 1) {
		if (msg == get_file_type_string(FileType::Dir)) {
			std::ostringstream ss;
			ss << s.substr(0, s.size()-1) << "ies";
			s = ss.str();
			assert(s.ends_with("directories"));
		} else {
			s += "s";
		}
	}
	return s;
}

void print_num_format_string(unsigned long n, const std::string& msg) {
	std::cout << get_num_format_string(n, msg) << std::endl;
}

void panic_file_type(const std::string& f, const std::string& how,
	const FileType& t) {
	if (!f.empty())
		std::cout << f << " has " << how << " file type "
			<< get_file_type_string(t) << std::endl;
	else
		std::cout << how << " file type " << get_file_type_string(t)
			<< std::endl;
	assert(false);
}

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestAssert.h>

#include "./cppunit.h"

void UtilTest::test_canonicalize_path(void) {
	const std::vector<std::tuple<std::string, std::string>> path_list{
		{"/", "/"},
		{"/////", "/"},
		{"/..", "/"},
		{"/../", "/"},
		{"/root", "/root"},
		{"/root/", "/root"},
		{"/root/..", "/"},
		{"/root/../dev", "/dev"},
	};
	for (const auto& x : path_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(input,
			canonicalize_path(input, false), output);
	}
}

void UtilTest::test_get_abspath(void) {
	const std::vector<std::tuple<std::string, std::string>> path_list{
		{"/", "/"},
		{"/////", "/"},
		{"/..", "/"},
		{"/../", "/"},
		{"/root", "/root"},
		{"/root/", "/root"},
		{"/root/..", "/"},
		{"/root/../dev", "/dev"},
		{"/does/not/exist", "/does/not/exist"},
		{"/does/not/./exist", "/does/not/exist"},
		{"/does/not/../NOT/exist", "/does/NOT/exist"},
	};
	for (const auto& x : path_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(input, get_abspath(input, false),
			output);
	}
}

void UtilTest::test_get_dirpath(void) {
	const std::vector<std::tuple<std::string, std::string>> path_list{
		{"/root", "/"},
		{"/root/", "/"},
		{"/root/../dev", "/"},
		{"/does/not/exist", "/does/not"},
		{"/does/not/./exist", "/does/not"},
		{"/does/not/../NOT/exist", "/does/NOT"},
	};
	for (const auto& x : path_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(input, get_dirpath(input, false),
			output);
	}
}

void UtilTest::test_get_basename(void) {
	const std::vector<std::tuple<std::string, std::string>> path_list{
		{"/root", "root"},
		{"/root/", "root"},
		{"/root/../dev", "dev"},
		{"/does/not/exist", "exist"},
		{"/does/not/./exist", "exist"},
		{"/does/not/../NOT/exist", "exist"},
	};
	for (const auto& x : path_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(input, get_basename(input, false),
			output);
	}
}

void UtilTest::test_is_abspath(void) {
	const std::vector<std::tuple<std::string, bool>> path_list{
		{"/", true},
		{"/////", true},
		{"/..", true},
		{"/../", true},
		{"/root", true},
		{"/root/", true},
		{"/root/..", true},
		{"/root/../dev", true},
		{"/does/not/exist", true},
		{"/does/not/../NOT/exist", true},
		{"xxx", false},
		{"does/not/exist", false},
	};
	for (const auto& x : path_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(input, is_abspath(input), output);
	}
}

void UtilTest::test_is_windows(void) {
	CPPUNIT_ASSERT(!is_windows());
}

void UtilTest::test_get_path_separator(void) {
	CPPUNIT_ASSERT_EQUAL(get_path_separator(), '/');
}

void UtilTest::test_get_raw_file_type(void) {
	const std::vector<std::string> dir_list{
		".",
		"..",
		"/",
		"/dev",
	};
	for (const auto& f : dir_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, get_raw_file_type(f),
			FileType::Dir);

	const std::vector<std::string> invalid_list{
		"",
		"516e7cb4-6ecf-11d6-8ff8-00022d09712b",
	};
	for (const auto& f : invalid_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, get_raw_file_type(f),
			FileType::Unsupported);
}

void UtilTest::test_get_file_type(void) {
	const std::vector<std::string> dir_list{
		".",
		"..",
		"/",
		"/dev",
	};
	for (const auto& f : dir_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, get_file_type(f),
			FileType::Dir);

	const std::vector<std::string> invalid_list{
		"",
		"516e7cb4-6ecf-11d6-8ff8-00022d09712b",
	};
	for (const auto& f : invalid_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, get_file_type(f),
			FileType::Unsupported);
}

void UtilTest::test_get_file_type_string(void) {
	const std::vector<std::tuple<FileType, std::string>> file_type_list{
		{FileType::Dir, "directory"},
		{FileType::Reg, "regular file"},
		{FileType::Device, "device"},
		{FileType::Symlink, "symlink"},
		{FileType::Unsupported, "unsupported file"},
		{FileType::Invalid, "invalid file"},
	};
	for (const auto& x : file_type_list) {
		const auto [input, output] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(output,
			get_file_type_string(input), output);
	}
}

void UtilTest::test_path_exists(void) {
	const std::vector<std::string> dir_list{
		".",
		"..",
		"/",
		"/dev",
	};
	for (const auto& f : dir_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, path_exists(f), true);

	const std::vector<std::string> invalid_list{
		"",
		"516e7cb4-6ecf-11d6-8ff8-00022d09712b",
	};
	for (const auto& f : invalid_list)
		CPPUNIT_ASSERT_EQUAL_MESSAGE(f, path_exists(f), false);
}

void UtilTest::test_is_valid_hexsum(void) {
	const std::vector<std::string> valid_list{
		"00000000000000000000000000000000",
		"11111111111111111111111111111111",
		"22222222222222222222222222222222",
		"33333333333333333333333333333333",
		"44444444444444444444444444444444",
		"55555555555555555555555555555555",
		"66666666666666666666666666666666",
		"77777777777777777777777777777777",
		"88888888888888888888888888888888",
		"99999999999999999999999999999999",
		"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
		"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
		"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
		"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD",
		"EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
		"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
		"cccccccccccccccccccccccccccccccc",
		"dddddddddddddddddddddddddddddddd",
		"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
		"ffffffffffffffffffffffffffffffff",
		"0123456789ABCDEFabcdef0123456789ABCDEFabcdef",
		"0x00000000000000000000000000000000",
		"0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
		"0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		"0x0123456789ABCDEFabcdef0123456789ABCDEFabcdef",
	};
	for (const auto& s : valid_list)
		CPPUNIT_ASSERT_MESSAGE(s, std::get<1>(is_valid_hexsum(s)));

	const std::vector<std::string> invalid_list{
		"gggggggggggggggggggggggggggggggg",
		"GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
		"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
		"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",
		"                                ",
		"################################",
		"--------------------------------",
		"................................",
		"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
		"________________________________",
		"0000000000000000000000000000000",
		"0x0000000000000000000000000000000",
		"0x",
		"0",
		"",
	};
	for (const auto& s : invalid_list)
		CPPUNIT_ASSERT_MESSAGE(s, !std::get<1>(is_valid_hexsum(s)));
}

void UtilTest::test_get_num_format_string(void) {
	const std::vector<std::tuple<unsigned long, std::string, std::string>>
	num_format_list{
		{0, "", "???"},
		{1, "", "???"},
		{2, "", "???"},
		{0, "file", "0 file"},
		{1, "file", "1 file"},
		{2, "file", "2 files"},
	};
	for (const auto& x : num_format_list) {
		const auto [n, msg, result] = x;
		CPPUNIT_ASSERT_EQUAL_MESSAGE(std::to_string(n),
			get_num_format_string(n, msg), result);
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(UtilTest);
#endif
