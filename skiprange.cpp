#include "skiprange.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static std::vector<SkipRange> ranges;

bool load_skip_ranges(const char *filename) {
    FILE *f = fopen(filename, "r");
    if(!f) return false;
    char s[128], e[128];
    while(fscanf(f, "%127s %127s", s, e) == 2) {
        SkipRange r;
        r.start.SetBase16(s);
        r.end.SetBase16(e);
        ranges.push_back(r);
    }
    fclose(f);
    return true;
}

const SkipRange* skip_current_range(const Int &key) {
    for(auto &r : ranges) {
        if(((Int*)&key)->IsGreaterOrEqual((Int*)&r.start) &&
           ((Int*)&key)->IsLowerOrEqual((Int*)&r.end))
            return &r;
    }
    return nullptr;
}
