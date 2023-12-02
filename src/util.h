#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <vector>

#ifdef __GNUC__
#include <fmt/core.h>
#define stdformat fmt::format
#else
#include <format>
#define stdformat std::format
#endif

namespace util
{
   typedef std::vector<std::string> strvec;
   std::string join( const util::strvec &l, const std::string &sep );
}

#endif
