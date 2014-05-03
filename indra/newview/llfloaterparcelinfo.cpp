/** 
 *
 * Copyright (c) 2012-2013, Kitty Barnett
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
#include "llbutton.h"
#include "llfloaterparcelinfo.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llinventorymodel.h"
#include "llpanelparcelinfo.h"
#include "llviewerinventory.h"

// ============================================================================
// LLFloaterParcelInfo class
//

LLFloaterParcelInfo::LLFloaterParcelInfo(const LLSD& sdKey)
	: LLFloater(sdKey)
	, m_pParcelInfo(NULL)
	, m_pTeleportBtn(NULL)
	, m_pMapBtn(NULL)
	, m_pSaveBtn(NULL)
	, m_pDiscardBtn(NULL)
{
}

LLFloaterParcelInfo::~LLFloaterParcelInfo()
{
}

BOOL LLFloaterParcelInfo::postBuild()
{
	m_pParcelInfo = findChild<LLPanelParcelInfo>("panel_parcel_info");

	m_pTeleportBtn = findChild<LLButton>("teleport_btn");
	m_pTeleportBtn->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickTeleport, this));

	m_pMapBtn = findChild<LLButton>("map_btn");
	m_pMapBtn->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickShowOnMap, this));

	m_pSaveBtn = findChild<LLButton>("save_btn");
	m_pSaveBtn->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickSave, this));

	m_pDiscardBtn = findChild<LLButton>("discard_btn");
	m_pDiscardBtn->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickDiscard, this));

	return TRUE;
}

void LLFloaterParcelInfo::refreshControls()
{
	bool fEditMode = m_pParcelInfo->getEditMode();
	m_pTeleportBtn->setVisible(!fEditMode);
	m_pMapBtn->setVisible(!fEditMode);
	m_pSaveBtn->setVisible(fEditMode);
	m_pDiscardBtn->setVisible(fEditMode);
}

void LLFloaterParcelInfo::onOpen(const LLSD& sdKey)
{
	const std::string strType = sdKey["type"].asString();
	if ("remote_place" == strType)
	{
		if (sdKey.has("id"))
		{
			m_pParcelInfo->setParcelFromId(sdKey["id"].asUUID());
		}
		else
		{
			LLVector3d posGlobal(sdKey["x"].asReal(), sdKey["y"].asReal(), sdKey["z"].asReal());
			if (!posGlobal.isExactlyZero())
				m_pParcelInfo->setParcelFromPos(posGlobal);
		}

		setTitle(getString("title_parcel"));
	}
	else if ("landmark" == strType)
	{
		m_pParcelInfo->setParcelFromItem(sdKey["id"].asUUID());
		m_pParcelInfo->setEditMode( (sdKey.has("action")) && ("edit" == sdKey["action"].asString()) );

		setTitle(getString("title_landmark"));
	}
	else
	{
		m_pParcelInfo->clearLocation();
	}
	refreshControls();
}

void LLFloaterParcelInfo::onClickShowOnMap()
{
	const LLVector3d posGlobal = m_pParcelInfo->getGlobalPos();
	if (!posGlobal.isExactlyZero())
	{
		LLSD sdParams;
		sdParams["target"]["x"] = posGlobal.mdV[VX];
		sdParams["target"]["y"] = posGlobal.mdV[VY];
		sdParams["target"]["z"] = posGlobal.mdV[VZ];
		sdParams["center"] = true;

		LLFloaterReg::showInstance("world_map", sdParams);
	}
}

void LLFloaterParcelInfo::onClickTeleport()
{
	const LLVector3d posGlobal = m_pParcelInfo->getGlobalPos();
	if (!posGlobal.isExactlyZero())
	{
		LLFloaterWorldMap* pWorldMap = LLFloaterWorldMap::getInstance();
		if (pWorldMap)
		{
			gAgent.teleportViaLocation(posGlobal);
			pWorldMap->trackLocation(posGlobal);
		}
	}
}

const LLUUID LLFloaterParcelInfo::getItemId() /*const*/
{
	const LLSD& sdKey = getKey();
	return ("landmark" == sdKey["type"].asString()) ? sdKey["id"].asUUID() : LLUUID::null;
}

void LLFloaterParcelInfo::onClickSave()
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(getItemId());
	if ( (pItem) && (LLAssetType::AT_LANDMARK == pItem->getType()) && (m_pParcelInfo->getEditMode()) )
	{
		std::string strName = m_pParcelInfo->getEditName();
		LLStringUtil::trim(strName);
		std::string strDescription = m_pParcelInfo->getEditDescription();
		LLStringUtil::trim(strDescription);

		if ( (!strName.empty()) && ((strName != pItem->getName()) || (strDescription != pItem->getDescription())) )
		{
			LLPointer<LLViewerInventoryItem> pNewItem = new LLViewerInventoryItem(pItem);
			pNewItem->rename(strName);
			pNewItem->setDescription(strDescription);
			pNewItem->updateServer(FALSE);

			gInventory.updateItem(pNewItem);
			gInventory.notifyObservers();
		}
	}

	closeFloater();
}

void LLFloaterParcelInfo::onClickDiscard()
{
	LLViewerInventoryItem* pItem = gInventory.getItem(getItemId());
	if ( (pItem) && (LLAssetType::AT_LANDMARK == pItem->getType()) )
	{
		const LLUUID idTrash = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (idTrash.notNull())
		{
			gInventory.changeItemParent(pItem, idTrash, TRUE);
		}
	}

	closeFloater();
}

// ============================================================================
