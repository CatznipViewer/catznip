/**
 *
 * Copyright (c) 2011-2017, Kitty Barnett
 * Copyright (c) 2010, Linden Research, Inc.
 *
 * The source code in this file is provided to you under the terms of the
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 *
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to
 * abide by those obligations.
 *
 */

#include "linden_common.h"
#include "llcorehttputil.h"
#include "llsd.h"
#include "lluri.h"
#if LL_DARWIN
	#include <CoreServices/CoreServices.h>
#endif

#include "llupdatechecker.h"

// ====================================================================================
// LLUpdateChecker
//

LLUpdateChecker::LLUpdateChecker(LLUpdateChecker::Client& client)
	: mClient(client)
	, mInProgress(false)
{
}

LLUpdateChecker::~LLUpdateChecker()
{
}

void LLUpdateChecker::checkVersion(const std::string& urlBase, const std::string& channel, const std::string& version, const std::string& platform, const std::string& platform_version)
{
	if (!mInProgress)
	{
		mInProgress = true;
		const std::string strCheckUrl = buildUrl(urlBase, channel, version, platform, platform_version);
		LL_INFOS("UpdaterService") << "Checking for updates at " << strCheckUrl << LL_ENDL;
		LLCoros::instance().launch("LLUpdateChecker::checkVersionCoro", boost::bind(&LLUpdateChecker::checkVersionCoro, this, strCheckUrl));
	}
	else
	{
		LL_WARNS("UpdaterService") << "Attempting to restart an update check when one is in progress; ignored" << LL_ENDL;
	}
}

void LLUpdateChecker::checkVersionCoro(const std::string& strUrl)
{
	LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
	LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("checkVersionCoro", httpPolicy));
	LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

	LL_INFOS("checkVersionCoro") << "Getting update information from " << strUrl << LL_ENDL;

	LLSD sdResult = httpAdapter->getJsonAndSuspend(httpRequest, strUrl);
	mInProgress = false;

	LLSD httpResults = sdResult[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
	LLCore::HttpStatus httpStatus = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
	if ( (httpStatus != LLCore::HttpStatus(HTTP_OK)) && (httpStatus != LLCore::HttpStatus(HTTP_NO_CONTENT)) )
	{
		std::string server_error;
		if (sdResult.has("error_code"))
		{
			server_error += sdResult["error_code"].asString();
		}
		if (sdResult.has("error_text"))
		{
			server_error += server_error.empty() ? "" : ": ";
			server_error += sdResult["error_text"].asString();
		}

		LL_WARNS("UpdaterService") << "Response error " << httpStatus.getStatus()
			                       << " " << httpStatus.toString()
			                       << " (" << server_error << ")"
			                       << LL_ENDL;
		mClient.error(httpStatus.toString());
		return;
	}

	sdResult.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
	mClient.response(sdResult);
}

// static
std::string LLUpdateChecker::buildUrl(const std::string& urlBase, const std::string& channel, const std::string& version, const std::string& platform, const std::string& platform_version)
{
	LLSD sdPath;
	sdPath.append(channel);
	sdPath.append(version);
	sdPath.append(platform);

	LLSD sdQuery;
	sdQuery["osVersion"] = platform_version;

	return LLURI::buildHTTP(urlBase, sdPath, sdQuery).asString();
}

// ====================================================================================
