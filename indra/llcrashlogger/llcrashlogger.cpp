 /** 
* @file llcrashlogger.cpp
* @brief Crash logger implementation
*
* $LicenseInfo:firstyear=2003&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
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
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <map>

#include "llcrashlogger.h"
#include "llcrashlock.h"
#include "linden_common.h"
#include "llstring.h"
#include "indra_constants.h"	// CRASH_BEHAVIOR_...
#include "llerror.h"
#include "llerrorcontrol.h"
#include "lltimer.h"
#include "lldir.h"
#include "llfile.h"
#include "llsdserialize.h"
#include "lliopipe.h"
#include "llpumpio.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llproxy.h"
 
// [SL:KB] - Patch: Viewer-CrashLookup | Checked: 2011-03-24 (Catznip-2.6)
#ifdef LL_WINDOWS
#include <shellapi.h>
#endif // LL_WINDOWS
#include <boost/lexical_cast.hpp>
// [/SL:KB]

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2013-06-27 (Catznip-3.4)
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>  

const std::ifstream::pos_type LOG_TRUNC_SIZE = 16384;
// [/SL:KB]

LLPumpIO* gServicePump = NULL;
BOOL gBreak = false;
BOOL gSent = false;

class LLCrashLoggerResponder : public LLHTTPClient::Responder
{
public:
	LLCrashLoggerResponder() 
	{
	}

	virtual void error(U32 status, const std::string& reason)
	{
		gBreak = true;
	}

	virtual void result(const LLSD& content)
	{
// [SL:KB] - Patch: Viewer-CrashLookup | Checked: 2012-05-26 (Catznip-3.3)
		LLSD sdCrashLog; std::string strCrashLog = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "crash.log");

		if (gDirUtilp->fileExists(strCrashLog))
		{
			std::ifstream fileCrashLogIn(strCrashLog.c_str());
			if (fileCrashLogIn.is_open())
			{
				LLSDSerialize::fromXML(sdCrashLog, fileCrashLogIn);
				fileCrashLogIn.close();
			}
		}

		while (sdCrashLog.size() > 15)
			sdCrashLog.erase(0);

		LLSD sdCrash;
		sdCrash["timestamp"] = LLDate::now();
		sdCrash["crash_freeze"] = (content.has("crash_freeze")) ? content["crash_freeze"].asBoolean() : false;
		sdCrash["crash_id"] =  (content.has("crash_id")) ? content["crash_id"].asUUID() : LLUUID::null;
		sdCrash["crash_link"] =  (content.has("crash_link")) ? content["crash_link"].asString() : "";
		sdCrash["crash_module"] = (content.has("crash_module_name")) ? content["crash_module_name"].asString() : "(Unknown)";
		sdCrash["crash_offset"] = (content.has("crash_module_offset")) ? content["crash_module_offset"].asString() : "";
		sdCrashLog.append(sdCrash);

		std::ofstream fileCrashLogOut(strCrashLog.c_str());
		LLSDSerialize::toPrettyXML(sdCrashLog, fileCrashLogOut);
		fileCrashLogOut.close();

		if ( (content.has("crash_link")) && (!content["crash_link"].asString().empty()) )
		{
			((LLCrashLogger*)LLCrashLogger::instance())->setCrashInformationLink(content["crash_link"].asString());
		}
// [/SL:KB]

		gBreak = true;
		gSent = true;
	}
};

LLCrashLogger::LLCrashLogger() :
// [SL:KB] - Patch: Viewer-CrashLookup | Checked: 2011-03-24 (Catznip-2.6)
	mCrashLookup(NULL),
// [/SL:KB]
	mCrashBehavior(CRASH_BEHAVIOR_ALWAYS_SEND),
	mCrashInPreviousExec(false),
	mCrashSettings("CrashSettings"),
	mSentCrashLogs(false),
	mCrashHost("")
{
}

LLCrashLogger::~LLCrashLogger()
{

}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2013-06-27 (Catznip-3.4)
bool getSLLog(const std::string& strLogPath, std::string& strLogFile)
{
	std::ifstream inLogFile(strLogPath.c_str(), std::ios::ate);
	if (inLogFile.is_open())
	{
		std::ifstream::pos_type szLogFile = inLogFile.tellg();
		std::ifstream::pos_type posTruncLog = 0;
		if (szLogFile > LOG_TRUNC_SIZE)
		{
			posTruncLog = szLogFile - LOG_TRUNC_SIZE;
			szLogFile = LOG_TRUNC_SIZE;
		}
		inLogFile.seekg(posTruncLog);

		std::stringstream s;
		s << inLogFile.rdbuf();

		strLogFile = s.str();
		if (0 != posTruncLog)
		{
			std::string::size_type posLR = strLogFile.find('\n');
			if (std::string::npos != posLR)
			{
				strLogFile.erase(0, posLR);
			}
		}

		inLogFile.close();
		return true;
	}
	return false;
}
// [/SL:KB]

//// TRIM_SIZE must remain larger than LINE_SEARCH_SIZE.
//const int TRIM_SIZE = 128000;
//const int LINE_SEARCH_DIST = 500;
//const std::string SKIP_TEXT = "\n ...Skipping... \n";
//void trimSLLog(std::string& sllog)
//{
//	if(sllog.length() > TRIM_SIZE * 2)
//	{
//		std::string::iterator head = sllog.begin() + TRIM_SIZE;
//		std::string::iterator tail = sllog.begin() + sllog.length() - TRIM_SIZE;
//		std::string::iterator new_head = std::find(head, head - LINE_SEARCH_DIST, '\n');
//		if(new_head != head - LINE_SEARCH_DIST)
//		{
//			head = new_head;
//		}
//
//		std::string::iterator new_tail = std::find(tail, tail + LINE_SEARCH_DIST, '\n');
//		if(new_tail != tail + LINE_SEARCH_DIST)
//		{
//			tail = new_tail;
//		}
//
//		sllog.erase(head, tail);
//		sllog.insert(head, SKIP_TEXT.begin(), SKIP_TEXT.end());
//	}
//}

//std::string getStartupStateFromLog(std::string& sllog)
//{
//	std::string startup_state = "STATE_FIRST";
//	std::string startup_token = "Startup state changing from ";
//
//	int index = sllog.rfind(startup_token);
//	if (index < 0 || index + startup_token.length() > sllog.length()) {
//		return startup_state;
//	}
//
//	// find new line
//	char cur_char = sllog[index + startup_token.length()];
//	std::string::size_type newline_loc = index + startup_token.length();
//	while(cur_char != '\n' && newline_loc < sllog.length())
//	{
//		newline_loc++;
//		cur_char = sllog[newline_loc];
//	}
//	
//	// get substring and find location of " to "
//	std::string state_line = sllog.substr(index, newline_loc - index);
//	std::string::size_type state_index = state_line.find(" to ");
//	startup_state = state_line.substr(state_index + 4, state_line.length() - state_index - 4);
//
//	return startup_state;
//}

bool LLCrashLogger::readDebugFromXML(LLSD& dest, const std::string& filename )
{
    std::string db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,filename);
    std::ifstream debug_log_file(db_file_name.c_str());
    
	// Look for it in the debug_info.log file
	if (debug_log_file.is_open())
	{
		LLSDSerialize::fromXML(dest, debug_log_file);
        debug_log_file.close();
        return true;
    }
    return false;
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
// static
void LLCrashLogger::mergeLogs(LLSD& dest_sd, const LLSD& src_sd)
{
	for (LLSD::map_const_iterator iter = src_sd.beginMap(), end = src_sd.endMap(); iter != end; ++iter)
	{
		if ( (dest_sd.has(iter->first)) && (dest_sd[iter->first].isMap()) && (iter->second.isMap()) )
			mergeLogs(dest_sd[iter->first], iter->second);
		else
			dest_sd[iter->first] = iter->second;
	}
}
// [/SL:KB]
//void LLCrashLogger::mergeLogs( LLSD src_sd )
//{
//    LLSD::map_iterator iter = src_sd.beginMap();
//	LLSD::map_iterator end = src_sd.endMap();
//	for( ; iter != end; ++iter)
//    {
//        mDebugLog[iter->first] = iter->second;
//    }
//}

//bool LLCrashLogger::readMinidump(std::string minidump_path)
//{
//	size_t length=0;
//
//	std::ifstream minidump_stream(minidump_path.c_str(), std::ios_base::in | std::ios_base::binary);
//	if(minidump_stream.is_open())
//	{
//		minidump_stream.seekg(0, std::ios::end);
//		length = (size_t)minidump_stream.tellg();
//		minidump_stream.seekg(0, std::ios::beg);
//		
//		LLSD::Binary data;
//		data.resize(length);
//		
//		minidump_stream.read(reinterpret_cast<char *>(&(data[0])),length);
//		minidump_stream.close();
//		
//		mCrashInfo["Minidump"] = data;
//	}
//	return (length>0?true:false);
//}

void LLCrashLogger::gatherFiles()
{
	updateApplication("Gathering logs...");

    LLSD static_sd;
    LLSD dynamic_sd;
    
    bool has_logs = readDebugFromXML( static_sd, "static_debug_info.log" );
    has_logs |= readDebugFromXML( dynamic_sd, "dynamic_debug_info.log" );
    
    if ( has_logs )
    {
        mDebugLog = static_sd;
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
        mergeLogs(mDebugLog, dynamic_sd);
// [/SL:KB]
//        mergeLogs(dynamic_sd);
		mCrashInPreviousExec = mDebugLog["CrashNotHandled"].asBoolean();

		mFileMap["SecondLifeLog"] = mDebugLog["SLLog"].asString();
		mFileMap["SettingsXml"] = mDebugLog["SettingsFilename"].asString();
		if(mDebugLog.has("CAFilename"))
		{
			LLCurl::setCAFile(mDebugLog["CAFilename"].asString());
		}
		else
		{
			LLCurl::setCAFile(gDirUtilp->getCAFile());
		}

		llinfos << "Using log file from debug log " << mFileMap["SecondLifeLog"] << llendl;
		llinfos << "Using settings file from debug log " << mFileMap["SettingsXml"] << llendl;
	}
	else
	{
		// Figure out the filename of the second life log
		LLCurl::setCAFile(gDirUtilp->getCAFile());
        
//		mFileMap["SecondLifeLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"SecondLife.log");
//        mFileMap["SettingsXml"] = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.xml");
	}

//    if (!gDirUtilp->fileExists(mFileMap["SecondLifeLog"]) ) //We would prefer to get this from the per-run but here's our fallback.
//    {
//        mFileMap["SecondLifeLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.old");
//    }

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2013-06-27 (Catznip-3.4)
	// Remove the log and settings path after we've retrieved it since it could contain the OS user name
	mDebugLog.erase("SLLog");
	mDebugLog.erase("SettingsFilename");
// [/SL:KB]

	gatherPlatformSpecificFiles();

	//Use the debug log to reconstruct the URL to send the crash report to
	if(mDebugLog.has("CrashHostUrl"))
	{
		// Crash log receiver has been manually configured.
		mCrashHost = mDebugLog["CrashHostUrl"].asString();
	}
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-06-18 (Catznip-2.6)
	else
	{
		mCrashHost = "http://viewer.catznip.com/crash/report/";
	}
// [/SL:KB]
//	else if(mDebugLog.has("CurrentSimHost"))
//	{
//		mCrashHost = "https://";
//		mCrashHost += mDebugLog["CurrentSimHost"].asString();
//		mCrashHost += ":12043/crash/report";
//	}
//	else if(mDebugLog.has("GridName"))
//	{
//		// This is a 'little' hacky, but its the best simple solution.
//		std::string grid_host = mDebugLog["GridName"].asString();
//		LLStringUtil::toLower(grid_host);
//
//		mCrashHost = "https://login.";
//		mCrashHost += grid_host;
//		mCrashHost += ".lindenlab.com:12043/crash/report";
//	}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	mAltCrashHost = "";
// [/SL:KB]
//	// Use login servers as the alternate, since they are already load balanced and have a known name
//	mAltCrashHost = "https://login.agni.lindenlab.com:12043/crash/report";

//	mCrashInfo["DebugLog"] = mDebugLog;
	mFileMap["StatsLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"stats.log");
	
	updateApplication("Encoding files...");

//	for(std::map<std::string, std::string>::iterator itr = mFileMap.begin(); itr != mFileMap.end(); ++itr)
//	{
//		std::ifstream f((*itr).second.c_str());
//		if(!f.is_open())
//		{
//			LL_INFOS("CRASHREPORT") << "Can't find file " << (*itr).second << LL_ENDL;
//			continue;
//		}
//		std::stringstream s;
//		s << f.rdbuf();
//
//		std::string crash_info = s.str();
//		if(itr->first == "SecondLifeLog")
//		{
//			if(!mCrashInfo["DebugLog"].has("StartupState"))
//			{
//				mCrashInfo["DebugLog"]["StartupState"] = getStartupStateFromLog(crash_info);
//			}
//			trimSLLog(crash_info);
//		}
//
//		mCrashInfo[(*itr).first] = LLStringFn::strip_invalid_xml(rawstr_to_utf8(crash_info));
//	}
	
//	std::string minidump_path;
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	std::string minidump_path = mDebugLog["MinidumpPath"].asString();
// [/SL:KB]

//	// Add minidump as binary.
//    bool has_minidump = mDebugLog.has("MinidumpPath");
//    
//	if (has_minidump)
//	{
//		minidump_path = mDebugLog["MinidumpPath"].asString();
//	}
//
//	if (has_minidump)
//	{
//		has_minidump = readMinidump(minidump_path);
//	}

//    if (!has_minidump)  //Viewer was probably so hosed it couldn't write remaining data.  Try brute force.
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	if (!gDirUtilp->fileExists(minidump_path))
// [/SL:KB]
    {
       //Look for a filename at least 30 characters long in the dump dir which contains the characters MDMP as the first 4 characters in the file.
        typedef std::vector<std::string> vec;
        std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"");
        vec file_vec = gDirUtilp->getFilesInDir(pathname);
        for(vec::const_iterator iter=file_vec.begin(); iter!=file_vec.end(); ++iter)
        {
            if ( ( iter->length() > 30 ) && (iter->rfind(".dmp") == (iter->length()-4) ) )
            {
                std::string fullname = pathname + *iter;
                std::ifstream fdat( fullname.c_str(), std::ifstream::binary);
                if (fdat)
                {
                    char buf[5];
                    fdat.read(buf,4);
                    fdat.close();  
                    if (!strncmp(buf,"MDMP",4))
                    {
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
                        minidump_path = fullname;
						break;
// [/SL:KB]
//                        minidump_path = *iter;
//                        has_minidump = readMinidump(fullname);
//						mDebugLog["MinidumpPath"] = fullname;
//						if (has_minidump) 
//						{
//							break;
//						}
                    }
                }
            }
        }
    }

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-09-23 (Catznip-2.8)
	if (!minidump_path.empty())
	{
		if (gDirUtilp->fileExists(minidump_path))
		{
			mFileMap["Minidump"] = minidump_path;

			LLMinidumpInfo dumpInfo;
			if (mCrashLookup)
			{
				mCrashLookup->getDumpInfo(minidump_path, dumpInfo);
				if (mCrashLookup->hasErorrMessage())
					mDebugLog["DumpInfoDbg"] = mCrashLookup->getErrorMessage();
			}
			mCrashInfo["DumpInfo"] = dumpInfo.asLLSD();
		}
		else
		{
			mDebugLog["DumpInfoDbg"] = "Minidump path does not exist";
		}

		// Remove the minidump path after we've retrieved it since it could contain the OS user name
		mDebugLog.erase("MinidumpPath");
	}
	else
	{
		mDebugLog["DumpInfoDbg"] = "Minidump path is empty";
	}

	// We're done with mDebugLog so move it into the crash report
	mCrashInfo["DebugLog"] = mDebugLog;
// [/SL:KB]
}

LLSD LLCrashLogger::constructPostData()
{
	return mCrashInfo;
}

const char* const CRASH_SETTINGS_FILE = "settings_crash_behavior.xml";

S32 LLCrashLogger::loadCrashBehaviorSetting()
{
	// First check user_settings (in the user's home dir)
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	if (! mCrashSettings.loadFromFile(filename))
	{
		// Next check app_settings (in the SL program dir)
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, CRASH_SETTINGS_FILE);
		mCrashSettings.loadFromFile(filename);
	}

	// If we didn't load any files above, this will return the default
	S32 value = mCrashSettings.getS32("CrashSubmitBehavior");

	// Whatever value we got, make sure it's valid
	switch (value)
	{
	case CRASH_BEHAVIOR_NEVER_SEND:
		return CRASH_BEHAVIOR_NEVER_SEND;
	case CRASH_BEHAVIOR_ALWAYS_SEND:
		return CRASH_BEHAVIOR_ALWAYS_SEND;
	}

	return CRASH_BEHAVIOR_ASK;
}

bool LLCrashLogger::saveCrashBehaviorSetting(S32 crash_behavior)
{
	switch (crash_behavior)
	{
	case CRASH_BEHAVIOR_ASK:
	case CRASH_BEHAVIOR_NEVER_SEND:
	case CRASH_BEHAVIOR_ALWAYS_SEND:
		break;
	default:
		return false;
	}

	mCrashSettings.setS32("CrashSubmitBehavior", crash_behavior);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	mCrashSettings.saveToFile(filename, FALSE);

	return true;
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-03-24 (Catznip-2.6)
static const std::string BOUNDARY("------------abcdef012345xyZ");

std::string getFormDataField(const std::string& strFieldName, const std::string& strFieldValue, const std::string& strBoundary)
{
	std::ostringstream streamFormPart;

	streamFormPart << "--" << strBoundary << "\r\n"
	               << "Content-Disposition: form-data; name=\"" << strFieldName << "\"\r\n\r\n"
	               << strFieldValue << "\r\n";

	return streamFormPart.str();
}

void addFormFile(std::ostringstream& body, const std::string strFileName, const char* pBuffer, unsigned int szBuffer)
{
	body << getFormDataField("filemap[]", llformat("%s;%d", strFileName.c_str(), szBuffer), BOUNDARY);
	body << "--" << BOUNDARY << "\r\n"
	     <<	"Content-Disposition: form-data; name=\"crash_report[]\"; "
	     << "filename=\"" << strFileName << "\"\r\n"
	     << "Content-Type: application/octet-stream"
	     << "\r\n\r\n";
	body.write(pBuffer, szBuffer);
	body <<	"\r\n";
}
// [/SL:KB]

//bool LLCrashLogger::runCrashLogPost(std::string host, LLSD data, std::string msg, int retries, int timeout)
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
bool LLCrashLogger::runCrashLogPost(const std::string& host, const std::string& msg, int retries, int timeout)
// [/SL:KB]
{
	gBreak = false;
	for(int i = 0; i < retries; ++i)
	{
		updateApplication(llformat("%s, try %d...", msg.c_str(), i+1));
//		LLHTTPClient::post(host, data, new LLCrashLoggerResponder(), timeout);
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-03-24 (Catznip-2.6)
		LLSD headers = LLSD::emptyMap();

		headers["Accept"] = "*/*";
		headers["Content-Type"] = "multipart/form-data; boundary=" + BOUNDARY;

		std::ostringstream body;

		/*
		 * Send viewer information for the upload handler's benefit
		 */
		if (mDebugLog.has("ClientInfo"))
		{
			body << getFormDataField("viewer_channel", mDebugLog["ClientInfo"]["Name"], BOUNDARY);
			body << getFormDataField("viewer_version", mDebugLog["ClientInfo"]["Version"], BOUNDARY);
			body << getFormDataField("viewer_platform", mDebugLog["ClientInfo"]["Platform"], BOUNDARY);
		}

		/*
		 * Include information about the last execution event
		 */
		S32 nLastExecEvent = mDebugLog["LastExecEvent"].asInteger(); std::string strLastExecEvent, strLastExecMsg;
		switch (nLastExecEvent)
		{
			case LAST_EXEC_NORMAL:
				strLastExecEvent = "normal";
				break;
			case LAST_EXEC_FROZE:
				strLastExecEvent = "froze";
				break;
			case LAST_EXEC_LLERROR_CRASH:
				strLastExecEvent = "llerror_crash";
				strLastExecMsg = mDebugLog["LastErrorMessage"].asString();
				break;
			case LAST_EXEC_OTHER_CRASH:
				strLastExecEvent = "other_crash";
				break;
			case LAST_EXEC_LOGOUT_FROZE:
				strLastExecEvent = "logout_froze";
				break;
			case LAST_EXEC_LOGOUT_CRASH:
				strLastExecEvent = "logout_crash";
				break;
		}
		body << getFormDataField("last_exec_freeze", boost::lexical_cast<std::string>(mCrashInPreviousExec), BOUNDARY);
		body << getFormDataField("last_exec_event", strLastExecEvent, BOUNDARY);
		if (!strLastExecMsg.empty())
			body << getFormDataField("last_exec_message", strLastExecMsg, BOUNDARY);

		/*
		 * Include crash analysis pony
		 */
		if ( (mCrashInfo.has("DumpInfo")) && (mCrashInfo["DumpInfo"].isMap()) )
		{
			for (LLSD::map_const_iterator itField = mCrashInfo["DumpInfo"].beginMap(), endField = mCrashInfo["DumpInfo"].endMap(); itField != endField; ++itField)
				body << getFormDataField(itField->first, itField->second.asString(), BOUNDARY);
		}

		/*
		 * Add the actual crash logs
		 */
		for (std::map<std::string, std::string>::const_iterator itFile = mFileMap.begin(), endFile = mFileMap.end(); itFile != endFile; ++itFile)
		{
			std::string strFileName = gDirUtilp->getBaseFileName(itFile->second);
			if (strFileName.empty())
				continue;

			// Treat the log file difference since we only want a portion of it
			if ("SecondLifeLog" != itFile->first)
			{
				llifstream fstream(itFile->second, std::iostream::binary | std::iostream::out);
				if (!fstream.is_open())
				{
					body << getFormDataField("filemap[]", llformat("%s (unable to open)", strFileName.c_str()), BOUNDARY);
					continue;
				}

				fstream.seekg(0, std::ios::end);
				U32 szFile = fstream.tellg();
				fstream.seekg(0, std::ios::beg);

				char* pBuffer = new char[szFile];
				fstream.read(pBuffer, szFile);

				addFormFile(body, strFileName, pBuffer, szFile);

				delete[] pBuffer;
				fstream.close();
			}
			else
			{
				std::string strLogFile;
				if (getSLLog(itFile->second, strLogFile))
				{
					addFormFile(body, "Catznip.log", strLogFile.c_str(), strLogFile.length());
				}
			}

			body <<	"\r\n";
		}

		/*
		 * Close the post
		 */
		body << "--" << BOUNDARY << "--\r\n";

		// postRaw() takes ownership of the buffer and releases it later.
		size_t size = body.str().size();
		U8 *data = new U8[size];
		memcpy(data, body.str().data(), size);

		// Send request
		LLHTTPClient::postRaw(host, data, size, new LLCrashLoggerResponder(), headers);
