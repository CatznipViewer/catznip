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

#include "llcombobox.h"
#include "llfloaterchatalerts.h"
#include "llfloaterchatsounds.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

// ============================================================================
// LLFloaterChatSounds class
//

LLFloaterChatSounds::LLFloaterChatSounds(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

LLFloaterChatSounds::~LLFloaterChatSounds()
{
}

BOOL LLFloaterChatSounds::postBuild(void)
{
	LLComboBox* pIMFriendSound = findChild<LLComboBox>("sound_im_friend");
	findChild<LLSoundDropTarget>("drop_im_friend")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMFriendSound));
	findChild<LLButton>("preview_sound_im_friend")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMFriendSound));

	LLComboBox* pIMNonFriendSound = findChild<LLComboBox>("sound_im_nonfriend");
	findChild<LLSoundDropTarget>("drop_im_nonfriend")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMNonFriendSound));
	findChild<LLButton>("preview_sound_im_nonfriend")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMNonFriendSound));

	LLComboBox* pIMConferenceSound = findChild<LLComboBox>("sound_im_conference");
	findChild<LLSoundDropTarget>("drop_im_conference")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMConferenceSound));
	findChild<LLButton>("preview_sound_im_conference")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMConferenceSound));

	LLComboBox* pIMGroupSound = findChild<LLComboBox>("sound_im_group");
	findChild<LLSoundDropTarget>("drop_im_group")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pIMGroupSound));
	findChild<LLButton>("preview_sound_im_group")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pIMGroupSound));

	LLComboBox* pChatNearbySound = findChild<LLComboBox>("sound_chat_nearby");
	findChild<LLSoundDropTarget>("drop_chat_nearby")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pChatNearbySound));
	findChild<LLButton>("preview_sound_chat_nearby")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pChatNearbySound));

	LLComboBox* pChatObjectSound = findChild<LLComboBox>("sound_chat_object");
	findChild<LLSoundDropTarget>("drop_chat_object")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pChatObjectSound));
	findChild<LLButton>("preview_sound_chat_object")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pChatObjectSound));

	LLComboBox* pConversationStartSound = findChild<LLComboBox>("sound_new_conversation");
	findChild<LLSoundDropTarget>("drop_new_conversation")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pConversationStartSound));
	findChild<LLButton>("preview_sound_new_conversation")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pConversationStartSound));

	LLComboBox* pVoiceCallSound = findChild<LLComboBox>("sound_voice_call");
	findChild<LLSoundDropTarget>("drop_voice_call")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pVoiceCallSound));
	findChild<LLButton>("preview_sound_voice_call")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pVoiceCallSound));

	LLComboBox* pTeleportOfferSound = findChild<LLComboBox>("sound_teleport_offer");
	findChild<LLSoundDropTarget>("drop_teleport_offer")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pTeleportOfferSound));
	findChild<LLButton>("preview_sound_teleport_offer")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pTeleportOfferSound));

	LLComboBox* pInventoryOfferSound = findChild<LLComboBox>("sound_inventory_offer");
	findChild<LLSoundDropTarget>("drop_inventory_offer")->setDropCallback(boost::bind(&LLFloaterChatSounds::onDropSound, this, _1, pInventoryOfferSound));
	findChild<LLButton>("preview_sound_inventory_offer")->setCommitCallback(boost::bind(&LLFloaterChatSounds::onPreviewSound, this, pInventoryOfferSound));

	return TRUE;
}

void LLFloaterChatSounds::onDropSound(const LLUUID& idSound, LLComboBox* pCombo)
{
	const LLInventoryItem* pItem = gInventory.getItem(idSound);
	if ( (pItem) && (pCombo) && (pCombo->getEnabled()) )
	{
		if (!pCombo->selectByValue(pItem->getUUID()))
		{
			LLScrollListItem* pNewItem = pCombo->add(pItem->getName(), pItem->getUUID());
			if (pNewItem)
			{
				LLScrollListText* pCell = dynamic_cast<LLScrollListText*>(pNewItem->getColumn(0));
				if (pCell)
				{
					pCell->setIcon(LLInventoryIcon::getIconName(LLInventoryType::ICONNAME_SOUND));
				}
			}
			pCombo->selectByValue(pItem->getUUID());
		}
	}
}

void LLFloaterChatSounds::onPreviewSound(LLComboBox* pCombo)
{
	if (!pCombo)
		return;

	const LLSD& sdValue = pCombo->getSelectedValue();
	if (sdValue.isUUID())
	{
		LLViewerInventoryItem* pItem = gInventory.getItem(sdValue.asUUID());
		if (pItem)
		{
			make_ui_sound(pItem->getAssetUUID());
		}
	}
	else if (sdValue.isString())
	{
		make_ui_sound(sdValue.asString().c_str());
	}
}

// ============================================================================
