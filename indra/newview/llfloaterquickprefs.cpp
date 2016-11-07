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

#include "llviewerprecompiledheaders.h"

#include "llfloaterquickprefs.h"

// ====================================================================================
// LLFloaterQuickPrefs class
//

const char* LLFloaterQuickPrefs::s_PanelNames[PANEL_COUNT] =
	{
		"wearing",
		"inventory",
		"windlight",
	};

LLFloaterQuickPrefs::LLFloaterQuickPrefs(const LLSD& sdKey)
	: LLFloater(sdKey)
{
//	memset(&m_pButtons[0], 0, sizeof(LLButton*) * PANEL_COUNT);
//	memset(&m_pPanels[0], 0, sizeof(LLPanel) * PANEL_COUNT);
}

LLFloaterQuickPrefs::~LLFloaterQuickPrefs()
{
}

// virtual
BOOL LLFloaterQuickPrefs::postBuild()
{
	for (size_t idxPanel = 0, cntPanel = sizeof(s_PanelNames) / sizeof(const char*); idxPanel < cntPanel; idxPanel++)
	{
		m_pButtons[idxPanel] = findChild<LLButton>(llformat("%s_btn", s_PanelNames[idxPanel]));
		llassert(m_pButtons[idxPanel]);
		m_pButtons[idxPanel]->setCommitCallback(boost::bind(&LLFloaterQuickPrefs::onShowPanel, this, (EPanelType)idxPanel));

		m_pPanels[idxPanel] = findChild<LLPanel>(llformat("%s_panel", s_PanelNames[idxPanel]));
		llassert(m_pPanels[idxPanel]);
	}

	// Show default panel
	onShowPanel(PANEL_WEARING);

	return TRUE;
}

void LLFloaterQuickPrefs::onShowPanel(LLFloaterQuickPrefs::EPanelType eNewPanel)
{
	if (m_eCurrentPanel == eNewPanel)
		return;

	if (PANEL_NONE != m_eCurrentPanel)
	{
		m_pButtons[m_eCurrentPanel]->setToggleState(false);
		m_pPanels[m_eCurrentPanel]->setVisible(false);
	}
	m_pButtons[eNewPanel]->setToggleState(true);
	m_pPanels[eNewPanel]->setVisible(true);

	m_eCurrentPanel = eNewPanel;
}

// ====================================================================================
// LLQuickPrefsPanel class
//

LLQuickPrefsPanel::LLQuickPrefsPanel()
	: LLPanel()
{
}

LLQuickPrefsPanel::~LLQuickPrefsPanel()
{
}

// ====================================================================================
