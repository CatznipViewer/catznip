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
#include "httpcommon.h"
#include "llexception.h"
#include "lldir.h"
#include "llevents.h"
#include "llfile.h"
#include "llmd5.h"
#include "llsd.h"
#include "llsdserialize.h"

#include "llupdatedownloader.h"
#include "llupdaterservice.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// ====================================================================================
// Helper functions
//

namespace {
	class DownloadError : public LLException
	{
	public:
		DownloadError(const char* pstrMessage) : LLException(pstrMessage)
		{
		}
	};

	size_t write_function(void* pData, size_t blockSize, size_t blocks, void* pDownloader)
	{
		return reinterpret_cast<LLUpdateDownloader*>(pDownloader)->onBody(pData, blockSize * blocks);
	}

	size_t header_function(void* pData, size_t blockSize, size_t blocks, void* pDownloader)
	{
		return reinterpret_cast<LLUpdateDownloader*>(pDownloader)->onHeader(pData, blockSize * blocks);
	}

	int xferinfo_callback(void * pDownloader, curl_off_t dowloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow)
	{
		return reinterpret_cast<LLUpdateDownloader*>(pDownloader)->onProgress(dowloadTotal, downloadNow);
	}
}

// ====================================================================================
// LLUpdateDownloader
//

LLUpdateDownloader::LLUpdateDownloader(Client& client)
	: LLThread("LLUpdateDownloader")
	, mClient(client)
	, mCancelled(false)
	, mBandwidthLimit(0)
	, mCurl()
	, mHeaderList(nullptr)
	, mDownloadPercent(0)
{
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	llverify(code == CURLE_OK); // TODO: real error handling here.
}

LLUpdateDownloader::~LLUpdateDownloader()
{
	if (isDownloading())
	{
		cancel();
		shutdown();
	}
	mCurl.reset();
}

void LLUpdateDownloader::cancel(void)
{
	mCancelled = true;
}

void LLUpdateDownloader::download(const LLSD& sdUpdateData)
{
	if (isDownloading())
		mClient.downloadError("Download already in progress");

	mDownloadData = LLSD();
	mDownloadData["update_data"] = sdUpdateData;
	mDownloadData["required"] = sdUpdateData["required"].asBoolean();
	mDownloadData["update_channel"] = sdUpdateData["channel"].asString();
	mDownloadData["update_version"] = sdUpdateData["version"].asString();
	mDownloadData["more_info"] = sdUpdateData["more_info"].asString();
	mDownloadData["info_url"] = sdUpdateData["info_url"].asString();
	mDownloadRecordPath = downloadMarkerPath();

	try
	{
		startDownloading(sdUpdateData["url"].asString(), sdUpdateData["hash"].asString());
	}
	catch (DownloadError const & e)
	{
		mClient.downloadError(e.what());
	}
}

void LLUpdateDownloader::resume(void)
{
	if (isDownloading())
		mClient.downloadError("Download already in progress");

	mCancelled = false;
	mDownloadRecordPath = downloadMarkerPath();

	llifstream dataStream(mDownloadRecordPath.c_str());
	if (!dataStream)
	{
		mClient.downloadError("No download marker found");
		return;
	}

	LLSDSerialize::fromXMLDocument(mDownloadData, dataStream);
	if (!mDownloadData.asBoolean())
	{
		mClient.downloadError("No download information found in marker");
		return;
	}

	const std::string strUpdaterPath = mDownloadData["path"].asString();
	try
	{
		if (LLFile::isfile(strUpdaterPath))
		{
			llstat fileStatus;
			LLFile::stat(strUpdaterPath, &fileStatus);
			if (fileStatus.st_size != mDownloadData["size"].asInteger())
			{
				resumeDownloading(fileStatus.st_size);
			}
			else if (!validateOrRemove(strUpdaterPath))
			{
				// download() will clear mDownloadData so we need a local copy
				const LLSD sdUpdateData = mDownloadData["update_data"];
				download(sdUpdateData);
			}
			else
			{
				mClient.downloadComplete(mDownloadData);
			}
		}
		else
		{
			// download() will clear mDownloadData so we need a local copy
			const LLSD sdUpdateData = mDownloadData["update_data"];
			download(sdUpdateData);
		}
	}
	catch(DownloadError& e)
	{
		mClient.downloadError(e.what());
	}
}

void LLUpdateDownloader::setBandwidthLimit(U64 bytesPerSecond)
{
	if ( (mBandwidthLimit != bytesPerSecond) && (isDownloading()) )
	{
		llassert(static_cast<bool>(mCurl));

		mBandwidthLimit = bytesPerSecond;
		CURLcode code = curl_easy_setopt(mCurl.get(), CURLOPT_MAX_RECV_SPEED_LARGE, &mBandwidthLimit);
		if (code != CURLE_OK)
		{
			LL_WARNS("UpdaterService") << "Unable to change dowload bandwidth" << LL_ENDL;
		}
	}
	else
	{
		mBandwidthLimit = bytesPerSecond;
	}
}

