#pragma once
#include <cassert>

// Simple assert wrapper.
// If you wish disable asserts, or do something else redefine this macro.
#ifdef DEBUG
#define SZV_ASSERT(x) \
	{ assert(x); }
#else
#define KK_ASSERT(x) {}
#endif

#define SZV_UNUSED(x) (void)x
