/** 
 * @file llfloaterimcontainertab.cpp
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
// [/SL:KB]
#include "llfloaterimcontainertab.h"
#include "llfloaterimsession.h"
#include "llfloaterreg.h"

//
// LLFloaterIMContainerTab
//
LLFloaterIMContainerTab::LLFloaterIMContainerTab(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
	: LLFloaterIMContainerBase(seed, params)
{
}

LLFloaterIMContainerTab::~LLFloaterIMContainerTab()
{
}

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
BOOL LLFloaterIMContainerTab::postBuild()
{
	BOOL fRet = LLFloaterIMContainerBase::postBuild();

	if (gSavedSettings.getBOOL("RearrangeIMTabs"))
	{
		mTabContainer->setAllowRearrange(true);
		mTabContainer->setRearrangeCallback(boost::bind(&LLFloaterIMContainerTab::onIMTabRearrange, this, _1, _2));
	}

	return fRet;
}
// [/SL:KB]

void LLFloaterIMContainerTab::setVisible(BOOL visible)
{
//	if (visible)
//	{
//		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(mSelectedSession);
//		if(session_floater && !session_floater->isMinimized())
//		{
//			//When in DND mode, remove stored IM notifications
//			//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
//			if(gAgent.isDoNotDisturb() && mSelectedSession.notNull())
//			{
//				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, mSelectedSession);
//			}
//		}
//	}

	// Set up the nearby chat floater if it hasn't been already (this is really hacky, bad Lindens)
	LLFloaterIMSessionTab* pNearbyChat = LLFloaterReg::findTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	if ( (!pNearbyChat) && (visible) )
	{
		pNearbyChat = LLFloaterReg::getTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	}
	if ( (pNearbyChat) && (!pNearbyChat->isHostAttached()) )
	{
		LLFloaterIMSessionTab::addToHost(LLUUID());
	}
	
	// Now, do the normal multifloater show/hide
	LLFloaterIMContainerBase::setVisible(visible);
}

void LLFloaterIMContainerTab::showConversation(const LLUUID& session_id)
{
	selectConversationPair(session_id, true);
}

bool LLFloaterIMContainerTab::selectConversationPair(const LLUUID& session_id, bool /*select_widget*/, bool focus_floater /*=true*/)
{
	LLFloaterIMSession* pConvFloater = LLFloaterIMSession::findInstance(session_id);
	if (pConvFloater)
	{
		if (!pConvFloater->isTornOff())
		{
			setVisibleAndFrontmost(false);
			mTabContainer->selectTabPanel(pConvFloater);

			// Set the focus on the selected floater
			if ( (!pConvFloater->hasFocus()) && (!pConvFloater->isMinimized()) )
			{
				pConvFloater->setFocus(focus_floater);
			}
		}
		else
		{
			pConvFloater->setVisibleAndFrontmost(true);
		}
	}
	return true;
}

// Checked: 2013-05-11 (Catznip-3.5)
void LLFloaterIMContainerTab::setConversationFlashing(const LLUUID& session_id, bool flashing)
{
	LLFloater* pIMSession = get_ptr_in_map(getSessionMap(), session_id);
	LLFloater* pCurSession = LLMultiFloater::getActiveFloater();
	if( (pIMSession) && (pCurSession) )
	{
		if (!flashing)
		{
			setFloaterFlashing(pIMSession, FALSE);
		}
		else if (pIMSession != pCurSession)
		{
			if (isFloaterFlashing(pIMSession))
				setFloaterFlashing(pIMSession, FALSE);
			setFloaterFlashing(pIMSession, TRUE);
		}
	}
}

// Checked: 2013-05-11 (Catznip-3.5)
const LLUUID& LLFloaterIMContainerTab::getSelectedSession() const
{
	const LLFloaterIMSession* pConvFloater = dynamic_cast<const LLFloaterIMSession*>(mTabContainer->getCurrentPanel());
	return (pConvFloater) ? pConvFloater->getSessionID() : LLUUID::null;
}

LLConversationItem* LLFloaterIMContainerTab::getSessionModel(const LLUUID& session_id) const
{
	return NULL;
}

const LLConversationSort& LLFloaterIMContainerTab::getSortOrder() const
{
	static LLConversationSort sort;
	return sort;
}

bool LLFloaterIMContainerTab::checkContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS)
{
	return false;
}

bool LLFloaterIMContainerTab::enableContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS)
{
	return false;
}

void LLFloaterIMContainerTab::doToParticipants(const std::string& item, uuid_vec_t& selectedIDS)
{
}

void LLFloaterIMContainerTab::setTimeNow(const LLUUID& session_id, const LLUUID& participant_id)
{
}

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-06-23 (Catznip-3.3)
void LLFloaterIMContainerTab::onIMTabRearrange(S32 tab_index, LLPanel* tab_panel)
{
	LLFloaterIMSession* pFloater = dynamic_cast<LLFloaterIMSession*>(tab_panel);
	if (!pFloater)
		return;

	if (LLChicletBar::instanceExists())
	{
		LLChicletPanel* pChicletPanel = LLChicletBar::instance().getChicletPanel();
		LLIMChiclet* pChiclet = pChicletPanel->findChiclet<LLIMChiclet>(pFloater->getKey());
		if (!pChiclet)
			return;

		if ( (tab_index > mTabContainer->getNumLockedTabs()) && (tab_index < mTabContainer->getTabCount() - 1) )
		{
			// Look for the first IM session to the left of this one
			while (--tab_index >= mTabContainer->getNumLockedTabs())
			{
				LLFloaterIMSession* pPrevFloater = dynamic_cast<LLFloaterIMSession*>(mTabContainer->getPanelByIndex(tab_index));
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
}
// [/SL:KB]

// EOF
