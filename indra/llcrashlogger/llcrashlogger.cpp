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
#include "llproxy.h"
#include "llcorehttputil.h"
#include "llhttpsdhandler.h"
#include "httpcommon.h"
#include "httpresponse.h"

#include <curl/curl.h>
#include <openssl/crypto.h>

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

BOOL gBreak = false;
BOOL gSent = false;

int LLCrashLogger::ssl_mutex_count = 0;
LLCoreInt::HttpMutex ** LLCrashLogger::ssl_mutex_list = NULL;

#define CRASH_UPLOAD_RETRIES 3 /* seconds */
#define CRASH_UPLOAD_TIMEOUT 180 /* seconds */

class LLCrashLoggerHandler : public LLHttpSDHandler
{
    LOG_CLASS(LLCrashLoggerHandler);
public:
    LLCrashLoggerHandler() {}

protected:
    virtual void onSuccess(LLCore::HttpResponse * response, const LLSD &content);
    virtual void onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status);

};

void LLCrashLoggerHandler::onSuccess(LLCore::HttpResponse * response, const LLSD &content)
{
    LL_DEBUGS("CRASHREPORT") << "Request to " << response->getRequestURL() << "succeeded" << LL_ENDL;

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
	sdCrash["crash_id"] = (content.has("crash_id")) ? content["crash_id"].asUUID() : LLUUID::null;
	sdCrash["crash_link"] = (content.has("crash_link")) ? content["crash_link"].asString() : "";
	sdCrash["crash_module"] = (content.has("crash_module_name")) ? content["crash_module_name"].asString() : "(Unknown)";
	sdCrash["crash_offset"] = (content.has("crash_module_offset")) ? content["crash_module_offset"].asString() : "";
	sdCrashLog.append(sdCrash);

	std::ofstream fileCrashLogOut(strCrashLog.c_str());
	LLSDSerialize::toPrettyXML(sdCrashLog, fileCrashLogOut);
	fileCrashLogOut.close();

	if ((content.has("crash_link")) && (!content["crash_link"].asString().empty()))
	{
		((LLCrashLogger*)LLCrashLogger::instance())->setCrashInformationLink(content["crash_link"].asString());
	}
// [/SL:KB]

	gBreak = true;
    gSent = true;
}

void LLCrashLoggerHandler::onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status)
{
    LL_WARNS("CRASHREPORT") << "Request to " << response->getRequestURL()
                            << " failed: " << status.toString() << LL_ENDL;
    gBreak = true;
}

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

bool LLCrashLogger::readFromXML(LLSD& dest, const std::string& filename )
{
    std::string db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,filename);
    std::ifstream log_file(db_file_name.c_str());
    
	// Look for it in the given file
	if (log_file.is_open())
	{
		LLSDSerialize::fromXML(dest, log_file);
        log_file.close();
        return true;
    }
    else
    {
        LL_WARNS("CRASHREPORT") << "Failed to open " << db_file_name << LL_ENDL;
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
//        LL_WARNS("CRASHREPORT") << "minidump length "<< length <<LL_ENDL;
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
//    else
//    {
//        LL_WARNS("CRASHREPORT") << "failed to open minidump "<<minidump_path<<LL_ENDL;
//    }
//    
//	return (length>0?true:false);
//}

void LLCrashLogger::gatherFiles()
{
	updateApplication("Gathering logs...");

    LLSD static_sd;
    LLSD dynamic_sd;
    //if we ever want to change the endpoint we send crashes to
    //we can construct a file download ( a la feature table filename for example)
    //containing the new endpoint
    LLSD endpoint;
    std::string grid;
    std::string fqdn;
    
    bool has_logs = readFromXML( static_sd, "static_debug_info.log" );
    has_logs |= readFromXML( dynamic_sd, "dynamic_debug_info.log" );

    
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
        mFileMap["CrashHostUrl"] = loadCrashURLSetting();
		if(mDebugLog.has("CAFilename"))
		{
            LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_CA_FILE,
                LLCore::HttpRequest::GLOBAL_POLICY_ID, mDebugLog["CAFilename"].asString(), NULL);
		}
		else
		{
            LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_CA_FILE,
                LLCore::HttpRequest::GLOBAL_POLICY_ID, gDirUtilp->getCAFile(), NULL);
		}

		LL_INFOS("CRASHREPORT") << "Using log file from debug log " << mFileMap["SecondLifeLog"] << LL_ENDL;
		LL_INFOS("CRASHREPORT") << "Using settings file from debug log " << mFileMap["SettingsXml"] << LL_ENDL;
	}
	else
	{
		// Figure out the filename of the second life log
        LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_CA_FILE,
            LLCore::HttpRequest::GLOBAL_POLICY_ID, gDirUtilp->getCAFile(), NULL);
        
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

    if ( has_logs && (mFileMap["CrashHostUrl"] != "") )
    {
        mCrashHost = mFileMap["CrashHostUrl"];
    }
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-06-18 (Catznip-2.6)
	else
	{
		mCrashHost = "http://viewer.catznip.com/crash/report/";
	}
