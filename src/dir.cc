#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

#include <cerrno>
#include <cassert>

#include "./dir.h"
#include "./global.h"
#include "./hash.h"
#include "./squash.h"
#include "./stat.h"
#include "./util.h"

namespace {
int walk_directory(const std::string&, const std::string&, Squash&, Stat&);
int walk_directory_impl(const std::string&, const std::string&, Squash&, Stat&);
bool test_ignore_entry(const std::string&, const FileType&);
void print_byte(const std::string&, const std::vector<char>&,
	const std::string&);
void handle_directory(const std::string&, const std::string&,
	const std::string&, Squash&, Stat&);
void print_file(const std::string&, const std::string&, const FileType&,
	const std::string&, Squash&, Stat&);
void print_symlink(const std::string&, const std::string&, Squash&, Stat&);
void print_unsupported(const std::string&, Stat&);
void print_invalid(const std::string&, Stat&);
void print_debug(const std::string&, const FileType&);
void print_verbose_stat(const std::string&, const Stat&);
void assert_file_path(const std::string&, const std::string&);
} // namespace

int print_input(const std::string& fx) {
	// keep symlink input as is
	// but unlike filepath.WalkDir,
	// std::filesystem::recursive_directory_iterator resolves symlink
	std::string f;
	if (get_raw_file_type(fx) == FileType::Symlink) {
		f = fx;
	} else {
		f = canonicalize_path(fx);
		if (f.empty())
			return 0;
		// assert exists
		if (!path_exists(f))
			throw std::runtime_error("No such path " + f);
	}

	// convert input to abs first
	f = get_abspath(f);
	assert_file_path(f, "");

	// keep input prefix based on raw type
	std::string inp;
	auto can_walk = true;
	switch (get_raw_file_type(f)) {
	case FileType::Dir:
		inp = f;
		break;
	case FileType::Reg:
		[[fallthrough]];
	case FileType::Device:
		[[fallthrough]];
	case FileType::Symlink:
		inp = get_dirpath(f);
		can_walk = false;
		break;
	default:
		return -EINVAL;
	}

	// prefix is a directory
	assert(get_file_type(inp) == FileType::Dir);

	// start directory walk
	// (unlike Rust or Go, std::filesystem::recursive_directory_iterator
	// can only handle directory)
	Squash squ;
	Stat sta;
	if (can_walk) {
		auto ret = walk_directory(f, inp, squ, sta);
		if (ret < 0)
			return ret;
	} else {
		auto ret = walk_directory_impl(f, inp, squ, sta);
		if (ret < 0)
			return ret;
	}

	// print various stats
	if (opt::verbose)
		print_verbose_stat(inp, sta);
	sta.print_stat_unsupported(inp);
	sta.print_stat_invalid(inp);

	// print squash hash if specified
	if (opt::squash) {
		auto b = squ.get_buffer();
		if (opt::verbose)
			print_num_format_string(b.size(), "squashed byte");
		print_byte(f, b, inp);
	}
	return 0;
}

