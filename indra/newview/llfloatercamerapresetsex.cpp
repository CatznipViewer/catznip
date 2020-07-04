/**
 *
 * Copyright (c) 2020, Kitty Barnett
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

#include "llagentcamera.h"
#include "llcheckboxctrl.h"
#include "llflatlistview.h"
#include "llfloatercamera.h"
#include "llfloatercamerapresetsex.h"
#include "llflyoutbutton.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"

// ============================================================================
// LLPanelCameraPresetItem class
//

class LLPanelCameraPresetItem : public LLPanel
{
public:
	LLPanelCameraPresetItem(const LLSD& sdValue);
	~LLPanelCameraPresetItem() override;

	/*
	 * LLView overrides
	 */
public:
	void onMouseEnter(S32 x, S32 y, MASK mask) override;
	void onMouseLeave(S32 x, S32 y, MASK mask) override;
	BOOL postBuild() override;
	void setValue(const LLSD& sdValue) override;

	/*
	 * Member functions
	 */
public:
	void setDeleteCallback(const commit_signal_t::slot_type& cb);
	void setResetCallback(const commit_signal_t::slot_type& cb);
protected:
	void refreshControls();

	/*
	 * Member variables
	 */
protected:
	bool m_fIsSelected = false;
	bool m_fIsHovered = false;
	LLButton* m_pDeleteBtn = nullptr;
	LLButton* m_pResetBtn = nullptr;
};

LLPanelCameraPresetItem::LLPanelCameraPresetItem(const LLSD& sdValue)
	: LLPanel()
{
	buildFromFile("panel_camera_preset_item_ex.xml");

	setValue(sdValue);
}

LLPanelCameraPresetItem::~LLPanelCameraPresetItem()
{
}

// override
BOOL LLPanelCameraPresetItem::postBuild()
{
	m_pDeleteBtn = findChild<LLButton>("delete_btn");
	m_pResetBtn = findChild<LLButton>("reset_btn");

	return TRUE;
}

// override
void LLPanelCameraPresetItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	m_fIsHovered = true;
	refreshControls();

	LLPanel::onMouseEnter(x, y, mask);
}

// override
void LLPanelCameraPresetItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	m_fIsHovered = false;
	refreshControls();

	LLPanel::onMouseLeave(x, y, mask);
}

// override
void LLPanelCameraPresetItem::setValue(const LLSD& sdValue)
{
	if (!sdValue.isMap())
		return;

	if ( (sdValue.size() == 1) && (sdValue.has("selected")) )
	{
		m_fIsSelected = sdValue["selected"];
	}

	if ( (sdValue.has("name")) && (sdValue.has("is_default")) )
	{
		findChild<LLTextBox>("preset_name")->setText(sdValue["name"].asStringRef());
		LLPanel::setValue(sdValue);
	}

	refreshControls();
}

void LLPanelCameraPresetItem::refreshControls()
{
	findChild<LLIconCtrl>("hovered_icon")->setVisible(m_fIsHovered);
	findChild<LLIconCtrl>("selected_icon")->setVisible(m_fIsSelected);

	bool fIsDefault = getValue()["is_default"].asBoolean();
	m_pDeleteBtn->setVisible(!fIsDefault && (m_fIsHovered | m_fIsSelected));
	m_pResetBtn->setVisible(fIsDefault && (m_fIsHovered | m_fIsSelected));
}

void LLPanelCameraPresetItem::setDeleteCallback(const commit_signal_t::slot_type& cb)
{
	m_pDeleteBtn->setCommitCallback(cb);
}

void LLPanelCameraPresetItem::setResetCallback(const commit_signal_t::slot_type& cb)
{
	m_pResetBtn->setCommitCallback(cb);
}

// ============================================================================
// LLFloaterCameraPresetsEx class
//

LLFloaterCameraPresetsEx::LLFloaterCameraPresetsEx(const LLSD& sdKey)
	: LLFloater(sdKey)
{
	mCommitCallbackRegistrar.add("CameraPresets.CommitSettings", std::bind(&LLFloaterCameraPresetsEx::onCommitSettings, this));
	mCommitCallbackRegistrar.add("CameraPresets.New", std::bind(&LLFloaterCameraPresetsEx::onNewPreset, this));
}

LLFloaterCameraPresetsEx::~LLFloaterCameraPresetsEx()
{
}

