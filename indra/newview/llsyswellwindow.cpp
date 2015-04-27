/** 
 * @file llsyswellwindow.cpp
 * @brief                                    // TODO
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
#include "llsyswellwindow.h"

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llgroupactions.h"
// [/SL:KB]
#include "llchiclet.h"
#include "llchicletbar.h"
#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llscriptfloater.h"
#include "llspeakers.h"
#include "lltoastpanel.h"

//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLTransientDockableFloater(NULL, true,  key),
//													mChannel(NULL),
													mMessageList(NULL),
													mSysWellChiclet(NULL),
													NOTIFICATION_WELL_ANCHOR_NAME("notification_well_panel"),
													IM_WELL_ANCHOR_NAME("im_well_panel"),
													mIsReshapedByUser(false)

{
	setOverlapsScreenChannel(true);
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mMessageList = getChild<LLFlatListView>("notification_list");

	// get a corresponding channel
	initChannel();

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	// Clicking on the syswell window should clear "new message" state (and 'lit' status)
	setMouseDownCallback(boost::bind(&LLSysWellWindow::releaseNewMessagesState, this));
// [/SL:KB]

	return LLTransientDockableFloater::postBuild();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setMinimized(BOOL minimize)
{
	LLTransientDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::handleReshape(const LLRect& rect, bool by_user)
{
	mIsReshapedByUser |= by_user; // mark floater that it is reshaped by user
	LLTransientDockableFloater::handleReshape(rect, by_user);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onStartUpToastClick(S32 x, S32 y, MASK mask)
{
	// just set floater visible. Screen channels will be cleared.
	setVisible(TRUE);
}

void LLSysWellWindow::setSysWellChiclet(LLSysWellChiclet* chiclet) 
{ 
	mSysWellChiclet = chiclet;
	if(NULL != mSysWellChiclet)
	{
		mSysWellChiclet->updateWidget(isWindowEmpty());
	}
}

//---------------------------------------------------------------------------------
LLSysWellWindow::~LLSysWellWindow()
{
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::removeItemByID(const LLUUID& id)
{
	if(mMessageList->removeItemByValue(id))
	{
		if (NULL != mSysWellChiclet)
		{
			mSysWellChiclet->updateWidget(isWindowEmpty());
		}
		reshapeWindow();
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

 LLPanel * LLSysWellWindow::findItemByID(const LLUUID& id)
{
       return mMessageList->getItemByValue(id);
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void LLSysWellWindow::initChannel() 
{
	LLNotificationsUI::LLScreenChannelBase* channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(
																LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	if (channel)
		mChannel = channel->getHandle();
	else
		LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
// [/SL:KB]
//	mChannel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(channel);
//	if(NULL == mChannel)
//	{
//		LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
//	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
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
	if (NULL == mMessageList || isWindowEmpty()) visible = FALSE;

	LLTransientDockableFloater::setVisible(visible);

	// update notification channel state	
	initChannel(); // make sure the channel still exists
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}
// [/SL:KB]
//	if(mChannel)
//	{
//		mChannel->updateShowToastsState();
//		mChannel->redrawToasts();
//	}

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	if (visible)
	{
		releaseNewMessagesState();
	}
// [/SL:KB]
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setDocked(bool docked, bool pop_on_undock)
{
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);

	// update notification channel state
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}
// [/SL:KB]
//	if(mChannel)
//	{
//		mChannel->updateShowToastsState();
//		mChannel->redrawToasts();
//	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// save difference between floater height and the list height to take it into account while calculating new window height
	// it includes height from floater top to list top and from floater bottom and list bottom
	static S32 parent_list_delta_height = getRect().getHeight() - mMessageList->getRect().getHeight();

	if (!mIsReshapedByUser) // Don't reshape Well window, if it ever was reshaped by user. See EXT-5715.
	{
		S32 notif_list_height = mMessageList->getItemsRect().getHeight() + 2 * mMessageList->getBorderWidth();

		LLRect curRect = getRect();

		S32 new_window_height = notif_list_height + parent_list_delta_height;

		if (new_window_height > MAX_WINDOW_HEIGHT)
		{
			new_window_height = MAX_WINDOW_HEIGHT;
		}
		S32 newWidth = curRect.getWidth() < MIN_WINDOW_WIDTH ? MIN_WINDOW_WIDTH	: curRect.getWidth();

// [SL:KB] - Patch: Chat-ChicletBarAligment | Checked: 2011-11-19 (Catznip-3.2)
		// Resize bottom-up if we're docked at the top and top-down if we're docked at the bottom
		S32 top = curRect.mTop;
		if ( (getDockControl()) && (LLDockControl::TOP == getDockControl()->getDockAt()) )
			top += new_window_height - curRect.getHeight();
		curRect.setLeftTopAndSize(curRect.mLeft, top, newWidth, new_window_height);
// [/SL:KB]
//		curRect.setLeftTopAndSize(curRect.mLeft, curRect.mTop, newWidth, new_window_height);
		reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
		setRect(curRect);
	}

	// update notification channel state
	// update on a window reshape is important only when a window is visible and docked
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if ( (channel) && (getVisible()) && (isDocked()) )
	{
		channel->updateShowToastsState();
	}
// [/SL:KB]
//	if(mChannel && getVisible() && isDocked())
//	{
//		mChannel->updateShowToastsState();
//	}
}

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
void LLSysWellWindow::releaseNewMessagesState()
{
	if (mSysWellChiclet)
		mSysWellChiclet->setNewMessagesState(false);
}
// [/SL:KB]

//---------------------------------------------------------------------------------
bool LLSysWellWindow::isWindowEmpty()
{
	return mMessageList->size() == 0;
}

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
/************************************************************************/
/*         IMRowPanel implementation                                    */
/************************************************************************/

