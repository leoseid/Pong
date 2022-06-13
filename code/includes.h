#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#include "utils.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wcomma"
#endif

#include "third_party/ryu_modified.c"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "strings.c"
#include "os.h"
#include "math.c"
#include "software_renderer.c"
#include "game.c"