// [/SL:KB]

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	mAltCrashHost = "";
// [/SL:KB]
//	//default to agni, per product
//	mAltCrashHost = "http://viewercrashreport.agni.lindenlab.com/cgi-bin/viewercrashreceiver.py";

//	mCrashInfo["DebugLog"] = mDebugLog;
	mFileMap["StatsLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"stats.log");
	
	updateApplication("Encoding files...");

//	for(std::map<std::string, std::string>::iterator itr = mFileMap.begin(); itr != mFileMap.end(); ++itr)
//	{
//        std::string file = (*itr).second;
//        if (!file.empty())
//        {
//            LL_DEBUGS("CRASHREPORT") << "trying to read " << itr->first << ": " << file << LL_ENDL;
//            std::ifstream f(file.c_str());
//            if(f.is_open())
//            {
//                std::stringstream s;
//                s << f.rdbuf();
//
//                std::string crash_info = s.str();
//                if(itr->first == "SecondLifeLog")
//                {
//                    if(!mCrashInfo["DebugLog"].has("StartupState"))
//                    {
//                        mCrashInfo["DebugLog"]["StartupState"] = getStartupStateFromLog(crash_info);
//                    }
//                    trimSLLog(crash_info);
//                }
//
//                mCrashInfo[(*itr).first] = LLStringFn::strip_invalid_xml(rawstr_to_utf8(crash_info));
//            }
//            else
//            {
//                LL_WARNS("CRASHREPORT") << "Failed to open file " << file << LL_ENDL;
//            }
//        }
//        else
//        {
//            LL_DEBUGS("CRASHREPORT") << "empty file in list for " << itr->first << LL_ENDL;
//        }
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
//		has_minidump = readMinidump(minidump_path);
//	}
//    else
//    {
//        LL_WARNS("CRASHREPORT") << "DebugLog does not have MinidumpPath" << LL_ENDL;
//    }

//    if (!has_minidump)  //Viewer was probably so hosed it couldn't write remaining data.  Try brute force.
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	if (!gDirUtilp->fileExists(minidump_path))
// [/SL:KB]
    {
        //Look for a filename at least 30 characters long in the dump dir which contains the characters MDMP as the first 4 characters in the file.
        typedef std::vector<std::string> vec;
        std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"");
        LL_WARNS("CRASHREPORT") << "Searching for minidump in " << pathname << LL_ENDL;
        vec file_vec = gDirUtilp->getFilesInDir(pathname);
//        for(vec::const_iterator iter=file_vec.begin(); !has_minidump && iter!=file_vec.end(); ++iter)
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
        for(vec::const_iterator iter=file_vec.begin(); iter!=file_vec.end(); ++iter)
// [/SL:KB]
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
                    else
                    {
                        LL_DEBUGS("CRASHREPORT") << "MDMP not found in " << fullname << LL_ENDL;
                    }
                }
                else
                {
                    LL_DEBUGS("CRASHREPORT") << "failed to open " << fullname << LL_ENDL;
                }
            }
            else
            {
                LL_DEBUGS("CRASHREPORT") << "Name does not match minidump name pattern " << *iter << LL_ENDL;
            }            
        }
    }
    else
    {
        LL_WARNS("CRASHREPORT") << "readMinidump returned no minidump" << LL_ENDL;
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

std::string LLCrashLogger::loadCrashURLSetting()
{

	// First check user_settings (in the user's home dir)
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	if (! mCrashSettings.loadFromFile(filename))
	{
		// Next check app_settings (in the SL program dir)
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, CRASH_SETTINGS_FILE);
		mCrashSettings.loadFromFile(filename);
	}

    if (! mCrashSettings.controlExists("CrashHostUrl"))
    {
        return "";
    }
    else
    {
        return mCrashSettings.getString("CrashHostUrl");
    }
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
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setTimeout(timeout);

	for(int i = 0; i < retries; ++i)
	{
		updateApplication(llformat("%s, try %d...", msg.c_str(), i+1));

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

		LLSD data;
		LLSDSerialize::toPrettyXML(data, body);
// [/SL:KB]

        LL_INFOS("CRASHREPORT") << "POST crash data to " << host << LL_ENDL;
        LLCore::HttpHandle handle = LLCoreHttpUtil::requestPostWithLLSD(httpRequest.get(), LLCore::HttpRequest::DEFAULT_POLICY_ID, 0,
            host, data, httpOpts, LLCore::HttpHeaders::ptr_t(), LLCore::HttpHandler::ptr_t(new LLCrashLoggerHandler));

        if (handle == LLCORE_HTTP_HANDLE_INVALID)
        {
            LLCore::HttpStatus status = httpRequest->getStatus();
            LL_WARNS("CRASHREPORT") << "Request POST failed to " << host << " with status of [" <<
                status.getType() << "]\"" << status.toString() << "\"" << LL_ENDL;
            return false;
        }

        while(!gBreak)
        {
            ms_sleep(250);
            updateApplication(); // No new message, just pump the IO
            httpRequest->update(0L);
        }
		if(gSent)
		{
			return gSent;
		}

        LL_WARNS("CRASHREPORT") << "Failed to send crash report to \"" << host << "\"" << LL_ENDL;
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

    LL_DEBUGS("CRASHREPORT") << "sending " << report_file << LL_ENDL;

	gatherFiles();
    
	LLSD post_data;
	post_data = constructPostData();
    
	updateApplication("Sending reports...");

	std::ofstream out_file(report_file.c_str());
	LLSDSerialize::toPrettyXML(post_data, out_file);
    out_file.flush();
	out_file.close();
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	mFileMap["CrashReportLog"] = report_file;
// [/SL:KB]
    
	bool sent = false;
    
    if(mCrashHost != "")
	{
        LL_WARNS("CRASHREPORT") << "Sending crash data to server from CrashHostUrl '" << mCrashHost << "'" << LL_ENDL;
        
        std::string msg = "Using override crash server... ";
        msg = msg+mCrashHost.c_str();
        updateApplication(msg.c_str());
        
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
		sent = runCrashLogPost(mCrashHost, std::string("Sending to server"), CRASH_UPLOAD_RETRIES, CRASH_UPLOAD_TIMEOUT);
// [/SL:KB]
//		sent = runCrashLogPost(mCrashHost, post_data, std::string("Sending to server"), CRASH_UPLOAD_RETRIES, CRASH_UPLOAD_TIMEOUT);
	}
    
//	if(!sent)
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
	if ( (!sent) && (!mAltCrashHost.empty()) )
// [/SL:KB]
	{
        updateApplication("Using default server...");
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2010-11-14 (Catznip-2.4)
		sent = runCrashLogPost(mAltCrashHost, std::string("Sending to alternate server"), CRASH_UPLOAD_RETRIES, CRASH_UPLOAD_TIMEOUT);
// [/SL:KB]
//		sent = runCrashLogPost(mAltCrashHost, post_data, std::string("Sending to default server"), CRASH_UPLOAD_RETRIES, CRASH_UPLOAD_TIMEOUT);
	}
    
	mSentCrashLogs = sent;
    
// [SL:KB] - Patch: Viewer-CrashLookup | Checked: 2011-03-24 (Catznip-2.6)
	if (!mCrashLink.empty())
	{
#if LL_WINDOWS && LL_SEND_CRASH_REPORTS
		if (IDYES == MessageBox(NULL, L"Additional information is available about this crash. Display?", L"Crash Information", MB_YESNO))
		{
			wchar_t wstrCrashLink[512];
			mbstowcs_s(NULL, wstrCrashLink, 512, mCrashLink.c_str(), _TRUNCATE);

			SHELLEXECUTEINFO sei = {0};
			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_NOASYNC;
			sei.lpVerb = L"open";
			sei.lpFile = wstrCrashLink;
			ShellExecuteEx(&sei);
		}
#endif // LL_WINDOWS && LL_SEND_CRASH_REPORTS
	}
// [/SL:KB]

	return sent;
}

bool LLCrashLogger::sendCrashLogs()
{
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
 //                       else
//                        {
//                            mKeyMaster.cleanupProcess((*lock)["dumpdir"].asString());
//                        }
                    }
				}
            }
            else
            {
                LL_INFOS() << "Discarding corrupted entry from lock table." << LL_ENDL;
            }
        }
    }

