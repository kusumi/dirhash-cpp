#include <iostream>
#include <sstream>
#include <iterator>
#include <array>
#include <string>
#include <algorithm>
#include <exception>

#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include "./cppunit.h"
#include "./dir.h"
#include "./global.h"
#include "./hash.h"
#include "./util.h"

extern char* optarg;
extern int optind;

namespace opt {
	std::string hash_algo("sha256");
	std::string hash_verify;
	bool hash_only;
	bool ignore_dot;
	bool ignore_dot_dir;
	bool ignore_dot_file;
	bool ignore_symlink;
	bool follow_symlink;
	bool abs;
	bool swap;
	bool sort;
	bool squash;
	bool verbose;
	bool debug;
} // namespace opt

namespace {
const std::array<int, 3> _version{0, 4, 6};

std::string get_version_string() {
	std::ostringstream ss;
	ss << _version[0] << "." << _version[1] << "." << _version[2];
	return ss.str();
}

void print_version() {
	std::cout << get_version_string() << std::endl;
}

void print_build_options(void) {
	std::cout << "Build options:" << std::endl
#ifdef DEBUG
		<< "  debug" << std::endl
#endif
#ifdef CONFIG_SQUASH1
		<< "  squash1" << std::endl
#endif
#ifdef CONFIG_SQUASH2
		<< "  squash2" << std::endl
#endif
#ifdef CONFIG_CPPUNIT
		<< "  cppunit" << std::endl
#endif
		;
}

void usage(const std::string& arg) {
	std::cout << "Usage: " << arg << " [options] <paths>" << std::endl
		<< "Options:" << std::endl
		<< "  --hash_algo - Hash algorithm to use (default \"sha256\")"
		<< std::endl
		<< "  --hash_verify - Message digest to verify in hex string"
		<< std::endl
		<< "  --hash_only - Do not print file paths" << std::endl
		<< "  --ignore_dot - Ignore entries start with ." << std::endl
		<< "  --ignore_dot_dir - Ignore directories start with ."
		<< std::endl
		<< "  --ignore_dot_file - Ignore files start with ."
		<< std::endl
		<< "  --ignore_symlink - Ignore symbolic links" << std::endl
		<< "  --follow_symlink - Follow symbolic links unless directory"
		<< std::endl
		<< "  --abs - Print file paths in absolute path" << std::endl
		<< "  --swap - Print file path first in each line" << std::endl
		<< "  --sort - Print sorted file paths" << std::endl
		<< "  --squash - Print squashed message digest instead of per file"
		<< std::endl
		<< "  --verbose - Enable verbose print" << std::endl
		<< "  --debug - Enable debug mode" << std::endl
		<< "  -v, --version - Print version and exit" << std::endl
		<< "  -h, --help - Print usage and exit" << std::endl;
}

int handle_long_option(const std::string& name, const std::string& arg) {
	if (name == "hash_algo")
		opt::hash_algo = arg;
	else if (name == "hash_verify")
		opt::hash_verify = arg;
	else if (name == "hash_only")
		opt::hash_only = true;
	else if (name == "ignore_dot")
		opt::ignore_dot = true;
	else if (name == "ignore_dot_dir")
		opt::ignore_dot_dir = true;
	else if (name == "ignore_dot_file")
		opt::ignore_dot_file = true;
	else if (name == "ignore_symlink")
		opt::ignore_symlink = true;
	else if (name == "follow_symlink")
		opt::follow_symlink = true;
	else if (name == "abs")
		opt::abs = true;
	else if (name == "swap")
		opt::swap = true;
	else if (name == "sort")
		opt::sort = true;
	else if (name == "squash")
		opt::squash = true;
	else if (name == "verbose")
		opt::verbose = true;
	else if (name == "debug")
		opt::debug = true;
	else
		return -1;
	return 0;
}
} // namespace

int main(int argc, char** argv) {
	const auto* progname_ptr = argv[0];
	std::string progname(progname_ptr);

	int i, c;
	option lo[] = {
		{ "hash_algo", 1, nullptr, 0 },
		{ "hash_verify", 1, nullptr, 0 },
		{ "hash_only", 0, nullptr, 0 },
		{ "ignore_dot", 0, nullptr, 0 },
		{ "ignore_dot_dir", 0, nullptr, 0 },
		{ "ignore_dot_file", 0, nullptr, 0 },
		{ "ignore_symlink", 0, nullptr, 0 },
		{ "follow_symlink", 0, nullptr, 0 },
		{ "abs", 0, nullptr, 0 },
		{ "swap", 0, nullptr, 0 },
		{ "sort", 0, nullptr, 0 },
		{ "squash", 0, nullptr, 0 },
		{ "verbose", 0, nullptr, 0 },
		{ "debug", 0, nullptr, 0 },
		{ "version", 0, nullptr, 'v' },
		{ "help", 0, nullptr, 'h' },
		{ nullptr, 0, nullptr, 0 },
	};

	while ((c = getopt_long(argc, argv, "vhxX", lo, &i)) != -1) {
		switch (c) {
		case 0:
			try {
				if (handle_long_option(lo[i].name,
					std::string(optarg ? optarg : ""))
					== -1) {
					usage(progname);
					exit(1);
				}
			} catch (const std::exception& e) {
				std::cout << lo[i].name << ": " << e.what()
					<< std::endl;
				exit(1);
			}
			break;
		case 'v':
			print_version();
			exit(1);
		default:
		case 'h':
			usage(progname);
			exit(1);
		case 'x': // hidden
			print_build_options();
			exit(0);
		case 'X': // hidden
			exit(-run_unittest());
		}
	}
	argv += optind;
	argc -= optind;

	if (argc == 0) {
		usage(progname);
		exit(1);
	}

	if (opt::hash_algo.empty()) {
		std::cout << "No hash algorithm specified" << std::endl;
		exit(1);
	}

	if (opt::verbose)
		std::cout << opt::hash_algo << std::endl;

	hash_init();
	if (!new_hash(opt::hash_algo)) {
		auto a = get_available_hash_algo();
		std::ostringstream ss;
		std::copy(a.begin(), a.end()-1,
			std::ostream_iterator<std::string>(ss, " "));
		std::cout << "Unsupported hash algorithm " << opt::hash_algo
			<< std::endl << "Available hash algorithm [" << ss.str()
			<< a.back() << "]" << std::endl;
		exit(1);
	}

	if (!opt::hash_verify.empty()) {
		auto [s, valid] = is_valid_hexsum(opt::hash_verify);
		if (!valid) {
			std::cout << "Invalid verify string "
				<< opt::hash_verify << std::endl;
			exit(1);
		}
		opt::hash_verify = s;
	}

	if (is_windows()) {
		std::cout << "Windows unsupported" << std::endl;
		exit(1);
	}

	auto s = get_path_separator();
	if (s != '/') {
		std::cout << "Invalid path separator " << s << std::endl;
		exit(1);
	}

	for (auto i = 0; i < argc; i++) {
		auto ret = print_input(argv[i]);
		if (ret < 0) {
			std::cout << strerror(-ret) << std::endl;
			exit(1);
		}
		if (opt::verbose && i != argc - 1)
			std::cout << std::endl;
	}
	hash_cleanup();

	return 0;
}
