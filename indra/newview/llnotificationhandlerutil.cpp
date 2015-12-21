/**
 * @file llnotificationofferhandler.cpp
 * @brief Provides set of utility methods for notifications processing.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llavatarnamecache.h"

#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llurlaction.h"

#include "llagent.h"
#include "llfloaterimsession.h"
// [SL:KB] - Patch: Chat-Logs | Checked: 2010-11-18 (Catznip-2.4)
#include "llavatarnamecache.h"
// [/SL:KB]
#include "llimview.h"
#include "llfloaterimnearbychat.h"
#include "llnotificationhandler.h"

using namespace LLNotificationsUI;

LLNotificationHandler::LLNotificationHandler(const std::string& name, const std::string& notification_type, const std::string& parentName)
:	LLNotificationChannel(name, parentName, LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, notification_type))
{}

LLSystemNotificationHandler::LLSystemNotificationHandler(const std::string& name, const std::string& notification_type)
	: LLNotificationHandler(name, notification_type, "System")
{}

LLCommunicationNotificationHandler::LLCommunicationNotificationHandler(const std::string& name, const std::string& notification_type)
	: LLNotificationHandler(name, notification_type, "Communication")
{}

// [SL:KB] - Patch: Notification-Logging | Checked: 2014-01-18 (Catznip-3.6)
static LLFloaterIMSession* getIMFloaterFromNotification(const LLNotificationPtr& notification)
{
	const LLUUID idFrom = notification->getPayload()["from_id"];
	return (idFrom.notNull()) ? LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, idFrom)) : NULL;
}

// static
bool LLHandlerUtil::hasIMFloater(const LLNotificationPtr& notification)
{
	// NOTE: simply checks for the existance of an IM floater and nothing else
	return (getIMFloaterFromNotification(notification) != NULL);
}

// static
bool LLHandlerUtil::isIMFloaterVisible(const LLNotificationPtr& notification)
{
	// NOTE: checks whether the IM floater is currently visible on the screen (LLFloaterIMSession has a custom getVisible() override)
	LLFloaterIMSession* pIMFloater = getIMFloaterFromNotification(notification);
	return (pIMFloater) && (pIMFloater->getVisible());
}
// [/SL:KB]
//// static
//bool LLHandlerUtil::isIMFloaterOpened(const LLNotificationPtr& notification)
//{
//	bool res = false;
//
//	LLUUID from_id = notification->getPayload()["from_id"];
//	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);
//	LLFloaterIMSession* im_floater = LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", session_id);
//
//	if (im_floater != NULL)
//	{
//		res = im_floater->getVisible() == TRUE;
//	}
//
//	return res;
//}

// [SL:KB] - Patch: Notification-Logging | Checked: 2013-10-14 (Catznip-3.6)
// static
bool LLHandlerUtil::canLogToChat(const LLNotificationPtr& notification)
{
	return notification->canLogToNearbyChat();
}

// static
bool LLHandlerUtil::canLogToIM(const LLNotificationPtr& notification)
{
	bool fOpenSession = (getIMFloaterFromNotification(notification) != NULL);
	return notification->canLogToIM(fOpenSession);
}
// [/SL:KB]

// static
//void LLHandlerUtil::logToIM(const EInstantMessage& session_type,
//		const std::string& session_name, const std::string& from_name,
//		const std::string& message, const LLUUID& session_owner_id,
//		const LLUUID& from_id)
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
void LLHandlerUtil::logToIM(const LLUUID& session_id, const std::string& file_name, const std::string& from_name, const LLUUID& from_id, const std::string& raw_message, const LLSD& substitutions)
// [/SL:KB]
{
	std::string from = from_name;
	if (from_name.empty())
	{
		from = SYSTEM_FROM;
	}

// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
	// Replace interactive system message marker with correct from string value
	if (INTERACTIVE_SYSTEM_FROM == from_name)
	{
		from = SYSTEM_FROM;
	}

	const std::string message = LLNotification::getMessage(raw_message, substitutions);
	const std::string log_message = LLNotification::getLogMessage(raw_message, substitutions);
// [/SL:KB]

//	LLUUID session_id = LLIMMgr::computeSessionID(session_type,
//			session_owner_id);
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
//		// replace interactive system message marker with correct from string value
//		if (INTERACTIVE_SYSTEM_FROM == from_name)
//		{
//			from = SYSTEM_FROM;
//		}

		// Build a new format username or firstname_lastname for legacy names
		// to use it for a history log filename.
// [SL:KB] - Patch: Chat-Logs | Checked: 2010-11-18 (Catznip-2.4)
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
		LLIMModel::instance().logToFile(file_name, from, from_id, log_message);
// [/SL:KB]
//		LLIMModel::instance().logToFile(file_name, from, from_id, message);
// [/SL:KB]
//		std::string user_name = LLCacheName::buildUsername(session_name);
//		LLIMModel::instance().logToFile(user_name, from, from_id, message);

	}
	else
	{
		S32 unread = session->mNumUnread;
		S32 participant_unread = session->mParticipantUnreadMessageCount;
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
		LLIMModel::instance().addMessageSilently(session_id, from, from_id, message, LLLogChat::timestamp(false), false);
		LLIMModel::instance().logToFile(LLIMModel::instance().getHistoryFileName(session_id), from, from_id, log_message);
// [/SL:KB]
//		LLIMModel::instance().addMessageSilently(session_id, from, from_id,
//				message);
		// we shouldn't increment counters when logging, so restore them
		session->mNumUnread = unread;
		session->mParticipantUnreadMessageCount = participant_unread;

		// update IM floater messages
		updateIMFLoaterMesages(session_id);
	}
}

// [SL:KB] - Patch: Chat-Logs | Checked: 2010-11-18 (Catznip-2.4)
void log_name_callback(const LLUUID& agent_id, const LLAvatarName& av_name, const std::string& from_name, const std::string& raw_message, const LLSD& substitutions, const LLUUID& from_id)
{
	std::string strFilename;
	if (LLLogChat::buildIMP2PLogFilename(agent_id, av_name.getCompleteName(), strFilename))
	{
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
		const LLUUID idSession = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);
		LLHandlerUtil::logToIM(idSession, av_name.getLegacyName(), from_name, LLUUID(), raw_message, substitutions);
// [/SL:KB]
//		LLHandlerUtil::logToIM(IM_NOTHING_SPECIAL, strFilename, from_name, message, from_id, LLUUID());
	}
}
// [/SL:KB]
//void log_name_callback(const std::string& full_name, const std::string& from_name, 
//					   const std::string& message, const LLUUID& from_id)
//
//{
//	LLHandlerUtil::logToIM(IM_NOTHING_SPECIAL, full_name, from_name, message,
//					from_id, LLUUID());
//}


// static
void LLHandlerUtil::logToIMP2P(const LLNotificationPtr& notification, bool to_file_only)
{
	if (!gCacheName)
	{
		return;
	}

	LLUUID from_id = notification->getPayload()["from_id"];

// [SL:KB] - Patch: Notification-Logging | Checked: 2012-01-27 (Catznip-3.2)
	// If the user triggered the notification log it to the destination instead
	if ( (gAgentID == from_id) && (notification->getPayload().has("dest_id")) )
	{
		from_id = notification->getPayload()["dest_id"];
	}
// [/SL:KB]

	if (from_id.isNull())
	{
		// Normal behavior for system generated messages, don't spam.
		// LL_WARNS() << " from_id for notification " << notification->getName() << " is null " << LL_ENDL;
		return;
	}

	if(to_file_only)
	{
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
// [SL:KB] - Patch: Chat-Logs | Checked: 2010-11-18 (Catznip-2.4)
		LLAvatarNameCache::get(from_id, boost::bind(&log_name_callback, _1, _2, "", notification->getRawMessage(), notification->getSubstitutions(), LLUUID()));
// [/SL:KB]
//		gCacheName->get(from_id, false, boost::bind(&log_name_callback, _2, "", notification->getRawMessage(), notification->getSubstitutions(), LLUUID()));
// [/SL:KB]
//		gCacheName->get(from_id, false, boost::bind(&log_name_callback, _2, "", notification->getMessage(), LLUUID()));
	}
	else
	{
// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
// [SL:KB] - Patch: Chat-Logs | Checked: 2010-11-18 (Catznip-2.4)
		LLAvatarNameCache::get(from_id, boost::bind(&log_name_callback, _1, _2, INTERACTIVE_SYSTEM_FROM, notification->getRawMessage(), notification->getSubstitutions(), from_id));
// [/SL:KB]
//		gCacheName->get(from_id, false, boost::bind(&log_name_callback, _2, INTERACTIVE_SYSTEM_FROM, notification->getRawMessage(), notification->getSubstitutions(), from_id));
// [/SL:KB]
//		gCacheName->get(from_id, false, boost::bind(&log_name_callback, _2, INTERACTIVE_SYSTEM_FROM, notification->getMessage(), from_id));
	}
}

// static
void LLHandlerUtil::logGroupNoticeToIMGroup(
		const LLNotificationPtr& notification)
{

	const LLSD& payload = notification->getPayload();
	LLGroupData groupData;
	if (!gAgent.getGroupData(payload["group_id"].asUUID(), groupData))
	{
		LL_WARNS()
						<< "Group notice for unknown group: "
								<< payload["group_id"].asUUID() << LL_ENDL;
		return;
	}

	const std::string group_name = groupData.mName;
	const std::string sender_name = payload["sender_name"].asString();

	// we can't retrieve sender id from group notice system message, so try to lookup it from cache
	LLUUID sender_id;
	gCacheName->getUUID(sender_name, sender_id);

// [SL:KB] - Patch: Notifications-Logging | Checked: 2014-01-18 (Catznip-3.6)
	const LLUUID idSession = LLIMMgr::computeSessionID(IM_SESSION_GROUP_START, payload["group_id"]);
	logToIM(idSession, group_name, sender_name, sender_id, payload["message"], LLSD());
// [/SL:KB]
//	logToIM(IM_SESSION_GROUP_START, group_name, sender_name, payload["message"],
//			payload["group_id"], sender_id);
}

// static
void LLHandlerUtil::logToNearbyChat(const LLNotificationPtr& notification, EChatSourceType type)
{
    LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
		LLChat chat_msg(notification->getMessage());
		chat_msg.mSourceType = type;
		chat_msg.mFromName = SYSTEM_FROM;
		chat_msg.mFromID = LLUUID::null;
// [SL:KB] - Patch: Notification-Logging | Checked: 2012-07-03 (Catznip-3.3)
		nearby_chat->addMessage(chat_msg, true, LLSD().with("do_not_log", true));

		chat_msg.mText = notification->getLogMessage();
		nearby_chat->logMessage(chat_msg);
// [/SL:KB]
//		nearby_chat->addMessage(chat_msg);
	}
}

// static
LLUUID LLHandlerUtil::spawnIMSession(const std::string& name, const LLUUID& from_id)
{
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);

	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
		session_id = LLIMMgr::instance().addSession(name, IM_NOTHING_SPECIAL, from_id);
	}

	return session_id;
}

// static
std::string LLHandlerUtil::getSubstitutionName(const LLNotificationPtr& notification)
{
	std::string res = notification->getSubstitutions().has("NAME")
		? notification->getSubstitutions()["NAME"]
		: notification->getSubstitutions()["[NAME]"];
	if (res.empty())
	{
		LLUUID from_id = notification->getPayload()["FROM_ID"];

		//*TODO all keys everywhere should be made of the same case, there is a mix of keys in lower and upper cases
		if (from_id.isNull()) 
		{
			from_id = notification->getPayload()["from_id"];
		}
		if(!gCacheName->getFullName(from_id, res))
		{
			res = "";
		}
	}
	return res;
}

// static
void LLHandlerUtil::addNotifPanelToIM(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
	LLUUID from_id = notification->getPayload()["from_id"];

	LLUUID session_id = spawnIMSession(name, from_id);
	// add offer to session
	LLIMModel::LLIMSession * session = LLIMModel::getInstance()->findIMSession(
			session_id);
	llassert_always(session != NULL);

	LLSD offer;
	offer["notification_id"] = notification->getID();
	offer["from"] = SYSTEM_FROM;
	offer["time"] = LLLogChat::timestamp(false);
	offer["index"] = (LLSD::Integer)session->mMsgs.size();
	session->mMsgs.push_front(offer);


	// update IM floater and counters
	LLSD arg;
	arg["session_id"] = session_id;
// [SL:KB] - Patch: Chat-Alerts | Checked: 2012-09-18 (Catznip-3.3)
	arg["notification_id"] = notification->getID();
// [/SL:KB]
	arg["num_unread"] = ++(session->mNumUnread);
	arg["participant_unread"] = ++(session->mParticipantUnreadMessageCount);
	LLIMModel::getInstance()->mNewMsgSignal(arg);
}

// static
void LLHandlerUtil::updateIMFLoaterMesages(const LLUUID& session_id)
{
	LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
	if (im_floater != NULL && im_floater->getVisible())
	{
		im_floater->updateMessages();
	}
}

//// static
//void LLHandlerUtil::updateVisibleIMFLoaterMesages(const LLNotificationPtr& notification)
//{
//	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
//	LLUUID from_id = notification->getPayload()["from_id"];
//	LLUUID session_id = spawnIMSession(name, from_id);
//
//	updateIMFLoaterMesages(session_id);
//}

// static
void LLHandlerUtil::decIMMesageCounter(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
	LLUUID from_id = notification->getPayload()["from_id"];
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);

	LLIMModel::LLIMSession * session = LLIMModel::getInstance()->findIMSession(session_id);

	if (session)
	{
	LLSD arg;
	arg["session_id"] = session_id;
	session->mNumUnread--;
	arg["num_unread"] = session->mNumUnread;
	session->mParticipantUnreadMessageCount--;
	arg["participant_unread"] = session->mParticipantUnreadMessageCount;
	LLIMModel::getInstance()->mNewMsgSignal(arg);
}
}

