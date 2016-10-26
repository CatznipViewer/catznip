/** 
 * @file llfloatersidepanelcontainer.cpp
 * @brief LLFloaterSidePanelContainer class definition
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llpaneleditwearable.h"

// newview includes
#include "llsidetraypanelcontainer.h"
#include "lltransientfloatermgr.h"
#include "llpaneloutfitedit.h"
#include "llsidepanelappearance.h"
// [SL:KB] - Patch: UI-ParcelInfoFloater | Checked: 2012-08-01 (Catznip-3.3)
#include "llviewercontrol.h"
// [/SL:KB]

//static
const std::string LLFloaterSidePanelContainer::sMainPanelName("main_panel");

// [RLVa:KB] - Checked: 2012-02-07 (RLVa-1.4.5) | Added: RLVa-1.4.5
LLFloaterSidePanelContainer::validate_signal_t LLFloaterSidePanelContainer::mValidateSignal;
// [/RLVa:KB]

LLFloaterSidePanelContainer::LLFloaterSidePanelContainer(const LLSD& key, const Params& params)
:	LLFloater(key, params)
{
	// Prevent transient floaters (e.g. IM windows) from hiding
	// when this floater is clicked.
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::GLOBAL, this);
}

LLFloaterSidePanelContainer::~LLFloaterSidePanelContainer()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::GLOBAL, this);
}

void LLFloaterSidePanelContainer::onOpen(const LLSD& key)
{
	getChild<LLPanel>(sMainPanelName)->onOpen(key);
}

void LLFloaterSidePanelContainer::closeFloater(bool app_quitting)
{
//	LLPanelOutfitEdit* panel_outfit_edit =
//		dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
//	if (panel_outfit_edit)
// [SL:KB] - Patch: UI-SidePanelInstance | Checked: Catznip-3.6
	LLPanelOutfitEdit* panel_outfit_edit =
		dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::findPanel("appearance", "panel_outfit_edit"));
	if ( (panel_outfit_edit) && (panel_outfit_edit->isInVisibleChain()) )
// [/SL:KB]
	{
		LLFloater *parent = gFloaterView->getParentFloater(panel_outfit_edit);
		if (parent == this )
		{
			LLSidepanelAppearance* panel_appearance = dynamic_cast<LLSidepanelAppearance*>(getPanel("appearance"));
			if ( panel_appearance )
			{
				LLPanelEditWearable *edit_wearable_ptr = panel_appearance->getWearable();
				if (edit_wearable_ptr)
				{
					edit_wearable_ptr->onClose();
				}
				panel_appearance->showOutfitsInventoryPanel();
			}
		}
	}
	
	LLFloater::closeFloater(app_quitting);
}

LLPanel* LLFloaterSidePanelContainer::openChildPanel(const std::string& panel_name, const LLSD& params)
{
	LLView* view = findChildView(panel_name, true);
	if (!view) return NULL;

	if (!getVisible())
	{
	openFloater();
	}

	LLPanel* panel = NULL;

	LLSideTrayPanelContainer* container = dynamic_cast<LLSideTrayPanelContainer*>(view->getParent());
	if (container)
	{
		container->openPanel(panel_name, params);
		panel = container->getCurrentPanel();
	}
	else if ((panel = dynamic_cast<LLPanel*>(view)) != NULL)
	{
		panel->onOpen(params);
	}

	return panel;
}

// [RLVa:KB] - Checked: 2012-02-07 (RLVa-1.4.5) | Added: RLVa-1.4.5
bool LLFloaterSidePanelContainer::canShowPanel(const std::string& floater_name, const LLSD& key)
{
	return mValidateSignal(floater_name, sMainPanelName, key);
}

bool LLFloaterSidePanelContainer::canShowPanel(const std::string& floater_name, const std::string& panel_name, const LLSD& key)
{
	return mValidateSignal(floater_name, panel_name, key);
}
// [/RLVa:KB]
	
void LLFloaterSidePanelContainer::showPanel(const std::string& floater_name, const LLSD& key)
{
// [SL:KB] - Patch: UI-ParcelInfoFloater | Checked: 2012-08-01 (Catznip-3.3)
	// Hack in case we forget a reference somewhere
	if ( ("places" == floater_name) && (key.has("type")) && (gSavedSettings.getBOOL("ShowPlaceFloater")) )
	{
		const std::string strType = key["type"].asString();
		if ( ("remote_place" == strType) || ("landmark" == strType) )
		{
			LLFloaterReg::showInstance("parcel_info", key);
#if !LL_RELEASE_FOR_DOWNLOAD
			LL_ERRS() << "Left-over reference to the places sidepanel" << LL_ENDL;
#endif // LL_RELEASE_FOR_DOWNLOAD
			return;
		}
	}
// [/SL:KB]

	LLFloaterSidePanelContainer* floaterp = LLFloaterReg::getTypedInstance<LLFloaterSidePanelContainer>(floater_name);
//	if (floaterp)
// [RLVa:KB] - Checked: 2013-04-16 (RLVa-1.4.8)
	if ( (floaterp) && ((floaterp->getVisible()) || (LLFloaterReg::canShowInstance(floater_name, key))) && (canShowPanel(floater_name, key)) )
// [/RLVa:KB]
	{
		floaterp->openChildPanel(sMainPanelName, key);
	}
}

void LLFloaterSidePanelContainer::showPanel(const std::string& floater_name, const std::string& panel_name, const LLSD& key)
{
// [SL:KB] - Patch: World-Derender | Checked: 2011-12-15 (Catznip-3.2)
	// Hack in case we forget a reference somewhere
	if ( (!panel_name.empty()) && ("panel_people" == panel_name) && (key.has("people_panel_tab_name")) && ("blocked_panel" == key["people_panel_tab_name"].asString()) )
	{
#ifndef LL_RELEASE_FOR_DOWNLOAD
		LL_ERRS() << "Request to open the blocked floater through the sidepanel!" << LL_ENDL;
#endif // LL_RELEASE_FOR_DOWNLOAD
		LLFloaterReg::showInstance("blocked", key);
		return;
	}
// [/SL:KB]

	LLFloaterSidePanelContainer* floaterp = LLFloaterReg::getTypedInstance<LLFloaterSidePanelContainer>(floater_name);
//	if (floaterp)
// [RLVa:KB] - Checked: 2013-04-16 (RLVa-1.4.8)
	if ( (floaterp) && ((floaterp->getVisible()) || (LLFloaterReg::canShowInstance(floater_name, key))) && (canShowPanel(floater_name, panel_name, key)) )
// [/RLVa:KB]
	{
		floaterp->openChildPanel(panel_name, key);
	}
}

// [SL:KB] - Patch: UI-SidePanelInstance | Checked: Catznip-3.4
LLPanel* LLFloaterSidePanelContainer::findPanel(const std::string& floater_name, const std::string& panel_name)
{
	LLFloaterSidePanelContainer* floaterp = LLFloaterReg::findTypedInstance<LLFloaterSidePanelContainer>(floater_name);
	return (floaterp) ? floaterp->findChild<LLPanel>(panel_name, true) : NULL;
}
// [/SL:KB]

LLPanel* LLFloaterSidePanelContainer::getPanel(const std::string& floater_name, const std::string& panel_name)
{
	LLFloaterSidePanelContainer* floaterp = LLFloaterReg::getTypedInstance<LLFloaterSidePanelContainer>(floater_name);

	if (floaterp)
	{
		return floaterp->findChild<LLPanel>(panel_name, true);
	}

	return NULL;
}