//    if (rec)
//    {
//        newlocks.append(rec);
//    }
    
    mKeyMaster.putProcessList(newlocks);
// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
	cleanupDumpDirs(false);
// [/SL:KB]
    return true;
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-05-18 (Catznip-3.7)
void LLCrashLogger::cleanCrashLogs()
{
	LLSD sdProcessesNew = LLSD::emptyArray();

	const LLSD sdProcesses = mKeyMaster.getProcessList();
	if (sdProcesses.isArray())
	{
		for (LLSD::array_const_iterator itProcess = sdProcesses.beginArray(); itProcess != sdProcesses.endArray(); ++itProcess)
		{
			if ( (*itProcess).has("pid") && (*itProcess).has("dumpdir") && (*itProcess).has("procname") )
			{
				if (mKeyMaster.isProcessAlive( (*itProcess)["pid"].asInteger(), (*itProcess)["procname"].asString()))
					sdProcessesNew.append(*itProcess);
			}
		}
	}

	mKeyMaster.putProcessList(sdProcessesNew);
	cleanupDumpDirs(false);
}

void LLCrashLogger::cleanupDumpDirs(bool fKeepCurrent)
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

		if (fKeepCurrent)
		{
			const LLSD sdOptions = getOptionData(PRIORITY_COMMAND_LINE);
			if (sdOptions.has("dumpdir"))
			{
				const std::string& strPath = sdOptions["dumpdir"].asString();
				if (activeFolders.end() == std::find(activeFolders.begin(), activeFolders.end(), strPath))
					activeFolders.push_back(strPath);
			}
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
					LL_INFOS() << "Removed all files from '" << strFolder << "'" << LL_ENDL;
				}
				catch (boost::filesystem::filesystem_error e)
				{
					LL_INFOS() << "Unable to remove all files from '" << strFolder << "'" << LL_ENDL;
					LL_INFOS() << e.what() << LL_ENDL;
				}
			}
		}
	}
}

