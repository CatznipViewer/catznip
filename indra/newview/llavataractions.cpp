/** 
 * @file llavataractions.cpp
 * @brief Friend-related actions (add, remove, offer teleport, etc)
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

#include "llavataractions.h"

#include "boost/lambda/lambda.hpp"	// for lambda::constant

#include "llavatarnamecache.h"	// IDEVO
#include "llsd.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "roles_constants.h"    // for GP_MEMBER_INVITE

#include "llagent.h"
#include "llappviewer.h"		// for gLastVersionChannel
#include "llcachename.h"
#include "llcallingcard.h"		// for LLAvatarTracker
#include "llconversationlog.h"
#include "llfloateravatarpicker.h"	// for LLFloaterAvatarPicker
#include "llfloaterconversationpreview.h"
#include "llfloatergroupinvite.h"
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llfloaterpay.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterwebcontent.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llgiveinventory.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"	// for gInventory.findCategoryUUIDForType
#include "llinventorypanel.h"
#include "llfloaterimcontainer.h"
#include "llimview.h"			// for gIMMgr
#include "llmutelist.h"
#include "llnotificationsutil.h"	// for LLNotificationsUtil
#include "llpaneloutfitedit.h"
#include "llpanelprofile.h"
#include "llrecentpeople.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewermessage.h"	// for handle_lure
#include "llviewerregion.h"
#include "lltrans.h"
#include "llcallingcard.h"
#include "llslurl.h"			// IDEVO
#include "llsidepanelinventory.h"
#include "llavatarname.h"
#include "llagentui.h"
// [SL:KB] - Patch: UI-AvatarNearbyActions | Checked: 2010-12-02 (Catznip-3.0)
#include "llfloaterreporter.h"
#include "llparcel.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llworld.h"
// [/SL:KB]
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
#include "rlvactions.h"
#include "rlvcommon.h"
// [/RLVa:KB]

// Flags for kick message
const U32 KICK_FLAGS_DEFAULT	= 0x0;
const U32 KICK_FLAGS_FREEZE		= 1 << 0;
const U32 KICK_FLAGS_UNFREEZE	= 1 << 1;


// static
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id, const std::string& name)
{
	if(id == gAgentID)
	{
		LLNotificationsUtil::add("AddSelfFriend");
		return;
	}

	LLSD args;
	args["NAME"] = LLSLURL("agent", id, "completename").getSLURLString();
	LLSD payload;
	payload["id"] = id;
	payload["name"] = name;
    
    	LLNotificationsUtil::add("AddFriendWithMessage", args, payload, &callbackAddFriendWithMessage);

	// add friend to recent people list
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	LLRecentPeople::instance().add(id, LLRecentPeople::IT_GENERAL);
// [/SL:KB]
//	LLRecentPeople::instance().add(id);
}

static void on_avatar_name_friendship(const LLUUID& id, const LLAvatarName av_name)
{
	LLAvatarActions::requestFriendshipDialog(id, av_name.getCompleteName());
}

// static
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id)
{
	if(id.isNull())
	{
		return;
	}

	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_friendship, _1, _2));
}

// static
void LLAvatarActions::removeFriendDialog(const LLUUID& id)
{
	if (id.isNull())
		return;

	uuid_vec_t ids;
	ids.push_back(id);
	removeFriendsDialog(ids);
}

// static
void LLAvatarActions::removeFriendsDialog(const uuid_vec_t& ids)
{
	if(ids.size() == 0)
		return;

	LLSD args;
	std::string msgType;
	if(ids.size() == 1)
	{
		LLUUID agent_id = ids[0];
		LLAvatarName av_name;
		if(LLAvatarNameCache::get(agent_id, &av_name))
		{
			args["NAME"] = av_name.getCompleteName();
		}

		msgType = "RemoveFromFriends";
	}
	else
	{
		msgType = "RemoveMultipleFromFriends";
	}

	LLSD payload;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		payload["ids"].append(*it);
	}

	LLNotificationsUtil::add(msgType,
		args,
		payload,
		&handleRemove);
}

// static
void LLAvatarActions::offerTeleport(const LLUUID& invitee)
{
	if (invitee.isNull())
		return;

	std::vector<LLUUID> ids;
	ids.push_back(invitee);
	offerTeleport(ids);
}

// static
void LLAvatarActions::offerTeleport(const uuid_vec_t& ids) 
{
	if (ids.size() == 0)
		return;

	handle_lure(ids);
}

static void on_avatar_name_cache_start_im(const LLUUID& agent_id,
										  const LLAvatarName& av_name)
{
//	std::string name = av_name.getDisplayName();
// [SL:KB] - Patch: Chat-Logs | Checked: 2013-08-15 (Catznip-3.6)
	std::string name = av_name.getUserName();
// [/SL:KB]
	LLUUID session_id = gIMMgr->addSession(name, IM_NOTHING_SPECIAL, agent_id);
	if (session_id != LLUUID::null)
	{
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
		LLFloaterIMContainerBase::getInstance()->showConversation(session_id);
// [/SL:KB]
//		LLFloaterIMContainer::getInstance()->showConversation(session_id);
	}
	make_ui_sound("UISndStartIM");
}

// static
void LLAvatarActions::startIM(const LLUUID& id)
{
	if (id.isNull() || gAgent.getID() == id)
		return;

// [RLVa:KB] - Checked: 2013-05-09 (RLVa-1.4.9)
	if (!RlvActions::canStartIM(id))
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
// [/RLVa:KB]

	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_im, _1, _2));
}

// static
void LLAvatarActions::endIM(const LLUUID& id)
{
	if (id.isNull())
		return;
	
	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	if (session_id != LLUUID::null)
	{
// [SL:KB] - Patch: Chat-Base | Checked: 2013-04-24 (Catznip-3.4)
		LLFloaterIMSession* pIMSession = LLFloaterIMSession::findInstance(session_id);
		if (pIMSession)
		{
			// See LLFloaterIMContainer::doToSelectedConversation()
			LLFloater::onClickClose(pIMSession);
		}
// [/SL:KB]
//		gIMMgr->leaveSession(session_id);
	}
}

static void on_avatar_name_cache_start_call(const LLUUID& agent_id,
											const LLAvatarName& av_name)
{
//	std::string name = av_name.getDisplayName();
// [SL:KB] - Patch: Chat-Logs | Checked: 2013-08-15 (Catznip-3.6)
	std::string name = av_name.getUserName();
// [/SL:KB]
	LLUUID session_id = gIMMgr->addSession(name, IM_NOTHING_SPECIAL, agent_id, true);
	if (session_id != LLUUID::null)
	{
		gIMMgr->startCall(session_id);
	}
	make_ui_sound("UISndStartIM");
}

// static
void LLAvatarActions::startCall(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}

// [RLVa:KB] - Checked: 2013-05-09 (RLVa-1.4.9)
	if (!RlvActions::canStartIM(id))
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
// [/RLVa:KB]

	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_call, _1, _2));
}

// static
void LLAvatarActions::startAdhocCall(const uuid_vec_t& ids, const LLUUID& floater_id)
{
	if (ids.size() == 0)
	{
		return;
	}

	// convert vector into std::vector for addSession
	std::vector<LLUUID> id_array;
	id_array.reserve(ids.size());
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
		id_array.push_back(idAgent);
// [/RLVa:KB]
//		id_array.push_back(*it);
	}

	// create the new ad hoc voice session
	const std::string title = LLTrans::getString("conference-title");
	LLUUID session_id = gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START,
										   ids[0], id_array, true, floater_id);
	if (session_id == LLUUID::null)
	{
		return;
	}

	gIMMgr->autoStartCallOnStartup(session_id);

	make_ui_sound("UISndStartIM");
}

/* AD *TODO: Is this function needed any more?
	I fixed it a bit(added check for canCall), but it appears that it is not used
	anywhere. Maybe it should be removed?
// static
bool LLAvatarActions::isCalling(const LLUUID &id)
{
	if (id.isNull() || !canCall())
	{
		return false;
	}

	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	return (LLIMModel::getInstance()->findIMSession(session_id) != NULL);
}*/

