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

#include "lloutfitsview.h"

static LLRegisterPanelClassWrapper<LLOutfitsView> t_outfits_view("outfits_view");

LLOutfitsView::LLOutfitsView()
	:	LLPanelOutfitsTab()
{
}

LLOutfitsView::~LLOutfitsView()
{
}

//virtual
BOOL LLOutfitsView::postBuild()
{
	return TRUE;
}

//virtual
void LLOutfitsView::onOpen(const LLSD& /*info*/)
{
}

// virtual
void LLOutfitsView::setFilterSubString(const std::string& string)
{
}

// virtual
bool LLOutfitsView::isActionEnabled(const LLSD& userdata)
{
	return false;
}

void LLOutfitsView::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
}

void LLOutfitsView::performAction(std::string action)
{
}

void LLOutfitsView::removeSelected()
{
}

void LLOutfitsView::setSelectedOutfitByUUID(const LLUUID& outfit_uuid)
{
}

void LLOutfitsView::wearSelectedItems()
{
}

bool LLOutfitsView::hasItemSelected()
{
	return false;
}

boost::signals2::connection LLOutfitsView::setSelectionChangeCallback(selection_change_callback_t cb)
{
	return mSelectionChangeSignal.connect(cb);
}

// EOF
