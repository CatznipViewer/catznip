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

// ====================================================================================
// LLUpdateChecker - Implements asynchronous checking for updates
//

class LLUpdateChecker
{
	LOG_CLASS(LLUpdateChecker);
public:
	class Client
	{
	public:
		// A successful response was received from the viewer version manager
		virtual void response(LLSD const & content) = 0;
		// An error occurred while checking for an update
		virtual void error(std::string const & message) = 0;
	};

	LLUpdateChecker(Client& client);
	virtual ~LLUpdateChecker();

	/*
	 * Member functions
	 */
public:
	// Check status of current app on the given host for the channel and version provided.
	void               checkVersion(const std::string& urlBase, const std::string& channel, const std::string& version, const std::string& platform, const std::string& platform_version);
protected:
	static std::string buildUrl(const std::string& urlBase, const std::string& channel, const std::string& version, const std::string& platform, const std::string& platform_version);
	void               checkVersionCoro(const std::string& strUrl);

	/*
	 * Member variables
	 */
protected:
	Client& mClient;
	bool    mInProgress;
};

// ====================================================================================
