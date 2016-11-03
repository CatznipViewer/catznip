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

#include "llpanelquickprefsappearance.h"
#include "llviewercontrol.h"

// Wearing panel
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llpanelwearing.h"

// ====================================================================================
// LLQuickPrefsWearingPanel class
//

static LLPanelInjector<LLQuickPrefsWearingPanel> t_quickprefs_wearing("quickprefs_wearing");

LLQuickPrefsWearingPanel::LLQuickPrefsWearingPanel()
	: LLQuickPrefsPanel()
{
	m_pCofObserver = new LLInventoryCategoriesObserver();
}

LLQuickPrefsWearingPanel::~LLQuickPrefsWearingPanel()
{
	if (gInventory.containsObserver(m_pCofObserver))
		gInventory.removeObserver(m_pCofObserver);
	delete m_pCofObserver;
	m_pCofObserver = nullptr;
}

// virtual
BOOL LLQuickPrefsWearingPanel::postBuild()
{
	if (m_idCOF.isNull())
		m_idCOF = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);

	m_pWornItemsList = getChild<LLWornItemsList>("cof_items_list");
	m_pWornItemsList->setSortOrder((LLWearableItemsList::ESortOrder)gSavedSettings.getU32("WearingListSortOrder"));

	return LLQuickPrefsPanel::postBuild();
}

// virtual
void LLQuickPrefsWearingPanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
	{
		if (!isInitialized())
		{
			// Make sure COF is fetched if it isn't already
			if (LLViewerInventoryCategory* pFolder = gInventory.getCategory(m_idCOF))
				pFolder->fetch();

			gInventory.addObserver(m_pCofObserver);
			m_pCofObserver->addCategory(m_idCOF, boost::bind(&LLQuickPrefsWearingPanel::onCOFChanged, this));

			setInitialized();
		}

		m_pWornItemsList->updateList(m_idCOF);
	}
}

void LLQuickPrefsWearingPanel::onCOFChanged()
{
	if (m_pWornItemsList->isInVisibleChain())
	{
		m_pWornItemsList->updateList(m_idCOF);
	}
}

// ====================================================================================