BOOL LLFloaterCameraPresetsEx::postBuild()
{
	m_pPresetsList = findChild<LLFlatListView>("preset_list");
	m_pPresetsList->setCommitCallback(std::bind(&LLFloaterCameraPresetsEx::onPresetClicked, this));
	m_pPresetsList->setCommitOnUserSelection(true);
	m_pNewBtn = findChild<LLButton>("new_btn");

	m_pEditPanel = findChild<LLPanel>("edit_panel");

	m_pNameCtrl = findChild<LLLineEditor>("edit_name");
	m_pCamOffsetXCtrl = findChild<LLSpinCtrl>("edit_camoffset_x");
	m_pCamOffsetYCtrl = findChild<LLSpinCtrl>("edit_camoffset_y");
	m_pCamOffsetZCtrl = findChild<LLSpinCtrl>("edit_camoffset_z");
	m_pFocusOffsetXCtrl = findChild<LLSpinCtrl>("edit_focusoffset_x");
	m_pFocusOffsetYCtrl = findChild<LLSpinCtrl>("edit_focusoffset_y");
	m_pFocusOffsetZCtrl = findChild<LLSpinCtrl>("edit_focusoffset_z");
	m_pFocusScaleSliderCtrl = findChild<LLSliderCtrl>("edit_offset_slider");
	m_pFocusScaleSpinCtrl = findChild<LLSpinCtrl>("edit_offset_spin");
	m_pCamFovSliderCtrl = findChild<LLSliderCtrl>("edit_fov_slider");
	m_pCamFovSpinCtrl = findChild<LLSpinCtrl>("edit_fov_spin");
	m_pSaveBtn = findChild<LLFlyoutButton>("save_btn");
	m_pSaveBtn->setCommitCallback(std::bind(&LLFloaterCameraPresetsEx::onPresetSave, this));
	m_pSyncCameraCtrl = findChild<LLCheckBoxCtrl>("camera_sync_check");
	m_pSyncCameraCtrl->setCommitCallback(std::bind(&LLFloaterCameraPresetsEx::onToggleSyncWithCamera, this));

	// Catch when the user changes the active preset through the dropdown
	m_PresetCameraActiveConn = gSavedSettings.getControl("PresetCameraActive")->getSignal()->connect(std::bind(&LLFloaterCameraPresetsEx::onPresetChanged, this, std::placeholders::_2));

	refreshPresets();

	return TRUE;
}

void LLFloaterCameraPresetsEx::draw()
{
	if ( (m_CameraSyncTimer.hasExpired()) && (m_pSyncCameraCtrl->get()) && (gAgentCamera.isJoystickCameraUsed()) )
	{
		static LLCachedControl<F32> sCamOffsetScale(gSavedSettings, "CameraOffsetScale");
		gSavedSettings.setVector3("CameraOffsetRearView", gAgentCamera.getCurrentCameraOffset() / ( (sCamOffsetScale) ? sCamOffsetScale : 1.f));
		gSavedSettings.setVector3d("FocusOffsetRearView", gAgentCamera.getCurrentFocusOffset());
		refreshEditControls();
		m_CameraSyncTimer.setTimerExpirySec(1.f / 15);
	}

	LLPanel::draw();
}

// ============================================================================
// LLFloaterCameraPresetsEx  - Member functions
//

void LLFloaterCameraPresetsEx::refreshEditControls()
{
	const bool fSyncCamera = m_pSyncCameraCtrl->get();

	const LLPanelCameraPresetItem* pPresetItem = dynamic_cast<const LLPanelCameraPresetItem*>(m_pPresetsList->getSelectedItem());
	if (pPresetItem)
		m_pNameCtrl->setText(pPresetItem->getValue()["name"].asStringRef());
	else if (!m_fNewEdit)
		m_pNameCtrl->setText(getString("custom_text"));
	m_pNameCtrl->setEnabled(m_fNewEdit);
	m_fNewEdit &= !pPresetItem;

	const LLVector3 vecCamOffset = gAgentCamera.getCameraOffsetInitial();
	m_pCamOffsetXCtrl->setValue(vecCamOffset[VX]);
	m_pCamOffsetXCtrl->setEnabled(!fSyncCamera);
	m_pCamOffsetYCtrl->setValue(vecCamOffset[VY]);
	m_pCamOffsetYCtrl->setEnabled(!fSyncCamera);
	m_pCamOffsetZCtrl->setValue(vecCamOffset[VZ]);
	m_pCamOffsetZCtrl->setEnabled(!fSyncCamera);

	const LLVector3d vecFocusOffset = gAgentCamera.getFocusOffsetInitial();
	m_pFocusOffsetXCtrl->setValue(vecFocusOffset[VX]);
	m_pFocusOffsetXCtrl->setEnabled(!fSyncCamera);
	m_pFocusOffsetYCtrl->setValue(vecFocusOffset[VY]);
	m_pFocusOffsetYCtrl->setEnabled(!fSyncCamera);
	m_pFocusOffsetZCtrl->setValue(vecFocusOffset[VZ]);
	m_pFocusOffsetZCtrl->setEnabled(!fSyncCamera);

	const LLViewerCamera* pCamera = LLViewerCamera::getInstance();
	m_pCamFovSliderCtrl->setMinValue(pCamera->getMinView());
	m_pCamFovSliderCtrl->setMaxValue(pCamera->getMaxView());
	m_pCamFovSpinCtrl->setMinValue(pCamera->getMinView());
	m_pCamFovSpinCtrl->setMaxValue(pCamera->getMaxView());

	m_pSaveBtn->getListControl()->getItem("save")->setEnabled(pPresetItem != nullptr);
	m_pSaveBtn->selectByValue( (pPresetItem) ? "save" : "save_as");
}