// [/SL:KB]

		while(!gBreak)
		{
			updateApplication(); // No new message, just pump the IO
		}
		if(gSent)
		{
			return gSent;
		}
	}
	return gSent;
}

bool LLCrashLogger::sendCrashLog(std::string dump_dir)
{
    gDirUtilp->setDumpDir( dump_dir );
    
//    std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
//                                                           "SecondLifeCrashReport");
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "CatznipCrashReport");
// [/SL:KB]
    std::string report_file = dump_path + ".log";
   
	gatherFiles();
    
	LLSD post_data;
	post_data = constructPostData();
    
	updateApplication("Sending reports...");

	std::ofstream out_file(report_file.c_str());
	LLSDSerialize::toPrettyXML(post_data, out_file);
	out_file.close();
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	mFileMap["CrashReportLog"] = report_file;
// [/SL:KB]
    
	bool sent = false;
    
	//*TODO: Translate
	if(mCrashHost != "")
	{
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
		sent = runCrashLogPost(mCrashHost, std::string("Sending to server"), 3, 5);
// [/SL:KB]
//		sent = runCrashLogPost(mCrashHost, post_data, std::string("Sending to server"), 3, 5);
	}
    
//	if(!sent)
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	if ( (!sent) && (!mAltCrashHost.empty()) )
// [/SL:KB]
	{
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
		sent = runCrashLogPost(mAltCrashHost, std::string("Sending to alternate server"), 3, 5);
// [/SL:KB]
//		sent = runCrashLogPost(mAltCrashHost, post_data, std::string("Sending to alternate server"), 3, 5);
	}
    
	mSentCrashLogs = sent;
    
	return sent;
}