std::string LLUpdateDownloader::downloadMarkerPath()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "CatznipUpdateDownload.xml");
}

void LLUpdateDownloader::initializeCurlGet(const std::string& strUrl, bool processHeader)
{
	if (!mCurl)
	{
		mCurl = LLCore::LLHttp::createEasyHandle();
	}
	else
	{
		curl_easy_reset(mCurl.get());
	}

	if (!mCurl)
	{
		LLTHROW(DownloadError("failed to initialize curl"));
	}
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_NOSIGNAL, true));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_FOLLOWLOCATION, true));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_WRITEFUNCTION, &write_function));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_WRITEDATA, this));
	if (processHeader)
	{
		throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_HEADERFUNCTION, &header_function));
		throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_HEADERDATA, this));
	}
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_HTTPGET, true));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_URL, strUrl.c_str()));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_XFERINFOFUNCTION, &xferinfo_callback));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_XFERINFODATA, this));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_NOPROGRESS, 0));
	// Bandwidth limit is determined by whether the user is still at login screen so always use the set bandwidth limit
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_MAX_RECV_SPEED_LARGE, mBandwidthLimit));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_CAINFO, gDirUtilp->getCAFile().c_str()));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_SSL_VERIFYHOST, 2));
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_SSL_VERIFYPEER, 1));

	mDownloadPercent = 0;
}

void LLUpdateDownloader::startDownloading(const LLURI& updaterUri, const std::string& strUpdaterHash)
{
	const LLSD sdPath = updaterUri.pathArray();
	if (sdPath.size() == 0)
		LLTHROW(DownloadError("No file path in updater url"));

	const std::string fileName = sdPath[sdPath.size() - 1].asString();
	const std::string filePath = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, fileName);

	mDownloadData["url"] = updaterUri.asString();
	mDownloadData["hash"] = strUpdaterHash;
	mDownloadData["current_version"] = ll_get_version();
	mDownloadData["path"] = filePath;

	LL_INFOS("UpdaterService") << "Downloading update installer " << filePath << " from " << updaterUri.asString() << LL_ENDL;
	LL_INFOS("UpdaterService") << "Hash of installer is " << strUpdaterHash << LL_ENDL;

	llofstream dataStream(mDownloadRecordPath.c_str());
	LLSDSerialize::toPrettyXML(mDownloadData, dataStream);

	mDownloadStream.open(filePath.c_str(), std::ios_base::out | std::ios_base::binary);
	initializeCurlGet(updaterUri.asString(), true);
	start();
}

void LLUpdateDownloader::resumeDownloading(size_t startByte)
{
	const std::string strUpdaterUrl = mDownloadData["url"].asString();

	LL_INFOS("UpdaterService") << "Resuming updater download from " << strUpdaterUrl << " at byte " << startByte << LL_ENDL;

	mDownloadStream.open(mDownloadData["path"].asString().c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::app);
	initializeCurlGet(strUpdaterUrl, false);

	// The header 'Range: bytes n-' will request the bytes remaining in the source begining with byte n and ending with the last byte.
	boost::format rangeHeaderFormat("Range: bytes=%u-");
	rangeHeaderFormat % startByte;
	mHeaderList = curl_slist_append(mHeaderList, rangeHeaderFormat.str().c_str());
	if (mHeaderList == 0)
	{
		LLTHROW(DownloadError("cannot add Range header"));
	}
	throwOnCurlError(curl_easy_setopt(mCurl.get(), CURLOPT_HTTPHEADER, mHeaderList));

	start();
}

void LLUpdateDownloader::throwOnCurlError(CURLcode code)
{
	if (CURLE_OK != code)
	{
		const char* errorString = curl_easy_strerror(code);
		if (errorString != 0)
			LLTHROW(DownloadError(curl_easy_strerror(code)));
		else
			LLTHROW(DownloadError("Unknown curl error"));
	}
}

bool LLUpdateDownloader::validateDownload(const std::string& filePath)
{
	llifstream fileStream(filePath.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!fileStream)
	{
		LL_INFOS("UpdaterService") << "Can't open " << filePath << ", invalid" << LL_ENDL;
		return false;
	}

	const std::string hash = mDownloadData["hash"].asString();
	if (!hash.empty())
	{
		char digest[33];
		LLMD5(fileStream).hex_digest(digest);
		if (hash == digest)
		{
			LL_INFOS("UpdaterService") << "verified hash " << hash << " for downloaded " << filePath << LL_ENDL;
			return true;
		}
		else
		{
			LL_WARNS("UpdaterService") << "download hash mismatch for " << filePath << ": expected " << hash << " but computed " << digest << LL_ENDL;
			return false;
		}
	}
	else
	{
		LL_INFOS("UpdaterService") << "no hash specified for " << filePath << ", unverified" << LL_ENDL;
		return true; // No hash check provided.
	}
}

