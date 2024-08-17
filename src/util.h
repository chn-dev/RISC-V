/*******************************************************************************
 *  Copyright (c) 2024 Christian Nowak <chnowak@web.de>                        *
 *   This file is part of chn's RISC-V-Emulator.                               *
 *                                                                             *
 *  RISC-V-Emulator is free software: you can redistribute it and/or modify it *
 *  under the terms of the GNU General Public License as published by the Free *
 *  Software Foundation, either version 3 of the License, or (at your option)  *
 *  any later version.                                                         *
 *                                                                             *          
 *  RISC-V-Emulator is distributed in the hope that it will be useful, but     * 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License    *
 *  for more details.                                                          *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License along    *
 *  with RISC-V-Emulator. If not, see <https://www.gnu.org/licenses/>.         *
 *******************************************************************************/


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