bool LLCrashLogger::hasCrashLog()
{
	const LLSD sdProcesses = mKeyMaster.getProcessList();
	if (sdProcesses.isArray())
	{
		for (LLSD::array_const_iterator itProcess = sdProcesses.beginArray(); itProcess != sdProcesses.endArray(); ++itProcess)
		{
			if ( (*itProcess).has("pid") && (*itProcess).has("dumpdir") && (*itProcess).has("procname") &&
			     (LLCrashLock::fileExists((*itProcess)["dumpdir"].asString())) &&
			     (!mKeyMaster.isProcessAlive((*itProcess)["pid"].asInteger(), (*itProcess)["procname"].asString())) )
			{
				return true;
			}
		}
	}
	return false;
}
// [/SL:KB]

void LLCrashLogger::updateApplication(const std::string& message)
{
	if (!message.empty()) LL_INFOS("CRASHREPORT") << message << LL_ENDL;
}

bool LLCrashLogger::init()
{
    LL_DEBUGS("CRASHREPORT") << LL_ENDL;
    
    LLCore::LLHttp::initialize();

	// We assume that all the logs we're looking for reside on the current drive
// [SL:KB] - Patch: Viewer-Branding | Checked: 2010-11-12 (Catznip-2.4)
	gDirUtilp->initAppDirs("Catznip");
// [/SL:KB]
//	gDirUtilp->initAppDirs("SecondLife");

	LLError::initForApplication(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));

	// Default to the product name "Second Life" (this is overridden by the -name argument)
