/** 
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llfloatercamera.h"

// Library includes
#include "llfloaterreg.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
#include "llflatbutton.h"
// [/SL:KB]
#include "llpresetsmanager.h"
#include "lljoystickbutton.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llslider.h"
#include "llfirstuse.h"
#include "llhints.h"
#include "lltabcontainer.h"
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
#include "llviewermenu.h"
// [/SL:KB]
#include "llvoavatarself.h"

//static LLDefaultChildRegistry::Register<LLPanelCameraItem> r("panel_camera_item");

const F32 NUDGE_TIME = 0.25f;		// in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

// constants
#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define ZOOM "zoom"
#define CONTROLS "controls"

bool LLFloaterCamera::sFreeCamera = false;
bool LLFloaterCamera::sAppearanceEditing = false;

// Zoom the camera in and out
class LLPanelCameraZoom
:	public LLPanel
{
	LOG_CLASS(LLPanelCameraZoom);
public:
	LLPanelCameraZoom();

	/* virtual */ BOOL	postBuild();
	/* virtual */ void	draw();

protected:
	void	onZoomPlusHeldDown();
	void	onZoomMinusHeldDown();
	void	onSliderValueChanged();
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	void	onViewHeldDown(F32 nMult);
	void	onViewSliderChanged();
// [/SL:KB]
	void	onCameraTrack();
	void	onCameraRotate();
	F32		getOrbitRate(F32 time);

private:
	LLButton*	mPlusBtn;
	LLButton*	mMinusBtn;
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	LLSlider*	mZoomSlider = nullptr;
	LLSlider*	mViewSlider = nullptr;
	LLTimer m_CameraSyncTimer;
// [/SL:KB]
//	LLSlider*	mSlider;
};

//LLPanelCameraItem::Params::Params()
//:	icon_over("icon_over"),
//	icon_selected("icon_selected"),
//	picture("picture"),
//	text("text"),
//	selected_picture("selected_picture"),
//	mousedown_callback("mousedown_callback")
//{
//}

//LLPanelCameraItem::LLPanelCameraItem(const LLPanelCameraItem::Params& p)
//:	LLPanel(p)
//{
//	LLIconCtrl::Params icon_params = p.picture;
//	mPicture = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
//	addChild(mPicture);
//
//	icon_params = p.icon_over;
//	mIconOver = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
//	addChild(mIconOver);
//
//	icon_params = p.icon_selected;
//	mIconSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
//	addChild(mIconSelected);
//
//	icon_params = p.selected_picture;
//	mPictureSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
//	addChild(mPictureSelected);
//
//	LLTextBox::Params text_params = p.text;
//	mText = LLUICtrlFactory::create<LLTextBox>(text_params);
//	addChild(mText);
//
//	if (p.mousedown_callback.isProvided())
//	{
//		setCommitCallback(initCommitCallback(p.mousedown_callback));
//	}
//}

//void set_view_visible(LLView* parent, const std::string& name, bool visible)
//{
//	parent->getChildView(name)->setVisible(visible);
//}

//BOOL LLPanelCameraItem::postBuild()
//{
//	setMouseEnterCallback(boost::bind(set_view_visible, this, "hovered_icon", true));
//	setMouseLeaveCallback(boost::bind(set_view_visible, this, "hovered_icon", false));
//	setMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
//	setRightMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
//	return TRUE;
//}

//void LLPanelCameraItem::onAnyMouseClick()
//{
//	if (mCommitSignal) (*mCommitSignal)(this, LLSD());
//}

//void LLPanelCameraItem::setValue(const LLSD& value)
//{
//	if (!value.isMap()) return;;
//	if (!value.has("selected")) return;
//	getChildView("selected_icon")->setVisible( value["selected"]);
//	getChildView("picture")->setVisible( !value["selected"]);
//	getChildView("selected_picture")->setVisible( value["selected"]);
//}

static LLPanelInjector<LLPanelCameraZoom> t_camera_zoom_panel("camera_zoom_panel");

//-------------------------------------------------------------------------------
// LLPanelCameraZoom
//-------------------------------------------------------------------------------

LLPanelCameraZoom::LLPanelCameraZoom()
:	mPlusBtn( NULL ),
	mMinusBtn( NULL )
