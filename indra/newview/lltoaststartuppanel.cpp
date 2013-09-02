/** 
 *
 * Copyright (c) 2013, Kitty Barnett
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

#include "llsyswellwindow.h"
#include "lltoaststartuppanel.h"
#include "lltrans.h"

// ============================================================================
// LLToastStartupPanel class
//

LLToastStartupPanel::LLToastStartupPanel()
	: LLToastPanel()
{
	buildFromFile("panel_startup_toast.xml");
}

LLToastStartupPanel::~LLToastStartupPanel()
{
}

BOOL LLToastStartupPanel::postBuild()
{
	LLTextBase* pMessageText = findChild<LLTextBase>("message");
	if (pMessageText)
	{
		setMouseUpCallback(boost::bind(&LLToastStartupPanel::onPanelClick, this, _2, _3, _4));

		pMessageText->setText(LLTrans::getString("StartUpNotifications"));
		snapToMessageHeight(pMessageText, 5);
	}

	return TRUE;
}

void LLToastStartupPanel::onPanelClick(S32 x, S32 y, MASK mask)
{
	notifyParent(LLSD().with("action", "hide_toast"));

	LLNotificationWellWindow::getInstance()->onStartUpToastClick(x, y, mask);
}

// ============================================================================
