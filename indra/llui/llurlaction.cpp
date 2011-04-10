/** 
 * @file llurlaction.cpp
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
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

#include "linden_common.h"

#include "llurlaction.h"
#include "llview.h"
#include "llwindow.h"
#include "llurlregistry.h"
// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-03-19 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
#include "llavatarnamecache.h"
// [/SL:KB]

// global state for the callback functions
void (*LLUrlAction::sOpenURLCallback) (const std::string& url) = NULL;
void (*LLUrlAction::sOpenURLInternalCallback) (const std::string& url) = NULL;
void (*LLUrlAction::sOpenURLExternalCallback) (const std::string& url) = NULL;
bool (*LLUrlAction::sExecuteSLURLCallback) (const std::string& url) = NULL;


void LLUrlAction::setOpenURLCallback(void (*cb) (const std::string& url))
{
	sOpenURLCallback = cb;
}

void LLUrlAction::setOpenURLInternalCallback(void (*cb) (const std::string& url))
{
	sOpenURLInternalCallback = cb;
}

void LLUrlAction::setOpenURLExternalCallback(void (*cb) (const std::string& url))
{
	sOpenURLExternalCallback = cb;
}

void LLUrlAction::setExecuteSLURLCallback(bool (*cb) (const std::string& url))
{
	sExecuteSLURLCallback = cb;
}

void LLUrlAction::openURL(std::string url)
{
	if (sOpenURLCallback)
	{
		(*sOpenURLCallback)(url);
	}
}

void LLUrlAction::openURLInternal(std::string url)
{
	if (sOpenURLInternalCallback)
	{
		(*sOpenURLInternalCallback)(url);
	}
}

void LLUrlAction::openURLExternal(std::string url)
{
	if (sOpenURLExternalCallback)
	{
		(*sOpenURLExternalCallback)(url);
	}
}

void LLUrlAction::executeSLURL(std::string url)
{
	if (sExecuteSLURLCallback)
	{
		(*sExecuteSLURLCallback)(url);
	}
}

void LLUrlAction::clickAction(std::string url)
{
	// Try to handle as SLURL first, then http Url
	if ( (sExecuteSLURLCallback) && !(*sExecuteSLURLCallback)(url) )
	{
		if (sOpenURLCallback)
		{
			(*sOpenURLCallback)(url);
		}
	}
}

void LLUrlAction::teleportToLocation(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		if (! match.getLocation().empty())
		{
			executeSLURL("secondlife:///app/teleport/" + match.getLocation());
		}
	}	
}

void LLUrlAction::showLocationOnMap(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		if (! match.getLocation().empty())
		{
			executeSLURL("secondlife:///app/worldmap/" + match.getLocation());
		}
	}	
}

void LLUrlAction::copyURLToClipboard(std::string url)
{
	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(url));
}

void LLUrlAction::copyLabelToClipboard(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(match.getLabel()));
	}	
}

// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-03-19 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
void LLUrlAction::copyToClipboard(std::string strURL, LLSD sdAction)
{
	// Get id from 'secondlife:///app/agent/{id}/{action}'
	LLURI sdURI(strURL); LLSD sdPath = sdURI.pathArray();
	if (sdPath.size() == 4)
	{
		std::string strCommand = sdPath.get(1).asString();
		LLUUID idAgent(sdPath.get(2).asString());
		if ( (idAgent.notNull()) && ("agent" == strCommand) )
		{
			LLAvatarName avName;
			if (LLAvatarNameCache::get(idAgent, &avName))
			{
				std::string strAction = sdAction.asString();
				if ("fullname" == strAction)
				{
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.getCompleteName()));
				}
				else if ("displayname" == strAction)
				{
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.mDisplayName));
				}
				else if ("username" == strAction)
				{
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.mUsername));
				}
			}
		}
	}
}
// [/SL:KB]

void LLUrlAction::showProfile(std::string url)
{
	// Get id from 'secondlife:///app/{cmd}/{id}/{action}'
	// and show its profile
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	if (path_array.size() == 4)
	{
		std::string id_str = path_array.get(2).asString();
		if (LLUUID::validate(id_str))
		{
			std::string cmd_str = path_array.get(1).asString();
			executeSLURL("secondlife:///app/" + cmd_str + "/" + id_str + "/about");
		}
	}
}