//---------------------------------------------------------------------------------
LLIMWellWindow::IMRowPanel::IMRowPanel(const LLUUID& sessionId,S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId)
	: LLPanel(LLPanel::Params())
	, mChiclet(NULL)
{
	LLIMChiclet::EType im_chiclet_type = LLIMChiclet::getIMSessionType(sessionId);
	switch (im_chiclet_type)
	{
		case LLIMChiclet::TYPE_GROUP:
			buildFromFile("panel_activeim_row_group.xml");
			mChiclet = getChild<LLIMGroupChiclet>("group_chiclet");
			break;
		case LLIMChiclet::TYPE_AD_HOC:
			buildFromFile("panel_activeim_row_adhoc.xml");
			mChiclet = getChild<LLAdHocChiclet>("adhoc_chiclet");
			break;
		case LLIMChiclet::TYPE_UNKNOWN: // assign mChiclet a non-null value anyway
		case LLIMChiclet::TYPE_IM:
			buildFromFile("panel_activeim_row_p2p.xml");
			mChiclet = getChild<LLIMP2PChiclet>("p2p_chiclet");
			break;
	}

	// Initialize chiclet.
	mChiclet->setChicletSizeChangedCallback(boost::bind(&LLIMWellWindow::IMRowPanel::onChicletSizeChanged, this, mChiclet, _2));
	mChiclet->enableCounterControl(true);
	mChiclet->setCounter(chicletCounter);
	mChiclet->setSessionId(sessionId);
	mChiclet->setIMSessionName(name);
	mChiclet->setOtherParticipantId(otherParticipantId);
	mChiclet->setVisible(true);

	if (LLIMChiclet::TYPE_IM == im_chiclet_type)
	{
		LLAvatarNameCache::get(otherParticipantId, boost::bind(&LLIMWellWindow::IMRowPanel::onAvatarNameCache, this, _1, _2));
	}
	else
	{
		LLTextBox* contactName = getChild<LLTextBox>("contact_name");
		contactName->setValue(name);
	}

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLIMWellWindow::IMRowPanel::closeConversation, this));
}

