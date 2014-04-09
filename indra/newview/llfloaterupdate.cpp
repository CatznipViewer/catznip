/** 
 *
 * Copyright (c) 2011-2012, Kitty Barnett
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

#include "llbutton.h"
#include "llevents.h"
#include "llfloaterupdate.h"

// ====================================================================================
// LLFloaterUpdate class
// 

LLFloaterUpdate::LLFloaterUpdate(const LLSD& sdData)
	: LLModalDialog(sdData["version"])
	, m_fRequired(sdData["required"].asBoolean())
	, m_strVersion(sdData["version"].asString())
	, m_strInformation(sdData["more_info"].asString())
	, m_strInformationUrl(sdData["info_url"].asString())
{
	if ("download" == sdData["type"].asString())
		m_eType = TYPE_DOWNLOAD;
	else if ("install" == sdData["type"].asString())
		m_eType = TYPE_INSTALL;
}

BOOL LLFloaterUpdate::postBuild()
{
	setTitle(getString((TYPE_INSTALL == m_eType) ? "string_title_install" : "string_title_download"));

	LLStringUtil::format_map_t args;
	args["VERSION"] = m_strVersion;
	getChild<LLUICtrl>("text_version")->setValue(getString((m_fRequired) ? "string_version_required" : "string_version_optional", args));
	getChild<LLUICtrl>("text_message")->setValue(m_strInformation);
	getChild<LLUICtrl>("text_website")->setTextArg("INFO_URL", m_strInformationUrl);

	LLButton* pAcceptBtn = getChild<LLButton>("button_accept");
	pAcceptBtn->setCommitCallback(boost::bind(&LLFloaterUpdate::onAcceptOrCancel, this, true));
	pAcceptBtn->setLabel(getString((TYPE_INSTALL == m_eType) ? "string_button_install" : "string_button_download"));

	LLButton* pCancelBtn = getChild<LLButton>("button_cancel");
	pCancelBtn->setCommitCallback(boost::bind(&LLFloaterUpdate::onAcceptOrCancel, this, false));
	pCancelBtn->setEnabled(!m_fRequired);

	return TRUE;
}

void LLFloaterUpdate::onAcceptOrCancel(bool fAccept)
{
	if (mCommitSignal)
		(*mCommitSignal)(this, LLSD().with("accept", fAccept));
	closeFloater();
}

// ====================================================================================
