/** 
 *
 * Copyright (c) 2010, Kitty Barnett
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
#ifndef LL_LLPANELOUTFITSTAB_H
#define LL_LLPANELOUTFITSTAB_H

#include "llpanelappearancetab.h"

// ============================================================================
// LLPanelOutfitsTab - Pure virtual base class for the different outfit views
//

class LLPanelOutfitsTab : public LLPanelAppearanceTab
{
public:
	LLPanelOutfitsTab() : LLPanelAppearanceTab() {}
	/*virtual*/ ~LLPanelOutfitsTab() {}

	typedef boost::function<void (const LLUUID&)> selection_change_callback_t;
	typedef boost::signals2::signal<void (const LLUUID&)> selection_change_signal_t;

	virtual bool hasItemSelected() = 0;
	virtual void performAction(std::string action) = 0;
	virtual void removeSelected() = 0;
	virtual void setSelectedOutfitByUUID(const LLUUID& outfit_uuid) = 0;
	virtual void wearSelectedItems() = 0;
	virtual boost::signals2::connection setSelectionChangeCallback(selection_change_callback_t cb) = 0;
};

// ============================================================================

#endif // LL_LLPANELOUTFITSTAB_H