//	mSlider( NULL )
{
	mCommitCallbackRegistrar.add("Zoom.minus", boost::bind(&LLPanelCameraZoom::onZoomMinusHeldDown, this));
	mCommitCallbackRegistrar.add("Zoom.plus", boost::bind(&LLPanelCameraZoom::onZoomPlusHeldDown, this));
	mCommitCallbackRegistrar.add("Slider.value_changed", boost::bind(&LLPanelCameraZoom::onSliderValueChanged, this));
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	mCommitCallbackRegistrar.add("View.minus", std::bind(&LLPanelCameraZoom::onViewHeldDown, this, -1));
	mCommitCallbackRegistrar.add("View.plus", std::bind(&LLPanelCameraZoom::onViewHeldDown, this, 1));
	mCommitCallbackRegistrar.add("View.slider", std::bind(&LLPanelCameraZoom::onViewSliderChanged, this));
// [/SL:KB]
	mCommitCallbackRegistrar.add("Camera.track", boost::bind(&LLPanelCameraZoom::onCameraTrack, this));
	mCommitCallbackRegistrar.add("Camera.rotate", boost::bind(&LLPanelCameraZoom::onCameraRotate, this));
}

BOOL LLPanelCameraZoom::postBuild()
{
	mPlusBtn  = getChild <LLButton> ("zoom_plus_btn");
	mMinusBtn = getChild <LLButton> ("zoom_minus_btn");
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	mZoomSlider = getChild<LLSlider>("zoom_slider");
	mViewSlider = getChild<LLSlider>("view_slider");
// [/SL:KB]
//	mSlider   = getChild <LLSlider> ("zoom_slider");
	return LLPanel::postBuild();
}

void LLPanelCameraZoom::draw()
{
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	if (m_CameraSyncTimer.hasExpired())
	{
		F32 zoom_level = gAgentCamera.getCameraZoomFraction();
		zoom_level = 1 - sqrt(1.f - zoom_level);
		mZoomSlider->setValue(zoom_level);

		// NOTE: the FoV value is inverted since most people end up using it as a secondary zoom slider and a tighter (= lower) FoV "zooms" in
		//       by pressing the minus button which feels wrong so we flip the values around instead
		const LLViewerCamera* pCamera = LLViewerCamera::getInstance();
		mViewSlider->setMinValue(pCamera->getMinView());
		mViewSlider->setMaxValue(pCamera->getMaxView());
		mViewSlider->setDefaultValue(pCamera->getMaxView() - DEFAULT_FIELD_OF_VIEW + pCamera->getMinView());
		mViewSlider->setValue(pCamera->getMaxView() - pCamera->getView() + pCamera->getMinView());

		m_CameraSyncTimer.setTimerExpirySec(1.f / 15);
	}
// [/SL:KB]
//	mSlider->setValue(gAgentCamera.getCameraZoomFraction());
	LLPanel::draw();
}

void LLPanelCameraZoom::onZoomPlusHeldDown()
{
//	F32 val = mSlider->getValueF32();
//	F32 inc = mSlider->getIncrement();
//	mSlider->setValue(val - inc);
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	F32 val = mZoomSlider->getValueF32();
	F32 inc = mZoomSlider->getIncrement();
	mZoomSlider->setValue(val - inc);
// [/SL:KB]
	F32 time = mPlusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitInKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onZoomMinusHeldDown()
{
//	F32 val = mSlider->getValueF32();
//	F32 inc = mSlider->getIncrement();
//	mSlider->setValue(val + inc);
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	F32 val = mZoomSlider->getValueF32();
	F32 inc = mZoomSlider->getIncrement();
	mZoomSlider->setValue(val + inc);
// [/SL:KB]
	F32 time = mMinusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitOutKey(getOrbitRate(time));
}

// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
void LLPanelCameraZoom::onViewHeldDown(F32 nMult)
{
	F32 val = mViewSlider->getValueF32();
	F32 inc = mViewSlider->getIncrement();
	mViewSlider->setValue(val + nMult * inc);
	onViewSliderChanged();
}
// [/SL:KB]

void LLPanelCameraZoom::onCameraTrack()
{
	// EXP-202 when camera panning activated, remove the hint
	LLFirstUse::viewPopup( false );
}

void LLPanelCameraZoom::onCameraRotate()
{
	// EXP-202 when camera rotation activated, remove the hint
	LLFirstUse::viewPopup( false );
}

F32 LLPanelCameraZoom::getOrbitRate(F32 time)
{
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1;
	}
}

void  LLPanelCameraZoom::onSliderValueChanged()
{
//	F32 zoom_level = mSlider->getValueF32();
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	F32 zoom_level = mZoomSlider->getValueF32();
	zoom_level = -zoom_level * (zoom_level - 2);
// [/SL:KB]
	gAgentCamera.setCameraZoomFraction(zoom_level);
}

// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
void LLPanelCameraZoom::onViewSliderChanged()
{
	LLViewerCamera* pCamera = LLViewerCamera::getInstance();
	pCamera->setDefaultFOV(pCamera->getMaxView() - mViewSlider->getValueF32() + pCamera->getMinView());
//	gSavedSettings.setF32("CameraAngle", pCamera->getView()); // setView may have clamped it.
}
// [/SL:KB]

void activate_camera_tool()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
};

//
// Member functions
//

/*static*/ bool LLFloaterCamera::inFreeCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
	{
		return true;
	}
	return false;
}

void LLFloaterCamera::resetCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->switchMode(CAMERA_CTRL_MODE_PAN);
}

void LLFloaterCamera::onAvatarEditingAppearance(bool editing)
{
	sAppearanceEditing = editing;
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
//	floater_camera->handleAvatarEditingAppearance(editing);
}

//void LLFloaterCamera::handleAvatarEditingAppearance(bool editing)
//{
//
//}

void LLFloaterCamera::update()
{
	ECameraControlMode mode = determineMode();
	if (mode != mCurrMode) setMode(mode);
}


void LLFloaterCamera::toPrevMode()
{
	switchMode(mPrevMode);
}

/*static*/ void LLFloaterCamera::onLeavingMouseLook()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera)
	{
		floater_camera->updateItemsSelection();
		if(floater_camera->inFreeCameraMode())
		{
			activate_camera_tool();
		}
	}
}

LLFloaterCamera* LLFloaterCamera::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
}

void LLFloaterCamera::onOpen(const LLSD& key)
{
	LLFirstUse::viewPopup();

	mZoom->onOpen(key);

	// Returns to previous mode, see EXT-2727(View tool should remember state).
	// In case floater was just hidden and it isn't reset the mode
	// just update state to current one. Else go to previous.
	if ( !mClosed )
		updateState();
	else
		toPrevMode();
	mClosed = FALSE;

	populatePresetCombo();
}

void LLFloaterCamera::onClose(bool app_quitting)
{
	//We don't care of camera mode if app is quitting
	if(app_quitting)
		return;
	// It is necessary to reset mCurrMode to CAMERA_CTRL_MODE_PAN so 
	// to avoid seeing an empty floater when reopening the control.
	if (mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA)
		mCurrMode = CAMERA_CTRL_MODE_PAN;
	// When mCurrMode is in CAMERA_CTRL_MODE_PAN
	// switchMode won't modify mPrevMode, so force it here.
	// It is needed to correctly return to previous mode on open, see EXT-2727.
	if (mCurrMode == CAMERA_CTRL_MODE_PAN)
		mPrevMode = CAMERA_CTRL_MODE_PAN;

	switchMode(CAMERA_CTRL_MODE_PAN);
	mClosed = TRUE;

	gAgent.setMovementLocked(FALSE);
}

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLFloater(val),
	mClosed(FALSE),
	mCurrMode(CAMERA_CTRL_MODE_PAN),
	mPrevMode(CAMERA_CTRL_MODE_PAN)
{
	LLHints::getInstance()->registerHintTarget("view_popup", getHandle());
	mCommitCallbackRegistrar.add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraItem, _2));
//	mCommitCallbackRegistrar.add("CameraPresets.Save", boost::bind(&LLFloaterCamera::onSavePreset, this));
	mCommitCallbackRegistrar.add("CameraPresets.ShowPresetsList", boost::bind(&LLFloaterReg::showInstance, "camera_presets", LLSD(), FALSE));
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)

	mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
	mZoom = findChild<LLPanelCameraZoom>(ZOOM);
	mTrack = getChild<LLJoystickCameraTrack>(PAN);
	mPresetCombo = getChild<LLComboBox>("preset_combo");

	getChild<LLTextBox>("precise_ctrs_label")->setShowCursorHand(false);
	getChild<LLTextBox>("precise_ctrs_label")->setSoundFlags(LLView::MOUSE_UP);
	getChild<LLTextBox>("precise_ctrs_label")->setClickedCallback(boost::bind(&LLFloaterReg::showInstance, "prefs_view_advanced", LLSD(), FALSE));

	mPresetCombo->setCommitCallback(boost::bind(&LLFloaterCamera::onCustomPresetSelected, this));
	LLPresetsManager::getInstance()->setPresetListChangeCameraCallback(boost::bind(&LLFloaterCamera::populatePresetCombo, this));

	update();

//	// ensure that appearance mode is handled while building. See EXT-7796.
//	handleAvatarEditingAppearance(sAppearanceEditing);

	return LLFloater::postBuild();
}