//---------------------------------------------------------------------------------
LLIMWellWindow::IMRowPanel::~IMRowPanel()
{
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::IMRowPanel::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	LLTextBox* contactName = getChild<LLTextBox>("contact_name");
	contactName->setValue(av_name.getCompleteName());
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::IMRowPanel::onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param)
{
	LLTextBox* text = getChild<LLTextBox>("contact_name");
	S32 new_text_left = mChiclet->getRect().mRight + CHICLET_HPAD;
	LLRect text_rect = text->getRect(); 
	text_rect.mLeft = new_text_left;
	text->setShape(text_rect);
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::IMRowPanel::closeConversation()
{
	switch (LLIMChiclet::getIMSessionType(mChiclet->getSessionId()))
	{
		case LLIMChiclet::TYPE_IM:
			LLAvatarActions::endIM(mChiclet->getOtherParticipantId());
			break;
		case LLIMChiclet::TYPE_GROUP:
		case LLIMChiclet::TYPE_AD_HOC:
			LLGroupActions::endIM(mChiclet->getSessionId());
			break;
	}
	// This row panel will be removed from the list in LLSysWellWindow::sessionRemoved().
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::IMRowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::IMRowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLIMWellWindow::IMRowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if ( (!mChiclet->pointInView(x, y)) && (!mCloseBtn->getRect().pointInRect(x, y)) ) // prevent double call of LLIMChiclet::onMouseDown()
	{
		mChiclet->onMouseDown();
		return TRUE;
	}
	return LLPanel::handleMouseDown(x, y, mask);
}

// virtual
BOOL LLIMWellWindow::IMRowPanel::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return mChiclet->handleRightMouseDown(x, y, mask);
}
// [/SL:KB]

/************************************************************************/
/*         ObjectRowPanel implementation                                */
/************************************************************************/

LLIMWellWindow::ObjectRowPanel::ObjectRowPanel(const LLUUID& notification_id, bool new_message/* = false*/)
 : LLPanel()
 , mChiclet(NULL)
{
	buildFromFile( "panel_active_object_row.xml");

	initChiclet(notification_id);

	LLTextBox* obj_name = getChild<LLTextBox>("object_name");
	obj_name->setValue(LLScriptFloaterManager::getObjectName(notification_id));

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLIMWellWindow::ObjectRowPanel::onClosePanel, this));
}

//---------------------------------------------------------------------------------
LLIMWellWindow::ObjectRowPanel::~ObjectRowPanel()
{
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onClosePanel()
{
	LLScriptFloaterManager::getInstance()->removeNotification(mChiclet->getSessionId());
}

void LLIMWellWindow::ObjectRowPanel::initChiclet(const LLUUID& notification_id, bool new_message/* = false*/)
{
	// Choose which of the pre-created chiclets to use.
	switch(LLScriptFloaterManager::getObjectType(notification_id))
	{
	case LLScriptFloaterManager::OBJ_GIVE_INVENTORY:
		mChiclet = getChild<LLInvOfferChiclet>("inv_offer_chiclet");
		break;
	default:
		mChiclet = getChild<LLScriptChiclet>("object_chiclet");
		break;
	}

	mChiclet->setVisible(true);
	mChiclet->setSessionId(notification_id);
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLIMWellWindow::ObjectRowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if (!mChiclet->pointInView(x, y) && !mCloseBtn->getRect().pointInRect(x, y)) // prevent double call of LLIMChiclet::onMouseDown()
	{
		mChiclet->onMouseDown();
		return TRUE;
	}

	return LLPanel::handleMouseDown(x, y, mask);
}

// virtual
BOOL LLIMWellWindow::ObjectRowPanel::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return mChiclet->handleRightMouseDown(x, y, mask);
}

