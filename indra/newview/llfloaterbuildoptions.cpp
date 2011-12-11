/** 
 * @file llfloaterbuildoptions.cpp
 * @brief LLFloaterBuildOptions class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/**
 * Panel for setting global object-editing options, specifically
 * grid size and spacing.
 */ 

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuildoptions.h"
#include "lluictrlfactory.h"

#include "llcombobox.h"
#include "llselectmgr.h"

// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2.0d) | Added: 3.2.0d
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
// [/SL:KB]

//
// Methods
//

//void commit_grid_mode(LLUICtrl *);

LLFloaterBuildOptions::LLFloaterBuildOptions(const LLSD& key)
//  : LLFloater(key),
//    mComboGridMode(NULL)
// [SL:KB] - Patch: Build-GridRuler | Checked: 2011-10-07 (Catznip-3.0.0a) | Added: Caznip-3.0.0a
  : LLFloater(key)
// [/SL:KB]
{
//	mCommitCallbackRegistrar.add("GridOptions.gridMode", boost::bind(&commit_grid_mode,_1));
}

LLFloaterBuildOptions::~LLFloaterBuildOptions()
{}

BOOL LLFloaterBuildOptions::postBuild()
{
//	mComboGridMode = getChild<LLComboBox>("combobox grid mode");

	return TRUE;
}

//void LLFloaterBuildOptions::setGridMode(EGridMode mode)
//{
//	mComboGridMode->setCurrentByIndex((S32)mode);
//}

//void LLFloaterBuildOptions::updateGridMode()
//{
//	if (mComboGridMode)
//	{
//		S32 index = mComboGridMode->getCurrentIndex();
//		mComboGridMode->removeall();
//
//		switch (mObjectSelection->getSelectType())
//		{
//		case SELECT_TYPE_HUD:
//		  mComboGridMode->add(getString("grid_screen_text"));
//		  mComboGridMode->add(getString("grid_local_text"));
//		  break;
//		case SELECT_TYPE_WORLD:
//		  mComboGridMode->add(getString("grid_world_text"));
//		  mComboGridMode->add(getString("grid_local_text"));
//		  mComboGridMode->add(getString("grid_reference_text"));
//		  break;
//		case SELECT_TYPE_ATTACHMENT:
//		  mComboGridMode->add(getString("grid_attachment_text"));
//		  mComboGridMode->add(getString("grid_local_text"));
//		  mComboGridMode->add(getString("grid_reference_text"));
//		  break;
//		}
//
//		mComboGridMode->setCurrentByIndex(index);
//	}
//}

// virtual
void LLFloaterBuildOptions::onOpen(const LLSD& key)
{
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}

// virtual
void LLFloaterBuildOptions::onClose(bool app_quitting)
{
	mObjectSelection = NULL;
}

//void commit_grid_mode(LLUICtrl *ctrl)
//{
//	LLComboBox* combo = (LLComboBox*)ctrl;
//
//	LLSelectMgr::getInstance()->setGridMode((EGridMode)combo->getCurrentIndex());
//}

// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2.0d) | Added: 3.2.0d
//
// LLFloaterBuildAxis
//

LLFloaterBuildAxis::LLFloaterBuildAxis(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

LLFloaterBuildAxis::~LLFloaterBuildAxis()
{
}

BOOL LLFloaterBuildAxis::postBuild()
{
	findChild<LLSliderCtrl>("AxisPosX")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, this, _2, 0));
	findChild<LLSliderCtrl>("AxisPosY")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, this, _2, 1));
	findChild<LLSliderCtrl>("AxisPosZ")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, this, _2, 2));

	findChild<LLSpinCtrl>("AxisOffsetX")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, this, _2, 0));
	findChild<LLSpinCtrl>("AxisOffsetY")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, this, _2, 1));
	findChild<LLSpinCtrl>("AxisOffsetZ")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, this, _2, 2));

	return TRUE;
}

void LLFloaterBuildAxis::onAxisPosChanged(const LLSD& sdValue, U32 idxAxis)
{
	if (idxAxis > 2)
		return;

	LLVector3 pos = gSavedSettings.getVector3("AxisPosition");
	pos.mV[idxAxis] = sdValue.asReal();
	gSavedSettings.setVector3("AxisPosition", pos);
}

void LLFloaterBuildAxis::onAxisOffsetChanged(const LLSD& sdValue, U32 idxAxis)
{
	if (idxAxis > 2)
		return;

	LLVector3 offset = gSavedSettings.getVector3("AxisOffset");
	offset.mV[idxAxis] = sdValue.asReal();
	gSavedSettings.setVector3("AxisOffset", offset);
}
// [/SL:KB]
