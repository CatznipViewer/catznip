/** 
 * @file llscrollingpanelparam.cpp
 * @brief UI panel for a list of visual param panels
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

#include "llviewerprecompiledheaders.h"

#include "llscrollingpanelparambase.h"
#include "llviewerjointmesh.h"
#include "llviewervisualparam.h"
#include "llwearable.h"
#include "llviewervisualparam.h"
#include "lltoolmorph.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llviewborder.h"
// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
#include "llviewerwearable.h"
// [/SL:KB]
#include "llvoavatarself.h"

//LLScrollingPanelParamBase::LLScrollingPanelParamBase( const LLPanel::Params& panel_params,
//						      LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, LLJoint* jointp, BOOL use_hints)
// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
LLScrollingPanelParamBase::LLScrollingPanelParamBase( const LLPanel::Params& panel_params,
						      LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLViewerWearable* wearable, LLJoint* jointp, BOOL use_hints)
// [/SL:KB]
	: LLScrollingPanel( panel_params ),
	  mParam(param),
	  mAllowModify(allow_modify),
	  mWearable(wearable)
{
	if (use_hints)
		buildFromFile( "panel_scrolling_param.xml");
	else
		buildFromFile( "panel_scrolling_param_base.xml");
	
	getChild<LLUICtrl>("param slider")->setValue(weightToPercent(param->getWeight()));

//	std::string display_name = LLTrans::getString(param->getDisplayName());
//	getChild<LLUICtrl>("param slider")->setLabelArg("[DESC]", display_name);
// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
	mLabel = LLTrans::getString(param->getDisplayName());
	getChild<LLUICtrl>("param slider")->setLabelArg("[DESC]", mLabel);
// [/SL:KB]
	getChildView("param slider")->setEnabled(mAllowModify);
	childSetCommitCallback("param slider", LLScrollingPanelParamBase::onSliderMoved, this);
// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
	getChild<LLSliderCtrl>("param slider")->setSliderMouseUpCallback(boost::bind(&LLScrollingPanelParamBase::refreshDirty, this));
// [/SL:KB]

	setVisible(FALSE);
	setBorderVisible( FALSE );
}

LLScrollingPanelParamBase::~LLScrollingPanelParamBase()
{
}

void LLScrollingPanelParamBase::updatePanel(BOOL allow_modify)
{
	LLViewerVisualParam* param = mParam;

	if (!mWearable)
	{
		// not editing a wearable just now, no update necessary
		return;
	}

	F32 current_weight = mWearable->getVisualParamWeight( param->getID() );
	getChild<LLUICtrl>("param slider")->setValue(weightToPercent( current_weight ) );
	mAllowModify = allow_modify;
// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
	refreshDirty();
// [/SL:KB]
	getChildView("param slider")->setEnabled(mAllowModify);
}

// [SL:KB] - Patch: Appearance-WearableChanges | Checked: Catznip-6.0
void LLScrollingPanelParamBase::refreshDirty()
{
	// Check if the new value differs from the saved value
	const F32 savedWeight = mWearable->getSavedVisualParamWeight(mParam);
	const U8 savedValue = F32_to_U8(savedWeight, mParam->getMinWeight(), mParam->getMaxWeight());
	const U8 curValue = F32_to_U8(mWearable->getVisualParamWeight(mParam->getID()), mParam->getMinWeight(), mParam->getMaxWeight());
	setDirty(savedValue != curValue);
}

void LLScrollingPanelParamBase::setDirty(bool is_dirty)
{
	if (mIsDirty != is_dirty)
	{
		mIsDirty = is_dirty;
		if (LLSliderCtrl* pCtrl = getChild<LLSliderCtrl>("param slider"))
		{
			pCtrl->setLabelColor(LLUIColorTable::instance().getColor( (mIsDirty) ? "LabelSelectedColor" : "LabelTextColor" ));
			pCtrl->setLabelArg("[DESC]", (mIsDirty) ? mLabel + " (*)" : mLabel);
			pCtrl->setEnabled(pCtrl->getEnabled());
		}
		notifyParent(LLSD().with("action", "update_dirty"));
	}
}
// [/SL:KB]

// static
void LLScrollingPanelParamBase::onSliderMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* slider = (LLSliderCtrl*) ctrl;
	LLScrollingPanelParamBase* self = (LLScrollingPanelParamBase*) userdata;
	LLViewerVisualParam* param = self->mParam;
	
	F32 current_weight = self->mWearable->getVisualParamWeight( param->getID() );
	F32 new_weight = self->percentToWeight( (F32)slider->getValue().asReal() );
	if (current_weight != new_weight )
	{
		self->mWearable->setVisualParamWeight( param->getID(), new_weight);
		self->mWearable->writeToAvatar(gAgentAvatarp);
		gAgentAvatarp->updateVisualParams();
	}
}

F32 LLScrollingPanelParamBase::weightToPercent( F32 weight )
{
	LLViewerVisualParam* param = mParam;
	return (weight - param->getMinWeight()) /  (param->getMaxWeight() - param->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParamBase::percentToWeight( F32 percent )
{
	LLViewerVisualParam* param = mParam;
	return percent / 100.f * (param->getMaxWeight() - param->getMinWeight()) + param->getMinWeight();
}

// [SL:KB] - Patch: Settings-ShapeHover | Checked: 2013-06-05 (Catznip-3.4)
const std::string& LLScrollingPanelParamBase::getParamDisplayName() const
{
	return mParam->getDisplayName();
}
// [/SL:KB]