F32	LLFloaterCamera::getCurrentTransparency()
{
// [SL:KB] - Patch: UI-Misc | Checked: 2014-04-23 (Catznip-3.6)
	static LLCachedControl<F32> sChromeFloaterTransparency(gSavedSettings, "ChromeFloaterTransparency");
	return sChromeFloaterTransparency;
// [/SL:KB]

//	static LLCachedControl<F32> camera_opacity(gSavedSettings, "CameraOpacity");
//	static LLCachedControl<F32> active_floater_transparency(gSavedSettings, "ActiveFloaterTransparency");
//	return llmin(camera_opacity(), active_floater_transparency());

}

void LLFloaterCamera::fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel)
{
	// copying child list and then iterating over a copy, because list itself
	// is changed in process
	const child_list_t child_list = *panel->getChildList();
	child_list_t::const_reverse_iterator iter = child_list.rbegin();
	child_list_t::const_reverse_iterator end = child_list.rend();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanel* item = dynamic_cast<LLPanel*>(view);
		if (panel)
			list->addItem(item);
	}

}

ECameraControlMode LLFloaterCamera::determineMode()
{
	if (sAppearanceEditing)
	{
		// this is the only enabled camera mode while editing agent appearance.
		return CAMERA_CTRL_MODE_PAN;
	}

	LLTool* curr_tool = LLToolMgr::getInstance()->getCurrentTool();
	if (curr_tool == LLToolCamera::getInstance())
	{
		return CAMERA_CTRL_MODE_FREE_CAMERA;
	} 

	if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
	{
		return CAMERA_CTRL_MODE_PRESETS;
	}

	return CAMERA_CTRL_MODE_PAN;
}


void clear_camera_tool()
{
	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	if (tool_mgr->usingTransientTool() && 
		tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
	{
		tool_mgr->clearTransientTool();
	}
}


void LLFloaterCamera::setMode(ECameraControlMode mode)
{
	if (mode != mCurrMode)
	{
		mPrevMode = mCurrMode;
		mCurrMode = mode;
	}
	
	updateState();
}

void LLFloaterCamera::switchMode(ECameraControlMode mode)
{
	setMode(mode);

	switch (mode)
	{
	case CAMERA_CTRL_MODE_PAN:
		sFreeCamera = false;
		clear_camera_tool();
		break;

	case CAMERA_CTRL_MODE_FREE_CAMERA:
		sFreeCamera = true;
		activate_camera_tool();
		break;

	case CAMERA_CTRL_MODE_PRESETS:
		if(sFreeCamera)
		{
			switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
		}
		break;

	default:
		//normally we won't occur here
		llassert_always(FALSE);
	}
}

void LLFloaterCamera::updateState()
{
	updateItemsSelection();

	if (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode)
	{
		return;
	}

	//updating buttons
	std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
	for (; iter != mMode2Button.end(); ++iter)
	{
		iter->second->setToggleState(iter->first == mCurrMode);
	}
}

void LLFloaterCamera::updateItemsSelection()
{
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
//	findChild<LLFlatButton>("normal_view")->setValue(mCurrMode == CAMERA_CTRL_MODE_PAN);
	findChild<LLFlatButton>("object_view")->setValue(mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA);
	findChild<LLFlatButton>("mouselook_view")->setValue(gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK);
// [/SL:KB]
//	ECameraPreset preset = (ECameraPreset) gSavedSettings.getU32("CameraPresetType");
//	LLSD argument;
//	argument["selected"] = (preset == CAMERA_PRESET_REAR_VIEW) && !sFreeCamera;
//	getChild<LLPanelCameraItem>("rear_view")->setValue(argument);
//	argument["selected"] = (preset == CAMERA_PRESET_GROUP_VIEW) && !sFreeCamera;
//	getChild<LLPanelCameraItem>("group_view")->setValue(argument);
//	argument["selected"] = (preset == CAMERA_PRESET_FRONT_VIEW) && !sFreeCamera;
//	getChild<LLPanelCameraItem>("front_view")->setValue(argument);
//	argument["selected"] = gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK;
//	getChild<LLPanelCameraItem>("mouselook_view")->setValue(argument);
//	argument["selected"] = mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA;
//	getChild<LLPanelCameraItem>("object_view")->setValue(argument);
}

void LLFloaterCamera::onClickCameraItem(const LLSD& param)
{
	std::string name = param.asString();

	if ("mouselook_view" == name)
	{
		gAgentCamera.changeCameraToMouselook();
	}
	else if ("object_view" == name)
	{
		LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
		if (camera_floater)
		{
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
			if (camera_floater->mCurrMode != CAMERA_CTRL_MODE_FREE_CAMERA)
			{
// [/SL:KB]
				camera_floater->switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
				camera_floater->updateItemsSelection();
				camera_floater->fromFreeToPresets();
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
			}
			else
			{
				handle_reset_view();
			}
// [/SL:KB]
		}
	}
	else
	{
		LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
		if (camera_floater)
			camera_floater->switchMode(CAMERA_CTRL_MODE_PAN);
		switchToPreset(name);
	}
}

/*static*/
void LLFloaterCamera::switchToPreset(const std::string& name)
{
	sFreeCamera = false;
	clear_camera_tool();
	if (PRESETS_REAR_VIEW == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
	}
	else if (PRESETS_SIDE_VIEW == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_GROUP_VIEW);
	}
	else if (PRESETS_FRONT_VIEW == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_FRONT_VIEW);
	}
	else
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_CUSTOM);
	}
	
