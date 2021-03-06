/** 
 * @file llurlaction.cpp
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2016, Kitty Barnett
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
// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-03-19 (Catznip-2.5)
#include "llavatarnamecache.h"
// [/SL:KB]


// global state for the callback functions
LLUrlAction::url_callback_t 		LLUrlAction::sOpenURLCallback;
LLUrlAction::url_callback_t 		LLUrlAction::sOpenURLInternalCallback;
LLUrlAction::url_callback_t 		LLUrlAction::sOpenURLExternalCallback;
LLUrlAction::execute_url_callback_t LLUrlAction::sExecuteSLURLCallback;


void LLUrlAction::setOpenURLCallback(url_callback_t cb)
{
	sOpenURLCallback = cb;
}

void LLUrlAction::setOpenURLInternalCallback(url_callback_t cb)
{
	sOpenURLInternalCallback = cb;
}

void LLUrlAction::setOpenURLExternalCallback(url_callback_t cb)
{
	sOpenURLExternalCallback = cb;
}

void LLUrlAction::setExecuteSLURLCallback(execute_url_callback_t cb)
{
	sExecuteSLURLCallback = cb;
}

void LLUrlAction::openURL(std::string url)
{
	if (sOpenURLCallback)
	{
		sOpenURLCallback(url);
	}
}

void LLUrlAction::openURLInternal(std::string url)
{
	if (sOpenURLInternalCallback)
	{
		sOpenURLInternalCallback(url);
	}
}

void LLUrlAction::openURLExternal(std::string url)
{
	if (sOpenURLExternalCallback)
	{
		sOpenURLExternalCallback(url);
	}
}

bool LLUrlAction::executeSLURL(std::string url, bool trusted_content)
{
	if (sExecuteSLURLCallback)
	{
		return sExecuteSLURLCallback(url, trusted_content);
	}
	return false;
}

void LLUrlAction::clickAction(std::string url, bool trusted_content)
{
	// Try to handle as SLURL first, then http Url
	if ( (sExecuteSLURLCallback) && !sExecuteSLURLCallback(url, trusted_content) )
	{
		if (sOpenURLCallback)
		{
			sOpenURLCallback(url);
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

// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-03-19 (Catznip-2.5)
void LLUrlAction::copyToClipboard(std::string strURL, const LLSD& sdAction)
{
	// Get id from 'secondlife:///app/agent/{id}/{action}'
	LLURI sdURI(strURL); LLSD sdPath = sdURI.pathArray();
	if (sdPath.size() == 4)
	{
		const std::string strCommand = sdPath.get(1).asString();
		const LLUUID idAgent(sdPath.get(2).asString());
		if ( (idAgent.notNull()) && ("agent" == strCommand) )
		{
			LLAvatarName avName;
			if (LLAvatarNameCache::get(idAgent, &avName))
			{
				std::string strAction = sdAction.asString();
				if ("fullname" == strAction)
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.getCompleteName(true, EShowUsername::SHOW_ALWAYS)));
				else if ("displayname" == strAction)
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.getDisplayName()));
				else if ("username" == strAction)
					LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(avName.getAccountName()));
			}
		}
	}
}

void LLUrlAction::muteAgent(std::string url)
{
	std::string id_str = getUserID(url);
	if (LLUUID::validate(id_str))
	{
		executeSLURL("secondlife:///app/agent/" + id_str + "/mute");
	}
}

void LLUrlAction::unmuteAgent(std::string url)
{
	std::string id_str = getUserID(url);
	if (LLUUID::validate(id_str))
	{
		executeSLURL("secondlife:///app/agent/" + id_str + "/unmute");
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

// [SL:KB] - Patch: UI-UrlContextMenu | Checked: Catznip-5.4
std::string LLUrlAction::getInventoryID(std::string url)
{
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	std::string id_str;
	// Only grab the UUID if it's an inventory URL
	const std::string strCommand = path_array.get(1).asString();
	if ( (path_array.size() == 4) && ("inventory" == strCommand) )
	{
		id_str = path_array.get(2).asString();
	}
	return id_str;
}
// [/SL:KB]

std::string LLUrlAction::getUserID(std::string url)
{
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	std::string id_str;
//	if (path_array.size() == 4)
// [SL:KB] - Patch: UI-UrlContextMenu | Checked: 2014-01-05 (Catznip-3.6)
	// Only grab the UUID if it's an agent URL
	const std::string strCommand = path_array.get(1).asString();
	if ( (path_array.size() == 4) && (("agent" == strCommand) || ("group" == strCommand)) )
// [/SL:KB]
	{
		id_str = path_array.get(2).asString();
	}
	return id_str;
}

std::string LLUrlAction::getObjectId(std::string url)
{
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	std::string id_str;
	if (path_array.size() >= 3)
	{
		id_str = path_array.get(2).asString();
	}
	return id_str;
}

std::string LLUrlAction::getObjectName(std::string url)
{
	LLURI uri(url);
	LLSD query_map = uri.queryMap();
	std::string name;
	if (query_map.has("name"))
	{
		name = query_map["name"].asString();
	}
	return name;
}

void LLUrlAction::sendIM(std::string url)
{
	std::string id_str = getUserID(url);
	if (LLUUID::validate(id_str))
	{
		executeSLURL("secondlife:///app/agent/" + id_str + "/im");
	}
}

// [SL:KB] - Patch: UI-UrlContextMenu | Checked: 2011-01-13 (Catznip-2.5)
// static
void LLUrlAction::executeGroupAction(const std::string& url, const std::string& action)
{
	const std::string strGroupId = getUserID(url);
	if (LLUUID::validate(strGroupId))
	{
		executeSLURL(llformat("secondlife:///app/group/%s/%s", strGroupId.c_str(), action.c_str()));
	}
}

void LLUrlAction::offerTeleport(const std::string& url)
{
	const std::string strAgentId = getUserID(url);
	if (LLUUID::validate(strAgentId))
	{
		executeSLURL("secondlife:///app/agent/" + strAgentId + "/offerteleport");
	}
}

void LLUrlAction::requestTeleport(const std::string& url)
{
	const std::string strAgentId = getUserID(url);
	if (LLUUID::validate(strAgentId))
	{
		executeSLURL("secondlife:///app/agent/" + strAgentId + "/requestteleport");
	}
}

void LLUrlAction::showItem(const std::string& url)
{
	const std::string strItemId = getInventoryID(url);
	if (LLUUID::validate(strItemId))
	{
		executeSLURL("secondlife:///app/inventory/" + strItemId + "/show");
	}
}

// [/SL:KB]

void LLUrlAction::addFriend(std::string url)
{
	std::string id_str = getUserID(url);
	if (LLUUID::validate(id_str))
	{
		executeSLURL("secondlife:///app/agent/" + id_str + "/requestfriend");
	}
}

void LLUrlAction::removeFriend(std::string url)
{
	std::string id_str = getUserID(url);
	if (LLUUID::validate(id_str))
	{
		executeSLURL("secondlife:///app/agent/" + id_str + "/removefriend");
	}
}

void LLUrlAction::blockObject(std::string url)
{
	std::string object_id = getObjectId(url);
	std::string object_name = getObjectName(url);
	if (LLUUID::validate(object_id))
	{
		executeSLURL("secondlife:///app/agent/" + object_id + "/block/" + LLURI::escape(object_name));
	}
}

void LLUrlAction::unblockObject(std::string url)
{
    std::string object_id = getObjectId(url);
    std::string object_name = getObjectName(url);
    if (LLUUID::validate(object_id))
    {
        executeSLURL("secondlife:///app/agent/" + object_id + "/unblock/" + object_name);
    }
}
