/** 
 * @file llchannelmanager.cpp
 * @brief This class rules screen notification channels.
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

#include "llchannelmanager.h"

#include "llappviewer.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llpersistentnotificationstorage.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llrootview.h"
#include "llsyswellwindow.h"
#include "llfloaternotificationstabbed.h"
// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
#include "lltoaststartuppanel.h"
// [/SL:KB]
#include "llfloaterreg.h"

#include <algorithm>

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLChannelManager::LLChannelManager()
{
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLChannelManager::onLoginCompleted, this));
	mChannelList.clear();
//	mStartUpChannel = NULL;

	if(!gViewerWindow)
	{
		LL_ERRS() << "LLChannelManager::LLChannelManager() - viwer window is not initialized yet" << LL_ENDL;
	}

	// We don't actually need this instance right now, but our
	// cleanupSingleton() method deletes LLScreenChannels, which need to
	// unregister from LLUI. Calling LLUI::instance() here establishes the
	// dependency so LLSingletonBase::deleteAll() calls our deleteSingleton()
	// before LLUI::deleteSingleton().
	LLUI::instance();
}

//--------------------------------------------------------------------------
LLChannelManager::~LLChannelManager()
{
}

//--------------------------------------------------------------------------
void LLChannelManager::cleanupSingleton()
{
    // Note: LLScreenChannelBase is a LLUICtrl and depends onto other singletions
    // not captured by singleton-dependency, so cleanup it here instead of destructor
    for (std::vector<ChannelElem>::iterator it = mChannelList.begin(); it != mChannelList.end(); ++it)
    {
        LLScreenChannelBase* channel = it->channel.get();
        if (!channel) continue;

        delete channel;
    }

    mChannelList.clear();
}

//--------------------------------------------------------------------------
LLScreenChannel* LLChannelManager::createNotificationChannel()
{
	//  creating params for a channel
	LLScreenChannelBase::Params p;
	p.id = LLUUID(gSavedSettings.getString("NotificationChannelUUID"));
	p.channel_align = CA_RIGHT;
// [SL:KB] - Patch: Notification-Misc | Checked: 2011-11-19 (Catznip-3.2)
	p.toast_align = (1 == gSavedSettings.getS32("NotificationToastAlignment")) ? NA_BOTTOM : NA_TOP;
// [/SL:KB]
//	p.toast_align = NA_TOP;

	// Getting a Channel for our notifications
	return dynamic_cast<LLScreenChannel*> (LLChannelManager::getInstance()->getChannel(p));
}

//--------------------------------------------------------------------------
void LLChannelManager::onLoginCompleted()
{
	S32 away_notifications = 0;

	// calc a number of all offline notifications
	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		LLScreenChannelBase* channel = it->channel.get();
		if (!channel) continue;

		// don't calc notifications for Nearby Chat
		if(channel->getChannelID() == LLUUID(gSavedSettings.getString("NearByChatChannelUUID")))
		{
			continue;
		}

		// don't calc notifications for channels that always show their notifications
		if(!channel->getDisplayToastsAlways())
		{
			away_notifications +=channel->getNumberOfHiddenToasts();
		}
	}

	away_notifications += gIMMgr->getNumberOfUnreadIM();

	if(!away_notifications)
	{
// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
		LLScreenChannel::setStartUpToastShown();
// [/SL:KB]
//		onStartUpToastClose();
	}
	else
	{
		// create a channel for the StartUp Toast
		LLScreenChannelBase::Params p;
		p.id = LLUUID(gSavedSettings.getString("StartUpChannelUUID"));
		p.channel_align = CA_RIGHT;
// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
		p.toast_align = NA_TOP;
		p.display_toasts_always = true;
		LLScreenChannel* channel_startup = createChannel<LLScreenChannelStartup>(p);
		if (!channel_startup)
		{
			LLScreenChannel::setStartUpToastShown();
		}
		else
		{
			mStartUpChannel = channel_startup->getHandle();

			// init channel's position and size
			S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
			S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
			channel_startup->init(channel_right_bound - channel_width, channel_right_bound);

			LLRect toast_rect;
			LLToast::Params p;
			p.panel = new LLToastStartupPanel();
			p.lifetime_secs = gSavedSettings.getS32("StartUpToastLifeTime");
			p.enable_hide_btn = false;
			p.can_be_stored = false;
			p.on_delete_toast = boost::bind(&LLScreenChannel::setStartUpToastShown);

			channel_startup->addToast(p);
		}
// [/SL:KB]
//		mStartUpChannel = createChannel(p);
//
//		if(!mStartUpChannel)
//		{
//			onStartUpToastClose();
//		}
//		else
//		{
//			gViewerWindow->getRootView()->addChild(mStartUpChannel);
//
//			// init channel's position and size
//			S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
//			S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
//			mStartUpChannel->init(channel_right_bound - channel_width, channel_right_bound);
//			mStartUpChannel->setMouseDownCallback(boost::bind(&LLFloaterNotificationsTabbed::onStartUpToastClick, LLFloaterNotificationsTabbed::getInstance(), _2, _3, _4));
//
//			mStartUpChannel->setCommitCallback(boost::bind(&LLChannelManager::onStartUpToastClose, this));
//			mStartUpChannel->createStartUpToast(away_notifications, gSavedSettings.getS32("StartUpToastLifeTime"));
//		}
	}

	LLPersistentNotificationStorage::getInstance()->loadNotifications();
	LLDoNotDisturbNotificationStorage::getInstance()->loadNotifications();
}

//--------------------------------------------------------------------------
//void LLChannelManager::onStartUpToastClose()
//{
//	if(mStartUpChannel)
//	{
//		mStartUpChannel->setVisible(FALSE);
//		mStartUpChannel->closeStartUpToast();
//		removeChannelByID(LLUUID(gSavedSettings.getString("StartUpChannelUUID")));
//		mStartUpChannel = NULL;
//	}
//
//	// set StartUp Toast Flag to allow all other channels to show incoming toasts
//	LLScreenChannel::setStartUpToastShown();
//}

//--------------------------------------------------------------------------

LLScreenChannelBase*	LLChannelManager::addChannel(LLScreenChannelBase* channel)
{
	if(!channel)
		return 0;

	ChannelElem new_elem;
	new_elem.id = channel->getChannelID();
	new_elem.channel = channel->getHandle();

	mChannelList.push_back(new_elem); 

	return channel;
}

//LLScreenChannel* LLChannelManager::createChannel(LLScreenChannelBase::Params& p)
// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
template <class T> T* LLChannelManager::createChannel(LLScreenChannelBase::Params& p)
// [/SL:KB]
{
//	LLScreenChannel* new_channel = new LLScreenChannel(p); 
// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
	T* new_channel = new T(p); 
// [/SL:KB]

	addChannel(new_channel);
	return new_channel;
}

LLScreenChannelBase* LLChannelManager::getChannel(LLScreenChannelBase::Params& p)
{
	LLScreenChannelBase* new_channel = findChannelByID(p.id);

	if(new_channel)
		return new_channel;

// [SL:KB] - Patch: Chat-ScreenChannelStartup | Checked: 2013-08-24 (Catznip-3.6)
	return createChannel<LLScreenChannel>(p);
// [/SL:KB]
//	return createChannel(p);
}

//--------------------------------------------------------------------------
LLScreenChannelBase* LLChannelManager::findChannelByID(const LLUUID& id)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), id); 
	if(it != mChannelList.end())
	{
		return (*it).channel.get();
	}

	return NULL;
}

//--------------------------------------------------------------------------
void LLChannelManager::removeChannelByID(const LLUUID& id)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), id); 
	if(it != mChannelList.end())
	{
		mChannelList.erase(it);
	}
}

//--------------------------------------------------------------------------
void LLChannelManager::muteAllChannels(bool mute)
{
	for (std::vector<ChannelElem>::iterator it = mChannelList.begin();
			it != mChannelList.end(); it++)
	{
		if (it->channel.get())
		{
			it->channel.get()->setShowToasts(!mute);
		}
	}
}

void LLChannelManager::killToastsFromChannel(const LLUUID& channel_id, const LLScreenChannel::Matcher& matcher)
{
	LLScreenChannel
			* screen_channel =
					dynamic_cast<LLScreenChannel*> (findChannelByID(channel_id));
	if (screen_channel != NULL)
	{
		screen_channel->killMatchedToasts(matcher);
	}
}

// static
LLNotificationsUI::LLScreenChannel* LLChannelManager::getNotificationScreenChannel()
{
	LLNotificationsUI::LLScreenChannel* channel = static_cast<LLNotificationsUI::LLScreenChannel*>
	(LLNotificationsUI::LLChannelManager::getInstance()->
										findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	if (channel == NULL)
	{
		LL_WARNS() << "Can't find screen channel by NotificationChannelUUID" << LL_ENDL;
		llassert(!"Can't find screen channel by NotificationChannelUUID");
	}

	return channel;
}

