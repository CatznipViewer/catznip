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

// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2)
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
// [/SL:KB]

//
// Methods
//

LLFloaterBuildOptions::LLFloaterBuildOptions(const LLSD& key)
  : LLFloater(key)
{
}

LLFloaterBuildOptions::~LLFloaterBuildOptions()
{}

BOOL LLFloaterBuildOptions::postBuild()
{
	return TRUE;
}

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

// [SL:KB] - Patch: Build-SelectionOptions | Checked: 2013-11-06 (Catznip-3.6)
//
// LLFloaterSelectionOptions
//

LLFloaterSelectionOptions::LLFloaterSelectionOptions(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

LLFloaterSelectionOptions::~LLFloaterSelectionOptions()
{
}

BOOL LLFloaterSelectionOptions::postBuild()
{
	// NOTE-Catznip: we're relying on the fact that the control is updated before the commit signal fires (and the viewer code doesn't actually have contracts so we need to explicitly point this out)
	findChild<LLUICtrl>("RectangleSelectInclusive")->setCommitCallback(boost::bind(&LLFloaterSelectionOptions::onToggleSelectInclusive, _2));
	findChild<LLUICtrl>("RenderHiddenSelections")->setCommitCallback(boost::bind(&LLFloaterSelectionOptions::onToggleHiddenSelection, _2));
	findChild<LLUICtrl>("RenderLightRadius")->setCommitCallback(boost::bind(&LLFloaterSelectionOptions::onToggleLightRadius, _2));

	return TRUE;
}

// static
void LLFloaterSelectionOptions::onToggleSelectInclusive(const LLSD& sdValue)
{
	LLSelectMgr::sRectSelectInclusive = !LLSelectMgr::sRectSelectInclusive;
}

// static
void LLFloaterSelectionOptions::onToggleHiddenSelection(const LLSD& sdValue)
{
	LLSelectMgr::sRenderHiddenSelections = sdValue.asBoolean();
}

// static
void LLFloaterSelectionOptions::onToggleLightRadius(const LLSD& sdValue)
{
	LLSelectMgr::sRenderLightRadius = sdValue.asBoolean();
}
// [/SL:KB]

// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2)
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

void LLFloaterBuildAxis::onOpen(const LLSD& sdKey)
{
	m_AxisPosConn = gSavedSettings.getControl("AxisPosition")->getSignal()->connect(boost::bind(&LLFloaterBuildAxis::refresh, this));
	m_AxisOffsetConn = gSavedSettings.getControl("AxisOffset")->getSignal()->connect(boost::bind(&LLFloaterBuildAxis::refresh, this));
	
	refresh();
}

void LLFloaterBuildAxis::onClose(bool fQuiting)
{
	m_AxisPosConn.disconnect();
	m_AxisOffsetConn.disconnect();
}

BOOL LLFloaterBuildAxis::postBuild()
{
	findChild<LLSliderCtrl>("AxisPosX")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, _2, 0));
	findChild<LLSliderCtrl>("AxisPosY")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, _2, 1));
	findChild<LLSliderCtrl>("AxisPosZ")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosChanged, _2, 2));
	findChild<LLButton>("AxisPosCenter")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisPosCenter));

	findChild<LLSpinCtrl>("AxisOffsetX")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, _2, 0));
	findChild<LLSpinCtrl>("AxisOffsetY")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, _2, 1));
	findChild<LLSpinCtrl>("AxisOffsetZ")->setCommitCallback(boost::bind(&LLFloaterBuildAxis::onAxisOffsetChanged, _2, 2));

	return TRUE;
}

void LLFloaterBuildAxis::refresh()
{
	LLVector3 pos = gSavedSettings.getVector3("AxisPosition");
	findChild<LLSliderCtrl>("AxisPosX")->setValue(pos.mV[0]);
	findChild<LLSliderCtrl>("AxisPosY")->setValue(pos.mV[1]);
	findChild<LLSliderCtrl>("AxisPosZ")->setValue(pos.mV[2]);

	LLVector3 offset = gSavedSettings.getVector3("AxisOffset");
	findChild<LLSpinCtrl>("AxisOffsetX")->setValue(offset.mV[0]);
	findChild<LLSpinCtrl>("AxisOffsetY")->setValue(offset.mV[1]);
	findChild<LLSpinCtrl>("AxisOffsetZ")->setValue(offset.mV[2]);
}

// static
void LLFloaterBuildAxis::onAxisPosChanged(const LLSD& sdValue, U32 idxAxis)
{
	if (idxAxis > 2)
		return;

	LLVector3 pos = gSavedSettings.getVector3("AxisPosition");
	pos.mV[idxAxis] = sdValue.asReal();
	gSavedSettings.setVector3("AxisPosition", pos);
}

// static
void LLFloaterBuildAxis::onAxisPosCenter()
{
	gSavedSettings.setVector3("AxisPosition", LLVector3::zero);
}

// static
void LLFloaterBuildAxis::onAxisOffsetChanged(const LLSD& sdValue, U32 idxAxis)
{
	if (idxAxis > 2)
		return;

	LLVector3 offset = gSavedSettings.getVector3("AxisOffset");
	offset.mV[idxAxis] = sdValue.asReal();
	gSavedSettings.setVector3("AxisOffset", offset);
}
// [/SL:KB]