namespace {
// std::filesystem::recursive_directory_iterator has different traversal order
// vs filepath.WalkDir, hence squash2 hash won't match the original golang
// implementation (but it does seem to match Rust's walkdir::WalkDir).
int walk_directory(const std::string& f, const std::string& inp, Squash& squ,
	Stat& sta) {
	std::vector<std::string> l;
	for (const auto& e : std::filesystem::recursive_directory_iterator(f)) {
		auto x = e.path();
		if (opt::sort) {
			l.push_back(x);
		} else {
			auto ret = walk_directory_impl(x, inp, squ, sta);
			if (ret < 0)
				return ret;
		}
	}
	if (opt::sort) {
		std::sort(l.begin(), l.end());
		for (const auto& f : l) {
			auto ret = walk_directory_impl(f, inp, squ, sta);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int walk_directory_impl(const std::string& f, const std::string& inp,
	Squash& squ, Stat& sta) {
	auto t = get_raw_file_type(f);
	if (test_ignore_entry(f, t)) {
		sta.append_stat_ignored(f);
		return 0;
	}

	// find target if symlink
	// l is symlink itself, not its target
	std::string x, l;
	if (t == FileType::Symlink) {
		if (opt::ignore_symlink) {
			sta.append_stat_ignored(f);
			return 0;
		}
		if (!opt::follow_symlink) {
			print_symlink(f, inp, squ, sta);
			return 0;
		}
		x = canonicalize_path(f);
		if (x.empty()) {
			print_invalid(f, sta);
			return 0;
		}
		assert(is_abspath(x));
		t = get_file_type(x); // update type
		assert(t != FileType::Symlink); // symlink chains resolved
		l = f;
	} else {
		x = f;
	}

	switch (t) {
	case FileType::Dir:
		handle_directory(x, l, inp, squ, sta);
		break;
	case FileType::Reg:
		[[fallthrough]];
	case FileType::Device:
		print_file(x, l, t, inp, squ, sta);
		break;
	case FileType::Unsupported:
		print_unsupported(x, sta);
		break;
	case FileType::Invalid:
		print_invalid(x, sta);
		break;
	case FileType::Symlink:
		panic_file_type(x, "symlink", t);
		break;
	}
	return 0;
}

bool test_ignore_entry(const std::string& f, const FileType& t) {
	assert(is_abspath(f));

	// only non directory types count
	if (t == FileType::Dir)
		return false;

	auto base_starts_with_dot = get_basename(f).starts_with(".");
	auto path_contains_slash_dot = f.find("/.") != std::string::npos;

	// ignore . directories if specified
	if (opt::ignore_dot_dir && !base_starts_with_dot &&
		path_contains_slash_dot)
		return true;

	// ignore . regular files if specified
	if (opt::ignore_dot_file) {
		// XXX limit to REG ?
		if (base_starts_with_dot)
			return true;
	}

	// ignore . entries if specified
	return opt::ignore_dot &&
		(base_starts_with_dot || path_contains_slash_dot);
}

std::string trim_input_prefix(const std::string& f, const std::string& inp) {
	if (f.starts_with(inp)) {
		auto x = f.substr(inp.size() + 1);
		assert(!x.starts_with("/"));
		return x;
	} else {
		return f;
	}
}
} // namespace

std::string get_real_path(const std::string& f, const std::string& inp) {
	if (opt::abs) {
		assert(is_abspath(f));
		return f;
	} else if (f == inp) {
		return ".";
	} else if (inp == "/") {
		return f.substr(1);
	} else {
		// f is probably symlink target if f unchanged
		return trim_input_prefix(f, inp);
	}
}

namespace {
void print_byte(const std::string& f, const std::vector<char>& inb,
	const std::string& inp) {
	assert_file_path(f, inp);

	// get hash value
	const auto [b, _ignore] = get_byte_hash(inb, opt::hash_algo);
	assert(!b.empty());
	auto hex_sum = get_hex_sum(b);

	// verify hash value if specified
	if (!opt::hash_verify.empty() && opt::hash_verify != hex_sum)
		return;

	if (opt::hash_only) {
		std::cout << hex_sum << std::endl;
	} else {
		// no space between two
		std::ostringstream ss;
		ss << "[" << SQUASH_LABEL << "][v" << SQUASH_VERSION << "]";
		auto s = ss.str();
		auto realf = get_real_path(f, inp);
		if (realf == ".")
			std::cout << hex_sum << s << std::endl;
		else
			std::cout << get_xsum_format_string(realf, hex_sum,
				opt::swap) << s << std::endl;
	}
}

// XXX Due to lexical=true by default, f isn't a symlink target when it's
// expected to be with non empty l.
#define f2t(f, l)	\
	((l).empty() ? (f) : std::string(std::filesystem::read_symlink(f)))

void handle_directory(const std::string& f, const std::string& l,
	const std::string& inp, Squash& squ, Stat& sta) {
	assert_file_path(f, inp);
	if (!l.empty())
		assert_file_path(l, inp);

	// nothing to do if input is input prefix
	if (f == inp)
		return;

	// nothing to do unless squash
	if (!opt::squash)
		return;

	// debug print first
	if (opt::debug)
		print_debug(f, FileType::Dir);

	// get hash value
	// path must be relative to input prefix
	auto s = trim_input_prefix(f2t(f, l), inp);
	const auto [b, written] = get_string_hash(s, opt::hash_algo);
	assert(!b.empty());

	// count this file
	sta.append_stat_total();
	sta.append_written_total(written);
	sta.append_stat_directory(f);
	sta.append_written_directory(written);

	// squash
	assert(opt::squash);
	if (opt::hash_only) {
		squ.update_buffer(b);
	} else {
		// make link -> target format if symlink
		auto realf = get_real_path(f2t(f, l), inp);
		if (!l.empty()) {
			assert_file_path(l, inp);
			auto ll = l;
			if (!opt::abs) {
				ll = trim_input_prefix(ll, inp);
				assert(!ll.starts_with("/"));
			}
			std::ostringstream ss;
			ss << ll << " -> " << realf;
			realf = ss.str();
		}
		std::vector<char> v(realf.begin(), realf.end());
		v.insert(v.end(), b.begin(), b.end());
		squ.update_buffer(v);
	}
}

void print_file(const std::string& f, const std::string& l, const FileType& t,
	const std::string& inp, Squash& squ, Stat& sta) {
	assert_file_path(f, inp);
	if (!l.empty())
		assert_file_path(l, inp);

	// debug print first
	if (opt::debug)
		print_debug(f, t);

	// get hash value
	const auto [b, written] = get_file_hash(f, opt::hash_algo);
	assert(!b.empty());
	auto hex_sum = get_hex_sum(b);

	// count this file
	sta.append_stat_total();
	sta.append_written_total(written);
	switch (t) {
	case FileType::Reg:
		sta.append_stat_regular(f);
		sta.append_written_regular(written);
		break;
	case FileType::Device:
		sta.append_stat_device(f);
		sta.append_written_device(written);
		break;
	default:
		panic_file_type(f, "invalid", t);
		break;
	}

	// verify hash value if specified
	if (!opt::hash_verify.empty() && opt::hash_verify != hex_sum)
		return;

	// squash or print this file
	if (opt::hash_only) {
		if (opt::squash)
			squ.update_buffer(b);
		else
			std::cout << hex_sum << std::endl;
	} else {
		// make link -> target format if symlink
		auto realf = get_real_path(f2t(f, l), inp);
		if (!l.empty()) {
			assert_file_path(l, inp);
			auto ll = l;
			if (!opt::abs) {
				ll = trim_input_prefix(ll, inp);
				assert(!ll.starts_with("/"));
			}
			std::ostringstream ss;
			ss << ll << " -> " << realf;
			realf = ss.str();
		}
		if (opt::squash) {
			std::vector<char> v(realf.begin(), realf.end());
			v.insert(v.end(), b.begin(), b.end());
			squ.update_buffer(v);
		} else {
			std::cout << get_xsum_format_string(realf, hex_sum,
				opt::swap) << std::endl;
		}
	}
}

void print_symlink(const std::string& f, const std::string& inp, Squash& squ,
	Stat& sta) {
	assert_file_path(f, inp);

	// debug print first
	if (opt::debug)
		print_debug(f, FileType::Symlink);

	// get hash value of symlink base name
	const auto [b, written] = get_string_hash(get_basename(f), opt::hash_algo);
	assert(!b.empty());
	auto hex_sum = get_hex_sum(b);

	// count this file
	sta.append_stat_total();
	sta.append_written_total(written);
	sta.append_stat_symlink(f);
	sta.append_written_symlink(written);

	// verify hash value if specified
	if (!opt::hash_verify.empty() && opt::hash_verify != hex_sum)
		return;

	// squash or print this file
	if (opt::hash_only) {
		if (opt::squash)
			squ.update_buffer(b);
		else
			std::cout << hex_sum << std::endl;
	} else {
		auto realf = get_real_path(f, inp);
		if (opt::squash) {
			std::vector<char> v(realf.begin(), realf.end());
			v.insert(v.end(), b.begin(), b.end());
			squ.update_buffer(v);
		} else {
			std::cout << get_xsum_format_string(realf, hex_sum,
				opt::swap) << std::endl;
		}
	}
}

void print_unsupported(const std::string& f, Stat& sta) {
	if (opt::debug)
		print_debug(f, FileType::Unsupported);
	sta.append_stat_unsupported(f);
}

void print_invalid(const std::string& f, Stat& sta) {
	if (opt::debug)
		print_debug(f, FileType::Invalid);
	sta.append_stat_invalid(f);
}

void print_debug(const std::string& f, const FileType& t) {
	assert(opt::debug);
	if (opt::abs)
		std::cout << "### " << get_abspath(f) << " "
			<< get_file_type_string(t) << std::endl;
	else
		std::cout << "### " << f << " " << get_file_type_string(t)
			<< std::endl;
}

void print_verbose_stat(const std::string& inp, const Stat& sta) {
	auto indent = " ";

	print_num_format_string(sta.num_stat_total(), "file");
	auto a0 = sta.num_stat_directory();
	auto a1 = sta.num_stat_regular();
	auto a2 = sta.num_stat_device();
	auto a3 = sta.num_stat_symlink();
	assert(a0 + a1 + a2 + a3 == sta.num_stat_total());
	if (a0 > 0) {
		std::cout << indent;
		print_num_format_string(a0,
			get_file_type_string(FileType::Dir));
	}
	if (a1 > 0) {
		std::cout << indent;
		print_num_format_string(a1,
			get_file_type_string(FileType::Reg));
	}
	if (a2 > 0) {
		std::cout << indent;
		print_num_format_string(a2,
			get_file_type_string(FileType::Device));
	}
	if (a3 > 0) {
		std::cout << indent;
		print_num_format_string(a3,
			get_file_type_string(FileType::Symlink));
	}

	print_num_format_string(sta.num_written_total(), "byte");
	auto b0 = sta.num_written_directory();
	auto b1 = sta.num_written_regular();
	auto b2 = sta.num_written_device();
	auto b3 = sta.num_written_symlink();
	assert(b0 + b1 + b2 + b3 == sta.num_written_total());
	if (b0 > 0) {
		std::cout << indent;
		std::ostringstream ss;
		ss << get_file_type_string(FileType::Dir) << " byte";
		print_num_format_string(b0, ss.str());
	}
	if (b1 > 0) {
		std::cout << indent;
		std::ostringstream ss;
		ss << get_file_type_string(FileType::Reg) << " byte";
		print_num_format_string(b1, ss.str());
	}
	if (b2 > 0) {
		std::cout << indent;
		std::ostringstream ss;
		ss << get_file_type_string(FileType::Device) << " byte";
		print_num_format_string(b2, ss.str());
	}
	if (b3 > 0) {
		std::cout << indent;
		std::ostringstream ss;
		ss << get_file_type_string(FileType::Symlink) << " byte";
		print_num_format_string(b3, ss.str());
	}

	sta.print_stat_ignored(inp);
}

void assert_file_path(const std::string& f, const std::string& inp) {
	// must always handle file as abs
	assert(is_abspath(f));

	// file must not end with "/"
	assert(!f.ends_with("/"));

	// inputPrefix must not end with "/"
	assert(!inp.ends_with("/"));
}
} // namespace
