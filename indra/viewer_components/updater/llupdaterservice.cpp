/** 
 * @file llupdaterservice.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "linden_common.h"

#include "llupdaterservice.h"

#include "llupdatedownloader.h"
#include "llevents.h"
#include "lltimer.h"
#include "llupdatechecker.h"
#include "llupdateinstaller.h"
#include "llexception.h"

#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "lldir.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "llviewernetwork.h"

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

#if ! defined(LL_VIEWER_VERSION_MAJOR)			\
 || ! defined(LL_VIEWER_VERSION_MINOR)			\
 || ! defined(LL_VIEWER_VERSION_PATCH)			\
 || ! defined(LL_VIEWER_VERSION_BUILD)
#error "Version information is undefined"
#endif

namespace 
{
	boost::weak_ptr<LLUpdaterServiceImpl> gUpdater;

//	const std::string UPDATE_MARKER_FILENAME("SecondLifeUpdateReady.xml");
// [SL:KB] - Patch: Viewer-Branding | Checked: 2014-05-20 (Catznip-3.7)
	const std::string UPDATE_MARKER_FILENAME("CatznipUpdateReady.xml");
// [/SL:KB]
	std::string update_marker_path()
	{
		return gDirUtilp->getExpandedFilename(LL_PATH_LOGS, 
											  UPDATE_MARKER_FILENAME);
	}
	
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.3
	bool get_update_marker_data(LLSD& update_info)
	{
		llifstream update_marker(update_marker_path(), std::ios::in | std::ios::binary);
		if (!update_marker.is_open())
			return false;

		// Found an update marker - now lets see if its valid.
		LLSDSerialize::fromXMLDocument(update_info, update_marker);
		update_marker.close();
		return true;
	}
// [/SL:KB]

	std::string install_script_path(void)
	{
#ifdef LL_WINDOWS
		std::string scriptFile = "update_install.bat";
#elif LL_DARWIN
		std::string scriptFile = "update_install.py";
#else
		std::string scriptFile = "update_install";
#endif
		return gDirUtilp->getExpandedFilename(LL_PATH_EXECUTABLE, scriptFile);
	}
	
	LLInstallScriptMode install_script_mode(void) 
	{
#ifdef LL_WINDOWS
		return LL_COPY_INSTALL_SCRIPT_TO_TEMP;
#else
		// This is important on Mac because update_install.py looks at its own
		// script pathname to discover the viewer app bundle to update.
		return LL_RUN_INSTALL_SCRIPT_IN_PLACE;
#endif
	};
	
}

class LLUpdaterServiceImpl : 
	public LLUpdateChecker::Client,
	public LLUpdateDownloader::Client
{
	static const std::string sListenerName;
	
	std::string   mProtocolVersion;
	std::string   mChannel;
	std::string   mVersion;
	std::string   mPlatform;
	std::string   mPlatformVersion;
	unsigned char mUniqueId[MD5HEX_STR_SIZE];
	bool          mWillingToTest;
	
	unsigned int mCheckPeriod;
	bool mIsChecking;
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
	bool mShowUserFeedback;
// [/SL:KB]
	bool mIsDownloading;
	
	LLUpdateChecker mUpdateChecker;
	LLUpdateDownloader mUpdateDownloader;
	LLTimer mTimer;

	LLUpdaterService::app_exit_callback_t mAppExitCallback;
	
	LLUpdaterService::eUpdaterState mState;
	
	LOG_CLASS(LLUpdaterServiceImpl);
	
public:
	LLUpdaterServiceImpl();
	virtual ~LLUpdaterServiceImpl();

	void initialize(const std::string& 	channel,
					const std::string& 	version,
					const std::string&  platform,
					const std::string&  platform_version,
					const unsigned char uniqueid[MD5HEX_STR_SIZE],
					const bool&         willing_to_test					
					);
	
	void setCheckPeriod(unsigned int seconds);
	void setBandwidthLimit(U64 bytesPerSecond);

	void startChecking(bool install_if_ready);
	void stopChecking();
//	bool forceCheck();
	bool isChecking();
	LLUpdaterService::eUpdaterState getState();
	
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.1
	bool isDownloading()	{ return mIsDownloading; }
	void startDownloading();
	const LLSD& getDownloadData() const;
// [/SL:KB]

	void setAppExitCallback(LLUpdaterService::app_exit_callback_t aecb) { mAppExitCallback = aecb;}
	std::string updatedVersion(void);

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
	void checkForUpdate(bool install_if_ready, bool user_feedback);
// [/SL:KB]
	bool checkForInstall(bool launchInstaller); // Test if a local install is ready.
	bool checkForResume(); // Test for resumeable d/l.

	// LLUpdateChecker::Client:
	virtual void error(std::string const & message);
	
	// A successful response was received from the viewer version manager
	virtual void response(LLSD const & content);

	// LLUpdateDownloader::Client
	void downloadComplete(LLSD const & data);
	void downloadError(std::string const & message);

	bool onMainLoop(LLSD const & event);

private:
	std::string mNewChannel;
	std::string mNewVersion;
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.1
	LLSD        mNewUpdateData;		// Used to store data for use by startDownloading()
// [/SL:KB]
	
	void restartTimer(unsigned int seconds);
	void setState(LLUpdaterService::eUpdaterState state);
	void stopTimer();
};

const std::string LLUpdaterServiceImpl::sListenerName = "LLUpdaterServiceImpl";

LLUpdaterServiceImpl::LLUpdaterServiceImpl() :
	mIsChecking(false),
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
	mShowUserFeedback(false),
// [/SL:KB]
	mIsDownloading(false),
	mCheckPeriod(0),
	mUpdateChecker(*this),
	mUpdateDownloader(*this),
	mState(LLUpdaterService::INITIAL)
{
}

LLUpdaterServiceImpl::~LLUpdaterServiceImpl()
{
	LL_INFOS("UpdaterService") << "shutting down updater service" << LL_ENDL;
	LLEventPumps::instance().obtain("mainloop").stopListening(sListenerName);
}

void LLUpdaterServiceImpl::initialize(const std::string&  channel,
									  const std::string&  version,
									  const std::string&  platform,
									  const std::string&  platform_version,
									  const unsigned char uniqueid[MD5HEX_STR_SIZE],
									  const bool&         willing_to_test)
{
	if(mIsChecking || mIsDownloading)
	{
		LLTHROW(LLUpdaterService::UsageError("LLUpdaterService::initialize call "
											  "while updater is running."));
	}
		
	mChannel = channel;
	mVersion = version;
	mPlatform = platform;
	mPlatformVersion = platform_version;
	memcpy(mUniqueId, uniqueid, MD5HEX_STR_SIZE);
	mWillingToTest = willing_to_test;
	LL_DEBUGS("UpdaterService")
		<< "\n  channel: " << mChannel
		<< "\n  version: " << mVersion
		<< "\n  uniqueid: " << mUniqueId
		<< "\n  willing: " << ( mWillingToTest ? "testok" : "testno" )
		<< LL_ENDL;
}

void LLUpdaterServiceImpl::setCheckPeriod(unsigned int seconds)
{
	mCheckPeriod = seconds;
}

void LLUpdaterServiceImpl::setBandwidthLimit(U64 bytesPerSecond)
{
	mUpdateDownloader.setBandwidthLimit(bytesPerSecond);
}

void LLUpdaterServiceImpl::startChecking(bool install_if_ready)
{
	if(mChannel.empty() || mVersion.empty())
	{
		LLTHROW(LLUpdaterService::UsageError("Set params before call to "
											 "LLUpdaterService::startCheck()."));
	}

	mIsChecking = true;
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
	checkForUpdate(install_if_ready, false);
// [/SL:KB]
//    // Check to see if an install is ready.
//	bool has_install = checkForInstall(install_if_ready);
//	if(!has_install)
//	{
//		checkForResume(); // will set mIsDownloading to true if resuming
//
//		if(!mIsDownloading)
//		{
//			setState(LLUpdaterService::CHECKING_FOR_UPDATE);
//			
//			// Checking can only occur during the mainloop.
//			// reset the timer to 0 so that the next mainloop event 
//			// triggers a check;
//			restartTimer(0); 
//		} 
//		else
//		{
//			setState(LLUpdaterService::DOWNLOADING);
//		}
//	}
}

void LLUpdaterServiceImpl::stopChecking()
{
	if(mIsChecking)
	{
		mIsChecking = false;
		stopTimer();
	}

    if(mIsDownloading)
    {
        mUpdateDownloader.cancel();
		mIsDownloading = false;
    }
	
	setState(LLUpdaterService::TERMINAL);
}

//bool LLUpdaterServiceImpl::forceCheck()
//{
//	if (!mIsDownloading && getState() != LLUpdaterService::CHECKING_FOR_UPDATE)
//	{
//		if (mIsChecking)
//		{
//			// Service is running, just reset the timer
//			if (mTimer.getStarted())
//			{
//				mTimer.setTimerExpirySec(0);
//				setState(LLUpdaterService::CHECKING_FOR_UPDATE);
//				return true;
//			}
//		}
//		else if (!mChannel.empty() && !mVersion.empty())
//		{
//			// one time check
//			bool has_install = checkForInstall(false);
//			if (!has_install)
//			{
//				std::string query_url = LLGridManager::getInstance()->getUpdateServiceURL();
//				if (!query_url.empty())
//				{
//					setState(LLUpdaterService::CHECKING_FOR_UPDATE);
//					mUpdateChecker.checkVersion(query_url, mChannel, mVersion,
//						mPlatform, mPlatformVersion, mUniqueId,
//						mWillingToTest);
//					return true;
//				}
//				else
//				{
//					LL_WARNS("UpdaterService")
//						<< "No updater service defined for grid '" << LLGridManager::getInstance()->getGrid() << LL_ENDL;
//				}
//			}
//		}
//	}
//	return false;
//}

bool LLUpdaterServiceImpl::isChecking()
{
	return mIsChecking;
}

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.1
void LLUpdaterServiceImpl::startDownloading()
{
	if (!mNewUpdateData.isDefined())
		return;

	stopTimer();
	mIsDownloading = true;
	setState(LLUpdaterService::DOWNLOADING);

	mUpdateDownloader.download(mNewUpdateData);

	mNewUpdateData.clear();
}

const LLSD& LLUpdaterServiceImpl::getDownloadData() const
{
	return mUpdateDownloader.getDownloadData();
}
// [/SL:KB]

LLUpdaterService::eUpdaterState LLUpdaterServiceImpl::getState()
{
	return mState;
}

std::string LLUpdaterServiceImpl::updatedVersion(void)
{
	return mNewVersion;
}

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
void LLUpdaterServiceImpl::checkForUpdate(bool install_if_ready, bool user_feedback)
{
	mShowUserFeedback = false;

	// Check to see if an install is ready.
	bool has_install = checkForInstall(install_if_ready);
	if (!has_install)
	{
		checkForResume(); // will set mIsDownloading to true if resuming

		if (!mIsDownloading)
		{
			mShowUserFeedback = user_feedback;
			setState(LLUpdaterService::CHECKING_FOR_UPDATE);

			// Checking can only occur during the mainloop.
			// reset the timer to 0 so that the next mainloop event
			// triggers a check;
			if (!LLEventPumps::instance().obtain("mainloop").getListener(sListenerName).connected())
				restartTimer(0);
			else
				mTimer.setTimerExpirySec(0.f);
		}
		else
		{
			setState(LLUpdaterService::DOWNLOADING);
		}
	}
	else if (!install_if_ready)
	{
		// Simulate a completed download so the user is informed about the update
		LLSD update_info;
		get_update_marker_data(update_info);

		mNewVersion = update_info["update_version"].asString();
		mNewChannel = update_info["update_channel"].asString();

		downloadComplete(update_info);
	}
}
// [/SL:KB]

bool LLUpdaterServiceImpl::checkForInstall(bool launchInstaller)
{
	bool foundInstall = false; // return true if install is found.

//	llifstream update_marker(update_marker_path().c_str(), 
//							 std::ios::in | std::ios::binary);
//
//	if(update_marker.is_open())
//	{
//		// Found an update info - now lets see if its valid.
//		LLSD update_info;
//		LLSDSerialize::fromXMLDocument(update_info, update_marker);
//		update_marker.close();
// [SL:KB] - Patch: Viewer-Updater | Catznip-3.3
	LLSD update_info;
	if (get_update_marker_data(update_info))
	{
// [/SL:KB]
		// Get the path to the installer file.
		std::string path(update_info.get("path"));
		std::string downloader_version(update_info["current_version"]);
		if (downloader_version != ll_get_version())
		{
			// This viewer is not the same version as the one that downloaded
			// the update. Do not install this update.
			LL_INFOS("UpdaterService") << "ignoring update downloaded by "
									   << "different viewer version "
									   << downloader_version << LL_ENDL;
			if (! path.empty())
			{
				LL_INFOS("UpdaterService") << "removing " << path << LL_ENDL;
				LLFile::remove(path);
				LLFile::remove(update_marker_path());
			}

			foundInstall = false;
		} 
		else if (path.empty())
		{
			LL_WARNS("UpdaterService") << "Marker file " << update_marker_path()
									   << " 'path' entry empty, ignoring" << LL_ENDL;
			foundInstall = false;
		}
		else if (! LLFile::isfile(path))
		{
			LL_WARNS("UpdaterService") << "Nonexistent installer " << path
									   << ", ignoring" << LL_ENDL;
			foundInstall = false;
		}
		else
		{
			if(launchInstaller)
			{
				setState(LLUpdaterService::INSTALLING);

				LLFile::remove(update_marker_path());

				int result = ll_install_update(install_script_path(),
											   path,
											   update_info["required"].asBoolean(),
											   install_script_mode());	

				if((result == 0) && mAppExitCallback)
				{
					mAppExitCallback();
				}
				else if(result != 0)
				{
					LL_WARNS("UpdaterService") << "failed to run update install script" << LL_ENDL;
				}
				else
				{
					; // No op.
				}
			}
			
			foundInstall = true;
		}
	}
	return foundInstall;
}

bool LLUpdaterServiceImpl::checkForResume()
{
	bool result = false;
	std::string download_marker_path = mUpdateDownloader.downloadMarkerPath();
	if(LLFile::isfile(download_marker_path))
	{
		llifstream download_marker_stream(download_marker_path.c_str(), 
								 std::ios::in | std::ios::binary);
		if(download_marker_stream.is_open())
		{
			LLSD download_info;
			LLSDSerialize::fromXMLDocument(download_info, download_marker_stream);
			download_marker_stream.close();
			std::string downloader_version(download_info["current_version"]);
			if (downloader_version == ll_get_version())
			{
				mIsDownloading = true;
				mNewVersion = download_info["update_version"].asString();
				mNewChannel = download_info["update_channel"].asString();
				mUpdateDownloader.resume();
				result = true;

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
				LLSD sdEventData;
				sdEventData["pump"] = LLUpdaterService::pumpName();
				sdEventData["payload"] = download_info;

				LLSD& sdPayload = sdEventData["payload"];
				sdPayload["type"] = LLSD(LLUpdaterService::DOWNLOAD_RESUME);
				sdPayload["show_ui"] = true;
				sdPayload["required"] = download_info["required"].asBoolean();
				sdPayload["channel"] = mNewChannel;
				sdPayload["version"] = mNewVersion;

				LLEventPumps::instance().obtain("mainlooprepeater").post(sdEventData);
// [/SL:KB]
			}
			else 
			{
				// The viewer that started this download is not the same as this viewer; ignore.
				LL_INFOS("UpdaterService") << "ignoring partial download "
										   << "from different viewer version "
										   << downloader_version << LL_ENDL;
				std::string path = download_info["path"].asString();
				if(!path.empty())
				{
					LL_INFOS("UpdaterService") << "removing " << path << LL_ENDL;
					LLFile::remove(path);
				}
				LLFile::remove(download_marker_path);
			}
		} 
	}
	return result;
}

void LLUpdaterServiceImpl::error(std::string const & message)
{
	setState(LLUpdaterService::TEMPORARY_ERROR);
	if(mIsChecking)
	{
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
		LLSD sdEventData;
		sdEventData["pump"] = LLUpdaterService::pumpName();

		LLSD& sdPayload = sdEventData["payload"];
		sdPayload["type"] = LLSD(LLUpdaterService::CHECK_ERROR);
		sdPayload["show_ui"] = mShowUserFeedback;

		LLEventPumps::instance().obtain("mainlooprepeater").post(sdEventData);

		mShowUserFeedback = false;
// [/SL:KB]

		restartTimer(mCheckPeriod);
	}
}

// A successful response was received from the viewer version manager
void LLUpdaterServiceImpl::response(LLSD const & content)
{
	if(!content.asBoolean()) // an empty response means "no update"
	{
		LL_INFOS("UpdaterService") << "up to date" << LL_ENDL;
		if(mIsChecking)
		{
			restartTimer(mCheckPeriod);
		}
	
		setState(LLUpdaterService::UP_TO_DATE);

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.5
		mNewChannel.clear();
		mNewVersion.clear();
		mNewUpdateData.clear();

		LLSD sdEventData;
		sdEventData["pump"] = LLUpdaterService::pumpName();
		sdEventData["payload"] = content;

		LLSD& sdPayload = sdEventData["payload"];
		sdPayload["type"] = LLSD(LLUpdaterService::CHECK_COMPLETE);
		sdPayload["up_to_date"] = true;
		sdPayload["show_ui"] = mShowUserFeedback;

		LLEventPumps::instance().obtain("mainlooprepeater").post(sdEventData);

		mShowUserFeedback = false;
// [/SL:KB]
	}
	else if ( content.isMap() && content.has("url") )
	{
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.5
		// There is an update available...
		stopTimer();
		setState(LLUpdaterService::UPDATE_AVAILABLE);

		mNewChannel = content["channel"].asString();
		if (mNewChannel.empty())
		{
			LL_INFOS("UpdaterService") << "no channel supplied, assuming current channel" << LL_ENDL;
			mNewChannel = mChannel;
		}
		mNewVersion = content["version"].asString();
		mNewUpdateData = content;

		LLSD sdEventData;
		sdEventData["pump"] = LLUpdaterService::pumpName();
		sdEventData["payload"] = content;

		LLSD& sdPayload = sdEventData["payload"];
		sdPayload["type"] = LLSD(LLUpdaterService::CHECK_COMPLETE);
		sdPayload["up_to_date"] = false;
		sdPayload["show_ui"] = mShowUserFeedback;
		sdPayload["required"] = content["required"].asBoolean();
		sdPayload["channel"] = mNewChannel;
		sdPayload["version"] = mNewVersion;

		LLEventPumps::instance().obtain("mainlooprepeater").post(sdEventData);

		mShowUserFeedback = false;
// [/SL:KB]
//		// there is an update available...
//		stopTimer();
//		mNewChannel = content["channel"].asString();
//		if (mNewChannel.empty())
//		{
//			LL_INFOS("UpdaterService") << "no channel supplied, assuming current channel" << LL_ENDL;
//			mNewChannel = mChannel;
//		}
//		mNewVersion = content["version"].asString();
//		mIsDownloading = true;
//		setState(LLUpdaterService::DOWNLOADING);
//		BOOL required = content["required"].asBoolean();
//		LLURI url(content["url"].asString());
//		std::string more_info = content["more_info"].asString();
//		LL_DEBUGS("UpdaterService")
//			<< "Starting download of "
//			<< ( required ? "required" : "optional" ) << " update"
//			<< " to channel '" << mNewChannel << "' version " << mNewVersion
//			<< " more info '" << more_info << "'"
//			<< LL_ENDL;
//		mUpdateDownloader.download(url, content["hash"].asString(), mNewChannel, mNewVersion, more_info, required);
	}
	else
	{
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
		mNewChannel.clear();
		mNewVersion.clear();
		mNewUpdateData.clear();

		LLSD sdEventData;
		sdEventData["pump"] = LLUpdaterService::pumpName();

		LLSD& sdPayload = sdEventData["payload"];
		sdPayload["type"] = LLSD(LLUpdaterService::CHECK_ERROR);
		sdPayload["show_ui"] = mShowUserFeedback;

		LLEventPumps::instance().obtain("mainlooprepeater").post(sdEventData);

		mShowUserFeedback = false;
// [/SL:KB]

		LL_WARNS("UpdaterService") << "Invalid update query response ignored; retry in "
			<< mCheckPeriod << " seconds" << LL_ENDL;
		setState(LLUpdaterService::TEMPORARY_ERROR);
		if (mIsChecking)
		{
			restartTimer(mCheckPeriod);
		}
	}
}

void LLUpdaterServiceImpl::downloadComplete(LLSD const & data) 
{ 
	mIsDownloading = false;

	// Save out the download data to the SecondLifeUpdateReady
	// marker file. 
	llofstream update_marker(update_marker_path().c_str());
	LLSDSerialize::toPrettyXML(data, update_marker);
	
	LLSD event;
	event["pump"] = LLUpdaterService::pumpName();
	LLSD payload;
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-2.6
	payload = data["update_data"];
// [/SL:KB]
	payload["type"] = LLSD(LLUpdaterService::DOWNLOAD_COMPLETE);
	payload["required"] = data["required"];
	payload["version"] = mNewVersion;
	payload["channel"] = mNewChannel;
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
	payload["more_info"] = data["more_info"];
// [/SL:KB]
	payload["info_url"] = data["info_url"];
	event["payload"] = payload;
	LL_DEBUGS("UpdaterService")
		<< "Download complete "
		<< ( data["required"].asBoolean() ? "required" : "optional" )
		<< " channel " << mNewChannel
		<< " version " << mNewVersion
// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.6
		<< " more_info " << data["more_info"].asString()
// [/SL:KB]
		<< " info " << data["info_url"].asString()
		<< LL_ENDL;

	LLEventPumps::instance().obtain("mainlooprepeater").post(event);

	setState(LLUpdaterService::TERMINAL);
}

void LLUpdaterServiceImpl::downloadError(std::string const & message) 
{ 
	LL_INFOS("UpdaterService") << "Error downloading: " << message << LL_ENDL;

	mIsDownloading = false;

	// Restart the timer on error
	if(mIsChecking)
	{
		restartTimer(mCheckPeriod); 
	}

	LLSD event;
	event["pump"] = LLUpdaterService::pumpName();
	LLSD payload;
	payload["type"] = LLSD(LLUpdaterService::DOWNLOAD_ERROR);
	payload["message"] = message;
	event["payload"] = payload;
	LLEventPumps::instance().obtain("mainlooprepeater").post(event);

	setState(LLUpdaterService::FAILURE);
}

void LLUpdaterServiceImpl::restartTimer(unsigned int seconds)
{
	LL_INFOS("UpdaterService") << "will check for update again in " << 
	seconds << " seconds" << LL_ENDL; 
	mTimer.start();
	mTimer.setTimerExpirySec((F32)seconds);
	LLEventPumps::instance().obtain("mainloop").listen(
		sListenerName, boost::bind(&LLUpdaterServiceImpl::onMainLoop, this, _1));
}

void LLUpdaterServiceImpl::setState(LLUpdaterService::eUpdaterState state)
{
	if(state != mState)
	{
		mState = state;
		
		LLSD event;
		event["pump"] = LLUpdaterService::pumpName();
		LLSD payload;
		payload["type"] = LLSD(LLUpdaterService::STATE_CHANGE);
		payload["state"] = state;
		event["payload"] = payload;
		LLEventPumps::instance().obtain("mainlooprepeater").post(event);
		
		LL_INFOS("UpdaterService") << "setting state to " << state << LL_ENDL;
	}
	else 
	{
		; // State unchanged; noop.
	}
}

void LLUpdaterServiceImpl::stopTimer()
{
	mTimer.stop();
	LLEventPumps::instance().obtain("mainloop").stopListening(sListenerName);
}

bool LLUpdaterServiceImpl::onMainLoop(LLSD const & event)
{
	if(mTimer.getStarted() && mTimer.hasExpired())
	{
		stopTimer();

		// Check for failed install.
		if(LLFile::isfile(ll_install_failed_marker_path()))
		{
			LL_DEBUGS("UpdaterService") << "found marker " << ll_install_failed_marker_path() << LL_ENDL;
			int requiredValue = 0; 
			{
				llifstream stream(ll_install_failed_marker_path().c_str());
				stream >> requiredValue;
				if(stream.fail())
				{
					requiredValue = 0;
				}
			}
			// TODO: notify the user.
			LL_WARNS("UpdaterService") << "last install attempt failed" << LL_ENDL;;
			LLFile::remove(ll_install_failed_marker_path());

			LLSD event;
			event["type"] = LLSD(LLUpdaterService::INSTALL_ERROR);
			event["required"] = LLSD(requiredValue);
			LLEventPumps::instance().obtain(LLUpdaterService::pumpName()).post(event);

			setState(LLUpdaterService::TERMINAL);
		}
		else
		{
			std::string query_url = LLGridManager::getInstance()->getUpdateServiceURL();
			if ( !query_url.empty() )
			{
				mUpdateChecker.checkVersion(query_url, mChannel, mVersion,
											mPlatform, mPlatformVersion, mUniqueId,
											mWillingToTest);
				setState(LLUpdaterService::CHECKING_FOR_UPDATE);
			}
			else
			{
				LL_WARNS("UpdaterService")
					<< "No updater service defined for grid '" << LLGridManager::getInstance()->getGrid()
					<< "' will check again in " << mCheckPeriod << " seconds"
					<< LL_ENDL;
				// Because the grid can be changed after the viewer is started (when the first check takes place)
				// but before the user logs in, the next check may be on a different grid, so set the retry timer
				// even though this check did not happen.  The default time is once an hour, and if we're not
				// doing the check anyway the performance impact is completely insignificant.
				restartTimer(mCheckPeriod);
			}
		}
	} 
	else 
	{
		// Keep on waiting...
	}
	
	return false;
}


//-----------------------------------------------------------------------
// Facade interface

std::string const & LLUpdaterService::pumpName(void)
{
	static std::string name("updater_service");
	return name;
}

bool LLUpdaterService::updateReadyToInstall(void)
{
	return LLFile::isfile(update_marker_path());
}

LLUpdaterService::LLUpdaterService()
{
	if(gUpdater.expired())
	{
		mImpl = 
			boost::shared_ptr<LLUpdaterServiceImpl>(new LLUpdaterServiceImpl());
		gUpdater = mImpl;
	}
	else
	{
		mImpl = gUpdater.lock();
	}
}

LLUpdaterService::~LLUpdaterService()
{
}

void LLUpdaterService::initialize(const std::string& channel,
								  const std::string& version,
								  const std::string& platform,
								  const std::string& platform_version,
								  const unsigned char uniqueid[MD5HEX_STR_SIZE],
								  const bool&         willing_to_test
)
{
	mImpl->initialize(channel, version, platform, platform_version, uniqueid, willing_to_test);
}

void LLUpdaterService::setCheckPeriod(unsigned int seconds)
{
	mImpl->setCheckPeriod(seconds);
}

void LLUpdaterService::setBandwidthLimit(U64 bytesPerSecond)
{
	mImpl->setBandwidthLimit(bytesPerSecond);
}
	
void LLUpdaterService::startChecking(bool install_if_ready)
{
	mImpl->startChecking(install_if_ready);
}

void LLUpdaterService::stopChecking()
{
	mImpl->stopChecking();
}

//bool LLUpdaterService::forceCheck()
//{
//	return mImpl->forceCheck();
//}

bool LLUpdaterService::isChecking()
{
	return mImpl->isChecking();
}

// [SL:KB] - Patch: Viewer-Updater | Checked: Catznip-3.1
void LLUpdaterService::checkForUpdate(bool user_feedback)
{
	mImpl->checkForUpdate(false, user_feedback);
}

void LLUpdaterService::checkForInstall(bool launch_installer)
{
	mImpl->checkForInstall(launch_installer);
}

bool LLUpdaterService::isDownloading()
{
	return mImpl->isDownloading();
}

void LLUpdaterService::startDownloading()
{
	return mImpl->startDownloading();
}

const LLSD& LLUpdaterService::getDownloadData() const
{
	return mImpl->getDownloadData();
}
// [/SL:KB]

LLUpdaterService::eUpdaterState LLUpdaterService::getState()
{
	return mImpl->getState();
}

void LLUpdaterService::setImplAppExitCallback(LLUpdaterService::app_exit_callback_t aecb)
{
	return mImpl->setAppExitCallback(aecb);
}

std::string LLUpdaterService::updatedVersion(void)
{
	return mImpl->updatedVersion();
}


std::string const & ll_get_version(void) {
	static std::string version("");
	
	if (version.empty()) {
		std::ostringstream stream;
		stream << LL_VIEWER_VERSION_MAJOR << "."
			   << LL_VIEWER_VERSION_MINOR << "."
			   << LL_VIEWER_VERSION_PATCH << "."
			   << LL_VIEWER_VERSION_BUILD;
		version = stream.str();
	}
	
	return version;
}

