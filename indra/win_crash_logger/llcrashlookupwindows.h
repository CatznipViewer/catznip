/** 
* @file llcrashlookupwindows.h
* @brief Basic Windows crash analysis
* 
* Copyright (C) 2011-2014, Kitty Barnett
* 
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* 
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* 
*/

#ifndef LLCRASHLOOKUPWINDOWS_H
#define LLCRASHLOOKUPWINDOWS_H

#if LL_SEND_CRASH_REPORTS

#include "llcrashlookup.h"

// ============================================================================
// LLCrashLookupWindows class
//

class LLCrashLookupWindows : public LLCrashLookup
{
public:
	LLCrashLookupWindows();
	virtual ~LLCrashLookupWindows();

public:
	/*virtual*/ bool getDumpInfo(const std::string& strDumpPath, LLMinidumpInfo& dumpInfo);
};

// ============================================================================

#endif // LL_SEND_CRASH_REPORTS

#endif // LLCRASHLOOKUPWINDOWS_H
