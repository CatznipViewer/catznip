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
