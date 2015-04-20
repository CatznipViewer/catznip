/** 
* @file llcrashlookup.cpp
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

#include "linden_common.h"

#include "llcrashlookup.h"

// ============================================================================
// LLMinidumpInfo structure
//

LLMinidumpInfo::LLMinidumpInfo()
{
	clear();
}

void LLMinidumpInfo::clear()
{
	m_nInstructionAddr = m_nModuleBaseAddr = 0;
	m_strModule = "unknown";
	m_nModuleTimeStamp = m_nModuleChecksum = 0;
	m_nModuleVersion = 0;
}

LLSD LLMinidumpInfo::asLLSD() const
{
	LLSD sdInfo;
	sdInfo["crash_module_name"] = getModuleName();
	sdInfo["crash_module_version"] = llformat("%I64d", getModuleVersion());
	sdInfo["crash_module_versionstring"] = getModuleVersionString();
	sdInfo["crash_module_displacement"] = llformat("%I64d", getModuleDisplacement());
	return sdInfo;
}

const std::string LLMinidumpInfo::getModuleVersionString() const
{
	std::string strVersion = llformat("%d.%d.%d.%d", 
		(U32)(m_nModuleVersion >> 48), (U32)((m_nModuleVersion >> 32) & 0xFFFF), 
		(U32)((m_nModuleVersion >> 16) & 0xFFFF), (U32)(m_nModuleVersion & 0xFFFF));
	return strVersion;
}

// ============================================================================