bool LLCrashLogger::sendCrashLogs()
{
    
    //pertinent code from below moved into a subroutine.
    LLSD locks = mKeyMaster.getProcessList();
    LLSD newlocks = LLSD::emptyArray();

//	LLSD opts = getOptionData(PRIORITY_COMMAND_LINE);
//    LLSD rec;
//
//	if ( opts.has("pid") && opts.has("dumpdir") && opts.has("procname") )
//    {
//        rec["pid"]=opts["pid"];
//        rec["dumpdir"]=opts["dumpdir"];
//        rec["procname"]=opts["procname"];
//    }
	
    if (locks.isArray())
    {
        for (LLSD::array_iterator lock=locks.beginArray();
             lock !=locks.endArray();
             ++lock)
        {
            if ( (*lock).has("pid") && (*lock).has("dumpdir") && (*lock).has("procname") )
            {
                if ( mKeyMaster.isProcessAlive( (*lock)["pid"].asInteger(), (*lock)["procname"].asString() ) )
                {
                    newlocks.append(*lock);
                }
                else
                {
					//TODO:  This is a hack but I didn't want to include boost in another file or retest everything related to lldir 
                    if (LLCrashLock::fileExists((*lock)["dumpdir"].asString()))
                    {
                        //the viewer cleans up the log directory on clean shutdown
                        //but is ignorant of the locking table. 
                        if (!sendCrashLog((*lock)["dumpdir"].asString()))
                        {
                            newlocks.append(*lock);    //Failed to send log so don't delete it.
                        }
//                        else
//                        {
//                            //mCrashInfo["DebugLog"].erase("MinidumpPath");
//
//                            mKeyMaster.cleanupProcess((*lock)["dumpdir"].asString());
//                        }
                    }
				}
            }
            else
            {
                llwarns << "Discarding corrupted entry from lock table." << llendl;
            }
        }
    }

//    if (rec)
//    {
//        newlocks.append(rec);
//    }
    
    mKeyMaster.putProcessList(newlocks);
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	cleanupDumpDirs();
// [/SL:KB]
   return true;
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
void LLCrashLogger::cleanupDumpDirs()
{
	// Grab a list of directories that we shouldn't be deleting
	std::vector<std::string> activeFolders;
	{
		const LLSD sdProcesses = mKeyMaster.getProcessList();
		if (sdProcesses.isArray())
		{
			for (LLSD::array_const_iterator itProcess = sdProcesses.beginArray(); itProcess != sdProcesses.endArray(); ++itProcess)
				activeFolders.push_back((*itProcess)["dumpdir"].asString());
		}
	}

	// Remove any dump directory not referenced by 'activeFolders'
	std::vector<std::string> folders = gDirUtilp->getDirectoriesInDir(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, ""));
	for (std::vector<std::string>::const_iterator itFolder = folders.begin(); itFolder != folders.end(); ++itFolder)
	{
		const std::string& strFolder = *itFolder;
		if (boost::starts_with(strFolder, "dump-"))
		{
			const std::string& strPath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, strFolder);
			if (activeFolders.end() == std::find(activeFolders.begin(), activeFolders.end(), strPath))
			{
				try
				{
					boost::filesystem::remove_all(strPath);
					llinfos << "Removed all files from '" << strFolder << "'" << llendl;
				}
				catch (boost::filesystem::filesystem_error e)
				{
					llinfos << "Unable to remove all files from '" << strFolder << "'" << llendl;
					llinfos << e.what() << llendl;
				}
			}
		}
	}
}
// [/SL:KB]

