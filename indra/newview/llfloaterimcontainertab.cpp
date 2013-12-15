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

//
// LLFloaterIMContainerTab
//
LLFloaterIMContainerTab::LLFloaterIMContainerTab(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
	: LLFloaterIMContainerBase(seed, params)
{
	sTabbedContainer = true;
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

void LLFloaterIMContainerTab::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	// NOTE: this function is a no-op, but dependent patch branched add code here and it saves us merging headaches to define it in the base patch
	if (!floaterp)
	{
		return;
	}

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
	// *TODO: need implementing for legacy tab container
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
