/**
 *
 * Copyright (c) 2016, Kitty Barnett
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

#include "llfloaterquickprefs.h"

// ====================================================================================
// Foward declarations
//

class LLCategoryItemsList;
class LLInventoryCategoriesObserver;
class LLSliderCtrl;
class LLTextBox;
class LLWearableItemsList;
class LLWornItemsList;

// ====================================================================================
// LLQuickPrefsAppearancePanel class
//

class LLQuickPrefsAppearancePanel : public LLQuickPrefsPanel
{
	LOG_CLASS(LLQuickPrefsAppearancePanel);
public:
	LLQuickPrefsAppearancePanel();
	~LLQuickPrefsAppearancePanel() override;

	/*
	 * LLPanel base class overrides
	 */
public:
	BOOL postBuild() override;
	void onVisibilityChange(BOOL fVisible) override;

	/*
	 * Member functions
	 */
protected:
	void refreshComplexity();
	void refreshHover();
	void refreshMaxComplexity();
	void refreshMaxNonImpostors();
	void refreshNotifications();

	/*
	 * Event handlers
	 */
protected:
	void onHoverChange(bool fCommit);
	void onMaxComplexityChange();
	void onMaxNonImpostorsChange();
	void onShowNotificationsToggle();

	/*
	 * Member variables
	 */
protected:
	static const std::string s_strNotifications[];

	boost::signals2::connection m_ComplexityChangedSlot;
	boost::signals2::connection m_HoverChangedSlot;
	boost::signals2::connection m_VisibilityChangedSlot;
	boost::signals2::connection m_MaxComplexityChangedSlot;
	boost::signals2::connection m_MaxNonImpostorsChangedSlot;

	LLSliderCtrl*               m_pHoverSlider = nullptr;
	LLTextBox*                  m_pComplexityText = nullptr;
	LLTextBox*                  m_pVisibilityText = nullptr;
	LLCheckBoxCtrl*             m_pShowNotificationsCheck = nullptr;
	LLSliderCtrl*               m_pMaxComplexitySlider = nullptr;
	LLTextBox*                  m_pMaxComplexityText = nullptr;
	LLSliderCtrl*               m_pMaxNonImpostorsSlider = nullptr;
	LLTextBox*                  m_pMaxNonImpostorsText = nullptr;
};

// ====================================================================================
// LLQuickPrefsInventoryPanel class
//

class LLQuickPrefsInventoryPanel : public LLQuickPrefsPanel
{
	LOG_CLASS(LLQuickPrefsInventoryPanel);
public:
	LLQuickPrefsInventoryPanel();
	~LLQuickPrefsInventoryPanel() override;

	/*
	* LLPanel base class overrides
	*/
public:
	BOOL postBuild() override;
	void onVisibilityChange(BOOL fVisible) override;

	/*
	* Event handlers
	*/
	void onBrowseFolder();
	void onBrowseFolderCb(const LLSD& sdData);
	void onFolderChanged();
	void onSortOrderChanged(const LLSD& sdParam);
	bool onSortOrderCheck(const LLSD& sdParam);

	/*
	* Member variables
	*/
protected:
	LLButton*            m_pFolderBrowseBtn = nullptr;
	LLHandle<LLFloater>  m_BrowseFloaterHandle;
	LLCategoryItemsList* m_pItemsList = nullptr;
};

// ====================================================================================
// LLQuickPrefsWearingPanel class
//

class LLQuickPrefsWearingPanel : public LLQuickPrefsPanel
{
	LOG_CLASS(LLQuickPrefsWearingPanel);
public:
	LLQuickPrefsWearingPanel();
	~LLQuickPrefsWearingPanel() override;

	/*
	 * LLPanel base class overrides
	 */
public:
	BOOL postBuild() override;
	void onSortOrderChanged(const LLSD& sdParam);
	bool onSortOrderCheck(const LLSD& sdParam);
	void onVisibilityChange(BOOL fVisible) override;

	/*
	 * Event handlers
	 */
protected:
	void onCOFChanged();

	/*
	 * Member variables
	 */
protected:
	LLUUID                         m_idCOF;
	LLInventoryCategoriesObserver* m_pCofObserver = nullptr;
	LLWornItemsList*               m_pWornItemsList = nullptr;
};

// ====================================================================================
