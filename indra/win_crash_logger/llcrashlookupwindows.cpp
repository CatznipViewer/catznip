/** 
* @file llcrashlookupwindows.cpp
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
#include "linden_common.h"

#include "stdafx.h"
#include "resource.h"

#include <DbgEng.h>
#include "llcrashlookupwindows.h"

#define MAX_STACK_FRAMES	64

#if LL_SEND_CRASH_REPORTS

// ============================================================================
// LLCrashLookupWindows class
//

LLCrashLookupWindows::LLCrashLookupWindows()
	: LLCrashLookup()
{
	CoInitialize(NULL);
}

LLCrashLookupWindows::~LLCrashLookupWindows()
{
	CoUninitialize();
}

bool LLCrashLookupWindows::getDumpInfo(const std::string& strDumpPath, LLMinidumpInfo& dumpInfo)
{
	IDebugClient* pDbgClient = NULL;
	IDebugControl4*	pDbgControl = NULL;
	IDebugSymbols2*	pDbgSymbols = NULL;
	dumpInfo.clear();

	//
	// Create the base IDebugClient object and then query it for the class instances we're interested in
	//
	HRESULT hRes = DebugCreate(__uuidof(IDebugClient), (void**)&pDbgClient);
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Unable to instantiate IDebugClient";
		return false;
	}

	hRes = pDbgClient->QueryInterface(__uuidof(IDebugControl4), (void**)&pDbgControl);
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Unable to query for IDebugControl4";
		return false;
	}

	hRes = pDbgClient->QueryInterface(__uuidof(IDebugSymbols2), (void**)&pDbgSymbols);
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Unable to query for IDebugSymbols2";
		return false;
	}

	//
	// Open the minidump and wait to finish processing
	//
	hRes = pDbgClient->OpenDumpFile(strDumpPath.c_str());
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Unable to open dump " + strDumpPath;
		return false;
	}
	pDbgControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

	//
	// Try to find an event that describes an exception
	//
	ULONG nEventType = 0, nProcessId = 0, nThreadId = 0;
	BYTE bufContext[1024] = {0}; ULONG szContext = 0;
	hRes = pDbgControl->GetStoredEventInformation(
		&nEventType, &nProcessId, &nThreadId, bufContext, sizeof(bufContext), &szContext, NULL, 0, 0);
	if ( (FAILED(hRes)) || (DEBUG_EVENT_EXCEPTION != nEventType) )
	{
		m_strErrorMessage = "GetStoredEventInformation failed to find an exception event";
		return false;
	}

	//
	// Get the stack trace for the exception
	//
	DEBUG_STACK_FRAME dbgStackFrames[MAX_STACK_FRAMES]; ULONG cntStackFrames = 0;
	BYTE* pbufStackFrameContexts = new BYTE[MAX_STACK_FRAMES * szContext];
	hRes = pDbgControl->GetContextStackTrace(
		bufContext, szContext, dbgStackFrames, ARRAYSIZE(dbgStackFrames), 
		pbufStackFrameContexts, MAX_STACK_FRAMES * szContext, szContext, &cntStackFrames);
	if ( (FAILED(hRes)) || (cntStackFrames < 1) )
	{
		m_strErrorMessage = "Failed to get a stack strace";
		return false;
	}

	// Since the user won't have any debug symbols present we're really only interested in the top stack frame
	dumpInfo.m_nInstructionAddr = dbgStackFrames[0].InstructionOffset;
	ULONG idxModule = 0;
	hRes = pDbgSymbols->GetModuleByOffset(dumpInfo.m_nInstructionAddr, 0, &idxModule, &dumpInfo.m_nModuleBaseAddr);
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Failed to get crash module";
		return false;
	}

	//
	// Lookup the name of the module where the crash occurred
	//
	CHAR strModule[MAX_PATH] = {0}; 
	hRes = pDbgSymbols->GetModuleNameString(
		DEBUG_MODNAME_MODULE, DEBUG_ANY_ID, dumpInfo.m_nModuleBaseAddr, strModule, ARRAYSIZE(strModule) - 1, NULL);
	if (FAILED(hRes))
	{
		m_strErrorMessage = "Failed to get crash module name";
		return false;
	}
	dumpInfo.m_strModule = strModule;

	// Grab some basic properties we use for verification of the image
	DEBUG_MODULE_PARAMETERS dbgModuleParams;
	hRes = pDbgSymbols->GetModuleParameters(1, &dumpInfo.m_nModuleBaseAddr, 0, &dbgModuleParams);
	if ( (SUCCEEDED(hRes)) && (DEBUG_INVALID_OFFSET != dbgModuleParams.Base) )
	{
		dumpInfo.m_nModuleTimeStamp = dbgModuleParams.TimeDateStamp;
		dumpInfo.m_nModuleChecksum = dbgModuleParams.Checksum;
	}

	// Grab the version number as well
	BYTE bufVersionInfo[1024] = {0};
	hRes = pDbgSymbols->GetModuleVersionInformation(DEBUG_ANY_ID, dumpInfo.m_nModuleBaseAddr, "\\", bufVersionInfo, 1024, NULL);
	if (SUCCEEDED(hRes))
	{
		VS_FIXEDFILEINFO* pFileInfo = (VS_FIXEDFILEINFO*)bufVersionInfo;
		dumpInfo.m_nModuleVersion = ((U64)pFileInfo->dwFileVersionMS << 32) + pFileInfo->dwFileVersionLS;
	}
	else
	{
		m_strErrorMessage = "Failed to get crash module version information";
	}

	//
	// Clean up
	//
	pDbgClient->EndSession(DEBUG_END_PASSIVE);

	pDbgSymbols->Release();
	pDbgSymbols = NULL;

	pDbgControl->Release();
	pDbgControl = NULL;

	pDbgClient->Release();
	pDbgClient = NULL;

	return true;
}

#endif // LL_SEND_CRASH_REPORTS
