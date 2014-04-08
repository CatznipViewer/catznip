/** 
 *
 * Copyright (c) 2014, Kitty Barnett
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

#include "linden_common.h"

#include "llbutton.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "lltextsearchctrl.h"

// ============================================================================
// LLTextSearchCtrl class
//

LLTextSearchCtrl::LLTextSearchCtrl(LLTextEditor* pEditor)
{
	buildFromFile("panel_editor_inlinesearch.xml");

	pEditor->addChild(this);
}

LLTextSearchCtrl::~LLTextSearchCtrl()
{
}

BOOL LLTextSearchCtrl::postBuild()
{
	m_pSearchEditor = getChild<LLLineEditor>("search_editor");
	m_pSearchEditor->setCommitCallback(boost::bind(&LLTextSearchCtrl::onSearchClicked, this, true));
	m_pSearchEditor->setCommitOnFocusLost(false);
	m_pSearchEditor->setKeystrokeCallback(boost::bind(&LLTextSearchCtrl::onRefreshHighlight, this), NULL);

	getChild<LLButton>("search_down")->setCommitCallback(boost::bind(&LLTextSearchCtrl::onSearchClicked, this, true));
	getChild<LLButton>("search_up")->setCommitCallback(boost::bind(&LLTextSearchCtrl::onSearchClicked, this, false));
	getChild<LLButton>("close_btn")->setCommitCallback(boost::bind(&LLTextSearchCtrl::setVisible, this, false));

	return LLPanel::postBuild();
}

void LLTextSearchCtrl::setVisible(BOOL fVisible)
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(getParent());
	if (pEditor)
	{
		if (fVisible)
		{
			// Place us in the upper right corner of our parent
			const LLRect& rctEditor = pEditor->getVisibleTextRect();

			LLRect rctClient(rctEditor.getWidth() - getRect().getWidth(), rctEditor.getHeight(), rctEditor.getWidth(), rctEditor.getHeight() - getRect().getHeight());
			setRect(rctClient);

			// If there's search text from a previous search, refresh highlights
			if (!m_pSearchEditor->getText().empty())
				onRefreshHighlight();

			// Listen for changes to the case insensitive setting
			m_CaseInsensitiveConn = LLUI::sSettingGroups["config"]->getControl("LSLFindCaseInsensitivity")->getSignal()->connect(boost::bind(&LLTextSearchCtrl::onRefreshHighlight, this));
		}
		else
		{
			pEditor->clearHighlights();
			m_CaseInsensitiveConn.disconnect();
		}
	}

	LLPanel::setVisible(fVisible);

	if (!fVisible)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
			gFocusMgr.setKeyboardFocus(NULL);
		getParent()->setFocus(true);
	}
}

LLSD LLTextSearchCtrl::getValue() const
{
	return m_pSearchEditor->getText();
}

void LLTextSearchCtrl::setValue(const LLSD& sdValue)
{
	m_pSearchEditor->setText(sdValue.asString());
	onRefreshHighlight();
}

void LLTextSearchCtrl::onRefreshHighlight()
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(getParent());
	if (pEditor)
	{
		pEditor->setHighlightWord(m_pSearchEditor->getText(), LLUI::sSettingGroups["config"]->getBOOL("LSLFindCaseInsensitivity"));
	}
}

void LLTextSearchCtrl::onSearchClicked(bool fSearchDown)
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(getParent());
	if (pEditor)
	{
		pEditor->selectNext(m_pSearchEditor->getText(), LLUI::sSettingGroups["config"]->getBOOL("LSLFindCaseInsensitivity"), TRUE, !fSearchDown);
	}
}

// ============================================================================
