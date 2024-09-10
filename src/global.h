#ifndef SRC_GLOBAL_H_
#define SRC_GLOBAL_H_

#include <string>

// readonly after getopt
namespace opt {
	extern std::string hash_algo;
	extern std::string hash_verify;
	extern bool hash_only;
	extern bool ignore_dot;
	extern bool ignore_dot_dir;
	extern bool ignore_dot_file;
	extern bool ignore_symlink;
	extern bool follow_symlink;
	extern bool abs;
	extern bool swap;
	extern bool sort;
	extern bool squash;
	extern bool verbose;
	extern bool debug;
} // namespace opt
#endif // SRC_GLOBAL_H_
