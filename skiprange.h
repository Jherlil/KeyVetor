#ifndef SKIPRANGE_H
#define SKIPRANGE_H

#include <vector>
#include "secp256k1/Int.h"

struct SkipRange {
    Int start;
    Int end;
};

bool load_skip_ranges(const char *filename);
const SkipRange* skip_current_range(const Int &key);

#endif // SKIPRANGE_H
