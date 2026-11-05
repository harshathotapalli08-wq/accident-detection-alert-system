#pragma once
#include <TinyGPSPlus.h>
#include "config.h"

namespace gps {

static TinyGPSPlus parser;

// Call often while the GPS port is the active software-serial listener.
inline void feed(Stream& s) { while (s.available()) parser.encode(s.read()); }

inline bool hasFix()  { return parser.location.isValid(); }
inline bool isLive()  { return parser.location.isValid() && parser.location.age() < GPS_LIVE_MAX_AGE_MS; }
inline double lat()   { return parser.location.lat(); }
inline double lng()   { return parser.location.lng(); }

}  // namespace gps
