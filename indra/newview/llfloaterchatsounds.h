/** 
 *
 * Copyright (c) 2013, Kitty Barnett
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
#ifndef LLFLOATERCHATSOUNDS_H
#define LLFLOATERCHATSOUNDS_H

#include "llfloater.h"

class LLComboBox;

// ============================================================================
// LLFloaterChatSounds class
//

class LLFloaterChatSounds : public LLFloater
{
	friend class LLFloaterReg;
protected:
	LLFloaterChatSounds(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterChatSounds();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions/Event handlers
	 */
protected:
	void initComboCallbacks(LLComboBox* pCombo);
	void onDropSound(const LLUUID& idSound, LLComboBox* pCombo);
	void onInitSoundsCombo(LLUICtrl* pCtrl, const LLSD& sdParam);
	void onPreviewSound(LLComboBox* pCombo);
	void onSelectSound(const LLUICtrl* pCtrl);

	/*
	 * Member variables
	 */
protected:
	LLSD m_sdSounds;

	typedef std::map<std::string, std::string> setttings_map_t;
	setttings_map_t m_SettingsMap;
};

// ============================================================================

#endif  // LLFLOATERCHATSOUNDS_H