/************************************************************************/
/*         LLNotificationWellWindow implementation                      */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
LLNotificationWellWindow::WellNotificationChannel::WellNotificationChannel(LLNotificationWellWindow* well_window)
:	LLNotificationChannel(LLNotificationChannel::Params().name(well_window->getPathname())),
	mWellWindow(well_window)
{
	connectToChannel("Notifications");
	connectToChannel("Group Notifications");
	connectToChannel("Offer");
}

LLNotificationWellWindow::LLNotificationWellWindow(const LLSD& key)
:	LLSysWellWindow(key)
{
	mNotificationUpdates.reset(new WellNotificationChannel(this));
}

// static
LLNotificationWellWindow* LLNotificationWellWindow::getInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::getTypedInstance<LLNotificationWellWindow>("notification_well_window", key);
}

// virtual
BOOL LLNotificationWellWindow::postBuild()
{
	BOOL rv = LLSysWellWindow::postBuild();
	setTitle(getString("title_notification_well_window"));
	return rv;
}

// virtual
void LLNotificationWellWindow::setVisible(BOOL visible)
{
	if (visible)
	{
		// when Notification channel is cleared, storable toasts will be added into the list.
		clearScreenChannels();
	}

	LLSysWellWindow::setVisible(visible);
}

//---------------------------------------------------------------------------------
void LLNotificationWellWindow::addItem(LLSysWellItem::Params p)
{
	LLSD value = p.notification_id;
	// do not add clones
	if( mMessageList->getItemByValue(value))
		return;

	LLSysWellItem* new_item = new LLSysWellItem(p);
	if (mMessageList->addItem(new_item, value, ADD_TOP))
	{
		mSysWellChiclet->updateWidget(isWindowEmpty());
		reshapeWindow();
		new_item->setOnItemCloseCallback(boost::bind(&LLNotificationWellWindow::onItemClose, this, _1));
		new_item->setOnItemClickCallback(boost::bind(&LLNotificationWellWindow::onItemClick, this, _1));
	}
	else
	{
		LL_WARNS() << "Unable to add Notification into the list, notification ID: " << p.notification_id
			<< ", title: " << p.title
			<< LL_ENDL;

		new_item->die();
	}
}

void LLNotificationWellWindow::closeAll()
{
	// Need to clear notification channel, to add storable toasts into the list.
	clearScreenChannels();
	std::vector<LLPanel*> items;
	mMessageList->getItems(items);
	for (std::vector<LLPanel*>::iterator
			 iter = items.begin(),
			 iter_end = items.end();
		 iter != iter_end; ++iter)
	{
		LLSysWellItem* sys_well_item = dynamic_cast<LLSysWellItem*>(*iter);
		if (sys_well_item)
			onItemClose(sys_well_item);
	}
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
void LLNotificationWellWindow::initChannel() 
{
	LLSysWellWindow::initChannel();
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->addOnStoreToastCallback(boost::bind(&LLNotificationWellWindow::onStoreToast, this, _1, _2));
	}
// [/SL:KB]
//	if(mChannel)
//	{
//		mChannel->addOnStoreToastCallback(boost::bind(&LLNotificationWellWindow::onStoreToast, this, _1, _2));
//	}
}

void LLNotificationWellWindow::clearScreenChannels()
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
//		LLNotificationsUI::LLChannelManager::getInstance()->onStartUpToastClose();
	}

	// 2 - remove toasts in Notification channel
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->removeAndStoreAllStorableToasts();
	}
// [/SL:KB]
//	if(mChannel)
//	{
//		mChannel->removeAndStoreAllStorableToasts();
//	}
}

void LLNotificationWellWindow::onStoreToast(LLPanel* info_panel, LLUUID id)
{
	LLSysWellItem::Params p;	
	p.notification_id = id;
	p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
	addItem(p);
}

void LLNotificationWellWindow::onItemClick(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	LLFloaterReg::showInstance("inspect_toast", id);
}

void LLNotificationWellWindow::onItemClose(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	
//	if(mChannel)
//	{
//		// removeItemByID() is invoked from killToastByNotificationID() and item will removed;
//		mChannel->killToastByNotificationID(id);
//	}
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
		removeItemByID(id);
	}

}