//static
bool LLAvatarActions::canCall()
{
	return LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
}

// static
void LLAvatarActions::startConference(const uuid_vec_t& ids, const LLUUID& floater_id)
{
	// *HACK: Copy into dynamic array
	std::vector<LLUUID> id_array;

	id_array.reserve(ids.size());
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
		id_array.push_back(idAgent);
// [/RLVa:KB]
//		id_array.push_back(*it);
	}
	const std::string title = LLTrans::getString("conference-title");
	LLUUID session_id = gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START, ids[0], id_array, false, floater_id);

	if (session_id == LLUUID::null)
	{
		return;
	}
	
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-25 (Catznip-3.5)
	LLFloaterIMContainerBase::getInstance()->showConversation(session_id);
// [/SL:KB]
//	LLFloaterIMContainer::getInstance()->showConversation(session_id);
	
	make_ui_sound("UISndStartIM");
}

static const char* get_profile_floater_name(const LLUUID& avatar_id)
{
	// Use different floater XML for our profile to be able to save its rect.
	return avatar_id == gAgentID ? "my_profile" : "profile";
}

static void on_avatar_name_show_profile(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	std::string url = getProfileURL(av_name.getAccountName());

	// PROFILES: open in webkit window
	LLFloaterWebContent::Params p;
	p.url(url).id(agent_id.asString());
	LLFloaterReg::showInstance(get_profile_floater_name(agent_id), p);
}

// static
void LLAvatarActions::showProfile(const LLUUID& id)
{
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2011-05-13 (Catznip-2.6)
	if ( (!gSavedSettings.getBOOL("ShowProfileFloaters")) || ((gAgent.getID() == id)) )
		showWebProfile(id);
	else
		showLegacyProfile(id);
// [/SL:KB]
}
//void LLAvatarActions::showProfile(const LLUUID& id)
//{
//	if (id.notNull())
//	{
//		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_show_profile, _1, _2));
//	}
//}

// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2011-05-13 (Catznip-2.6)
// static
void LLAvatarActions::showLegacyProfile(const LLUUID& id)
{
	if (id.notNull())
	{
		LLFloaterReg::showInstance("floater_profile_view", LLSD().with("id", id));
	}
}

// static
void LLAvatarActions::showWebProfile(const LLUUID& id)
{
	if (id.notNull())
	{
		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_show_profile, _1, _2));
	}
}
// [/SL:KB]

//static 
bool LLAvatarActions::profileVisible(const LLUUID& id)
{
	LLSD sd;
	sd["id"] = id;
	LLFloater* browser = getProfileFloater(id);
	return browser && browser->isShown();
}

//static
LLFloater* LLAvatarActions::getProfileFloater(const LLUUID& id)
{
// [SL:KB] - Patch: UI-ProfileFloaters | Checked: 2011-11-05 (Catznip-3.2)
	LLFloater* pFloater = LLFloaterReg::findInstance("floater_profile_view", LLSD().with("id", id));
	if (!pFloater)
		pFloater = LLFloaterReg::findInstance(get_profile_floater_name(id), LLSD().with("id", id));
	return pFloater;
// [/SL:KB]
//	LLFloaterWebContent *browser = dynamic_cast<LLFloaterWebContent*>
//		(LLFloaterReg::findInstance(get_profile_floater_name(id), LLSD().with("id", id)));
}

//static 
void LLAvatarActions::hideProfile(const LLUUID& id)
{
	LLSD sd;
	sd["id"] = id;
	LLFloater* browser = getProfileFloater(id);
	if (browser)
	{
		browser->closeFloater();
	}
}

// static
void LLAvatarActions::showOnMap(const LLUUID& id)
{
	LLAvatarName av_name;
	if (!LLAvatarNameCache::get(id, &av_name))
	{
		LLAvatarNameCache::get(id, boost::bind(&LLAvatarActions::showOnMap, id));
		return;
	}

	gFloaterWorldMap->trackAvatar(id, av_name.getDisplayName());
	LLFloaterReg::showInstance("world_map", "center");
}

// static
void LLAvatarActions::pay(const LLUUID& id)
{
	LLNotification::Params params("DoNotDisturbModePay");
	params.functor.function(boost::bind(&LLAvatarActions::handlePay, _1, _2, id));

	if (gAgent.isDoNotDisturb())
	{
		// warn users of being in do not disturb mode during a transaction
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 1);
	}
}

void LLAvatarActions::teleport_request_callback(const LLSD& notification, const LLSD& response)
{
	S32 option;
	if (response.isInteger()) 
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}

	if (0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;

// [RLVa:KB] - Checked: RLVa-2.0.0
		const LLUUID idRecipient = notification["substitutions"]["uuid"];
		std::string strMessage = response["message"];

		// Filter the request message if the recipients is IM-blocked
		if ( (RlvActions::isRlvEnabled()) && ((!RlvActions::canStartIM(idRecipient)) || (!RlvActions::canSendIM(idRecipient))) )
		{
			strMessage = RlvStrings::getString(RLV_STRING_HIDDEN);
		}
// [/RLVa:KB]

		msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_MessageBlock);
		msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
		msg->addUUIDFast(_PREHASH_ToAgentID, notification["substitutions"]["uuid"] );
		msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
		msg->addU8Fast(_PREHASH_Dialog, IM_TELEPORT_REQUEST);
		msg->addUUIDFast(_PREHASH_ID, LLUUID::null);
		msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

		std::string name;
		LLAgentUI::buildFullname(name);

		msg->addStringFast(_PREHASH_FromAgentName, name);
// [RLVa:KB] - Checked: RLVa-2.0.0
		msg->addStringFast(_PREHASH_Message, strMessage);
// [/RLVa:KB]
//		msg->addStringFast(_PREHASH_Message, response["message"]);
		msg->addU32Fast(_PREHASH_ParentEstateID, 0);
		msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
		msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());

		gMessageSystem->addBinaryDataFast(
				_PREHASH_BinaryBucket,
				EMPTY_BINARY_BUCKET,
				EMPTY_BINARY_BUCKET_SIZE);

		gAgent.sendReliableMessage();
	}
}