void LLFloaterCameraPresetsEx::refreshPresets()
{
	std::list<std::string> presetList;
	LLPresetsManager::instance().loadPresetNamesFromDir(PRESETS_CAMERA, presetList, DEFAULT_SHOW);

	m_pPresetsList->clear();
	for (const std::string& strPresetName : presetList)
	{
		LLSD sdValue;
		sdValue["name"] = strPresetName;
		sdValue["is_default"] = LLPresetsManager::instance().isDefaultCameraPreset(strPresetName);
		LLPanelCameraPresetItem* pPresetItem = new LLPanelCameraPresetItem(sdValue);
		pPresetItem->setDeleteCallback(std::bind(&LLFloaterCameraPresetsEx::onPresetDelete, this));
		pPresetItem->setResetCallback(std::bind(&LLFloaterCameraPresetsEx::onPresetReset, this));

		m_pPresetsList->addItem(pPresetItem, strPresetName, ADD_BOTTOM);
	}

	const std::string strCurPreset = gSavedSettings.getString("PresetCameraActive");
	if (!strCurPreset.empty())
		m_pPresetsList->selectItemByValue(strCurPreset);
	else
		m_pPresetsList->resetSelection();
	refreshEditControls();
}

void LLFloaterCameraPresetsEx::savePreset(const std::string& strNewPresetName)
{
	LLVector3 vecCameraOffset = gSavedSettings.getVector3("CameraOffsetRearView") * gAgentCamera.getCurrentCameraZoomFraction();
	gSavedSettings.setVector3("CameraOffsetRearView", vecCameraOffset);
	gAgentCamera.resetCameraZoomFraction();

	if (LLPresetsManager::getInstance()->savePreset(PRESETS_CAMERA, strNewPresetName))
	{
		onPresetChanged(strNewPresetName);
	}
	else
	{
		LLNotificationsUtil::add("PresetNotSaved", LLSD().with("NAME", strNewPresetName));
	}
	refreshPresets();
}

// ============================================================================
// LLFloaterCameraPresetsEx - Event handlers
//

void LLFloaterCameraPresetsEx::onCommitSettings()
{
	// Reset the camera if it got detached from the avatar since value changes yield no visible feedback anymore
	if ( (CAMERA_MODE_THIRD_PERSON != gAgentCamera.getCameraMode()) || (!gAgentCamera.getFocusOnAvatar()) )
	{
		gAgentCamera.resetView(TRUE, TRUE);
		gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);
		LLFloaterCamera::resetCameraMode();
	}

	LLVector3 vecCamOffset(m_pCamOffsetXCtrl->getValueF32(), m_pCamOffsetYCtrl->getValueF32(), m_pCamOffsetZCtrl->getValueF32());
	gSavedSettings.setVector3("CameraOffsetRearView", vecCamOffset);

	LLVector3d vecFocusOffset(m_pFocusOffsetXCtrl->getValueF32(), m_pFocusOffsetYCtrl->getValueF32(), m_pFocusOffsetZCtrl->getValueF32());
	gSavedSettings.setVector3d("FocusOffsetRearView", vecFocusOffset);
}

void LLFloaterCameraPresetsEx::onNewPreset()
{
	m_fNewEdit = true;
	gSavedSettings.setString("PresetCameraActive", "");
	LLPresetsManager::instance().triggerChangeCameraSignal();

	m_pPresetsList->resetSelection();
	m_pNameCtrl->setText(getString("default_name"));
	refreshEditControls();
}

