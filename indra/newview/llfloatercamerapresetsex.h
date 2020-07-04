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

#pragma once

#include "llfloater.h"

// ============================================================================
// Forward declarations
//

class LLButton;
class LLCheckBoxCtrl;
class LLFlatListView;
class LLFlyoutButton;
class LLLineEditor;
class LLSliderCtrl;
class LLSpinCtrl;

// ============================================================================
// LLFloaterCameraPresetsEx class
//

class LLFloaterCameraPresetsEx : public LLFloater
{
public:
	LLFloaterCameraPresetsEx(const LLSD& sdKey);
	~LLFloaterCameraPresetsEx() override;

	/*
	 * LLView overrides
	 */
public:
	void draw() override;
	BOOL postBuild() override;

	/*
	 * Member functions
	 */
public:
protected:
	void refreshEditControls();
	void refreshPresets();
	void savePreset(const std::string& strNewPresetName);

	/*
	 * Event handlers
	 */
protected:
	void onCommitSettings();
	void onNewPreset();
	void onPresetChanged(const LLSD& sdValue);
	void onPresetClicked();
	void onPresetDelete();
	void onPresetDeleteCb(const LLSD& sdNotification, const LLSD& sdResponse);
	void onPresetReset();
	void onPresetSave();
	void onPresetSaveAsCb(const LLSD& sdNotification, const LLSD& sdResponse);
	void onToggleSyncWithCamera();

	/*
	 * Member variables
	 */
protected:
	LLFlatListView* m_pPresetsList = nullptr;

	LLPanel* m_pEditPanel = nullptr;
	bool m_fNewEdit = false;
	LLLineEditor* m_pNameCtrl = nullptr;
	LLSpinCtrl* m_pCamOffsetXCtrl = nullptr;
	LLSpinCtrl* m_pCamOffsetYCtrl = nullptr;
	LLSpinCtrl* m_pCamOffsetZCtrl = nullptr;
	LLSpinCtrl* m_pFocusOffsetXCtrl = nullptr;
	LLSpinCtrl* m_pFocusOffsetYCtrl = nullptr;
	LLSpinCtrl* m_pFocusOffsetZCtrl = nullptr;
	LLSliderCtrl* m_pFocusScaleSliderCtrl = nullptr;
	LLSpinCtrl* m_pFocusScaleSpinCtrl = nullptr;
	LLSliderCtrl* m_pCamFovSliderCtrl = nullptr;
	LLSpinCtrl* m_pCamFovSpinCtrl = nullptr;
	LLButton* m_pNewBtn = nullptr;
	LLFlyoutButton* m_pSaveBtn = nullptr;
	LLCheckBoxCtrl* m_pSyncCameraCtrl = nullptr;

	LLTimer m_CameraSyncTimer;
	boost::signals2::scoped_connection m_PresetCameraActiveConn;
};

// ============================================================================