// [SL:KB] - Patch: Viewer-Branding | Checked: 2014-05-20 (Catznip-3.7)
	mProductName = "Catznip";
// [/SL:KB]
//	mProductName = "Second Life";

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

    LL_INFOS("CRASHREPORT") << "Crash reporter file rotation complete." << LL_ENDL;

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

    mCrashSettings.declareS32("CrashSubmitBehavior", CRASH_BEHAVIOR_ALWAYS_SEND,
							  "Controls behavior when viewer crashes "
							  "(0 = ask before sending crash report, "
							  "1 = always send crash report, "
							  "2 = never send crash report)");
    
    init_curl();
    LLCore::HttpRequest::createService();
    LLCore::HttpRequest::startThread();

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2012-07-09 (Catznip-3.3)
//	LL_INFOS() << "Loading crash behavior setting" << LL_ENDL;
//	mCrashBehavior = loadCrashBehaviorSetting();
// [/SL:KB]

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

	return true;
}

// For cleanup code common to all platforms.
void LLCrashLogger::commonCleanup()
{
    term_curl();
	LLError::logToFile("");   //close crashreport.log
	LLProxy::cleanupClass();
}

void LLCrashLogger::init_curl()
{
    curl_global_init(CURL_GLOBAL_ALL);

    ssl_mutex_count = CRYPTO_num_locks();
    if (ssl_mutex_count > 0)
    {
        ssl_mutex_list = new LLCoreInt::HttpMutex *[ssl_mutex_count];

        for (int i(0); i < ssl_mutex_count; ++i)
        {
            ssl_mutex_list[i] = new LLCoreInt::HttpMutex;
        }

        CRYPTO_set_locking_callback(ssl_locking_callback);
        CRYPTO_set_id_callback(ssl_thread_id_callback);
    }
}


void LLCrashLogger::term_curl()
{
    CRYPTO_set_locking_callback(NULL);
    for (int i(0); i < ssl_mutex_count; ++i)
    {
        delete ssl_mutex_list[i];
    }
    delete[] ssl_mutex_list;
}


unsigned long LLCrashLogger::ssl_thread_id_callback(void)
{
#if LL_WINDOWS
    return (unsigned long)GetCurrentThread();
#else
    return (unsigned long)pthread_self();
#endif
}


void LLCrashLogger::ssl_locking_callback(int mode, int type, const char * /* file */, int /* line */)
{
    if (type >= 0 && type < ssl_mutex_count)
    {
        if (mode & CRYPTO_LOCK)
        {
            ssl_mutex_list[type]->lock();
        }
        else
        {
            ssl_mutex_list[type]->unlock();
        }
    }
}

