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

#include "llviewerprecompiledheaders.h"

#include "llavatariconctrl.h"
#include "llavatariconctrl.h"
#include "llpanelimcontrolpanel.h"
#include "lltrans.h"

// ============================================================================
// LLPanelIMControlPanel class
//

LLPanelIMControlPanel::LLPanelIMControlPanel(const LLUUID& idAvatar)
	: LLPanel()
	, LLAvatarPropertiesObserver()
	, mAvatarId(idAvatar)
	, mRequestSent(false)
{
	buildFromFile("panel_im_control_panel.xml");
}

LLPanelIMControlPanel::~LLPanelIMControlPanel()
{
	if (mAvatarId.notNull())
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
}

void LLPanelIMControlPanel::refreshFromProperties()
{
	if ( (mAvatarId.notNull()) && (!mRequestSent) )
	{
		getChild<LLAvatarIconCtrl>("avatar_icon")->setValue(mAvatarId);

		LLAvatarPropertiesProcessor* pAvPropProcessor = LLAvatarPropertiesProcessor::getInstance();
		pAvPropProcessor->addObserver(mAvatarId, this);
		pAvPropProcessor->sendAvatarPropertiesRequest(mAvatarId);

		mRequestSent = true;
	}
}

BOOL LLPanelIMControlPanel::postBuild()
{
	getChild<LLUICtrl>("avatar_description")->setValue(LLStringUtil::null);
	return TRUE;
}

void LLPanelIMControlPanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
		refreshFromProperties();
	return LLPanel::onVisibilityChange(fVisible);
}

void LLPanelIMControlPanel::processProperties(void* pData, EAvatarProcessorType eType)
{
	if (APT_PROPERTIES == eType)
	{
		LLAvatarData* pAvatarData = static_cast<LLAvatarData*>(pData);
		if (pAvatarData)
		{
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(pAvatarData->agent_id, this);
			if (pAvatarData->avatar_id != mAvatarId)
				return;

			std::string strBirthDate = LLTrans::getString("AvatarBirthDateFormat");
			LLStringUtil::format(strBirthDate, LLSD().with("datetime", (S32)pAvatarData->born_on.secondsSinceEpoch()));
			getChild<LLUICtrl>("avatar_birthdate")->setValue(strBirthDate);

			getChild<LLUICtrl>("avatar_website")->setValue(pAvatarData->profile_url);
			getChild<LLUICtrl>("avatar_description")->setValue(pAvatarData->about_text);

			LLAvatarIconIDCache::getInstance()->add(mAvatarId, pAvatarData->image_id);
		}
	}
}

// ============================================================================
