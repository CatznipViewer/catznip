/** 
 * @file llimfloatercontainer.cpp
 * @brief Multifloater containing active IM sessions in separate tab container tabs
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

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
#include "llchiclet.h"
#include "llchicletbar.h"
#include "llimfloater.h"
// [/SL:KB]
#include "llimfloatercontainer.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llavatariconctrl.h"
#include "llgroupiconctrl.h"
#include "llagent.h"
#include "lltransientfloatermgr.h"
// [SL:KB] - Patch: UI-TabRearrange | Checked: 2010-06-05 (Catznip-3.3)
#include "llviewercontrol.h"
// [/SL:KB]

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed)
{
	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	mNewMessageConnection.disconnect();
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

BOOL LLIMFloaterContainer::postBuild()
{
	mNewMessageConnection = LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLIMFloaterContainer::onNewMessageReceived, this, _1));
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
	if (gSavedSettings.getBOOL("RearrangeIMTabs"))
	{
		mTabContainer->setAllowRearrange(true);
		mTabContainer->setRearrangeCallback(boost::bind(&LLIMFloaterContainer::onIMTabRearrange, this, _1, _2));
	}
// [/SL:KB]
	return TRUE;
}

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-06-23 (Catznip-3.3)
void LLIMFloaterContainer::onIMTabRearrange(S32 tab_index, LLPanel* tab_panel)
{
	LLIMFloater* pFloater = dynamic_cast<LLIMFloater*>(tab_panel);
	if (!pFloater)
		return;

	LLChicletPanel* pChicletPanel = LLChicletBar::instance().getChicletPanel();
	LLIMChiclet* pChiclet = pChicletPanel->findChiclet<LLIMChiclet>(pFloater->getKey());
	if (!pChiclet)
		return;

	if ( (tab_index > mTabContainer->getNumLockedTabs()) && (tab_index < mTabContainer->getTabCount() - 1) )
	{
		// Look for the first IM session to the left of this one
		while (--tab_index >= mTabContainer->getNumLockedTabs())
		{
			LLIMFloater* pPrevFloater = dynamic_cast<LLIMFloater*>(mTabContainer->getPanelByIndex(tab_index));
			if (pPrevFloater)
			{
				pChicletPanel->setChicletIndex(pChiclet, LLChicletPanel::RIGHT_OF_SESSION, pPrevFloater->getKey().asUUID());
				break;
			}
		}
	}
	else
	{
		pChicletPanel->setChicletIndex(pChiclet, (tab_index <= mTabContainer->getNumLockedTabs()) ? LLChicletPanel::START : LLChicletPanel::END);
	}
}
// [/SL:KB]

void LLIMFloaterContainer::onOpen(const LLSD& key)
{
	LLMultiFloater::onOpen(key);
/*
	if (key.isDefined())
	{
		LLIMFloater* im_floater = LLIMFloater::findInstance(key.asUUID());
		if (im_floater)
		{
			im_floater->openFloater();
		}
	}
*/
}

void LLIMFloaterContainer::addFloater(LLFloater* floaterp, 
									BOOL select_added_floater, 
									LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-06-22 (Catznip-3.3)
	// If we're redocking a torn off IM floater, return it back to its previous place
	if ( (floaterp->isTornOff()) && (LLTabContainer::END == insertion_point) )
	{
		LLChicletPanel* pChicletPanel = LLChicletBar::instance().getChicletPanel();

		LLIMChiclet* pChiclet = pChicletPanel->findChiclet<LLIMChiclet>(floaterp->getKey());
		S32 idxChiclet = pChicletPanel->getChicletIndex(pChiclet);
		if ( (idxChiclet > 0) && (idxChiclet < pChicletPanel->getChicletCount() - 1) )
		{
			// Look for the first IM session to the left of this one
			while (--idxChiclet >= 0)
			{
				if (pChiclet = dynamic_cast<LLIMChiclet*>(pChicletPanel->getChiclet(idxChiclet)))
				{
					const LLIMFloater* pFloater = LLIMFloater::findInstance(pChiclet->getSessionId());
					if (pFloater)
					{
						insertion_point = (LLTabContainer::eInsertionPoint)(mTabContainer->getIndexForPanel(pFloater) + 1);
						break;
					}
				}
			}
		}
		else 
		{
			insertion_point = (0 == idxChiclet) ? LLTabContainer::START : LLTabContainer::END;
		}
	}
// [/SL:KB]

	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLUUID session_id = floaterp->getKey();

	LLIconCtrl* icon = 0;

	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	mTabContainer->setTabImage(floaterp, icon);
}

void LLIMFloaterContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
	setFocus(TRUE);
}

void LLIMFloaterContainer::onNewMessageReceived(const LLSD& data)
{
	LLUUID session_id = data["session_id"].asUUID();
	LLFloater* floaterp = get_ptr_in_map(mSessions, session_id);
	LLFloater* current_floater = LLMultiFloater::getActiveFloater();

	if(floaterp && current_floater && floaterp != current_floater)
	{
		if(LLMultiFloater::isFloaterFlashing(floaterp))
			LLMultiFloater::setFloaterFlashing(floaterp, FALSE);
		LLMultiFloater::setFloaterFlashing(floaterp, TRUE);
	}
}

LLIMFloaterContainer* LLIMFloaterContainer::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLIMFloaterContainer>("im_container");
}

LLIMFloaterContainer* LLIMFloaterContainer::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLIMFloaterContainer>("im_container");
}

void LLIMFloaterContainer::setMinimized(BOOL b)
{
	if (isMinimized() == b) return;
	
	LLMultiFloater::setMinimized(b);
	// Hide minimized floater (see EXT-5315)
	setVisible(!b);

	if (isMinimized()) return;

	if (getActiveFloater())
	{
		getActiveFloater()->setVisible(TRUE);
	}
}

// EOF