// static
void LLAvatarActions::teleportRequest(const LLUUID& id)
{
	LLSD notification;
	notification["uuid"] = id;
// [RLVa:KB] - Checked: RLVa-1.5.0
	notification["NAME"] = LLSLURL("agent", id, (RlvActions::canShowName(RlvActions::SNC_TELEPORTREQUEST, id)) ? "completename" : "rlvanonym").getSLURLString();
// [/RLVa:KB]
//	LLAvatarName av_name;
//	if (!LLAvatarNameCache::get(id, &av_name))
//	{
//		// unlikely ... they just picked this name from somewhere...
//		LLAvatarNameCache::get(id, boost::bind(&LLAvatarActions::teleportRequest, id));
//		return; // reinvoke this when the name resolves
//	}
//	notification["NAME"] = av_name.getCompleteName();

	LLSD payload;

	LLNotificationsUtil::add("TeleportRequestPrompt", notification, payload, teleport_request_callback);
}

// static
void LLAvatarActions::kick(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("KickUser", LLSD(), payload, handleKick);
}

// static
//void LLAvatarActions::freezeAvatar(const LLUUID& id)
//{
//	std::string fullname;
//	gCacheName->getFullName(id, fullname);
//	LLSD payload;
//	payload["avatar_id"] = id;
//
//	if (!fullname.empty())
//	{
//		LLSD args;
//		args["AVATAR_NAME"] = fullname;
//		LLNotificationsUtil::add("FreezeAvatarFullname", args, payload, handleFreezeAvatar);
//	}
//	else
//	{
//		LLNotificationsUtil::add("FreezeAvatar", LLSD(), payload, handleFreezeAvatar);
//	}
//}

// static
//void LLAvatarActions::ejectAvatar(const LLUUID& id, bool ban_enabled)
//{
//	std::string fullname;
//	gCacheName->getFullName(id, fullname);
//	LLSD payload;
//	payload["avatar_id"] = id;
//	payload["ban_enabled"] = ban_enabled;
//	LLSD args;
//	if (!fullname.empty())
//	{
//		args["AVATAR_NAME"] = fullname;
//	}
//
//	if (ban_enabled)
//	{
//			LLNotificationsUtil::add("EjectAvatarFullname", args, payload, handleEjectAvatar);
//	}
//	else
//	{
//		if (!fullname.empty())
//		{
//			LLNotificationsUtil::add("EjectAvatarFullnameNoBan", args, payload, handleEjectAvatar);
//		}
//		else
//		{
//			LLNotificationsUtil::add("EjectAvatarNoBan", LLSD(), payload, handleEjectAvatar);
//		}
//	}
//}

// static
void LLAvatarActions::freeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("FreezeUser", LLSD(), payload, handleFreeze);
}
// static
void LLAvatarActions::unfreeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("UnFreezeUser", LLSD(), payload, handleUnfreeze);
}

//static 
void LLAvatarActions::csr(const LLUUID& id, std::string name)
{
	if (name.empty()) return;
	
	std::string url = "http://csr.lindenlab.com/agent/";
	
	// slow and stupid, but it's late
	S32 len = name.length();
	for (S32 i = 0; i < len; i++)
	{
		if (name[i] == ' ')
		{
			url += "%20";
		}
		else
		{
			url += name[i];
		}
	}
	
	LLWeb::loadURL(url);
}

//static 
void LLAvatarActions::share(const LLUUID& id)
{
	LLSD key;
	LLFloaterSidePanelContainer::showPanel("inventory", key);
	LLFloaterReg::showInstance("im_container");

	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL,id);

	if (!gIMMgr->hasSession(session_id))
	{
		startIM(id);
	}

	if (gIMMgr->hasSession(session_id))
	{
		// we should always get here, but check to verify anyways
// [SL:KB] - Patch: Chat-Logging | Checked: 2014-04-08 (Catznip-3.6)
		LLIMModel::getInstance()->addMessage(session_id, SYSTEM_FROM, LLUUID::null, LLTrans::getString("share_alert"), LLLogChat::timestamp(false), false, true);
// [/SL:KB]
//		LLIMModel::getInstance()->addMessage(session_id, SYSTEM_FROM, LLUUID::null, LLTrans::getString("share_alert"), false);

		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
		if (session_floater && session_floater->isMinimized())
		{
			session_floater->setMinimized(false);
		}
//		LLFloaterIMContainer *im_container = LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
// [SL:KB] - Patch: Chat-Tabs | Checked: 2015-04-27 (Catznip-3.7)
		LLFloaterIMContainerBase* im_container = LLFloaterIMContainerBase::getInstance();
// [/SL:KB]
		im_container->selectConversationPair(session_id, true);
	}
}

namespace action_give_inventory
{
//	/**
//	 * Returns a pointer to 'Add More' inventory panel of Edit Outfit SP.
//	 */
//	static LLInventoryPanel* get_outfit_editor_inventory_panel()
//	{
//		LLPanelOutfitEdit* panel_outfit_edit = dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
//		if (NULL == panel_outfit_edit) return NULL;
//
//		LLInventoryPanel* inventory_panel = panel_outfit_edit->findChild<LLInventoryPanel>("folder_view");
//		return inventory_panel;
//	}
//
//	/**
//	 * @return active inventory panel, or NULL if there's no such panel
//	 */
//	static LLInventoryPanel* get_active_inventory_panel()
//	{
//		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
//		LLFloater* floater_appearance = LLFloaterReg::findInstance("appearance");
//		if (!active_panel || (floater_appearance && floater_appearance->hasFocus()))
//		{
//			active_panel = get_outfit_editor_inventory_panel();
//		}
//
//		return active_panel;
//	}

