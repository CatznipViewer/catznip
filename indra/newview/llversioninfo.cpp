/** 
 * @file llversioninfo.cpp
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include <iostream>
#include <sstream>
#include "llversioninfo.h"

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-04-09 (Catznip-3.6)
#include <boost/regex.hpp>
// [/SL:KB]

#if ! defined(LL_VIEWER_CHANNEL)       \
 || ! defined(LL_VIEWER_VERSION_MAJOR) \
 || ! defined(LL_VIEWER_VERSION_MINOR) \
 || ! defined(LL_VIEWER_VERSION_PATCH) \
 || ! defined(LL_VIEWER_VERSION_BUILD)
 #error "Channel or Version information is undefined"
#endif

//
// Set the version numbers in indra/VIEWER_VERSION
//

//static
S32 LLVersionInfo::getMajor()
{
	return LL_VIEWER_VERSION_MAJOR;
}

//static
S32 LLVersionInfo::getMinor()
{
	return LL_VIEWER_VERSION_MINOR;
}

//static
S32 LLVersionInfo::getPatch()
{
	return LL_VIEWER_VERSION_PATCH;
}

//static
S32 LLVersionInfo::getBuild()
{
	return LL_VIEWER_VERSION_BUILD;
}

//static
const std::string &LLVersionInfo::getVersion()
{
	static std::string version("");
	if (version.empty())
	{
		std::ostringstream stream;
		stream << LLVersionInfo::getShortVersion() << "." << LLVersionInfo::getBuild();
		// cache the version string
		version = stream.str();
	}
	return version;
}

//static
const std::string &LLVersionInfo::getShortVersion()
{
	static std::string short_version("");
	if(short_version.empty())
	{
		// cache the version string
		std::ostringstream stream;
		stream << LL_VIEWER_VERSION_MAJOR << "."
		       << LL_VIEWER_VERSION_MINOR << "."
		       << LL_VIEWER_VERSION_PATCH;
		short_version = stream.str();
	}
	return short_version;
}

namespace
{
	/// Storage of the channel name the viewer is using.
	//  The channel name is set by hardcoded constant, 
	//  or by calling LLVersionInfo::resetChannel()
	std::string sWorkingChannelName(LL_VIEWER_CHANNEL);

	// Storage for the "version and channel" string.
	// This will get reset too.
	std::string sVersionChannel("");
}

//static
const std::string &LLVersionInfo::getChannelAndVersion()
{
	if (sVersionChannel.empty())
	{
		// cache the version string
		sVersionChannel = LLVersionInfo::getChannel() + " " + LLVersionInfo::getVersion();
	}

	return sVersionChannel;
}

//static
const std::string &LLVersionInfo::getChannel()
{
	return sWorkingChannelName;
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2014-04-09 (Catznip-3.6)
LLVersionInfo::EChannelType LLVersionInfo::getChannelType()
{
	static EChannelType sChannelType = CHANNEL_UNKNOWN;

	if (sChannelType == CHANNEL_UNKNOWN)
	{
		const boost::regex is_release_channel("\\bRelease\\b");
		const boost::regex is_beta_channel("\\bBeta\\b");
		const boost::regex is_project_channel("\\bProject\\b");
		const boost::regex is_test_channel("\\bTest$");

		const std::string& strChannel = getChannel();
		if (boost::regex_search(strChannel, is_release_channel))
			sChannelType = CHANNEL_RELEASE;
		else if (boost::regex_search(strChannel, is_beta_channel))
			sChannelType = CHANNEL_BETA;
		else if (boost::regex_search(strChannel, is_project_channel))
			sChannelType = CHANNEL_PROJECT;
		else if (boost::regex_search(strChannel, is_test_channel))
			sChannelType = CHANNEL_TEST;
		else
			sChannelType = CHANNEL_DEVELOP;
	}

	return sChannelType;
}
// [/SL:KB]

void LLVersionInfo::resetChannel(const std::string& channel)
{
	sWorkingChannelName = channel;
	sVersionChannel.clear(); // Reset version and channel string til next use.
}

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-05-08 (Catznip-2.6)
const char* getBuildPlatformString()
{
#if LL_WINDOWS
	#ifndef _WIN64
			return "Win32";
	#else
			return "Win64";
	#endif // _WIN64
#elif LL_SDL
	#if LL_GNUC
		#if ( defined(__amd64__) || defined(__x86_64__) )
			return "Linux64";
		#else
			return "Linux32";
		#endif
	#endif
#elif LL_DARWIN
			return "Darwin";
#else
			return "Unknown";
#endif
}

const std::string& LLVersionInfo::getBuildPlatform()
{
	static std::string strPlatform = getBuildPlatformString();
	return strPlatform;
}
// [/SL:KB]
