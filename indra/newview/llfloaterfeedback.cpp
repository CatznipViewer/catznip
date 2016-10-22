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

#include "llagent.h"
#include "llfloaterfeedback.h"
#include "llmediactrl.h"
#include "llweb.h"

// =========================================================================
// LLFloaterFeedback class
//

LLFloaterFeedback::LLFloaterFeedback(const LLSD& sdKey)
	: LLFloater(sdKey),
	  m_pWebBrowser(nullptr)
{
}

LLFloaterFeedback::~LLFloaterFeedback()
{
}

BOOL LLFloaterFeedback::postBuild()
{
	const LLSD& sdFeedbackInfo = gAgent.mFeedbackInfo;

	/*
	 * Resize the floater
	 */
	S32 nWidth = getRect().getWidth();
	if (sdFeedbackInfo.has("window-width"))
		nWidth = sdFeedbackInfo["window-width"].asInteger();

	S32 nHeight = getRect().getHeight();
	if (sdFeedbackInfo.has("window-height"))
		nHeight = sdFeedbackInfo["window-height"].asInteger();

	reshape(nWidth, nHeight, 0);

	/*
	 * Navigate to the URL
	 */
	m_pWebBrowser = getChild<LLMediaCtrl>("floater_feedback_browser");
	if (sdFeedbackInfo.has("url"))
		m_pWebBrowser->navigateTo(LLWeb::expandURLSubstitutions(sdFeedbackInfo["url"], LLSD().with("AGENT_NAME", gAgentUsername)));

	return TRUE;
}

// =========================================================================
