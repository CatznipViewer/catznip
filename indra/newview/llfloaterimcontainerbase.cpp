/** 
 *
 * Copyright (c) 2013, Kitty Barnett
 * Copyright (C) 2010-2013, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llavatariconctrl.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llgroupiconctrl.h"
#include "llfloaterimsession.h"
#include "llfloaterreg.h"
#include "llfloaterimcontainer.h"
#include "llfloaterimcontainerbase.h"
#include "llfloaterimcontainertab.h"
#include "lltoolbarview.h"
#include "llviewercontrol.h"

//
// LLFloaterIMContainerBase
//

bool LLFloaterIMContainerBase::sTabbedContainer = false;

// Checked: 2013-09-01 (Catznip-3.6)
LLFloaterIMContainerBase::LLFloaterIMContainerBase(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
	: LLMultiFloater(seed, params)
{
	// Firstly add our self to IMSession observers, so we catch session events
	LLIMMgr::getInstance()->addSessionObserver(this);

	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

// Checked: 2013-09-01 (Catznip-3.6)
LLFloaterIMContainerBase::~LLFloaterIMContainerBase()
{
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}

	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

// Checked: 2013-09-01 (Catznip-3.6)
BOOL LLFloaterIMContainerBase::postBuild()
{
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));

	return TRUE;
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::setMinimized(BOOL b)
{
	bool was_minimized = isMinimized();
	LLMultiFloater::setMinimized(b);

	// Switching from minimized to un-minimized
	if (was_minimized && !b)
	{
		LLFloaterIMSessionTab* pNearbyChat = LLFloaterIMSessionTab::getConversation(LLUUID::null);
		if ( (pNearbyChat) && (!pNearbyChat->isTornOff()) )
			gToolBarView->flashCommand(LLCommandId("chat"), false);
		gToolBarView->flashCommand(LLCommandId("conversations"), false);

		const LLUUID& session_id = getSelectedSession();
		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
		if (session_floater && !session_floater->isTornOff())
		{
			// When in DND mode, remove stored IM notifications
			// Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
			if (gAgent.isDoNotDisturb() && session_id.notNull())
			{
				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, session_id);
			}
		}
	}
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	if (!floaterp)
		return;

	// Check if we're already hosting this floater
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	const LLUUID idSession = floaterp->getKey();

	// Add the floater
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLIconCtrl* icon = NULL;
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2)
	if (idSession.isNull())
	{
		// Don't allow the nearby chat tab to be drag-rearranged
		mTabContainer->lockTabs(1);

		// Add an icon for the nearby chat floater
		LLIconCtrl::Params icon_params;
		icon_params.image = LLUI::getUIImage("Command_Chat_Icon");
		icon = LLUICtrlFactory::instance().create<LLIconCtrl>(icon_params);
	}
	else if (gAgent.isInGroup(idSession, TRUE))
// [/SL:KB]
//	if (gAgent.isInGroup(idSession, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = idSession;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);
	}
	else
	{
		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = idSession.notNull() ? LLIMModel::getInstance()->getOtherParticipantID(idSession) : LLUUID();
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);
	}
	mTabContainer->setTabImage(floaterp, icon);

	mSessions[idSession] = floaterp;
	floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainerBase::onCloseFloater, this, idSession));
}
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2)
void LLFloaterIMContainerBase::removeFloater(LLFloater* floaterp)
{
	LLUUID idSession = floaterp->getKey();
	if (idSession.isNull())
	{
		mTabContainer->unlockTabs();
	}
	LLMultiFloater::removeFloater(floaterp);
}
// [/SL:KB]

// static
void LLFloaterIMContainerBase::onCurrentChannelChanged(const LLUUID& session_id)
{
	if (session_id != LLUUID::null)
	{
		LLFloaterIMContainerBase::getInstance()->showConversation(session_id);
	}
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::onCloseFloater(const LLUUID& session_id)
{
	mSessions.erase(session_id);
	setFocus(TRUE);
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg)
{
	LLFloaterIMSessionTab::addToHost(session_id);
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	selectConversationPair(session_id, true);
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::sessionVoiceOrIMStarted(const LLUUID& session_id)
{
	LLFloaterIMSessionTab::addToHost(session_id);
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	// The general strategy when a session id is modified is to delete all related objects and create them anew.
	
	// Note however that the LLFloaterIMSession has its session id updated through a call to sessionInitReplyReceived() 
	// and do not need to be deleted and recreated (trying this creates loads of problems). We do need however to suppress 
	// its related mSessions record as it's indexed with the wrong id.
	// Grabbing the updated LLFloaterIMSession and readding it in mSessions will eventually be done by addConversationListItem().
	mSessions.erase(old_session_id);

	LLFloaterIMSessionTab::addToHost(new_session_id);
}

// Checked: 2013-09-01 (Catznip-3.6)
void LLFloaterIMContainerBase::sessionRemoved(const LLUUID& session_id)
{
}

// static
LLFloaterIMContainerBase* LLFloaterIMContainerBase::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterIMContainerBase>("im_container");
}

// static
LLFloaterIMContainerBase* LLFloaterIMContainerBase::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterIMContainerBase>("im_container");
}

// static
LLFloater* LLFloaterIMContainerBase::buildFloater(const LLSD& sdKey)
{
	LLFloater* pIMContainer = NULL;
	if (gSavedSettings.getBOOL("IMUseTabbedContainer"))
		pIMContainer = new LLFloaterIMContainerTab(sdKey);
	else
		pIMContainer = new LLFloaterIMContainerView(sdKey);
	return pIMContainer;
}

// static
const std::string& LLFloaterIMContainerBase::getFloaterXMLFile()
{
	static std::string strFile;
	strFile = 
		(gSavedSettings.getBOOL("IMUseTabbedContainer")) 
			? (!gSavedSettings.getBOOL("IMUseVerticalTabs")) ? "floater_im_container_tab_horiz.xml" : "floater_im_container_tab_vert.xml"
			: "floater_im_container.xml";
	return strFile;
}

// EOF