bool LLUpdateDownloader::validateOrRemove(const std::string& filePath)
{
	bool valid = validateDownload(filePath);
	if (!valid)
	{
		LL_INFOS("UpdaterService") << "removing " << filePath << LL_ENDL;
		LLFile::remove(filePath);
	}
	return valid;
}

// ====================================================================================
// LLUpdateDownloader - LLThread overrides
//

void LLUpdateDownloader::run()
{
	CURLcode code = curl_easy_perform(mCurl.get());
	mDownloadStream.close();
	if(code == CURLE_OK)
	{
		LLFile::remove(mDownloadRecordPath);
		if (validateOrRemove(mDownloadData["path"]))
		{
			LL_INFOS("UpdaterService") << "download successful" << LL_ENDL;
			mClient.downloadComplete(mDownloadData);
		}
		else
		{
			mClient.downloadError("failed hash check");
		}
	}
	else if ( (mCancelled) && (code == CURLE_WRITE_ERROR) )
	{
		LL_INFOS("UpdaterService") << "download canceled by user" << LL_ENDL;
	}
	else
	{
		LL_WARNS("UpdaterService") << "download failed with error '" << curl_easy_strerror(code) << "'" << LL_ENDL;
		LLFile::remove(mDownloadRecordPath);
		if (mDownloadData.has("path"))
		{
			std::string filePath = mDownloadData["path"].asString();
			LL_INFOS("UpdaterService") << "removing " << filePath << LL_ENDL;
			LLFile::remove(filePath);
		}
		mClient.downloadError("curl error");
	}

	if (mHeaderList)
	{
		curl_slist_free_all(mHeaderList);
		mHeaderList = 0;
	}
}

// ====================================================================================
// LLUpdateDownloader - Event handlers
//

size_t LLUpdateDownloader::onHeader(void* pData, size_t sizeData)
{
	const char* pHeaderData = reinterpret_cast<const char*>(pData);
	const std::string strHeader(pHeaderData, pHeaderData + sizeData);

	size_t colonPosition = strHeader.find(':');
	if ( (std::string::npos != colonPosition) && (strHeader.substr(0, colonPosition) == "Content-Length") )
	{
		try {
			size_t firstDigitPos = strHeader.find_first_of("0123456789", colonPosition);
			size_t lastDigitPos = strHeader.find_last_of("0123456789");

			std::string contentLength = strHeader.substr(firstDigitPos, lastDigitPos - firstDigitPos + 1);
			size_t size = boost::lexical_cast<size_t>(contentLength);

			LL_INFOS("UpdaterService") << "Download size is " << size << LL_ENDL;

			mDownloadData["size"] = LLSD(LLSD::Integer(size));

			llofstream odataStream(mDownloadRecordPath.c_str());
			LLSDSerialize::toPrettyXML(mDownloadData, odataStream);
		}
		catch (const std::exception& e) {
			LL_WARNS("UpdaterService") << "Unable to read content length (" << e.what() << ")" << LL_ENDL;
		}
	}
	return sizeData;
}

size_t LLUpdateDownloader::onBody(void* pData, size_t sizeData)
{
	// Forces a write error which will halt curl thread
	if (mCancelled)
		return 0;
	if ( (sizeData == 0) || (pData == nullptr) )
		return 0;

	mDownloadStream.write(static_cast<const char *>(pData), sizeData);
	if (mDownloadStream.bad())
	{
		return 0;
	}

	return sizeData;
}

int LLUpdateDownloader::onProgress(curl_off_t downloadSize, curl_off_t bytesDownloaded)
{
	int downloadPercent = static_cast<int>(100.0 * ((double)bytesDownloaded / (double)downloadSize));
	if (downloadPercent > mDownloadPercent)
	{
		mDownloadPercent = downloadPercent;

		LLSD sdPayload;
		sdPayload["type"] = LLSD(LLUpdaterService::PROGRESS);
		sdPayload["download_size"] = (LLSD::Integer) downloadSize;
		sdPayload["bytes_downloaded"] = (LLSD::Integer) bytesDownloaded;

		LLSD sdEvent;
		sdEvent["pump"] = LLUpdaterService::pumpName();
		sdEvent["payload"] = sdPayload;

		LLEventPumps::instance().obtain("mainlooprepeater").post(sdEvent);

		LL_INFOS("UpdaterService") << "Progress event " << sdPayload << LL_ENDL;
	}

	return 0;
}

// ====================================================================================
