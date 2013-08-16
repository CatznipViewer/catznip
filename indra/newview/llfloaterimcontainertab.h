/** 
 * @file llfloaterimcontainertab.h
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

#ifndef LL_LLFLOATERIMCONTAINERTAB_H
#define LL_LLFLOATERIMCONTAINERTAB_H

#include "llfloaterimcontainerbase.h"

class LLFloaterIMContainerTab
	: public LLFloaterIMContainerBase
{
public:
	LLFloaterIMContainerTab(const LLSD& seed, const Params& params = getDefaultParams());
	virtual ~LLFloaterIMContainerTab();

	/*virtual*/ void setVisible(BOOL visible);

	/*virtual*/ const LLUUID& getSelectedSession() const;
	/*virtual*/ bool isTabbedContainer() const { return true; }
	/*virtual*/ void showConversation(const LLUUID& session_id);
	/*virtual*/ bool selectConversationPair(const LLUUID& session_id, bool select_widget, bool focus_floater = true);
	/*virtual*/ void setConversationFlashing(const LLUUID& session_id, bool flashing);

	/*virtual*/ LLConversationItem* getSessionModel(const LLUUID& session_id) const;
	/*virtual*/ const LLConversationSort& getSortOrder() const;
	/*virtual*/ void setTimeNow(const LLUUID& session_id, const LLUUID& participant_id);

	// Handling of lists of participants is public so to be common with llfloatersessiontab
	// *TODO : Find a better place for this.
	/*virtual*/ bool checkContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
	/*virtual*/ bool enableContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
	/*virtual*/ void doToParticipants(const std::string& item, uuid_vec_t& selectedIDS);

// [SL:KB] - Patch: UI-TabRearrange | Checked: 2012-05-05 (Catznip-3.3)
public:
	/*virtual*/ BOOL postBuild();
protected:
	void onIMTabRearrange(S32 tab_index, LLPanel* tab_panel);
// [/SL:KB]
};

#endif // LL_LLFLOATERIMCONTAINERTAB_H
