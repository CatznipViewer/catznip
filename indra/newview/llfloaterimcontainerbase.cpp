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
LLFloaterIMContainerBase::LLFloaterIMContainerBase(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
	: LLMultiFloater(seed, params)
{
	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLFloaterIMContainerBase::~LLFloaterIMContainerBase()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

// static
void LLFloaterIMContainerBase::onCurrentChannelChanged(const LLUUID& session_id)
{
	if (session_id != LLUUID::null)
	{
		LLFloaterIMContainerBase::getInstance()->showConversation(session_id);
	}
}

BOOL LLFloaterIMContainerBase::postBuild()
{
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));

	return TRUE;
}

// virtual
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

	LLIconCtrl* icon = 0;
	if (gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);
	}
	else
	{
		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = session_id.notNull() ? LLIMModel::getInstance()->getOtherParticipantID(session_id) : LLUUID();
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);
	}
	mTabContainer->setTabImage(floaterp, icon);

	mSessions[session_id] = floaterp;
	floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainerBase::onCloseFloater, this, session_id));
}

void LLFloaterIMContainerBase::onCloseFloater(const LLUUID& session_id)
{
	mSessions.erase(session_id);
	setFocus(TRUE);
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
	static const std::string strFile = 
		(gSavedSettings.getBOOL("IMUseTabbedContainer")) 
			? (!gSavedSettings.getBOOL("IMUseVerticalTabs")) ? "floater_im_container_tab_horiz.xml" : "floater_im_container_tab_vert.xml"
			: "floater_im_container.xml";
	return strFile;
}

void LLFloaterIMContainerBase::setMinimized(BOOL b)
{
	bool was_minimized = isMinimized();
	LLMultiFloater::setMinimized(b);

	// Switching from minimized to un-minimized
	if (was_minimized && !b)
	{
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

// static
bool LLFloaterIMContainerBase::isConversationLoggingAllowed()
{
	return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
}

// EOF
