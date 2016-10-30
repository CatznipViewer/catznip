/** 
 * @file llfloatertranslationsettings.cpp
 * @brief Machine translation settings for chat
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatertranslationsettings.h"

// Viewer includes
#include "llfloaterimnearbychat.h"
#include "lltranslate.h"
#include "llviewercontrol.h" // for gSavedSettings

// Linden library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"

// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
static LLPanelInjector<LLPanelPreferenceTranslationSettings> t_pref_chat_translation("panel_preference_chat_translation");

LLPanelPreferenceTranslationSettings::LLPanelPreferenceTranslationSettings()
:	LLPanelPreference()
,	mMachineTranslationCB(NULL)
,	mLanguageCombo(NULL)
,	mTranslationServiceRadioGroup(NULL)
,	mBingAPIKeyEditor(NULL)
,	mGoogleAPIKeyEditor(NULL)
,	mBingVerifyBtn(NULL)
,	mGoogleVerifyBtn(NULL)
,	mBingKeyVerified(false)
,	mGoogleKeyVerified(false)
{
}
// [/SL:KB]
//LLFloaterTranslationSettings::LLFloaterTranslationSettings(const LLSD& key)
//:	LLFloater(key)
//,	mMachineTranslationCB(NULL)
//,	mLanguageCombo(NULL)
//,	mTranslationServiceRadioGroup(NULL)
//,	mBingAPIKeyEditor(NULL)
//,	mGoogleAPIKeyEditor(NULL)
//,	mBingVerifyBtn(NULL)
//,	mGoogleVerifyBtn(NULL)
//,	mOKBtn(NULL)
//,	mBingKeyVerified(false)
//,	mGoogleKeyVerified(false)
//{
//}

// virtual
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
BOOL LLPanelPreferenceTranslationSettings::postBuild()
{
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
	LLPanelPreference::postBuild();
// [/SL:KB]

	mMachineTranslationCB = getChild<LLCheckBoxCtrl>("translate_chat_checkbox");
	mLanguageCombo = getChild<LLComboBox>("translate_language_combo");
	mTranslationServiceRadioGroup = getChild<LLRadioGroup>("translation_service_rg");
	mBingAPIKeyEditor = getChild<LLLineEditor>("bing_api_key");
	mGoogleAPIKeyEditor = getChild<LLLineEditor>("google_api_key");
	mBingVerifyBtn = getChild<LLButton>("verify_bing_api_key_btn");
	mGoogleVerifyBtn = getChild<LLButton>("verify_google_api_key_btn");

	mMachineTranslationCB->setCommitCallback(boost::bind(&LLPanelPreferenceTranslationSettings::updateControlsEnabledState, this));
	mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLPanelPreferenceTranslationSettings::updateControlsEnabledState, this));
	mBingVerifyBtn->setClickedCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onBtnBingVerify, this));
	mGoogleVerifyBtn->setClickedCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onBtnGoogleVerify, this));

	mBingAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onEditorFocused, this, _1));
	mBingAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onBingKeyEdited, this), NULL);
	mGoogleAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onEditorFocused, this, _1));
	mGoogleAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLPanelPreferenceTranslationSettings::onGoogleKeyEdited, this), NULL);

	return TRUE;
}
// [/SL:KB]
//BOOL LLFloaterTranslationSettings::postBuild()
//{
//	mMachineTranslationCB = getChild<LLCheckBoxCtrl>("translate_chat_checkbox");
//	mLanguageCombo = getChild<LLComboBox>("translate_language_combo");
//	mTranslationServiceRadioGroup = getChild<LLRadioGroup>("translation_service_rg");
//	mBingAPIKeyEditor = getChild<LLLineEditor>("bing_api_key");
//	mGoogleAPIKeyEditor = getChild<LLLineEditor>("google_api_key");
//	mBingVerifyBtn = getChild<LLButton>("verify_bing_api_key_btn");
//	mGoogleVerifyBtn = getChild<LLButton>("verify_google_api_key_btn");
//	mOKBtn = getChild<LLButton>("ok_btn");
//
//	mMachineTranslationCB->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
//	mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
//	mOKBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnOK, this));
//	getChild<LLButton>("cancel_btn")->setClickedCallback(boost::bind(&LLFloater::closeFloater, this, false));
//	mBingVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnBingVerify, this));
//	mGoogleVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnGoogleVerify, this));
//
//	mBingAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
//	mBingAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onBingKeyEdited, this), NULL);
//	mGoogleAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
//	mGoogleAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onGoogleKeyEdited, this), NULL);
//
//	center();
//	return TRUE;
//}

// virtual
//void LLFloaterTranslationSettings::onOpen(const LLSD& key)
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::refresh()
// [/SL:KB]
{
	mMachineTranslationCB->setValue(gSavedSettings.getBOOL("TranslateChat"));
	mLanguageCombo->setSelectedByValue(gSavedSettings.getString("TranslateLanguage"), TRUE);
	mTranslationServiceRadioGroup->setSelectedByValue(gSavedSettings.getString("TranslationService"), TRUE);

	std::string bing_key = gSavedSettings.getString("BingTranslateAPIKey");
	if (!bing_key.empty())
	{
		mBingAPIKeyEditor->setText(bing_key);
		mBingAPIKeyEditor->setTentative(FALSE);
		verifyKey(LLTranslate::SERVICE_BING, bing_key, false);
	}
	else
	{
		mBingAPIKeyEditor->setTentative(TRUE);
		mBingKeyVerified = FALSE;
	}

	std::string google_key = gSavedSettings.getString("GoogleTranslateAPIKey");
	if (!google_key.empty())
	{
		mGoogleAPIKeyEditor->setText(google_key);
		mGoogleAPIKeyEditor->setTentative(FALSE);
		verifyKey(LLTranslate::SERVICE_GOOGLE, google_key, false);
	}
	else
	{
		mGoogleAPIKeyEditor->setTentative(TRUE);
		mGoogleKeyVerified = FALSE;
	}

	updateControlsEnabledState();
}

//void LLFloaterTranslationSettings::setBingVerified(bool ok, bool alert)
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::setBingVerified(bool ok, bool alert)
// [/SL:KB]
{
	if (alert)
	{
		showAlert(ok ? "bing_api_key_verified" : "bing_api_key_not_verified");
	}

	mBingKeyVerified = ok;
	updateControlsEnabledState();
}

//void LLFloaterTranslationSettings::setGoogleVerified(bool ok, bool alert)
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::setGoogleVerified(bool ok, bool alert)
// [/SL:KB]
{
	if (alert)
	{
		showAlert(ok ? "google_api_key_verified" : "google_api_key_not_verified");
	}

	mGoogleKeyVerified = ok;
	updateControlsEnabledState();
}

//std::string LLFloaterTranslationSettings::getSelectedService() const
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
std::string LLPanelPreferenceTranslationSettings::getSelectedService() const
// [/SL:KB]
{
	return mTranslationServiceRadioGroup->getSelectedValue().asString();
}

//std::string LLFloaterTranslationSettings::getEnteredBingKey() const
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
std::string LLPanelPreferenceTranslationSettings::getEnteredBingKey() const
// [/SL:KB]
{
	return mBingAPIKeyEditor->getTentative() ? LLStringUtil::null : mBingAPIKeyEditor->getText();
}

//std::string LLFloaterTranslationSettings::getEnteredGoogleKey() const
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
std::string LLPanelPreferenceTranslationSettings::getEnteredGoogleKey() const
// [/SL:KB]
{
	return mGoogleAPIKeyEditor->getTentative() ? LLStringUtil::null : mGoogleAPIKeyEditor->getText();
}

//void LLFloaterTranslationSettings::showAlert(const std::string& msg_name) const
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::showAlert(const std::string& msg_name) const
// [/SL:KB]
{
	LLSD args;
	args["MESSAGE"] = getString(msg_name);
	LLNotificationsUtil::add("GenericAlert", args);
}

//void LLFloaterTranslationSettings::updateControlsEnabledState()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::updateControlsEnabledState()
// [/SL:KB]
{
	// Enable/disable controls based on the checkbox value.
	bool on = mMachineTranslationCB->getValue().asBoolean();
	std::string service = getSelectedService();
	bool bing_selected = service == "bing";
	bool google_selected = service == "google";

	mTranslationServiceRadioGroup->setEnabled(on);
	mLanguageCombo->setEnabled(on);

	getChild<LLTextBox>("bing_api_key_label")->setEnabled(on);
	mBingAPIKeyEditor->setEnabled(on);

	getChild<LLTextBox>("google_api_key_label")->setEnabled(on);
	mGoogleAPIKeyEditor->setEnabled(on);

	mBingAPIKeyEditor->setEnabled(on && bing_selected);
	mGoogleAPIKeyEditor->setEnabled(on && google_selected);

	mBingVerifyBtn->setEnabled(on && bing_selected &&
		!mBingKeyVerified && !getEnteredBingKey().empty());
	mGoogleVerifyBtn->setEnabled(on && google_selected &&
		!mGoogleKeyVerified && !getEnteredGoogleKey().empty());

	bool service_verified = (bing_selected && mBingKeyVerified) || (google_selected && mGoogleKeyVerified);
	gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);

//	mOKBtn->setEnabled(!on || service_verified);
}

/*static*/
//void LLFloaterTranslationSettings::setVerificationStatus(int service, bool ok, bool alert)
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::setVerificationStatus(int service, bool ok, bool alert)
// [/SL:KB]
{
//    LLFloaterTranslationSettings* floater =
//        LLFloaterReg::getTypedInstance<LLFloaterTranslationSettings>("prefs_translation");
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
	LLPanelPreferenceTranslationSettings* floater =
		LLFloaterReg::getTypedInstance<LLPanelPreferenceTranslationSettings>("prefs_translation");
// [/SL:KB]

    if (!floater)
    {
        LL_WARNS() << "Cannot find translation settings floater" << LL_ENDL;
        return;
    }

    switch (service)
    {
    case LLTranslate::SERVICE_BING:
        floater->setBingVerified(ok, alert);
        break;
    case LLTranslate::SERVICE_GOOGLE:
        floater->setGoogleVerified(ok, alert);
        break;
    }
}


// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::verifyKey(int service, const std::string& key, bool alert)
{
    LLTranslate::verifyKey(static_cast<LLTranslate::EService>(service), key,
		boost::bind(&LLPanelPreferenceTranslationSettings::setVerificationStatus, _1, _2, alert));
}
// [/SL:KB]
//void LLFloaterTranslationSettings::verifyKey(int service, const std::string& key, bool alert)
//{
//    LLTranslate::verifyKey(static_cast<LLTranslate::EService>(service), key,
//        boost::bind(&LLFloaterTranslationSettings::setVerificationStatus, _1, _2, alert));
//}

//void LLFloaterTranslationSettings::onEditorFocused(LLFocusableElement* control)
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::onEditorFocused(LLFocusableElement* control)
// [/SL:KB]
{
	LLLineEditor* editor = dynamic_cast<LLLineEditor*>(control);
	if (editor && editor->hasTabStop()) // if enabled. getEnabled() doesn't work
	{
		if (editor->getTentative())
		{
			editor->setText(LLStringUtil::null);
			editor->setTentative(FALSE);
		}
	}
}

//void LLFloaterTranslationSettings::onBingKeyEdited()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::onBingKeyEdited()
// [/SL:KB]
{
	if (mBingAPIKeyEditor->isDirty())
	{
		setBingVerified(false, false);
	}
}