void LLCrashLogger::updateApplication(const std::string& message)
{
	gServicePump->pump();
    gServicePump->callback();
	if (!message.empty()) llinfos << message << llendl;
}

bool LLCrashLogger::init()
{
	LLCurl::initClass(false);

	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	LLError::initForApplication(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));

	// Default to the product name "Second Life" (this is overridden by the -name argument)
	mProductName = "Second Life";

	// Rename current log file to ".old"
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "crashreport.log.old");
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "crashreport.log");

//#if LL_WINDOWS
//	LLAPRFile::remove(old_log_file);
//#endif 

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2012-07-09 (Catznip-3.3)
	if (LLFile::isfile(old_log_file))
		LLFile::remove(old_log_file);
// [/SL:KB]
	LLFile::rename(log_file.c_str(), old_log_file.c_str());
    
	// Set the log file to crashreport.log
	LLError::logToFile(log_file);  //NOTE:  Until this line, LL_INFOS LL_WARNS, etc are blown to the ether. 

    // Handle locking
    bool locked = mKeyMaster.requestMaster();  //Request master locking file.  wait time is defaulted to 300S
    
    while (!locked && mKeyMaster.isWaiting())
    {
		LL_INFOS("CRASHREPORT") << "Waiting for lock." << LL_ENDL;
#if LL_WINDOWS
		Sleep(1000);
#else
        sleep(1);
#endif 
        locked = mKeyMaster.checkMaster();
    }
    
    if (!locked)
    {
        LL_WARNS("CRASHREPORT") << "Unable to get master lock.  Another crash reporter may be hung." << LL_ENDL;
        return false;
    }

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	// Save the current process information now in case we crash later (also sends any pending crash reports in this session rather than the next)
	const LLSD sdOptionData = getOptionData(PRIORITY_COMMAND_LINE);
	if ( (sdOptionData.has("pid")) && (sdOptionData.has("dumpdir")) && (sdOptionData.has("procname")) )
	{
		LLSD sdProcesses = mKeyMaster.getProcessList();
		
		LLSD sdProcess;
		sdProcess["pid"] = sdOptionData["pid"];
		sdProcess["dumpdir"] = sdOptionData["dumpdir"];
		sdProcess["procname"] = sdOptionData["procname"];
		sdProcesses.append(sdProcess);
		
		mKeyMaster.putProcessList(sdProcesses);
	}
// [/SL:KB]

    mCrashSettings.declareS32("CrashSubmitBehavior", CRASH_BEHAVIOR_ALWAYS_SEND,
							  "Controls behavior when viewer crashes "
							  "(0 = ask before sending crash report, "
							  "1 = always send crash report, "
							  "2 = never send crash report)");
    
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2012-07-09 (Catznip-3.3)
	llinfos << "Loading crash behavior setting" << llendl;
	mCrashBehavior = loadCrashBehaviorSetting();
// [/SL:KB]
	// llinfos << "Loading crash behavior setting" << llendl;
	// mCrashBehavior = loadCrashBehaviorSetting();
    
	// If user doesn't want to send, bail out
	if (mCrashBehavior == CRASH_BEHAVIOR_NEVER_SEND)
	{
		llinfos << "Crash behavior is never_send, quitting" << llendl;
		return false;
	}
    
	gServicePump = new LLPumpIO(gAPRPoolp);
	gServicePump->prime(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);
 	
	return true;
}

// For cleanup code common to all platforms.
void LLCrashLogger::commonCleanup()
{
	LLError::logToFile("");   //close crashreport.log
	LLProxy::cleanupClass();
}
