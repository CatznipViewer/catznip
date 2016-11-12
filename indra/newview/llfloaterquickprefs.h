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

#include "llfloater.h"

// ====================================================================================
// LLFloaterQuickPrefs class
//

class LLFloaterQuickPrefs : public LLFloater
{
	LOG_CLASS(LLFloaterQuickPrefs);

	friend class LLFloaterReg;
	typedef enum { PANEL_WEARING = 0, PANEL_INVENTORY, PANEL_WINDLIGHT, PANEL_COUNT, PANEL_NONE = -1 } EPanelType;
	static const char* s_PanelNames[PANEL_COUNT];
protected:
	LLFloaterQuickPrefs(const LLSD& sdKey);
public:
	~LLFloaterQuickPrefs() override;

	/*
	 * LLFloater base class overrides
	 */
public:
	BOOL postBuild() override;

	/*
	 * Event handlers
	 */
protected:
	void onShowPanel(EPanelType eNewPanel);

	/*
	 * Member variables
	 */
protected:
	// General
	EPanelType                     m_eCurrentPanel = PANEL_NONE;
	LLButton*                      m_pButtons[PANEL_COUNT];
	LLPanel*                       m_pPanels[PANEL_COUNT];
};

// ====================================================================================
// LLQuickPrefsPanel class
//

class LLQuickPrefsPanel : public LLPanel
{
	LOG_CLASS(LLQuickPrefsPanel);
public:
	LLQuickPrefsPanel();
	~LLQuickPrefsPanel() override;

	/*
	 * LLPanel base class overrides
	 */
public:
	//BOOL postBuild() override;
	//void onVisibilityChange(BOOL fIsVisible) override;

	/*
	 * Member functions
	 */
public:
	bool isInitialized() const { return m_fInitialized; }
protected:
	void setInitialized() { m_fInitialized = true; }

	/*
	 * Member variables
	 */
protected:
	bool m_fInitialized = false;
};

// ====================================================================================
