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

// EOF
