/** 
 * @file llfloaterimcontainerbase.h
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

#ifndef LL_LLFLOATERIMCONTAINERBASE_H
#define LL_LLFLOATERIMCONTAINERBASE_H

#include "llmultifloater.h"

class LLConversationItem;
class LLConversationSort;

class LLFloaterIMContainerBase
	: public LLMultiFloater
{
public:
	LLFloaterIMContainerBase(const LLSD& seed, const Params& params = getDefaultParams());
	virtual ~LLFloaterIMContainerBase();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setMinimized(BOOL b);
	            void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	virtual bool isTabbedContainer() const = 0;
	virtual void showConversation(const LLUUID& session_id) = 0;
	virtual void selectConversation(const LLUUID& session_id) = 0;
	virtual bool selectConversationPair(const LLUUID& session_id, bool select_widget, bool focus_floater = true) = 0;
	virtual void expandConversation() = 0;

	static LLFloaterIMContainerBase* findInstance();
	static LLFloaterIMContainerBase* getInstance();

	static void onCurrentChannelChanged(const LLUUID& session_id);

	virtual void collapseMessagesPane(bool collapse) = 0;

	virtual const LLUUID& getSelectedSession() const = 0;
	virtual LLConversationItem* getSessionModel(const LLUUID& session_id) const = 0;
	virtual const LLConversationSort& getSortOrder() const = 0;

	virtual void onNearbyChatClosed() = 0;

	// Handling of lists of participants is public so to be common with llfloatersessiontab
	// *TODO : Find a better place for this.
	virtual bool checkContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS) = 0;
	virtual bool enableContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS) = 0;
    virtual void doToParticipants(const std::string& item, uuid_vec_t& selectedIDS) = 0;

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	void onNewMessageReceived(const LLSD& data);
protected:
	avatarID_panel_map_t& getSessionMap() { return mSessions; }

public:
	virtual void setTimeNow(const LLUUID& session_id, const LLUUID& participant_id) = 0;
	static bool isConversationLoggingAllowed();
	virtual void flashConversationItemWidget(const LLUUID& session_id, bool is_flashes) = 0;
};

#endif // LL_LLFLOATERIMCONTAINERBASE_H
