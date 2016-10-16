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

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
#include "llchiclet.h"
#include "llchicletbar.h"
#include "llviewercontrol.h"
// [/SL:KB]
#include "llfloaterimcontainertab.h"
#include "llfloaterimsession.h"
#include "llfloaterreg.h"
#include "lltoggleablemenu.h"
#include "llviewercontrol.h"

//
// LLFloaterIMContainerTab
//
LLFloaterIMContainerTab::LLFloaterIMContainerTab(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
	: LLFloaterIMContainerBase(seed, params)
{
	sContainerType = (!gSavedSettings.getBOOL("IMUseSeparateFloaters")) ? CT_TABBED : CT_SEPARATE;
}

LLFloaterIMContainerTab::~LLFloaterIMContainerTab()
{
}

BOOL LLFloaterIMContainerTab::postBuild()
{
	LLFloaterIMContainerBase::postBuild();

	mTabContainer->setRightMouseDownCallback(boost::bind(&LLFloaterIMContainerTab::onTabContainerRightMouseDown, this, _2, _3));

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
	if (gSavedSettings.getBOOL("RearrangeIMTabs"))
	{
		mTabContainer->setAllowRearrange(true);
		mTabContainer->setRearrangeCallback(boost::bind(&LLFloaterIMContainerTab::onIMTabRearrange, this, _1, _2));
	}
// [/SL:KB]

	return TRUE;
}

void LLFloaterIMContainerTab::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	// NOTE: this function is a no-op, but dependent patch branched add code here and it saves us merging headaches to define it in the base patch
	if (!floaterp)
	{
		return;
	}

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-06-22 (Catznip-3.3)
	const LLUUID idSession = floaterp->getKey();

	// NOTE: this will only do work on legacy IM-tabs but shouldn't actually harm CHUI's hidden tabs
	if ( (LLChicletBar::instanceExists()) && (floaterp->isTornOff()) && (LLTabContainer::END == insertion_point) )
	{
		// If we're redocking a torn off IM floater, return it back to its previous place
		LLChicletPanel* pChicletPanel = LLChicletBar::instance().getChicletPanel();

		LLIMChiclet* pChiclet = pChicletPanel->findChiclet<LLIMChiclet>(idSession);
		S32 idxChiclet = pChicletPanel->getChicletIndex(pChiclet);
		if ( (idxChiclet > 0) && (idxChiclet < pChicletPanel->getChicletCount() - 1) )
		{
			// Look for the first IM session to the left of this one
			while (--idxChiclet >= 0)
			{
				if (pChiclet = dynamic_cast<LLIMChiclet*>(pChicletPanel->getChiclet(idxChiclet)))
				{
					const LLFloaterIMSession* pFloater = LLFloaterIMSession::findInstance(pChiclet->getSessionId());
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

	LLFloaterIMContainerBase::addFloater(floaterp, select_added_floater, insertion_point);
}

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

void LLFloaterIMContainerTab::showConversation(const LLUUID& session_id, bool focus_floater)
{
	selectConversationPair(session_id, true, focus_floater);
}

void LLFloaterIMContainerTab::toggleConversation(const LLUUID& session_id)
{
	if (LLFloaterIMContainerBase::CT_SEPARATE == LLFloaterIMContainerBase::getContainerType())
	{
		// Clicking the chiclet while the conversation has focus toggles its visibility
		if (LLFloaterIMSession* pIMFloater = LLFloaterIMSession::findInstance(session_id))
		{
			if ( (pIMFloater->getVisible()) && (pIMFloater->hasFocus()) )
			{
				pIMFloater->setVisible(false);
				return;
			}
		}
	}

	showConversation(session_id);
}

bool LLFloaterIMContainerTab::selectConversationPair(const LLUUID& session_id, bool /*select_widget*/, bool focus_floater /*=true*/)
{
	LLFloaterIMSession* pConvFloater = LLFloaterIMSession::findInstance(session_id);
	if (pConvFloater)
	{
		if (!pConvFloater->isTornOff())
		{
			setMinimized(false);
			setVisibleAndFrontmost(true);
			mTabContainer->selectTabPanel(pConvFloater);

			// Set the focus on the selected floater
			if ( (!pConvFloater->hasFocus()) && (!pConvFloater->isMinimized()) )
			{
				pConvFloater->setFocus(focus_floater);
			}
		}
		else
		{
			pConvFloater->setMinimized(false);
			pConvFloater->setVisibleAndFrontmost(focus_floater);
		}
	}
	return true;
}

// Checked: 2013-05-11 (Catznip-3.5)
void LLFloaterIMContainerTab::setConversationFlashing(const LLUUID& session_id, bool flashing)
{
	LLFloater* pIMSession = get_ptr_in_map(getSessionMap(), session_id);
	LLFloater* pCurSession = getActiveFloater();
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

void LLFloaterIMContainerTab::setConversationHighlighted(const LLUUID& session_id, bool flashing)
{
	LLFloater* pIMSession = get_ptr_in_map(getSessionMap(), session_id);
	LLFloater* pCurSession = getActiveFloater();
	if ( (pIMSession) && (pCurSession) )
	{
		mTabContainer->setTabPanelFlashing(pIMSession, (flashing) && (pIMSession != pCurSession), false);
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

void LLFloaterIMContainerTab::setTimeNow(const LLUUID& session_id, const LLUUID& participant_id)
{
}

void LLFloaterIMContainerTab::onTabContainerRightMouseDown(S32 x, S32 y)
{
	LLFloaterIMSessionTab* pTabPanel = dynamic_cast<LLFloaterIMSessionTab*>(mTabContainer->getPanelFromPoint(x, y));
	if (pTabPanel)
	{
		mTabContainer->selectTabPanel(pTabPanel);
		if (!pTabPanel->isNearbyChat())
		{
			LLToggleableMenu* pMenu = pTabPanel->getGearMenu();
			if (pMenu)
			{
				pMenu->buildDrawLabels();
				pMenu->arrangeAndClear();
				pMenu->updateParent(LLMenuGL::sMenuContainer);

				LLMenuGL::showPopup(mTabContainer, pMenu, x, y);
			}
		}
	}
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
