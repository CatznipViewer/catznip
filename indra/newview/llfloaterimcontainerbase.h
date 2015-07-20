/** 
 *
 * Copyright (c) 2013-2014, Kitty Barnett
 * Copyright (C) 2010-2014, Linden Research, Inc.
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

#ifndef LL_LLFLOATERIMCONTAINERBASE_H
#define LL_LLFLOATERIMCONTAINERBASE_H

#include "llimview.h"
#include "llmultifloater.h"

// ============================================================================
// Forward declarations
//

class LLConversationItem;
class LLConversationSort;

// ============================================================================
// LLFloaterIMContainerBase class
//

class LLFloaterIMContainerBase
	: public LLMultiFloater
	, public LLIMSessionObserver
{
public:
	LLFloaterIMContainerBase(const LLSD& sdKey, const Params& p = getDefaultParams());
	virtual ~LLFloaterIMContainerBase();

	/*
	 * Member functions
	 */
public:
	static LLFloater*         buildFloater(const LLSD& sdKey);
	static const std::string& getFloaterXMLFile();

	static LLFloaterIMContainerBase* findInstance();
	static LLFloaterIMContainerBase* getInstance();

	enum EContainerType
	{
		CT_VIEW,	// Default v3 CHUI
		CT_TABBED,	// Legacy horizontal/vertical tab container
		CT_SEPARATE	// Legacy separate/independent IM floaters (no container)
	};
	static EContainerType getContainerType() { return sContainerType; }

protected:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t&            getSessionMap() { return mSessions; }

	/*
	 * LLView/LLMultiFloater overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setMinimized(BOOL b);
	/*virtual*/ void addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);
	/*virtual*/ void updateFloaterTitle(LLFloater* floaterp);

	/*
	 * LLIMSessionObserver overrides
	 */
public:
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg);
	/*virtual*/ void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	/*
	 * Event handlers
	 */
public:
	static void onCurrentChannelChanged(const LLUUID& session_id);
protected:
	       void onCloseFloater(const LLUUID& session_id);
// [SL:KB] - Patch: Chat-Misc | Checked: 2013-08-18 (Catznip-3.6)
	       void onSelectConversation();
// [/SL:KB]

	/*
	 * Misc
	 */
public:
	virtual const LLUUID& getSelectedSession() const = 0;
	virtual void showConversation(const LLUUID& session_id, bool focus_floater = true) = 0;
	virtual void toggleConversation(const LLUUID& session_id) = 0;
	virtual bool selectConversationPair(const LLUUID& session_id, bool select_widget, bool focus_floater = true) = 0;
	virtual void setConversationFlashing(const LLUUID& session_id, bool flashing) = 0;
	virtual void setConversationHighlighted(const LLUUID& session_id, bool is_highlighted) = 0;

	static bool                      isConversationLoggingAllowed();

	virtual LLConversationItem* getSessionModel(const LLUUID& session_id) const = 0;
	virtual const LLConversationSort& getSortOrder() const = 0;
	virtual void setTimeNow(const LLUUID& session_id, const LLUUID& participant_id) = 0;

	/*
	 * Moved over from LLFloaterIMContainerView
	 */
public:
	// Handling of lists of participants is public so to be common with llfloatersessiontab
	virtual bool checkContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
	virtual bool enableContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
	virtual void doToParticipants(const std::string& item, uuid_vec_t& selectedIDS);
// [SL:KB] - Patch: Chat-BaseGearBtn | Checked: 2014-04-10 (Catznip-3.6)
	virtual bool enableContextGroupMenuItem(const std::string& action, const LLUUID& group_id);
	virtual void doToGroup(const std::string& action, const LLUUID& group_id);
// [/SL:KB]
protected:
	void toggleMute(const LLUUID& participant_id, U32 flags);

	/*
	 * Member variables
	 */
protected:
	static EContainerType sContainerType;
private:
	avatarID_panel_map_t mSessions;
};

// ============================================================================

#endif // LL_LLFLOATERIMCONTAINERBASE_H
