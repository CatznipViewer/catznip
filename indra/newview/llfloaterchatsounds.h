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
private:
	LLFloaterChatSounds(const LLSD& sdKey);
public:
	/*virtual*/ ~LLFloaterChatSounds();

	/*virtual*/ BOOL postBuild();

protected:
	void onDropSound(const LLUUID& idSound, LLComboBox* pCombo);
	void onPreviewSound(LLComboBox* pCombo);
};

// ============================================================================

#endif  // LLFLOATERCHATSOUNDS_H
