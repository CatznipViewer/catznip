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

#include "llviewerprecompiledheaders.h"

#include "llpaneloutfitstab.h"
#include "lloutfitslist.h"
#include "lloutfitsview.h"
#include "llviewercontrol.h"

// ============================================================================

static LLPanelOutfitsTab* PanelOutfitsTabClassBuilder()
{
	LLPanelOutfitsTab* pOutfitsTab = NULL;
	if (gSavedSettings.getBOOL("UseOutfitInventoryView"))
		pOutfitsTab = new LLOutfitsView();
	else
		pOutfitsTab = new LLOutfitsList();
	return pOutfitsTab;
}

static LLPanelInjector<LLPanelOutfitsTab> t_outfits_tab("outfits_tab", PanelOutfitsTabClassBuilder);

// ============================================================================