	/**
	 * Checks My Inventory visibility.
	 */

//	static bool is_give_inventory_acceptable()
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
	static bool is_give_inventory_acceptable(LLHandle<LLInventoryPanel> inv_panel_handle)
// [/SL:KB]
	{
//		// check selection in the panel
//		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs(inv_panel_handle.get());
// [/SL:KB]
		if (inventory_selected_uuids.empty()) return false; // nothing selected

		bool acceptable = false;
		std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();
		const std::set<LLUUID>::const_iterator it_end = inventory_selected_uuids.end();
		for (; it != it_end; ++it)
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			// any category can be offered.
			if (inv_cat)
			{
				acceptable = true;
				continue;
			}

			LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
			// check if inventory item can be given
			if (LLGiveInventory::isInventoryGiveAcceptable(inv_item))
			{
				acceptable = true;
				continue;
			}

			// there are neither item nor category in inventory
			acceptable = false;
			break;
		}
		return acceptable;
	}

	static void build_items_string(const std::set<LLUUID>& inventory_selected_uuids , std::string& items_string)
	{
		llassert(inventory_selected_uuids.size() > 0);

		const std::string& separator = LLTrans::getString("words_separator");
		for (std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin(); ; )
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			if (NULL != inv_cat)
			{
				items_string = inv_cat->getName();
				break;
			}
			LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
			if (NULL != inv_item)
			{
				items_string.append(inv_item->getName());
			}
			if(++it == inventory_selected_uuids.end())
			{
				break;
			}
			items_string.append(separator);
		}
	}

	struct LLShareInfo : public LLSingleton<LLShareInfo>
	{
		std::vector<LLAvatarName> mAvatarNames;
		uuid_vec_t mAvatarUuids;
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2013-09-07 (Catznip-3.6)
		std::set<LLUUID> mItemIds;	// Use std::set because it requires the least amount of LL code changes (see give_inventory_cb)
// [/SL:KB]
	};

	static void give_inventory_cb(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		// if Cancel pressed
		if (option == 1)
		{
			return;
		}

//		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
//		if (inventory_selected_uuids.empty())
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2013-09-07 (Catznip-3.6)
		const std::set<LLUUID>& inventory_selected_uuids = LLShareInfo::instance().mItemIds;
// [/SL:KB]
		if (inventory_selected_uuids.empty())
		{
			return;
		}

		S32 count = LLShareInfo::instance().mAvatarNames.size();
		bool shared = count && !inventory_selected_uuids.empty();

		// iterate through avatars
		for(S32 i = 0; i < count; ++i)
		{
			const LLUUID& avatar_uuid = LLShareInfo::instance().mAvatarUuids[i];

			// We souldn't open IM session, just calculate session ID for logging purpose. See EXT-6710
			const LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, avatar_uuid);

			std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();
			const std::set<LLUUID>::const_iterator it_end = inventory_selected_uuids.end();

			const std::string& separator = LLTrans::getString("words_separator");
			std::string noncopy_item_names;
			LLSD noncopy_items = LLSD::emptyArray();
			// iterate through selected inventory objects
			for (; it != it_end; ++it)
			{
				LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
				if (inv_cat)
				{
					if (!LLGiveInventory::doGiveInventoryCategory(avatar_uuid, inv_cat, session_id, "ItemsShared"))
					{
						shared = false;
					}
					break;
				}
				LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
				if (!inv_item->getPermissions().allowCopyBy(gAgentID))
				{
					if (!noncopy_item_names.empty())
					{
						noncopy_item_names.append(separator);
					}
					noncopy_item_names.append(inv_item->getName());
					noncopy_items.append(*it);
				}
				else
				{
					if (!LLGiveInventory::doGiveInventoryItem(avatar_uuid, inv_item, session_id))
					{
						shared = false;
					}
				}
			}
			if (noncopy_items.beginArray() != noncopy_items.endArray())
			{
				LLSD substitutions;
				substitutions["ITEMS"] = noncopy_item_names;
				LLSD payload;
				payload["agent_id"] = avatar_uuid;
				payload["items"] = noncopy_items;
				payload["success_notification"] = "ItemsShared";
				LLNotificationsUtil::add("CannotCopyWarning", substitutions, payload,
					&LLGiveInventory::handleCopyProtectedItem);
				shared = false;
				break;
			}
		}
		if (shared)
		{
			LLFloaterReg::hideInstance("avatar_picker");
			LLNotificationsUtil::add("ItemsShared");
		}
	}

	/**
	 * Performs "give inventory" operations for provided avatars.
	 *
	 * Sends one requests to give all selected inventory items for each passed avatar.
	 * Avatars are represent by two vectors: names and UUIDs which must be sychronized with each other.
	 *
	 * @param avatar_names - avatar names request to be sent.
	 * @param avatar_uuids - avatar names request to be sent.
	 */
//	static void give_inventory(const uuid_vec_t& avatar_uuids, const std::vector<LLAvatarName> avatar_names)
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
	static void give_inventory(const uuid_vec_t& avatar_uuids, const std::vector<LLAvatarName> avatar_names, LLHandle<LLInventoryPanel> inv_panel_handle)
// [/SL:KB]
	{
		llassert(avatar_names.size() == avatar_uuids.size());

//		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs(inv_panel_handle.get());
// [/SL:KB]
		if (inventory_selected_uuids.empty())
		{
			return;
		}

		std::string residents;
		LLAvatarActions::buildResidentsString(avatar_names, residents, true);

		std::string items;
		build_items_string(inventory_selected_uuids, items);

		int folders_count = 0;
		std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();

		//traverse through selected inventory items and count folders among them
		for ( ; it != inventory_selected_uuids.end() && folders_count <=1 ; ++it)
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			if (NULL != inv_cat)
			{
				folders_count++;
			}
		}

		// EXP-1599
		// In case of sharing multiple folders, make the confirmation
		// dialog contain a warning that only one folder can be shared at a time.
		std::string notification = (folders_count > 1) ? "ShareFolderConfirmation" : "ShareItemsConfirmation";
		LLSD substitutions;
		substitutions["RESIDENTS"] = residents;
		substitutions["ITEMS"] = items;
		LLShareInfo::instance().mAvatarNames = avatar_names;
		LLShareInfo::instance().mAvatarUuids = avatar_uuids;
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2013-09-07 (Catznip-3.6)
		LLShareInfo::instance().mItemIds = inventory_selected_uuids;
// [/SL:KB]
		LLNotificationsUtil::add(notification, substitutions, LLSD(), &give_inventory_cb);
	}
}

// static
void LLAvatarActions::buildResidentsString(std::vector<LLAvatarName> avatar_names, std::string& residents_string, bool complete_name)
{
	llassert(avatar_names.size() > 0);
	
	std::sort(avatar_names.begin(), avatar_names.end());
	const std::string& separator = LLTrans::getString("words_separator");
	for (std::vector<LLAvatarName>::const_iterator it = avatar_names.begin(); ; )
	{
		if(complete_name)
		{
			residents_string.append((*it).getCompleteName());
		}
		else
		{
			residents_string.append((*it).getDisplayName());
		}

		if	(++it == avatar_names.end())
		{
			break;
		}
		residents_string.append(separator);
	}
}

// static
void LLAvatarActions::buildResidentsString(const uuid_vec_t& avatar_uuids, std::string& residents_string)
{
	std::vector<LLAvatarName> avatar_names;
	uuid_vec_t::const_iterator it = avatar_uuids.begin();
	for (; it != avatar_uuids.end(); ++it)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(*it, &av_name))
		{
			avatar_names.push_back(av_name);
		}
	}
	
	// We should check whether the vector is not empty to pass the assertion
	// that avatar_names.size() > 0 in LLAvatarActions::buildResidentsString.
	if (!avatar_names.empty())
	{
		LLAvatarActions::buildResidentsString(avatar_names, residents_string);
	}
}

