/** 
 *
 * Copyright (c) 2011-2014, Kitty Barnett
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
#include "lllogininstance.h"
#include "llprogressbar.h"
#include "lltextbox.h"
#include "llupdaterservice.h"

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
// LLFloaterUpdateProgress class
// 

LLFloaterUpdateProgress::LLFloaterUpdateProgress(const LLSD& sdKey, bool fModal)
	: LLModalDialog(LLSD(), fModal)
	, m_fRequired(false)
	, m_pProgressBar(NULL)
	, m_pProgressText(NULL)
	, m_pInstallBtn(NULL)
{
}

LLFloaterUpdateProgress::~LLFloaterUpdateProgress()
{
}

BOOL LLFloaterUpdateProgress::postBuild()
{
	const LLSD& sdDownloadData = LLLoginInstance::instance().getUpdaterService()->getDownloadData();
	m_fRequired = sdDownloadData["required"].asBoolean();

	LLStringUtil::format_map_t args;
	args["VERSION"] = sdDownloadData["update_version"].asString();
	getChild<LLUICtrl>("version_text")->setValue(getString((m_fRequired) ? "string_version_required" : "string_version_optional", args));
	getChild<LLUICtrl>("info_text")->setTextArg("INFO_URL", sdDownloadData["info_url"].asString());

	m_pProgressBar = getChild<LLProgressBar>("progress_bar");
	m_pProgressText = getChild<LLTextBox>("progress_text");
	m_pInstallBtn = getChild<LLButton>("install_btn");
	m_pInstallBtn->setCommitCallback(boost::bind(&LLFloaterUpdateProgress::onInstallBtn, this));

	return LLFloater::postBuild();
}

void LLFloaterUpdateProgress::onOpen(const LLSD& sdKey)
{
	m_pProgressBar->setValue(0.f);
	m_pProgressBar->setVisible(true);
	m_pProgressText->setVisible(false);
	m_pInstallBtn->setVisible(false);

	getChild<LLTextBox>("required_text")->setVisible(m_fRequired);
	getChild<LLTextBox>("optional_text")->setVisible(!m_fRequired);

	LLFloater::onOpen(LLSD());
	setKey(LLSD());
}

void LLFloaterUpdateProgress::onDownloadProgress(const LLSD& sdData)
{
	double mDownloadProgress = sdData["bytes_downloaded"].asReal() / (1024 * 1024);
	double nDownloadTotal = sdData["download_size"].asReal() / (1024 * 1024);
	
	m_pProgressBar->setValue(mDownloadProgress / nDownloadTotal * 100.f);
	m_pProgressText->setText(llformat("%.2f / %.2f Mb", mDownloadProgress, nDownloadTotal));
	m_pProgressText->setVisible(true);
}

void LLFloaterUpdateProgress::onDownloadCompleted()
{
	m_pProgressBar->setVisible(false);
	m_pProgressText->setVisible(false);
	m_pInstallBtn->setVisible(true);
}

void LLFloaterUpdateProgress::onInstallBtn()
{
	LLLoginInstance::instance().getUpdaterService()->checkForInstall(true);
	closeFloater();
}

// ====================================================================================
