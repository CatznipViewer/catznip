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

#pragma once

#include "httpcommon.h"
#include "llthread.h"
#include <curl/curl.h>

// ====================================================================================
// LLUpdateDownloader - An asynchronous download service for fetching updates
//

class LLUpdateDownloader : public LLThread
{
	LOG_CLASS(LLUpdateDownloader);
public:
	class Client {
	public:
		// The download has completed successfully
		virtual void downloadComplete(LLSD const & data) = 0;
		// The download failed
		virtual void downloadError(std::string const & message) = 0;
	};

	LLUpdateDownloader(Client& client);
	~LLUpdateDownloader() override;

	/*
	 * LLThread overrides
	 */
protected:
	void run() override;

	/*
	 * Member functions
	 */
public:
	// Cancel any in progress download; a no op if none is in progress (The client will not receive a complete or error callback)
	void cancel(void);
	// Start a new download
	void download(const LLSD& sdUpdateData);
	const LLSD& getDownloadData() const { return mDownloadData; }
	// Returns true if a download is in progress
	bool isDownloading(void) const { return !isStopped(); }
	// Resume a partial download
	void resume(void);
	// Set a limit on the dowload rate
	void setBandwidthLimit(U64 bytesPerSecond);

	// Returns the path to the download marker file containing details of the latest download
	static std::string downloadMarkerPath();
protected:
	void initializeCurlGet(const std::string& strUrl, bool processHeader);
	void startDownloading(const LLURI& updaterUri, const std::string& strUpdaterHash);
	void resumeDownloading(size_t startByte);
	void throwOnCurlError(CURLcode code);
	bool validateDownload(const std::string& filePath);
	bool validateOrRemove(const std::string& filePath);

	/*
	 * Event handlers
	 */
public:
	size_t onHeader(void* pData, size_t sizeData);
	size_t onBody(void* pData, size_t sizeData);
	int    onProgress(curl_off_t downloadSize, curl_off_t bytesDownloaded);

	/*
	 * Member variables
	 */
protected:
	Client&       mClient;
	bool          mCancelled;
	curl_off_t    mBandwidthLimit;
	LLCore::LLHttp::CURL_ptr mCurl;
	curl_slist*   mHeaderList;
	LLSD          mDownloadData;
	std::string   mDownloadRecordPath;
	llofstream    mDownloadStream;
	unsigned char mDownloadPercent;
};

// ====================================================================================