void LLNotificationWellWindow::onAdd( LLNotificationPtr notify )
{
	removeItemByID(notify->getID());
}

/************************************************************************/
/*         LLIMWellWindow  implementation                               */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
LLIMWellWindow::LLIMWellWindow(const LLSD& key)
: LLSysWellWindow(key)
{
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	LLIMMgr::getInstance()->addSessionObserver(this);
// [/SL:KB]
}

LLIMWellWindow::~LLIMWellWindow()
{
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
// [/SL:KB]
}

// static
LLIMWellWindow* LLIMWellWindow::getInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::getTypedInstance<LLIMWellWindow>("im_well_window", key);
}


// static
LLIMWellWindow* LLIMWellWindow::findInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::findTypedInstance<LLIMWellWindow>("im_well_window", key);
}

BOOL LLIMWellWindow::postBuild()
{
	BOOL rv = LLSysWellWindow::postBuild();
	setTitle(getString("title_im_well_window"));

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLIMWellWindow::findIMChiclet, this, _1));
// [/SL:KB]
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLIMWellWindow::findObjectChiclet, this, _1));

	return rv;
}

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
//virtual
void LLIMWellWindow::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg)
{
	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!session) 
		return;

	// No need to add a row for participants in P2P calls called through Avaline
	if ( (session->isP2P()) && (session->isOtherParticipantAvaline()) )
		return;

	// No need to add a row if one already exists for the session
	if (mMessageList->getItemByValue(session_id))
		return;

	addIMRow(session_id, 0, name, other_participant_id);	
}

//virtual
void LLIMWellWindow::sessionRemoved(const LLUUID& sessionId)
{
	delIMRow(sessionId);
}

//virtual
void LLIMWellWindow::sessionIDUpdated(const LLUUID& idSessionOld, const LLUUID& idSessionNew)
{
	// TODO-Catznip: long term we should really recreate this row panel when it changes from P2P -> ad-hoc
	mMessageList->updateValue(idSessionOld, idSessionNew);
	LLChiclet* pChiclet = findIMChiclet(idSessionOld);
	if (pChiclet)
	{
		pChiclet->setSessionId(idSessionNew);
	}
}
// [/SL:KB]