//void LLFloaterTranslationSettings::onGoogleKeyEdited()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::onGoogleKeyEdited()
// [/SL:KB]
{
	if (mGoogleAPIKeyEditor->isDirty())
	{
		setGoogleVerified(false, false);
	}
}

//void LLFloaterTranslationSettings::onBtnBingVerify()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::onBtnBingVerify()
// [/SL:KB]
{
	std::string key = getEnteredBingKey();
	if (!key.empty())
	{
		verifyKey(LLTranslate::SERVICE_BING, key);
	}
}

//void LLFloaterTranslationSettings::onBtnGoogleVerify()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::onBtnGoogleVerify()
// [/SL:KB]
{
	std::string key = getEnteredGoogleKey();
	if (!key.empty())
	{
		verifyKey(LLTranslate::SERVICE_GOOGLE, key);
	}
}

// [SL:KB] - updateControlsEnabledState() already takes care of this and is called by both setBingVerified() and setGoogleVerified()
//void LLFloaterTranslationSettings::onClose(bool app_quitting)
//{
//	std::string service = gSavedSettings.getString("TranslationService");
//	bool bing_selected = service == "bing";
//	bool google_selected = service == "google";
//
//	bool service_verified = (bing_selected && mBingKeyVerified) || (google_selected && mGoogleKeyVerified);
//	gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);
//
//}

// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::saveSettings()
{
	LLPanelPreference::saveSettings();

	// See apply() below
	const char* strControls[] =
		{
			"TranslateChat", "TranslateLanguage", "TranslationService", "BingTranslateAPIKey", "GoogleTranslateAPIKey"
		};
	for (int idxControl = 0, cntControl = sizeof(strControls) / sizeof(char*); idxControl < cntControl; idxControl++)
	{
		LLControlVariable* pControl = gSavedSettings.getControl(strControls[idxControl]);
		if (pControl)
		{
			mSavedValues[pControl] = pControl->getValue();
		}
	}
}
// [/SL:KB]

//void LLFloaterTranslationSettings::onBtnOK()
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
void LLPanelPreferenceTranslationSettings::apply()
// [/SL:KB]
{
// [SL:KB] - Patch: Preferences-Translation | Checked: 2014-03-03 (Catznip-3.6)
	LLPanelPreference::apply();
// [/SL:KB]

	gSavedSettings.setBOOL("TranslateChat", mMachineTranslationCB->getValue().asBoolean());
	gSavedSettings.setString("TranslateLanguage", mLanguageCombo->getSelectedValue().asString());
	gSavedSettings.setString("TranslationService", getSelectedService());
	gSavedSettings.setString("BingTranslateAPIKey", getEnteredBingKey());
	gSavedSettings.setString("GoogleTranslateAPIKey", getEnteredGoogleKey());
	(LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))->
			showTranslationCheckbox(LLTranslate::isTranslationConfigured());
//	closeFloater(false);
}
