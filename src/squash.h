#ifndef SRC_SQUASH_H_
#define SRC_SQUASH_H_

#ifdef CONFIG_SQUASH1
#include "./squash1.h"
#else
#include "./squash2.h"
#endif
#endif // SRC_SQUASH_H_
