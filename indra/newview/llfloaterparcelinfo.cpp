/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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
#include "llfloaterparcelinfo.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llpanelparcelinfo.h"

// ============================================================================
// LLFloaterParcelInfo class
//

LLFloaterParcelInfo::LLFloaterParcelInfo(const LLSD& sdKey)
	: LLFloater(sdKey)
	, m_pParcelInfo(NULL)
{
}

LLFloaterParcelInfo::~LLFloaterParcelInfo()
{
}

BOOL LLFloaterParcelInfo::postBuild()
{
	m_pParcelInfo = findChild<LLPanelParcelInfo>("panel_parcel_info");

	findChild<LLUICtrl>("teleport_btn")->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickTeleport, this));
	findChild<LLUICtrl>("map_btn")->setCommitCallback(boost::bind(&LLFloaterParcelInfo::onClickShowOnMap, this));

	return TRUE;
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
	}
	else if ("landmark" == strType)
	{
		m_pParcelInfo->setParcelFromItem(sdKey["id"].asUUID());
	}
	else
	{
		m_pParcelInfo->clearLocation();
	}
}

void LLFloaterParcelInfo::onClickShowOnMap()
{
	LLVector3d posGlobal = m_pParcelInfo->getCurrentParcelPos();
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
	LLVector3d posGlobal = m_pParcelInfo->getCurrentParcelPos();
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

// ============================================================================
