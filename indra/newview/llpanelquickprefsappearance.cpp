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

#include "llcheckboxctrl.h"
#include "llpanelquickprefsappearance.h"
#include "llviewercontrol.h"

// Appearance panel
#include "llavatarrendernotifier.h"
#include "llvoavatarself.h"

// Wearing panel
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llpanelwearing.h"

// ====================================================================================
// LLQuickPrefsAppearancePanel class
//

static LLPanelInjector<LLQuickPrefsAppearancePanel> t_quickprefs_appearance("quickprefs_appearance");

// From llviewermenu.cpp
void menu_toggle_attached_lights(void* user_data);
void menu_toggle_attached_particles(void* user_data);

LLQuickPrefsAppearancePanel::LLQuickPrefsAppearancePanel()
	: LLQuickPrefsPanel()
{
}

LLQuickPrefsAppearancePanel::~LLQuickPrefsAppearancePanel()
{
	if (m_ComplexityChangedSlot.connected())
		m_ComplexityChangedSlot.disconnect();
	if (m_VisibilityChangedSlot.connected())
		m_VisibilityChangedSlot.disconnect();
}

// virtual
BOOL LLQuickPrefsAppearancePanel::postBuild()
{
	m_ComplexityChangedSlot = LLAvatarRenderNotifier::instance().addComplexityChangedCallback(boost::bind(&LLQuickPrefsAppearancePanel::refreshComplexity, this));
	m_VisibilityChangedSlot = LLAvatarRenderNotifier::instance().addVisibilityChangedCallback(boost::bind(&LLQuickPrefsAppearancePanel::refreshComplexity, this));

	m_pComplexityText = getChild<LLTextBox>("appearance_complexity_value");
	m_pVisibilityText = getChild<LLTextBox>("appearance_visibility_value");
	m_pMaxComplexityText = getChild<LLTextBox>("appearance_maxcomplexity_text");
	m_pMaxNonImpostorsText = getChild<LLTextBox>("appearance_nonimpostors_value");

	getChild<LLCheckBoxCtrl>("appearance_attachedlights_check")->setCommitCallback(boost::bind(&menu_toggle_attached_lights, nullptr));
	getChild<LLCheckBoxCtrl>("appearance_attachedparticles_check")->setCommitCallback(boost::bind(&menu_toggle_attached_particles, nullptr));

	return LLQuickPrefsPanel::postBuild();
}

// virtual
void LLQuickPrefsAppearancePanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
	{
		refreshComplexity();
	}
}

void LLQuickPrefsAppearancePanel::refreshComplexity()
{
	if (isInVisibleChain())
	{
		U32 nComplexity = (isAgentAvatarValid()) ? gAgentAvatarp->getVisualComplexity() : 0;
		m_pComplexityText->setText(llformat("%d", nComplexity));

		LLAvatarRenderNotifier* pAvRenderNotif = LLAvatarRenderNotifier::getInstance();
		m_pVisibilityText->setText(llformat("%d / %d (%.0f%%)", pAvRenderNotif->getLatestAgentsCount() - pAvRenderNotif->getLatestOverLimitAgents(),
		                                                        pAvRenderNotif->getLatestAgentsCount(),
																100.f - pAvRenderNotif->getLatestOverLimitPct()));
	}
}

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
