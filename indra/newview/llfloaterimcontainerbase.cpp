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
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llgroupiconctrl.h"
#include "llfloaterimsession.h"
#include "llfloaterimsessiontab.h"
#include "llfloaterreg.h"
#include "llfloaterimcontainer.h"
#include "llfloaterimcontainerbase.h"
#include "llfloaterimcontainertab.h"
#include "lltransientfloatermgr.h"
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
// [SL:KB] - Patch: Chat-Misc | Checked: 2013-08-18 (Catznip-3.6)
	mTabContainer->setCommitCallback(boost::bind(&LLFloaterIMContainerBase::onSelectConversation, this));

	// Save the title so we can refer back to it whenever a session is selected
	setShortTitle(getTitle());
// [/SL:KB]

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
//		gToolBarView->flashCommand(LLCommandId("chat"), false);

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

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	LLUUID session_id = floaterp->getKey();

	// Add the floater
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLIconCtrl* icon = NULL;
	if (gAgent.isInGroup(session_id, TRUE))
	{
// [SL:KB] - Patch: Chat-VertIMTabs | Checked: 2011-01-16 (Catznip-2.4)
		if (gSavedSettings.getBOOL("IMShowTabImage"))
		{
// [/SL:KB]
			LLGroupIconCtrl::Params icon_params;
			icon_params.group_id = session_id;
			icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);
// [SL:KB] - Patch: Chat-VertIMTabs | Checked: 2011-01-16 (Catznip-2.4)
		}
// [/SL:KB]
	}
	else
	{
// [SL:KB] - Patch: Chat-VertIMTabs | Checked: 2011-01-16 (Catznip-2.4)
		if (gSavedSettings.getBOOL("IMShowTabImage"))
		{
// [/SL:KB]
			LLAvatarIconCtrl::Params icon_params;
			icon_params.avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);
			icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);
// [SL:KB] - Patch: Chat-VertIMTabs | Checked: 2011-01-16 (Catznip-2.4)
		}
// [/SL:KB]
	}

// [SL:KB] - Patch: Chat-VertIMTabs | Checked: 2011-01-16 (Catznip-2.4)
	if (icon)
	{
		mTabContainer->setTabImage(floaterp, icon);
	}
// [/SL:KB]
//	mTabContainer->setTabImage(floaterp, icon);

	mSessions[session_id] = floaterp;
	floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainerBase::onCloseFloater, this, session_id));
}

// static
bool LLFloaterIMContainerBase::isConversationLoggingAllowed()
{
	return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
}

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

// [SL:KB] - Patch: Chat-Misc | Checked: 2013-08-18 (Catznip-3.6)
void LLFloaterIMContainerBase::updateFloaterTitle(LLFloater* floaterp)
{
	LLMultiFloater::updateFloaterTitle(floaterp);
	if (mTabContainer->getCurrentPanel() == (LLPanel*)floaterp)
	{
		// Update the container's title
		onSelectConversation();
	}
}

void LLFloaterIMContainerBase::onSelectConversation()
{
	const LLFloater* pIMFloater = (mTabContainer) ? dynamic_cast<const LLFloater*>(mTabContainer->getCurrentPanel()) : NULL;
	if (pIMFloater)
	{
		const std::string strTitle = llformat("%s - %s", getShortTitle().c_str(), pIMFloater->getTitle().c_str());
		setTitle(strTitle);
	}
	else
	{
		setTitle(getShortTitle());
	}
}
// [/SL:KB]

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

// static
void LLFloaterIMContainerBase::onToggleTabbedContainer()
{
	// Don't do anything if there isn't actually an instance yet
	if (!findInstance())
		return;

	// Build a collection of P2P and group IMs (conference chats won't/can't be restored)
	uuid_vec_t idsAvatars, idsGroup;

	std::map<LLUUID, LLIMModel::LLIMSession*>::const_iterator itEntry = LLIMModel::instance().mId2SessionMap.begin();
	while (itEntry != LLIMModel::instance().mId2SessionMap.end())
	{
		const LLIMModel::LLIMSession* pSession = itEntry->second;
		switch (pSession->mSessionType)
		{
			case LLIMModel::LLIMSession::P2P_SESSION:
				idsAvatars.push_back(pSession->mOtherParticipantID);
				break;
			case LLIMModel::LLIMSession::GROUP_SESSION:
				idsAvatars.push_back(pSession->mSessionID);
				break;
			default:
				// Not something we can (currently) restore
				break;
		}

		++itEntry;

		LLFloaterIMSession* pFloater = LLFloaterIMSession::findInstance(pSession->mSessionID);
		if (pFloater)
		{
            LLFloater::onClickClose(pFloater);
		}
	}

	// NOTE: * LLFloater::closeFloater() won't call LLFloater::destroy() since the floater is single instanced
	//       * we can't call LLFloater::destroy() since it will call LLMortician::die() which defers destruction until a later time
	//   => we'll have created a new instance and the delayed destructor calling LLFloaterReg::removeInstance() will make all future
	//      LLFloaterReg::getTypedInstance() calls return NULL so we need to destruct manually [see LLFloaterReg::destroyInstance()]
	LLFloaterIMContainerBase* pInstance = getInstance();
	bool fInstanceVisible = pInstance->isInVisibleChain();
	pInstance->closeFloater();
	LLFloaterReg::destroyInstance("im_container", LLSD());

	// Call getInstance() to instantiate the new IM container
	pInstance = getInstance();
	if (fInstanceVisible)
		pInstance->openFloater();

	// Restore all P2P chat sessions
	for (uuid_vec_t::const_iterator itAvatar = idsAvatars.begin(); itAvatar != idsAvatars.end(); ++itAvatar)
	{
		LLAvatarActions::startIM(*itAvatar);
	}
}

// static
void LLFloaterIMContainerBase::onToggleVerticalTabs()
{
}

// EOF