//	if (gSavedSettings.getString("PresetCameraActive") != name)
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	if ( (gSavedSettings.getString("PresetCameraActive") != name) || (LLPresetsManager::instance().isCameraDirty()) )
// [/SL:KB]
	{
		LLPresetsManager::getInstance()->loadPreset(PRESETS_CAMERA, name);
	}

	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLQuaternion sit_rot(gSavedSettings.getLLSD("AvatarSitRotation"));
		if (sit_rot != LLQuaternion())
		{
			gAgent.rotate(~gAgent.getFrameAgent().getQuaternion());
			gAgent.rotate(sit_rot);
		}
		else
		{
			gAgentCamera.rotateToInitSitRot();
		}
	}
	gAgentCamera.resetCameraZoomFraction();

	LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
	if (camera_floater)
	{
		camera_floater->updateItemsSelection();
		camera_floater->fromFreeToPresets();
	}
}

void LLFloaterCamera::fromFreeToPresets()
{
	if (!sFreeCamera && mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && mPrevMode == CAMERA_CTRL_MODE_PRESETS)
	{
		switchMode(CAMERA_CTRL_MODE_PRESETS);
	}
}

void LLFloaterCamera::populatePresetCombo()
{
//	LLPresetsManager::getInstance()->setPresetNamesInComboBox(PRESETS_CAMERA, mPresetCombo, EDefaultOptions::DEFAULT_HIDE);
// [SL:KB] - Patch: Control-FlatButton | Checked: Catznip-6.4
	LLPresetsManager::getInstance()->setPresetNamesInComboBox(PRESETS_CAMERA, mPresetCombo, EDefaultOptions::DEFAULT_SHOW);
// [/SL:KB]
	std::string active_preset_name = gSavedSettings.getString("PresetCameraActive");
// [SL:KB] - Patch: Control-FlatButton | Checked: Catznip-6.4
	if ( (active_preset_name.empty()) || (LLPresetsManager::instance().isCameraDirty()) )
	{
		gSavedSettings.setU32("CameraPresetType", CAMERA_PRESET_CUSTOM);
		updateItemsSelection();
		mPresetCombo->setLabel(getString("custom_combo_text"));
	}
// [/SL:KB]
//	if (active_preset_name.empty())
//	{
//		gSavedSettings.setU32("CameraPresetType", CAMERA_PRESET_CUSTOM);
//		updateItemsSelection();
//		mPresetCombo->setLabel(getString("inactive_combo_text"));
//	}
//	else if ((ECameraPreset)gSavedSettings.getU32("CameraPresetType") == CAMERA_PRESET_CUSTOM)
// [SL:KB] - Patch: Control-FlatButton | Checked: Catznip-6.4
	else
// [/SL:KB]
	{
		mPresetCombo->selectByValue(active_preset_name);
	}
//	else
//	{
//		mPresetCombo->setLabel(getString("inactive_combo_text"));
//	}
	updateItemsSelection();
}

//void LLFloaterCamera::onSavePreset()
//{
//	LLFloaterReg::hideInstance("delete_pref_preset", PRESETS_CAMERA);
//	LLFloaterReg::hideInstance("load_pref_preset", PRESETS_CAMERA);
//	
//	LLFloaterReg::showInstance("save_camera_preset");
//}

void LLFloaterCamera::onCustomPresetSelected()
{
	std::string selected_preset = mPresetCombo->getSelectedItemLabel();
//	if (getString("inactive_combo_text") != selected_preset)
// [SL:KB] - Patch: Control-FlatButton | Checked: Catznip-6.4
	if (-1 != mPresetCombo->getFirstSelectedIndex())
// [/SL:KB]
	{
		switchToPreset(selected_preset);
	}
}