//static
//std::set<LLUUID> LLAvatarActions::getInventorySelectedUUIDs()
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2012-07-19 (Catznip-3.3)
std::set<LLUUID> LLAvatarActions::getInventorySelectedUUIDs(LLInventoryPanel* inv_panel)
// [/SL:KB]
{
	std::set<LLFolderViewItem*> inventory_selected;

//	LLInventoryPanel* active_panel = action_give_inventory::get_active_inventory_panel();
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2012-07-19 (Catznip-3.3)
	LLInventoryPanel* active_panel = inv_panel;
// [/SL:KB]
	if (active_panel)
	{
		inventory_selected= active_panel->getRootFolder()->getSelectionList();
	}

	if (inventory_selected.empty())
	{
//		LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
// [SL:KB] - Patch: Inventory-ActivePanel | Checked: 2011-11-02 (Catznip-3.2)
		LLSidepanelInventory *sidepanel_inventory = (active_panel) ? active_panel->getParentByType<LLSidepanelInventory>() : NULL;
// [/SL:KB]
		if (sidepanel_inventory)
		{
			inventory_selected= sidepanel_inventory->getInboxSelectionList();
		}
	}

	std::set<LLUUID> inventory_selected_uuids;
	for (std::set<LLFolderViewItem*>::iterator it = inventory_selected.begin(), end_it = inventory_selected.end();
		it != end_it;
		++it)
	{
		inventory_selected_uuids.insert(static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem())->getUUID());
	}
	return inventory_selected_uuids;
}

//static
//void LLAvatarActions::shareWithAvatars(LLView * panel)
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
void LLAvatarActions::shareWithAvatars(LLInventoryPanel* inv_panel)
// [/SL:KB]
{
	using namespace action_give_inventory;

//    LLFloater* root_floater = gFloaterView->getParentFloater(panel);
//	LLFloaterAvatarPicker* picker =
//		LLFloaterAvatarPicker::show(boost::bind(give_inventory, _1, _2), TRUE, FALSE, FALSE, root_floater->getName());
//	if (!picker)
//	{
//		return;
//	}
//
//	picker->setOkBtnEnableCb(boost::bind(is_give_inventory_acceptable));
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2011-06-29 (Catznip-2.6)
	LLFloater* root_floater = gFloaterView->getParentFloater(inv_panel);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(give_inventory, _1, _2, inv_panel->getInventoryPanelHandle()), TRUE, FALSE, FALSE, root_floater->getName());
	if (!picker)
	{
		return;
	}
	picker->setOkBtnEnableCb(boost::bind(is_give_inventory_acceptable, inv_panel->getInventoryPanelHandle()));
// [/SL:KB]
	picker->openFriendsTab();
    
    if (root_floater)
    {
        root_floater->addDependentFloater(picker);
    }
	LLNotificationsUtil::add("ShareNotification");
}


// static
//bool LLAvatarActions::canShareSelectedItems(LLInventoryPanel* inv_panel /* = NULL*/)
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2012-07-19 (Catznip-3.3)
bool LLAvatarActions::canShareSelectedItems(LLInventoryPanel* inv_panel)
// [/SL:KB]
{
//	using namespace action_give_inventory;
//
//	if (!inv_panel)
//	{
//		LLInventoryPanel* active_panel = get_active_inventory_panel();
//		if (!active_panel) return false;
//		inv_panel = active_panel;
//	}
// [SL:KB] - Patch: Inventory-ShareSelection | Checked: 2012-07-19 (Catznip-3.3)
	// We modified shareWithAvatars() and getInventorySelectedUUIDs() to take an inventory panel parameter so it's not optional here
	if (!inv_panel)
	{
		return false;
	}
// [/SL:KB]

	// check selection in the panel
	LLFolderView* root_folder = inv_panel->getRootFolder();
    if (!root_folder)
    {
        return false;
    }
	const std::set<LLFolderViewItem*> inventory_selected = root_folder->getSelectionList();
	if (inventory_selected.empty()) return false; // nothing selected

	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	bool can_share = true;
	std::set<LLFolderViewItem*>::const_iterator it = inventory_selected.begin();
	const std::set<LLFolderViewItem*>::const_iterator it_end = inventory_selected.end();
	for (; it != it_end; ++it)
	{
		LLUUID cat_id = static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem())->getUUID();
		LLViewerInventoryCategory* inv_cat = gInventory.getCategory(cat_id);
		// any category can be offered if it's not in trash.
		if (inv_cat)
		{
			if ((cat_id == trash_id) || gInventory.isObjectDescendentOf(cat_id, trash_id))
			{
				can_share = false;
				break;
			}
			continue;
		}

		// check if inventory item can be given
		LLFolderViewItem* item = *it;
		if (!item) return false;
		LLInvFVBridge* bridge = dynamic_cast<LLInvFVBridge*>(item->getViewModelItem());
		if (bridge && bridge->canShare())
		{
			continue;
		}

		// there are neither item nor category in inventory
		can_share = false;
		break;
	}

	return can_share;
}

// static
void LLAvatarActions::toggleBlock(const LLUUID& id)
{
	std::string name;

	gCacheName->getFullName(id, name); // needed for mute
	LLMute mute(id, name, LLMute::AGENT);

	if (LLMuteList::getInstance()->isMuted(mute.mID, mute.mName))
	{
		LLMuteList::getInstance()->remove(mute);
	}
	else
	{
		LLMuteList::getInstance()->add(mute);
	}
}

// static
void LLAvatarActions::toggleMuteVoice(const LLUUID& id)
{
	std::string name;
	gCacheName->getFullName(id, name); // needed for mute

	LLMuteList* mute_list = LLMuteList::getInstance();
	bool is_muted = mute_list->isMuted(id, LLMute::flagVoiceChat);

	LLMute mute(id, name, LLMute::AGENT);
	if (!is_muted)
	{
		mute_list->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		mute_list->remove(mute, LLMute::flagVoiceChat);
	}
}

// [SL:KB] - Patch: Agent-DisplayNames | Checked: 2011-11-10 (Catznip-3.2)
void LLAvatarActions::copyToClipboard(const LLUUID& id, const LLSD& param)
{
	LLAvatarName avName;
	if (LLAvatarNameCache::get(id, &avName))
	{
		std::string strResult, strParam = param.asString();
		if ( ("fullname" == strParam) || (strParam.empty()) )
			strResult = avName.getCompleteName();
		else if ("displayname" == strParam)
			strResult = avName.getDisplayName();
		else if ("username" == strParam)
			strResult = avName.getAccountName();
		else if ("slurl" == strParam)
			strResult = LLSLURL("agent", id, "about").getSLURLString();
		else if ("uuid" == strParam)
			strResult = id.asString();
		LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(strResult));
	}
}
// [/SL:KB]

// static
bool LLAvatarActions::canOfferTeleport(const LLUUID& id)
{
	// First use LLAvatarTracker::isBuddy()
	// If LLAvatarTracker::instance().isBuddyOnline function only is used
	// then for avatars that are online and not a friend it will return false.
	// But we should give an ability to offer a teleport for such avatars.
	if(LLAvatarTracker::instance().isBuddy(id))
	{
		return LLAvatarTracker::instance().isBuddyOnline(id);
	}

	return true;
}

// static
bool LLAvatarActions::canOfferTeleport(const uuid_vec_t& ids)
{
	// We can't send more than 250 lures in a single message, so disable this
	// button when there are too many id's selected.
	if(ids.size() > 250) return false;
	
	bool result = true;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		if(!canOfferTeleport(*it))
		{
			result = false;
			break;
		}
	}
	return result;
}

void LLAvatarActions::inviteToGroup(const LLUUID& id)
{
	LLFloaterGroupPicker* widget = LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(id));
	if (widget)
	{
		widget->center();
		widget->setPowersMask(GP_MEMBER_INVITE);
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(callback_invite_to_group, _1, id));
	}
}

