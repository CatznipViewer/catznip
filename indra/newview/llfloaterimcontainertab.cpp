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

void LLFloaterIMContainerTab::selectConversation(const LLUUID& session_id)
{
	selectConversationPair(session_id, true);
}

void LLFloaterIMContainerTab::selectNextConversationByID(const LLUUID& session_id)
{
}

bool LLFloaterIMContainerTab::selectConversationPair(const LLUUID& session_id, bool /*select_widget*/, bool focus_floater /*=true*/)
{
	LLFloaterIMSession* pConvFloater = LLFloaterIMSession::findInstance(session_id);
	if (pConvFloater)
	{
		if (!pConvFloater->isTornOff())
		{
			setVisibleAndFrontmost(true);
			mTabContainer->selectTabPanel(pConvFloater);
		}
		else
		{
			pConvFloater->setVisibleAndFrontmost(true);
		}
	}
	return true;
}

bool LLFloaterIMContainerTab::selectAdjacentConversation(bool focus_selected)
{
	return false;
}

bool LLFloaterIMContainerTab::selectNextorPreviousConversation(bool select_next, bool focus_selected /*=true*/)
{
	return false;
}

void LLFloaterIMContainerTab::expandConversation()
{
}

void LLFloaterIMContainerTab::collapseMessagesPane(bool collapse)
{
}

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

void LLFloaterIMContainerTab::onNearbyChatClosed()
{
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

void LLFloaterIMContainerTab::flashConversationItemWidget(const LLUUID& session_id, bool is_flashes)
{
}

S32 LLFloaterIMContainerTab::getConversationListItemSize() const
{
	return 1;
}

// EOF
