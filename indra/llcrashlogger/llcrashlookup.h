/** 
* @file llcrashlookup.h
* @brief Base crash analysis class
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

#ifndef LLCRASHLOOKUP_H
#define LLCRASHLOOKUP_H

#include <string>
#include "llsd.h"

// ============================================================================
// LLMinidumpInfo structure
//

struct LLMinidumpInfo
{
	friend class LLCrashLookup;
	friend class LLCrashLookupWindows;
public:
	LLMinidumpInfo();

	LLSD               asLLSD() const;
	void               clear();
	U64                getInstructionAddress() const	{ return m_nInstructionAddr; }
	const std::string& getModuleName() const			{ return m_strModule; }
	U64                getModuleBaseAdress() const		{ return m_nModuleBaseAddr; }
	U64                getModuleDisplacement() const	{ return m_nInstructionAddr - m_nModuleBaseAddr; }
	U32                getModuleTimeStamp() const		{ return m_nModuleTimeStamp; }
	U32                getModuleChecksum() const		{ return m_nModuleChecksum; }
	U64                getModuleVersion() const			{ return m_nModuleVersion; }
	const std::string  getModuleVersionString() const;

protected:
	U64         m_nInstructionAddr;		// Memory pony where the crash occurred
	std::string m_strModule;			// Name of the module in which the crash occurred
	U64         m_nModuleBaseAddr;		// Base memory address of the module
	U32         m_nModuleTimeStamp;		// The date and time stamp of the module
	U32         m_nModuleChecksum;		// The 32-bit checksum of the module image
	U64         m_nModuleVersion;		// The version of the module, packed into 64-bits
};

// ============================================================================
// LLCrashLookup class
//

class LLCrashLookup
{
public:
	LLCrashLookup() {}
	virtual ~LLCrashLookup() {}

public:
	virtual bool       getDumpInfo(const std::string& strDumpPath, LLMinidumpInfo& dumpInfo) = 0;
	const std::string& getErrorMessage() const			{ return m_strErrorMessage; }
	bool               hasErorrMessage() const			{ return !m_strErrorMessage.empty(); }

protected:
	std::string m_strErrorMessage;	// Error message in case something went wrong during lookup
};

// ============================================================================

#endif // LLCRASHLOOKUP_H