// [SL:KB] - Patch: UI-AddContact | Checked: 2013-12-12 (Catznip-3.6)
void LLAvatarActions::addContact(const LLUUID& id)
{
	if (id.notNull())
	{
		create_inventory_callingcard(id, gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD));
	}
}
// [/SL:KB]

// static
void LLAvatarActions::viewChatHistory(const LLUUID& id)
{
	const std::vector<LLConversation>& conversations = LLConversationLog::instance().getConversations();
	std::vector<LLConversation>::const_iterator iter = conversations.begin();

	for (; iter != conversations.end(); ++iter)
	{
		if (iter->getParticipantID() == id)
		{
			LLFloaterReg::showInstance("preview_conversation", iter->getSessionID(), true);
			return;
		}
	}

	if (LLLogChat::isTranscriptExist(id))
	{
		LLAvatarName avatar_name;
		LLSD extended_id(id);

		LLAvatarNameCache::get(id, &avatar_name);
		extended_id[LL_FCP_COMPLETE_NAME] = avatar_name.getCompleteName();
		extended_id[LL_FCP_ACCOUNT_NAME] = avatar_name.getAccountName();
		LLFloaterReg::showInstance("preview_conversation", extended_id, true);
	}
}

//== private methods ========================================================================================

// static
bool LLAvatarActions::handleRemove(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	const LLSD& ids = notification["payload"]["ids"];
	for (LLSD::array_const_iterator itr = ids.beginArray(); itr != ids.endArray(); ++itr)
	{
		LLUUID id = itr->asUUID();
		const LLRelationship* ip = LLAvatarTracker::instance().getBuddyInfo(id);
		if (ip)
		{
			switch (option)
			{
			case 0: // YES
				if( ip->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
				{
					LLAvatarTracker::instance().empower(id, FALSE);
					LLAvatarTracker::instance().notifyObservers();
				}
				LLAvatarTracker::instance().terminateBuddy(id);
				LLAvatarTracker::instance().notifyObservers();
				break;

			case 1: // NO
			default:
				LL_INFOS() << "No removal performed." << LL_ENDL;
				break;
			}
		}
	}
	return false;
}

// static
bool LLAvatarActions::handlePay(const LLSD& notification, const LLSD& response, LLUUID avatar_id)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		gAgent.setDoNotDisturb(false);
	}

	LLFloaterPayUtil::payDirectly(&give_money, avatar_id, /*is_group=*/false);
	return false;
}

// static
void LLAvatarActions::callback_invite_to_group(LLUUID group_id, LLUUID id)
{
	uuid_vec_t agent_ids;
	agent_ids.push_back(id);
	
	LLFloaterGroupInvite::showForGroup(group_id, &agent_ids);
}


// static
bool LLAvatarActions::callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		requestFriendship(notification["payload"]["id"].asUUID(), 
		    notification["payload"]["name"].asString(),
		    response["message"].asString());
	}
	return false;
}

// static
bool LLAvatarActions::handleKick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}

bool LLAvatarActions::handleFreezeAvatar(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (0 == option || 1 == option)
	{
	    U32 flags = 0x0;
	    if (1 == option)
	    {
	        // unfreeze
	        flags |= 0x1;
	    }
	    LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessage("FreezeUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		gAgent.sendReliableMessage();
	}
	return false;
}

bool LLAvatarActions::handleEjectAvatar(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (2 == option)
	{
		return false;
	}
	LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
	bool ban_enabled = notification["payload"]["ban_enabled"].asBoolean();

	if (0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		U32 flags = 0x0;
		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID() );
		msg->addUUID("SessionID", gAgent.getSessionID() );
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		gAgent.sendReliableMessage();
	}
	else if (ban_enabled)
	{
		LLMessageSystem* msg = gMessageSystem;

		U32 flags = 0x1;
		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID() );
		msg->addUUID("SessionID", gAgent.getSessionID() );
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		gAgent.sendReliableMessage();
	}
	return false;
}

bool LLAvatarActions::handleFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_FREEZE );
		msg->addStringFast(_PREHASH_Reason, response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}

bool LLAvatarActions::handleUnfreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_UNFREEZE );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
	return false;
}

// static
void LLAvatarActions::requestFriendship(const LLUUID& target_id, const std::string& target_name, const std::string& message)
{
	const LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
	send_improved_im(target_id,
					 target_name,
					 message,
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED,
					 calling_card_folder_id);

	LLSD args;
	args["TO_NAME"] = target_name;

	LLSD payload;
	payload["from_id"] = target_id;
	LLNotificationsUtil::add("FriendshipOffered", args, payload);
}

//static
bool LLAvatarActions::isFriend(const LLUUID& id)
{
	return ( NULL != LLAvatarTracker::instance().getBuddyInfo(id) );
}

// static
bool LLAvatarActions::isBlocked(const LLUUID& id)
{
	std::string name;
	gCacheName->getFullName(id, name); // needed for mute
	return LLMuteList::getInstance()->isMuted(id, name);
}

// static
bool LLAvatarActions::isVoiceMuted(const LLUUID& id)
{
	return LLMuteList::getInstance()->isMuted(id, LLMute::flagVoiceChat);
}

// static
bool LLAvatarActions::canBlock(const LLUUID& id)
{
	std::string full_name;
	gCacheName->getFullName(id, full_name); // needed for mute
	bool is_linden = (full_name.find("Linden") != std::string::npos);
	bool is_self = id == gAgentID;
	return !is_self && !is_linden;
}

// [SL:KB] - Patch: UI-AvatarNearbyActions | Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::report(const LLUUID& idAgent)
{
	LLAvatarName avName;
	LLAvatarNameCache::get(idAgent, &avName);

	LLFloaterReporter::showFromAvatar(idAgent, avName.getCompleteName());
}

bool LLAvatarActions::canZoomIn(const LLUUID& idAgent)
{
	return gObjectList.findObject(idAgent);
}

void LLAvatarActions::zoomIn(const LLUUID& idAgent)
{
	handle_zoom_to_object(idAgent);
}

//
// Parcel actions
//

// Defined in llworld.cpp
LLVector3d unpackLocalToGlobalPosition(U32 compact_local, const LLVector3d& region_origin);

// Checked: 2010-12-03 (Catznip-2.4)
static bool getRegionAndPosGlobalFromAgentID(const LLUUID& idAgent, const LLViewerRegion** ppRegion, LLVector3d* pPosGlobal)
{
	// Try looking up the agent in gObjectList
	const LLViewerObject* pAvatarObj = gObjectList.findObject(idAgent);
	if ( (pAvatarObj) && (!pAvatarObj->isDead()) )
	{
		if (ppRegion)
			*ppRegion = pAvatarObj->getRegion();
		if (pPosGlobal)
			*pPosGlobal = pAvatarObj->getPositionGlobal();
		return (pAvatarObj->isAvatar()) && (NULL != pAvatarObj->getRegion());
	}

	// Walk over each region we're connected to and try finding the agent on one of them
	LLWorld::region_list_t::const_iterator itRegion = LLWorld::getInstance()->getRegionList().begin();
	LLWorld::region_list_t::const_iterator endRegion = LLWorld::getInstance()->getRegionList().end();
	for (; itRegion != endRegion; ++itRegion)
	{
		const LLViewerRegion* pRegion = *itRegion;
		
		std::vector<LLUUID>::const_iterator itAgent = std::find(pRegion->mMapAvatarIDs.begin(), pRegion->mMapAvatarIDs.end(), idAgent);
		if (pRegion->mMapAvatarIDs.end() != itAgent)
		{
			if (ppRegion)
				*ppRegion = pRegion;
			if (pPosGlobal)
				*pPosGlobal = unpackLocalToGlobalPosition(pRegion->mMapAvatars.at(itAgent - pRegion->mMapAvatarIDs.begin()), pRegion->getOriginGlobal());
			return (NULL != pRegion);
		}
	}

	// Couldn't find the agent anywhere
	return false;
}

