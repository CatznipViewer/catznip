/** 
 * @file llfloaternotificationstabbed.cpp
 * @brief                                  
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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
#include "llfloaternotificationstabbed.h"

#include "llchiclet.h"
#include "llchicletbar.h"
#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llscriptfloater.h"
#include "llspeakers.h"
#include "lltoastpanel.h"
#include "lltoastnotifypanel.h"

// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
#include "llfloaternotificationsflat.h"

LLNotificationDateComparator NOTIF_DATE_COMPARATOR;
// [/SL:KB]

//---------------------------------------------------------------------------------
LLFloaterNotifications::LLFloaterNotifications(const LLSD& key) : LLTransientDockableFloater(NULL, true,  key),
//    mChannel(NULL),
    mSysWellChiclet(NULL),
//    mGroupInviteMessageList(NULL),
//    mGroupNoticeMessageList(NULL),
//    mTransactionMessageList(NULL),
//    mSystemMessageList(NULL),
//    mNotificationsSeparator(NULL),
//    mNotificationsTabContainer(NULL),
    NOTIFICATION_TABBED_ANCHOR_NAME("notification_well_panel"),
    IM_WELL_ANCHOR_NAME("im_well_panel"),
    mIsReshapedByUser(false)

{
    setOverlapsScreenChannel(true);
    mNotificationUpdates.reset(new NotificationChannel(this));
//    mNotificationsSeparator = new LLNotificationSeparator();
}

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
LLFloaterNotificationsTabbed::LLFloaterNotificationsTabbed(const LLSD& key) : LLFloaterNotifications(key),
    mGroupInviteMessageList(NULL),
    mGroupNoticeMessageList(NULL),
    mTransactionMessageList(NULL),
    mSystemMessageList(NULL),
    mNotificationsSeparator(NULL),
    mNotificationsTabContainer(NULL)
{
    mNotificationsSeparator = new LLNotificationSeparator();
}
// [/SL:KB]

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
BOOL LLFloaterNotificationsTabbed::postBuild()
{
	mGroupInviteMessageList = getChild<LLNotificationListView>("group_invite_notification_list");
	mGroupInviteMessageList->setComparator(&NOTIF_DATE_COMPARATOR);
	mGroupNoticeMessageList = getChild<LLNotificationListView>("group_notice_notification_list");
	mGroupNoticeMessageList->setComparator(&NOTIF_DATE_COMPARATOR);
	mTransactionMessageList = getChild<LLNotificationListView>("transaction_notification_list");
	mTransactionMessageList->setComparator(&NOTIF_DATE_COMPARATOR);
	mSystemMessageList = getChild<LLNotificationListView>("system_notification_list");
	mSystemMessageList->setComparator(&NOTIF_DATE_COMPARATOR);
	mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupInviteTypes(), mGroupInviteMessageList);
	mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupNoticeTypes(), mGroupNoticeMessageList);
	mNotificationsSeparator->initTaggedList(LLNotificationListItem::getTransactionTypes(), mTransactionMessageList);
	mNotificationsSeparator->initUnTaggedList(mSystemMessageList);
	mNotificationsTabContainer = getChild<LLTabContainer>("notifications_tab_container");
	
	return LLFloaterNotifications::postBuild();
}
// [/SL:KB]

//---------------------------------------------------------------------------------
BOOL LLFloaterNotifications::postBuild()
{
//    mGroupInviteMessageList = getChild<LLNotificationListView>("group_invite_notification_list");
//    mGroupNoticeMessageList = getChild<LLNotificationListView>("group_notice_notification_list");
//    mTransactionMessageList = getChild<LLNotificationListView>("transaction_notification_list");
//    mSystemMessageList = getChild<LLNotificationListView>("system_notification_list");
//    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupInviteTypes(), mGroupInviteMessageList);
//    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupNoticeTypes(), mGroupNoticeMessageList);
//    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getTransactionTypes(), mTransactionMessageList);
//    mNotificationsSeparator->initUnTaggedList(mSystemMessageList);
//    mNotificationsTabContainer = getChild<LLTabContainer>("notifications_tab_container");

    mDeleteAllBtn = getChild<LLButton>("delete_all_button");
    mDeleteAllBtn->setClickedCallback(boost::bind(&LLFloaterNotifications::onClickDeleteAllBtn,this));

    mCollapseAllBtn = getChild<LLButton>("collapse_all_button");
    mCollapseAllBtn->setClickedCallback(boost::bind(&LLFloaterNotifications::onClickCollapseAllBtn,this));

    // get a corresponding channel
    initChannel();
    BOOL rv = LLTransientDockableFloater::postBuild();
    
    setTitle(getString("title_notification_tabbed_window"));
    return rv;
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::setMinimized(BOOL minimize)
{
    LLTransientDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::handleReshape(const LLRect& rect, bool by_user)
{
    mIsReshapedByUser |= by_user; // mark floater that it is reshaped by user
    LLTransientDockableFloater::handleReshape(rect, by_user);
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onStartUpToastClick(S32 x, S32 y, MASK mask)
{
    // just set floater visible. Screen channels will be cleared.
    setVisible(TRUE);
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::setSysWellChiclet(LLSysWellChiclet* chiclet) 
{ 
    mSysWellChiclet = chiclet;
    if(NULL != mSysWellChiclet)
    {
        mSysWellChiclet->updateWidget(isWindowEmpty());
    }
}

//---------------------------------------------------------------------------------
LLFloaterNotifications::~LLFloaterNotifications()
{
}

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
LLFloaterNotificationsTabbed::~LLFloaterNotificationsTabbed()
{
}
// [/SL:KB]

//---------------------------------------------------------------------------------
void LLFloaterNotifications::removeItemByID(const LLUUID& id, std::string type)
{
//    if(mNotificationsSeparator->removeItemByID(type, id))
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	if (removeNotificationByID(id, type))
// [/SL:KB]
    {
        if (NULL != mSysWellChiclet)
        {
            mSysWellChiclet->updateWidget(isWindowEmpty());
        }
        reshapeWindow();
        updateNotificationCounters();
    }
    else
    {
        LL_WARNS() << "Unable to remove notification from the list, ID: " << id
            << LL_ENDL;
    }

    // hide chiclet window if there are no items left
    if(isWindowEmpty())
    {
        setVisible(FALSE);
    }
}

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLFloaterNotificationsTabbed::removeNotificationByID(const LLUUID& id, const std::string& type)
{
	return mNotificationsSeparator->removeItemByID(type, id);
}

LLPanel * LLFloaterNotificationsTabbed::findNotificationByID(const LLUUID& id, const std::string& type)
{
    return mNotificationsSeparator->findItemByID(type, id);
}
// [/SL:KB]

//---------------------------------------------------------------------------------
LLPanel * LLFloaterNotifications::findItemByID(const LLUUID& id, std::string type)
{
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	return findNotificationByID(id, type);
// [/SL:KB]
//    return mNotificationsSeparator->findItemByID(type, id);
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::initChannel() 
{
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = 
		dynamic_cast<LLNotificationsUI::LLScreenChannel*>(LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
    if(NULL == channel)
    {
        LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
    }

    if (channel)
    {
        channel->addOnStoreToastCallback(boost::bind(&LLFloaterNotificationsTabbed::onStoreToast, this, _1, _2));
	    mChannel = channel->getHandle();
    }
// [/SL:KB]
//    LLNotificationsUI::LLScreenChannelBase* channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(
//        LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
//    mChannel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(channel);
//    if(NULL == mChannel)
//    {
//        LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
//    }
//
//    if(mChannel)
//    {
//        mChannel->addOnStoreToastCallback(boost::bind(&LLFloaterNotificationsTabbed::onStoreToast, this, _1, _2));
//    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::setVisible(BOOL visible)
{
    if (visible)
    {
        // when Notification channel is cleared, storable toasts will be added into the list.
        clearScreenChannels();
    }
    if (visible)
    {
        if (NULL == getDockControl() && getDockTongue().notNull())
        {
// [SL:KB] - Patch: Chat-ChicletBarAligment | Checked: 2011-11-19 (Catznip-3.2)
			setDockControl(new LLDockControl(LLChicletBar::getInstance()->getChild<LLView>(getAnchorViewName()), this, getDockTongue(),
				(LLChicletBar::ALIGN_TOP == LLChicletBar::getInstance()->getAlignment()) ? LLDockControl::BOTTOM : LLDockControl::TOP));
// [/SL:KB]
//			setDockControl(new LLDockControl(
//				LLChicletBar::getInstance()->getChild<LLView>(getAnchorViewName()), this,
//				getDockTongue(), LLDockControl::BOTTOM));
        }
    }

    // do not show empty window
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	if (isWindowEmpty())
		visible = FALSE;
// [/SL:KB]
//    if (NULL == mNotificationsSeparator || isWindowEmpty()) visible = FALSE;

    LLTransientDockableFloater::setVisible(visible);

    // update notification channel state	
    initChannel(); // make sure the channel still exists
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2015-12-07 (Catznip-3.8)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}
// [/SL:KB]
//    if(mChannel)
//    {
//        mChannel->updateShowToastsState();
//        mChannel->redrawToasts();
//    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::setDocked(bool docked, bool pop_on_undock)
{
    LLTransientDockableFloater::setDocked(docked, pop_on_undock);

    // update notification channel state
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2015-12-07 (Catznip-3.8)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}
// [/SL:KB]
//    if(mChannel)
//    {
//        mChannel->updateShowToastsState();
//        mChannel->redrawToasts();
//    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::reshapeWindow()
{
    // update notification channel state
    // update on a window reshape is important only when a window is visible and docked
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2015-12-07 (Catznip-3.8)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
    if (channel && getVisible() && isDocked())
    {
        channel->updateShowToastsState();
    }
// [/SL:KB]
//    if(mChannel && getVisible() && isDocked())
//    {
//        mChannel->updateShowToastsState();
//    }
}

//---------------------------------------------------------------------------------
//bool LLFloaterNotificationsTabbed::isWindowEmpty()
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLFloaterNotificationsTabbed::isWindowEmpty() const
// [/SL:KB]
{
    return mNotificationsSeparator->size() == 0;
}

//---------------------------------------------------------------------------------
LLFloaterNotifications::NotificationChannel::NotificationChannel(LLFloaterNotifications* notifications_window)
    : LLNotificationChannel(LLNotificationChannel::Params().name(notifications_window->getPathname())),
    mNotificationsWindow(notifications_window)
{
    connectToChannel("Notifications");
    connectToChannel("Group Notifications");
    connectToChannel("Offer");
}

// static
//---------------------------------------------------------------------------------
LLFloaterNotifications* LLFloaterNotifications::getInstance(const LLSD& key /*= LLSD()*/)
{
    return LLFloaterReg::getTypedInstance<LLFloaterNotifications>("notification_well_window", key);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::updateNotificationCounter(S32 panelIndex, S32 counterValue, std::string stringName)
{
    LLStringUtil::format_map_t string_args;
    string_args["[COUNT]"] = llformat("%d", counterValue);
    std::string label = getString(stringName, string_args);
    mNotificationsTabContainer->setPanelTitle(panelIndex, label);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::updateNotificationCounters()
{
    updateNotificationCounter(0, mSystemMessageList->size(), "system_tab_title");
    updateNotificationCounter(1, mTransactionMessageList->size(), "transactions_tab_title");
    updateNotificationCounter(2, mGroupInviteMessageList->size(), "group_invitations_tab_title");
    updateNotificationCounter(3, mGroupNoticeMessageList->size(), "group_notices_tab_title");
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::addItem(LLNotificationListItem::Params p)
{
    // do not add clones
//    if (mNotificationsSeparator->findItemByID(p.notification_name, p.notification_id))
//        return;
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	if (findNotificationByID(p.notification_id, p.notification_name))
		return;
// [/SL:KB]
    LLNotificationListItem* new_item = LLNotificationListItem::create(p);
    if (new_item == NULL)
    {
        return;
    }
//    if (mNotificationsSeparator->addItem(new_item->getNotificationName(), new_item))
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	if (addNotification(new_item))
// [/SL:KB]
    {
        mSysWellChiclet->updateWidget(isWindowEmpty());
        reshapeWindow();
        updateNotificationCounters();
        new_item->setOnItemCloseCallback(boost::bind(&LLFloaterNotifications::onItemClose, this, _1));
        new_item->setOnItemClickCallback(boost::bind(&LLFloaterNotifications::onItemClick, this, _1));
    }
    else
    {
        LL_WARNS() << "Unable to add Notification into the list, notification ID: " << p.notification_id
            << ", title: " << new_item->getTitle()
            << LL_ENDL;

        new_item->die();
    }
}

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLFloaterNotificationsTabbed::addNotification(LLNotificationListItem* item)
{
	return mNotificationsSeparator->addItem(item->getNotificationName(), item);
}

void LLFloaterNotificationsTabbed::getNotifications(std::vector<LLNotificationListItem*>& items)
{
	mNotificationsSeparator->getItems(items);
}
// [/SL:KB]

//---------------------------------------------------------------------------------
void LLFloaterNotifications::closeAll()
{
    // Need to clear notification channel, to add storable toasts into the list.
    clearScreenChannels();

    std::vector<LLNotificationListItem*> items;
//    mNotificationsSeparator->getItems(items);
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	getNotifications(items);
// [/SL:KB]
    std::vector<LLNotificationListItem*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        onItemClose(*iter);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::getAllItemsOnCurrentTab(std::vector<LLPanel*>& items) const
{
    switch (mNotificationsTabContainer->getCurrentPanelIndex())
    {
    case 0:
        mSystemMessageList->getItems(items);
        break;
    case 1:
        mTransactionMessageList->getItems(items);
        break;
    case 2:
        mGroupInviteMessageList->getItems(items);
        break;
    case 3:
        mGroupNoticeMessageList->getItems(items);
        break;
    default:
        break;
    }
}

//---------------------------------------------------------------------------------
//void LLFloaterNotificationsTabbed::closeAllOnCurrentTab()
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
void LLFloaterNotificationsTabbed::closeVisibleNotifications()
// [/SL:KB]
{
    // Need to clear notification channel, to add storable toasts into the list.
    clearScreenChannels();
    std::vector<LLPanel*> items;
    getAllItemsOnCurrentTab(items);
    std::vector<LLPanel*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*iter);
        if (notify_item)
            onItemClose(notify_item);
    }
}

//---------------------------------------------------------------------------------
//void LLFloaterNotificationsTabbed::collapseAllOnCurrentTab()
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
void LLFloaterNotificationsTabbed::collapseVisibleNotifications()
// [/SL:KB]
{
    std::vector<LLPanel*> items;
    getAllItemsOnCurrentTab(items);
    std::vector<LLPanel*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*iter);
        if (notify_item)
            notify_item->setExpanded(FALSE);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::clearScreenChannels()
{
    // 1 - remove StartUp toast and channel if present
    if(!LLNotificationsUI::LLScreenChannel::getStartUpToastShown())
    {
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
		LLNotificationsUI::LLScreenChannelBase* pChannel = LLNotificationsUI::LLChannelManager::getInstance()->getStartUpChannel();
		if (pChannel)
		{
			pChannel->removeToastsFromChannel();
			pChannel->setVisible(FALSE);
		}
		LLNotificationsUI::LLScreenChannel::setStartUpToastShown();
// [/SL:KB]
//        LLNotificationsUI::LLChannelManager::getInstance()->onStartUpToastClose();
    }

    // 2 - remove toasts in Notification channel
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->removeAndStoreAllStorableToasts();
	}
// [/SL:KB]
//    if(mChannel)
//    {
//        mChannel->removeAndStoreAllStorableToasts();
//    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onStoreToast(LLPanel* info_panel, LLUUID id)
{
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2015-12-07 (Catznip-3.8)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (!channel)
	{
		return;
	}
// [/SL:KB]

	LLNotificationListItem::Params p;	
    p.notification_id = id;
    p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2015-12-07 (Catznip-3.8)
    LLNotificationPtr notify = channel->getToastByNotificationID(id)->getNotification();
// [/SL:KB]
//    LLNotificationPtr notify = mChannel->getToastByNotificationID(id)->getNotification();
    LLSD payload = notify->getPayload();
    p.notification_name = notify->getName();
    p.transaction_id = payload["transaction_id"];
    p.group_id = payload["group_id"];
    p.fee =  payload["fee"];
    p.subject = payload["subject"].asString();
    p.message = payload["message"].asString();
    p.sender = payload["sender_name"].asString();
    p.time_stamp = notify->getDate();
    p.received_time = payload["received_time"].asDate();
    p.paid_from_id = payload["from_id"];
    p.paid_to_id = payload["dest_id"];
    p.inventory_offer = payload["inventory_offer"];
    p.notification_priority = notify->getPriority();
    addItem(p);
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onItemClick(LLNotificationListItem* item)
{
    LLUUID id = item->getID();
    if (item->showPopup())
    {
        LLFloaterReg::showInstance("inspect_toast", id);
    }
    else
    {
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-02 (Catznip-4.0)
		item->setExpanded(!item->isExpanded());
// [/SL:KB]
//        item->setExpanded(TRUE);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onItemClose(LLNotificationListItem* item)
{
    LLUUID id = item->getID();

//    if(mChannel)
//    {
//        // removeItemByID() is invoked from killToastByNotificationID() and item will removed;
//        mChannel->killToastByNotificationID(id);
//    }
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if(channel)
	{
		// removeItemByID() is invoked from killToastByNotificationID() and item will removed;
		channel->killToastByNotificationID(id);
	}
// [/SL:KB]
    else
    {
        // removeItemByID() should be called one time for each item to remove it from notification well
        removeItemByID(id, item->getNotificationName());
    }

}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onAdd( LLNotificationPtr notify )
{
    removeItemByID(notify->getID(), notify->getName());
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onClickDeleteAllBtn()
{
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	closeVisibleNotifications();
// [/SL:KB]
//    closeAllOnCurrentTab();
}

//---------------------------------------------------------------------------------
void LLFloaterNotifications::onClickCollapseAllBtn()
{
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	collapseVisibleNotifications();
// [/SL:KB]
//    collapseAllOnCurrentTab();
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initTaggedList(const std::string& tag, LLNotificationListView* list)
{
    mNotificationListMap.insert(notification_list_map_t::value_type(tag, list));
    mNotificationLists.push_back(list);
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initTaggedList(const std::set<std::string>& tags, LLNotificationListView* list)
{
    std::set<std::string>::const_iterator it = tags.begin();
    for(;it != tags.end();it++)
    {
        initTaggedList(*it, list);
    }
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initUnTaggedList(LLNotificationListView* list)
{
    mUnTaggedList = list;
}

//---------------------------------------------------------------------------------
//bool LLNotificationSeparator::addItem(std::string& tag, LLNotificationListItem* item)
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLNotificationSeparator::addItem(const std::string& tag, LLNotificationListItem* item)
// [/SL:KB]
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
// [SL:KB] - Patch: Notification-Misc | Checked: 2016-01-01 (Catznip-4.0)
		if (it->second->addNotification(item, false))
		{
			it->second->sort();
			return true;
		}
// [/SL:KB]
//        return it->second->addNotification(item);
    }
    else if (mUnTaggedList != NULL)
    {
// [SL:KB] - Patch: Notification-Misc | Checked: 2016-01-01 (Catznip-4.0)
		if (mUnTaggedList->addNotification(item, false))
		{
			mUnTaggedList->sort();
			return true;
		}
// [/SL:KB]
//        return mUnTaggedList->addNotification(item);
    }
    return false;
}

//---------------------------------------------------------------------------------
//bool LLNotificationSeparator::removeItemByID(std::string& tag, const LLUUID& id)
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLNotificationSeparator::removeItemByID(const std::string& tag, const LLUUID& id)
// [/SL:KB]
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
        return it->second->removeItemByValue(id);
    }
    else if (mUnTaggedList != NULL)
    {
        return mUnTaggedList->removeItemByValue(id);
    }
    return false;
}

//---------------------------------------------------------------------------------
U32 LLNotificationSeparator::size() const
{
    U32 size = 0;
    notification_list_list_t::const_iterator it = mNotificationLists.begin();
    for (; it != mNotificationLists.end(); it++)
    {
        size = size + (*it)->size();
    }
    if (mUnTaggedList != NULL)
    {
        size = size + mUnTaggedList->size();
    }
    return size;
}

//---------------------------------------------------------------------------------
//LLPanel* LLNotificationSeparator::findItemByID(std::string& tag, const LLUUID& id)
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
LLPanel* LLNotificationSeparator::findItemByID(const std::string& tag, const LLUUID& id)
// [/SL:KB]
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
        return it->second->getItemByValue(id);
    }
    else if (mUnTaggedList != NULL)
    {
        return mUnTaggedList->getItemByValue(id);
    }

    return NULL;    
}

//static
//---------------------------------------------------------------------------------
void LLNotificationSeparator::getItemsFromList(std::vector<LLNotificationListItem*>& items, LLNotificationListView* list)
{
    std::vector<LLPanel*> list_items;
    list->getItems(list_items);
    std::vector<LLPanel*>::iterator it = list_items.begin();
    for (; it != list_items.end(); ++it)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*it);
        if (notify_item)
            items.push_back(notify_item);
    }
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::getItems(std::vector<LLNotificationListItem*>& items) const
{
    items.clear();
    notification_list_list_t::const_iterator lists_it = mNotificationLists.begin();
    for (; lists_it != mNotificationLists.end(); lists_it++)
    {
        getItemsFromList(items, *lists_it);
    }
    if (mUnTaggedList != NULL)
    {
        getItemsFromList(items, mUnTaggedList);
    }
}

//---------------------------------------------------------------------------------
LLNotificationSeparator::LLNotificationSeparator()
    : mUnTaggedList(NULL)
{}

//---------------------------------------------------------------------------------
LLNotificationSeparator::~LLNotificationSeparator()
{}

//---------------------------------------------------------------------------------
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
bool LLNotificationDateComparator::compare(const LLPanel* pLHS, const LLPanel* pRHS) const
{
	const LLNotificationListItem* pItemLeft = dynamic_cast<const LLNotificationListItem*>(pLHS);
	const LLNotificationListItem* pItemRight= dynamic_cast<const LLNotificationListItem*>(pRHS);
	if ( (pItemLeft) && (pItemRight) )
	{
		LLNotificationPtr notifLeft = LLNotifications::instance().find(pItemLeft->getID());
		LLNotificationPtr notifRight= LLNotifications::instance().find(pItemRight->getID());
		// NOTE: we want to sort notifications from new to top
		return (notifLeft.get()) && (notifRight.get()) && (notifLeft.get()->getDate() > notifRight.get()->getDate());
	}
	return false;
}

void LLFloaterNotificationsUtil::registerFloater()
{
	LLFloaterReg::add("notification_well_window", "floater_notifications_flat.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationsFlat>);
//	LLFloaterReg::add("notification_well_window", "floater_notifications_tabbed.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationsTabbed>);
}
// [/SL:KB]
//---------------------------------------------------------------------------------
