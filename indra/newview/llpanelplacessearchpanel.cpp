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
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llpanelplacessearch.h"
#include "llpanelplacessearchpanel.h"

// ============================================================================
// LLPanelPlacesSearchPanel class
//

LLPanelPlacesSearchPanel::LLPanelPlacesSearchPanel()
	: LLPanelPlacesTab()
	, m_pSearchPanel(NULL)

{
	buildFromFile("panel_places_search_panel.xml");
}

LLPanelPlacesSearchPanel::~LLPanelPlacesSearchPanel()
{
}

BOOL LLPanelPlacesSearchPanel::postBuild()
{
	m_pSearchPanel = findChild<LLPanelPlacesSearch>("search_panel");
	m_pSearchPanel->setSelectCallback(boost::bind(&LLPanelPlacesSearchPanel::onSearchResultSelect, this));
	m_pSearchPanel->setRefreshOnCategoryChange(true);
	m_pSearchPanel->setRefreshOnMaturityToggle(true);

	updateVerbs();

	return TRUE;
}

// ============================================================================
// LLPanelPlacesTab overrides
//

bool LLPanelPlacesSearchPanel::isSingleItemSelected()
{
	return m_pSearchPanel->hasCurrentParcel();
}

void LLPanelPlacesSearchPanel::onSearchEdit(const std::string& strQuery)
{
	setFilterSubString(strQuery);

	if (!strQuery.empty())
	{
		m_pSearchPanel->searchStart(strQuery);
	}
	else
	{
		m_pSearchPanel->searchClear();
		findChild<LLUICtrl>("search_info_panel")->setVisible(false);
	}
	updateVerbs();
}

void LLPanelPlacesSearchPanel::onShowOnMap()
{
	LLVector3d posGlobal = m_pSearchPanel->getCurrentParcelPos();
	if (!posGlobal.isExactlyZero())
	{
		LLFloaterWorldMap::getInstance()->trackLocation(posGlobal);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

void LLPanelPlacesSearchPanel::onShowProfile()
{
	LLUUID idParcel = m_pSearchPanel->getCurrentParcelId();
	if (idParcel.notNull())
	{
		LLSD sdKey;
		sdKey["type"] = "remote_place";
		sdKey["id"] = idParcel;
		LLFloaterReg::showInstance("parcel_info", sdKey);
	}
}

void LLPanelPlacesSearchPanel::onTeleport()
{
	LLVector3d posGlobal = m_pSearchPanel->getCurrentParcelPos();
	if (!posGlobal.isExactlyZero())
	{
		gAgent.teleportViaLocation(posGlobal);
	}
}

void LLPanelPlacesSearchPanel::updateVerbs()
{
	if (isTabVisible())
	{
		bool fSelection = m_pSearchPanel->hasCurrentParcel();
		mShowOnMapBtn->setEnabled(fSelection);
		mShowProfile->setEnabled(fSelection);
		mTeleportBtn->setEnabled(fSelection);
	}
}

void LLPanelPlacesSearchPanel::onSearchResultSelect()
{
	findChild<LLUICtrl>("search_info_panel")->setVisible(m_pSearchPanel->getCurrentParcelId().notNull());

	updateVerbs();
}

// ============================================================================