// Checked: 2010-12-03 (Catznip-2.4)
inline static bool getRegionFromAgentID(const LLUUID& idAgent, const LLViewerRegion** ppRegion)
{
	return getRegionAndPosGlobalFromAgentID(idAgent, ppRegion, NULL);
}

// Checked: 2010-12-03 (Catznip-2.4)
inline static bool getPosGlobalFromAgentID(const LLUUID& idAgent, LLVector3d& posGlobal)
{
	return getRegionAndPosGlobalFromAgentID(idAgent, NULL, &posGlobal);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::canLandFreezeOrEject(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	return canLandFreezeOrEjectMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::canLandFreezeOrEjectMultiple(uuid_vec_t& idAgents, bool fFilter /*=false*/)
{
	if (gAgent.isGodlikeWithoutAdminMenuFakery())
		return true;					// Gods can always freeze

	uuid_vec_t::iterator itAgent = idAgents.begin(); bool fCanFreeze = false;
	while ( (itAgent != idAgents.end()) && ((fFilter) || (!fCanFreeze)) )
	{
		const LLViewerRegion* pRegion = NULL; LLVector3d posGlobal;
		if (getRegionAndPosGlobalFromAgentID(*itAgent, &pRegion, &posGlobal))
		{
			// NOTE: we actually don't always need the parcel, but attempting to get it now will help with setting fBanEnabled when ejecting
			const LLParcel* pParcel = LLViewerParcelMgr::getInstance()->selectParcelAt(posGlobal)->getParcel();
			const LLVector3 posRegion = pRegion->getPosRegionFromGlobal(posGlobal);
			if ( (pRegion->getOwner() == gAgent.getID()) || (pRegion->isEstateManager()) || (pRegion->isOwnedSelf(posRegion)) ||
				 ((pRegion->isOwnedGroup(posRegion)) && (pParcel) && (gAgent.hasPowerInGroup(pParcel->getOwnerID(), GP_LAND_ADMIN))) )
			{
				fCanFreeze = true;
				++itAgent;
				continue;
			}
		}
		if (fFilter)
			itAgent = idAgents.erase(itAgent);
		else
			++itAgent;
	}
	return fCanFreeze;
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::landEject(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	landEjectMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::landEjectMultiple(const uuid_vec_t& idAgents)
{
	uuid_vec_t idEjectAgents(idAgents);
	if (!canLandFreezeOrEjectMultiple(idEjectAgents, true))
		return;

	LLSD args, payload; std::string strMsgName, strResidents; bool fBanEnabled = false;
	for (uuid_vec_t::const_iterator itAgent = idEjectAgents.begin(); itAgent != idEjectAgents.end(); ++itAgent)
	{
		const LLUUID& idAgent = *itAgent; LLVector3d posGlobal;
		if ( (!fBanEnabled) && (getPosGlobalFromAgentID(idAgent, posGlobal)) )
		{
			const LLParcel* pParcel = LLViewerParcelMgr::getInstance()->selectParcelAt(posGlobal)->getParcel();
			fBanEnabled = (pParcel) && (LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(pParcel, GP_LAND_MANAGE_BANNED));
		}

		if (idEjectAgents.begin() != itAgent)
			strResidents += "\n";
		strResidents += LLSLURL("agent", idAgent, "completename").getSLURLString();
		payload["ids"].append(*itAgent);
	}

	if (1 == payload["ids"].size())
	{
		args["AVATAR_NAME"] = strResidents;
		strMsgName = (fBanEnabled) ? "EjectAvatarFullname" : "EjectAvatarFullnameNoBan";
	}
	else
	{
		args["RESIDENTS"] = strResidents;
		strMsgName = (fBanEnabled) ? "EjectAvatarMultiple" : "EjectAvatarMultipleNoBan";
	}

	payload["ban_enabled"] = fBanEnabled;
	LLNotificationsUtil::add(strMsgName, args, payload, &callbackLandEject);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::callbackLandEject(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if (2 == idxOption)							// Cancel button.
		return false;

	bool fBanEnabled = notification["payload"]["ban_enabled"].asBoolean();
	if ( (0 == idxOption) || (fBanEnabled) )	// Eject button (or Eject + Ban)
	{
		// This is tricky. It is similar to say if it is not an 'Eject' button, and it is also not an 'Cancel' button, 
		// and ban_enabled==true, it should be the 'Eject and Ban' button.
		U32 flags = ( (0 != idxOption) && (fBanEnabled)	) ? 0x1 : 0x0;

		const LLSD& idAgents = notification["payload"]["ids"];
		for (LLSD::array_const_iterator itAgent = idAgents.beginArray(); itAgent != idAgents.endArray(); ++itAgent)
		{
			const LLUUID idAgent = itAgent->asUUID(); const LLViewerRegion* pAgentRegion = NULL;
			if (getRegionFromAgentID(idAgent, &pAgentRegion))
			{
				gMessageSystem->newMessage("EjectUser");
				gMessageSystem->nextBlock("AgentData");
				gMessageSystem->addUUID("AgentID", gAgent.getID());
				gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
				gMessageSystem->nextBlock("Data");
				gMessageSystem->addUUID("TargetID", idAgent);
				gMessageSystem->addU32("Flags", flags);
				gMessageSystem->sendReliable(pAgentRegion->getHost());
			}
		}
	}
	return false;
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::landFreeze(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	landFreezeMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::landFreezeMultiple(const uuid_vec_t& idAgents)
{
	uuid_vec_t idEjectAgents(idAgents);
	if (!canLandFreezeOrEjectMultiple(idEjectAgents, true))
		return;

	LLSD args, payload; std::string strMsgName, strResidents;
	for (uuid_vec_t::const_iterator itAgent = idEjectAgents.begin(); itAgent != idEjectAgents.end(); ++itAgent)
	{
		const LLUUID& idAgent = *itAgent;
		if (idEjectAgents.begin() != itAgent)
			strResidents += "\n";
		strResidents += LLSLURL("agent", idAgent, "completename").getSLURLString();
		payload["ids"].append(*itAgent);
	}

	if (1 == payload["ids"].size())
	{
		args["AVATAR_NAME"] = strResidents;
		strMsgName = "FreezeAvatarFullname";
	}
	else
	{
		args["RESIDENTS"] = strResidents;
		strMsgName = "FreezeAvatarMultiple";
	}

	LLNotificationsUtil::add(strMsgName, args, payload, &callbackLandFreeze);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::callbackLandFreeze(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( (0 == idxOption) || (1 == idxOption) )
	{
		U32 flags = (0 == idxOption) ? 0x0 : 0x1;

		const LLSD& idAgents = notification["payload"]["ids"];
		for (LLSD::array_const_iterator itAgent = idAgents.beginArray(); itAgent != idAgents.endArray(); ++itAgent)
		{
			const LLUUID idAgent = itAgent->asUUID(); const LLViewerRegion* pAgentRegion = NULL;
			if (getRegionFromAgentID(idAgent, &pAgentRegion))
			{
				gMessageSystem->newMessage("FreezeUser");
				gMessageSystem->nextBlock("AgentData");
				gMessageSystem->addUUID("AgentID", gAgent.getID());
				gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
				gMessageSystem->nextBlock("Data");
				gMessageSystem->addUUID("TargetID", idAgent);
				gMessageSystem->addU32("Flags", flags);
				gMessageSystem->sendReliable(pAgentRegion->getHost());
			}
		}
	}
	return false;
}

//
// Estate actions
//

typedef std::vector<std::string> strings_t;

// Copy/paste from LLPanelRegionInfo::sendEstateOwnerMessage
static void sendEstateOwnerMessage(const LLViewerRegion* pRegion, const std::string& request, const LLUUID& invoice, const strings_t& strings)
{
	if (pRegion)
	{
		LL_INFOS() << "Sending estate request '" << request << "'" << LL_ENDL;
		gMessageSystem->newMessage("EstateOwnerMessage");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
		gMessageSystem->nextBlock("MethodData");
		gMessageSystem->addString("Method", request);
		gMessageSystem->addUUID("Invoice", invoice);
		if(strings.empty())
		{
			gMessageSystem->nextBlock("ParamList");
			gMessageSystem->addString("Parameter", NULL);
		}
		else
		{
			strings_t::const_iterator it = strings.begin();
			strings_t::const_iterator end = strings.end();
			for(; it != end; ++it)
			{
				gMessageSystem->nextBlock("ParamList");
				gMessageSystem->addString("Parameter", *it);
			}
		}
		gMessageSystem->sendReliable(pRegion->getHost());
	}
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::canEstateKickOrTeleportHome(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	return canEstateKickOrTeleportHomeMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::canEstateKickOrTeleportHomeMultiple(uuid_vec_t& idAgents, bool fFilter /*=false*/)
{
	if (gAgent.isGodlikeWithoutAdminMenuFakery())
		return true;		// Gods can always kick

	uuid_vec_t::iterator itAgent = idAgents.begin(); bool fCanKick = false;
	while ( (itAgent != idAgents.end()) && ((fFilter) || (!fCanKick)) )
	{
		const LLViewerRegion* pRegion = NULL;
		if ( (getRegionFromAgentID(*itAgent, &pRegion)) && ((pRegion->getOwner() == gAgent.getID()) || (pRegion->isEstateManager())) )
		{
			fCanKick = true;
			++itAgent;		// Estate owners/managers can kick
			continue;
		}

		if (fFilter)
			itAgent = idAgents.erase(itAgent);
		else
			++itAgent;
	}
	return fCanKick;
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::estateKick(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	estateKickMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::estateKickMultiple(const uuid_vec_t& idAgents)
{
	uuid_vec_t idEjectAgents(idAgents);
	if (!canEstateKickOrTeleportHomeMultiple(idEjectAgents, true))
		return;

	LLSD args, payload; std::string strMsgName, strResidents;
	for (uuid_vec_t::const_iterator itAgent = idEjectAgents.begin(); itAgent != idEjectAgents.end(); ++itAgent)
	{
		const LLUUID& idAgent = *itAgent;
		if (idEjectAgents.begin() != itAgent)
			strResidents += "\n";
		strResidents += LLSLURL("agent", idAgent, "completename").getSLURLString();
		payload["ids"].append(*itAgent);
	}

	if (1 == payload["ids"].size())
	{
		args["EVIL_USER"] = strResidents;
		strMsgName = "EstateKickUser";
	}
	else
	{
		args["RESIDENTS"] = strResidents;
		strMsgName = "EstateKickMultiple";
	}

	LLNotificationsUtil::add(strMsgName, args, payload, &callbackEstateKick);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::callbackEstateKick(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == idxOption)
	{
		const LLSD& idAgents = notification["payload"]["ids"];
		for (LLSD::array_const_iterator itAgent = idAgents.beginArray(); itAgent != idAgents.endArray(); ++itAgent)
		{
			const LLViewerRegion* pRegion = NULL;
			if (getRegionFromAgentID(itAgent->asUUID(), &pRegion))
			{
				strings_t strings;
				strings.push_back(itAgent->asString());

				sendEstateOwnerMessage(pRegion, "kickestate", LLUUID::generateNewID(), strings);
			}
		}
	}
	return false;
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::estateTeleportHome(const LLUUID& idAgent)
{
	uuid_vec_t idAgents;
	idAgents.push_back(idAgent);
	estateTeleportHomeMultiple(idAgents);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
void LLAvatarActions::estateTeleportHomeMultiple(const uuid_vec_t& idAgents)
{
	uuid_vec_t idEjectAgents(idAgents);
	if (!canEstateKickOrTeleportHomeMultiple(idEjectAgents, true))
		return;

	LLSD args, payload; std::string strMsgName, strResidents;
	for (uuid_vec_t::const_iterator itAgent = idEjectAgents.begin(); itAgent != idEjectAgents.end(); ++itAgent)
	{
		const LLUUID& idAgent = *itAgent;
		if (idEjectAgents.begin() != itAgent)
			strResidents += "\n";
		strResidents += LLSLURL("agent", idAgent, "completename").getSLURLString();
		payload["ids"].append(*itAgent);
	}

	if (1 == payload["ids"].size())
	{
		args["AVATAR_NAME"] = strResidents;
		strMsgName = "EstateTeleportHomeUser";
	}
	else
	{
		args["RESIDENTS"] = strResidents;
		strMsgName = "EstateTeleportHomeMultiple";
	}

	LLNotificationsUtil::add(strMsgName, args, payload, &callbackEstateTeleportHome);
}

// static - Checked: 2010-12-03 (Catznip-2.4)
bool LLAvatarActions::callbackEstateTeleportHome(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == idxOption)
	{
		const LLSD& idAgents = notification["payload"]["ids"];
		for (LLSD::array_const_iterator itAgent = idAgents.beginArray(); itAgent != idAgents.endArray(); ++itAgent)
		{
			const LLViewerRegion* pRegion = NULL;
			if (getRegionFromAgentID(itAgent->asUUID(), &pRegion))
			{
				strings_t strings;
				strings.push_back(gAgent.getID().asString());
				strings.push_back(itAgent->asString());

				sendEstateOwnerMessage(pRegion, "teleporthomeuser", LLUUID::generateNewID(), strings);
			}
		}
	}
	return false;
}
// [/SL:KB]
