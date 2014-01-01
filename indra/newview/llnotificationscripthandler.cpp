/** 
 * @file llnotificationscripthandler.cpp
 * @brief Notification Handler Class for Simple Notifications and Notification Tips
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llagent.h"
#include "llnotificationhandler.h"
#include "lltoastnotifypanel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llnotificationmanager.h"
#include "llnotifications.h"
#include "llscriptfloater.h"
#include "llfacebookconnect.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLScriptHandler::LLScriptHandler()
:	LLSystemNotificationHandler("Notifications", "notify")
{
	// Getting a Channel for our notifications
	LLScreenChannel* channel = LLChannelManager::getInstance()->createNotificationChannel();
	if(channel)
	{
		channel->setControlHovering(true);
		mChannel = channel->getHandle();
	}
}

//--------------------------------------------------------------------------
LLScriptHandler::~LLScriptHandler()
{
}

//--------------------------------------------------------------------------
void LLScriptHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel.get()->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLScriptHandler::processNotification(const LLNotificationPtr& notification)
{
	if(mChannel.isDead())
	{
		return false;
	}

	// arrange a channel on a screen
	if(!mChannel.get()->getVisible())
	{
		initChannel();
	}
	
// [SL:KB] - Patch: Notification-Persisted | Checked: 2012-01-27 (Catznip-3.2)
	// Don't log persisted notifications a second time
	if (!notification->isPersisted())
	{
// [/SL:KB]
// [SL:KB] - Patch: Notification-Logging | Checked: 2012-01-29 (Catznip-3.2)
		if (LLHandlerUtil::canLogToChat(notification))
		{
			LLHandlerUtil::logToNearbyChat(notification, CHAT_SOURCE_SYSTEM);
		}

		if (LLHandlerUtil::canLogToIM(notification))
		{
			LLHandlerUtil::logToIMP2P(notification);
		}
// [/SL:KB]
// [SL:KB] - Patch: Notification-Persisted | Checked: 2012-01-27 (Catznip-3.2)
	}
// [/SL:KB]
//	if (notification->canLogToIM())
//	{
//		LLHandlerUtil::logToIMP2P(notification);
//	}

	if(notification->hasFormElements() && !notification->canShowToast())
	{
		LLScriptFloaterManager::getInstance()->onAddNotification(notification->getID());
	}
	else if (notification->canShowToast())
	{
		LLToastPanel* notify_box = LLToastPanel::buidPanelFromNotification(notification);

		LLToast::Params p;
		p.notif_id = notification->getID();
		p.notification = notification;
		p.panel = notify_box;
		p.on_delete_toast = boost::bind(&LLScriptHandler::onDeleteToast, this, _1);
		if(gAgent.isDoNotDisturb())
		{ 
			p.force_show = notification->getName() == "SystemMessage" 
							||	notification->getName() == "GodMessage" 
							|| notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH;
		}

// [SL:KB] - Patch: Notification-Misc | Checked: 2011-11-23 (Catznip-3.2.1) | Added: Catznip-3.2.0
		if ( ("ScriptQuestion" == notification->getName()) || ("ScriptQuestionCaution" == notification->getName()) )
		{
			p.lifetime_secs = gSavedSettings.getS32("ScriptQuestionToastLifeTime");
		}
// [/SL:KB]

		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel.get());
		if(channel)
		{
			channel->addToast(p);
		}
	}

	return false;
}


void LLScriptHandler::onDelete( LLNotificationPtr notification )
	{
	if(notification->hasFormElements() && !notification->canShowToast())
		{
			LLScriptFloaterManager::getInstance()->onRemoveNotification(notification->getID());
		}
		else
		{
			mChannel.get()->removeToastByNotificationID(notification->getID());
		}
	}


//--------------------------------------------------------------------------

void LLScriptHandler::onDeleteToast(LLToast* toast)
{
	// send a signal to a listener to let him perform some action
	// in this case listener is a SysWellWindow and it will remove a corresponding item from its list
	LLNotificationPtr notification = LLNotifications::getInstance()->find(toast->getNotificationID());
	
	if( notification && notification->hasFormElements() && !notification->canShowToast())
	{
		LLScriptFloaterManager::getInstance()->onRemoveNotification(notification->getID());
	}

}