void LLFloaterCameraPresetsEx::onPresetChanged(const LLSD& sdValue)
{
	if (m_pPresetsList->getSelectedValue().asStringRef() != sdValue.asStringRef())
	{
		if (LLPanel* pPresetItem = m_pPresetsList->getItemByValue(sdValue))
		{
			m_pPresetsList->setCommitOnSelectionChange(false);
			m_pPresetsList->resetSelection(true);
			m_pPresetsList->selectItem(pPresetItem);
			m_pPresetsList->scrollToShowFirstSelectedItem();
			m_pPresetsList->setCommitOnSelectionChange(true);
			refreshEditControls();
		}
	}
}

void LLFloaterCameraPresetsEx::onPresetClicked()
{
	const LLSD sdPresetItem = m_pPresetsList->getSelectedValue();
	LLFloaterCamera::switchToPreset(sdPresetItem.asStringRef());
	refreshEditControls();
}

void LLFloaterCameraPresetsEx::onPresetDelete()
{
	const LLSD sdPresetItem = m_pPresetsList->getSelectedValue();
	if (sdPresetItem.isUndefined())
		return;

	LLSD sdParams;
	sdParams["PRESET"] = sdPresetItem.asStringRef();
	LLSD sdPayload;
	sdPayload["preset"] = sdPresetItem.asStringRef();
	LLNotificationsUtil::add("DeleteCameraPreset", sdParams, sdPayload, std::bind(&LLFloaterCameraPresetsEx::onPresetDeleteCb, this, std::placeholders::_1, std::placeholders::_2));
}

void LLFloaterCameraPresetsEx::onPresetDeleteCb(const LLSD& sdNotification, const LLSD& sdResponse)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
	if (0 == idxOption /*YES*/)
	{
		const std::string strPresetName = sdNotification["payload"]["preset"];
		if (!LLPresetsManager::getInstance()->deletePreset(PRESETS_CAMERA, strPresetName))
		{
			LLNotificationsUtil::add("PresetNotDeleted", LLSD().with("NAME", strPresetName));
			return;
		}
		else if (gSavedSettings.getString("PresetCameraActive") == strPresetName)
		{
			gSavedSettings.setString("PresetCameraActive", "");
		}
		refreshPresets();
	}
}

void LLFloaterCameraPresetsEx::onPresetReset()
{
	const LLSD sdPresetItem = m_pPresetsList->getSelectedValue();
	if (sdPresetItem.isUndefined())
		return;

	LLPresetsManager::getInstance()->resetCameraPreset(sdPresetItem.asStringRef());
	refreshEditControls();
}

void LLFloaterCameraPresetsEx::onPresetSave()
{
	std::string strAction = m_pSaveBtn->getValue().asString();
	if (strAction.empty())
	{
		strAction = (m_pPresetsList->getSelectedItem()) ? "save" : "save_as";
	}

	if ("save" == strAction)
	{
		const LLSD sdPresetItem = m_pPresetsList->getSelectedValue();
		if (!sdPresetItem.isUndefined())
		{
			savePreset(sdPresetItem.asStringRef());
		}
	}
	else if ("save_as" == strAction)
	{
		std::string strNewPresetName;
		if (m_pPresetsList->getSelectedItem())
			strNewPresetName = m_pNameCtrl->getText() + " " + getString("new_preset_suffix");
		else if (m_fNewEdit)
			strNewPresetName = m_pNameCtrl->getText();
		else
			strNewPresetName = getString("default_name");

		LLNotificationsUtil::add("SavePresetAs", LLSD().with("DESC", strNewPresetName), LLSD(),
								 std::bind(&LLFloaterCameraPresetsEx::onPresetSaveAsCb, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void LLFloaterCameraPresetsEx::onPresetSaveAsCb(const LLSD& sdNotification, const LLSD& sdResponse)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
	if (0 == idxOption /*YES*/)
	{
		std::string strNewPresetName = sdResponse["message"].asStringRef();
		LLStringUtil::trim(strNewPresetName);
		if (!strNewPresetName.empty())
		{
			if (m_pPresetsList->getItemByValue(strNewPresetName))
			{
				LLNotificationsUtil::add("PresetAlreadyExists", LLSD().with("NAME", strNewPresetName));
				return;
			}

			savePreset(strNewPresetName);
		}
	}
}

void LLFloaterCameraPresetsEx::onToggleSyncWithCamera()
{
	if (m_pSyncCameraCtrl->get())
	{
		gAgentCamera.setFocusOnAvatar(FALSE, FALSE, FALSE);
	}
	else
	{
		if (gAgentCamera.isJoystickCameraUsed())
			gAgentCamera.resetCameraZoomFraction();
		gAgentCamera.setFocusOnAvatar(TRUE, FALSE, FALSE);
		refreshEditControls();
	}
}

// ============================================================================