LLChiclet* LLIMWellWindow::findObjectChiclet(const LLUUID& notification_id)
{
	if (!mMessageList) return NULL;

	LLChiclet* res = NULL;
	ObjectRowPanel* panel = mMessageList->getTypedItemByValue<ObjectRowPanel>(notification_id);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
LLChiclet* LLIMWellWindow::findIMChiclet(const LLUUID& sessionId)
{
	if (!mMessageList)
		return NULL;

	IMRowPanel* panel = mMessageList->getTypedItemByValue<IMRowPanel>(sessionId);
	return (panel != NULL) ? panel->mChiclet : NULL;
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::addIMRow(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId)
{
	IMRowPanel* item = new IMRowPanel(sessionId, chicletCounter, name, otherParticipantId);
	if (mMessageList->addItem(item, sessionId))
	{
		if (mSysWellChiclet)
			mSysWellChiclet->updateWidget(isWindowEmpty());
	}
	else
	{
		LL_WARNS() << "Unable to add IM Row into the list, sessionID: " << sessionId << ", name: " << name << LL_ENDL;
		item->die();
	}
	reshapeWindow();
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::delIMRow(const LLUUID& sessionId)
{
	//fix for EXT-3252
	//without this line LLIMWellWindow receive onFocusLost
	//and hide itself. It was becaue somehow LLIMChicklet was in focus group for
	//LLIMWellWindow...
	//But I didn't find why this happen..
	gFocusMgr.clearLastFocusForGroup(this);

	if (mMessageList->removeItemByValue(sessionId))
	{
		if (mSysWellChiclet)
			mSysWellChiclet->updateWidget(isWindowEmpty());
	}
	else
	{
		LL_WARNS() << "Unable to remove IM Row from the list, sessionID: " << sessionId << LL_ENDL;
	}
	reshapeWindow();

	// Remove all toasts that belong to this session from a screen
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mChannel.get());
	if (channel)
	{
		channel->removeToastsBySessionID(sessionId);
	}
// [/SL:KB]
//	if (mChannel)
//		mChannel->removeToastsBySessionID(sessionId);
	// Hide chiclet window if there are no items left
	if (isWindowEmpty())
		setVisible(FALSE);
	else
		setFocus(true);
}
// [/SL:KB]

void LLIMWellWindow::addObjectRow(const LLUUID& notification_id, bool new_message/* = false*/)
{
	if (mMessageList->getItemByValue(notification_id) == NULL)
	{
		ObjectRowPanel* item = new ObjectRowPanel(notification_id, new_message);
//		if (!mMessageList->addItem(item, notification_id))
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
		if (mMessageList->addItem(item, notification_id))
		{
			if (mSysWellChiclet)
				mSysWellChiclet->updateWidget(isWindowEmpty());
		}
		else
// [/SL:KB]
		{
			LL_WARNS() << "Unable to add Object Row into the list, notificationID: " << notification_id << LL_ENDL;
			item->die();
		}
		reshapeWindow();
	}
}

void LLIMWellWindow::removeObjectRow(const LLUUID& notification_id)
{
//	if (!mMessageList->removeItemByValue(notification_id))
// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
	if (mMessageList->removeItemByValue(notification_id))
	{
		if (mSysWellChiclet)
			mSysWellChiclet->updateWidget(isWindowEmpty());
	}
	else
// [/SL:KB]
	{
		LL_WARNS() << "Unable to remove Object Row from the list, notificationID: " << notification_id << LL_ENDL;
	}

	reshapeWindow();
	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}

void LLIMWellWindow::closeAll()
{
	// Generate an ignorable alert dialog if there is an active voice IM sesion
	bool need_confirmation = false;
	const LLIMModel& im_model = LLIMModel::instance();
	std::vector<LLSD> values;
	mMessageList->getValues(values);
	for (std::vector<LLSD>::iterator
			 iter = values.begin(),
			 iter_end = values.end();
		 iter != iter_end; ++iter)
	{
		LLIMSpeakerMgr* speaker_mgr =  im_model.getSpeakerManager(*iter);
		if (speaker_mgr && speaker_mgr->isVoiceActive())
		{
			need_confirmation = true;
			break;
		}
	}
	if ( need_confirmation )
	{
		//Bring up a confirmation dialog
		LLNotificationsUtil::add
			("ConfirmCloseAll", LLSD(), LLSD(),
			 boost::bind(&LLIMWellWindow::confirmCloseAll, this, _1, _2));
	}
	else
	{
		closeAllImpl();
	}
}

void LLIMWellWindow::closeAllImpl()
{
	std::vector<LLSD> values;
	mMessageList->getValues(values);

	for (std::vector<LLSD>::iterator
			 iter = values.begin(),
			 iter_end = values.end();
		 iter != iter_end; ++iter)
	{
		LLPanel* panel = mMessageList->getItemByValue(*iter);

// [SL:KB] - Patch: Chat-Chiclets | Checked: 2013-04-25 (Catznip-3.6)
		IMRowPanel* im_panel = dynamic_cast <IMRowPanel*> (panel);
		if (im_panel)
		{
			im_panel->closeConversation();
			continue;
		}
// [/SL:KB]

		ObjectRowPanel* obj_panel = dynamic_cast <ObjectRowPanel*> (panel);
		if (obj_panel)
		{
			LLScriptFloaterManager::instance().removeNotification(*iter);
		}
	}
}

bool LLIMWellWindow::confirmCloseAll(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		{
			closeAllImpl();
			return  true;
		}
	default:
		break;
	}
	return false;
}


