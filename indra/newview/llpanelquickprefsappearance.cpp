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
#include "llfloaterreg.h"
#include "llpanelquickprefsappearance.h"
#include "llsliderctrl.h"
#include "lltrans.h"
#include "llviewercontrol.h"

// Appearance panel
#include "llavatarrendernotifier.h"
#include "llfloaterpreference.h"
#include "llnotificationtemplate.h"
#include "llvoavatarself.h"

// Wearing panel
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llpanelwearing.h"

// ====================================================================================
// Externals
//

// Defined in llviewermenu.cpp
void menu_toggle_attached_lights(void* user_data);
void menu_toggle_attached_particles(void* user_data);

// ====================================================================================
// LLQuickPrefsAppearancePanel class
//

static LLPanelInjector<LLQuickPrefsAppearancePanel> t_quickprefs_appearance("quickprefs_appearance");

const std::string LLQuickPrefsAppearancePanel::s_strNotifications[] = { "AgentComplexity", "AgentComplexityWithVisibility" };

LLQuickPrefsAppearancePanel::LLQuickPrefsAppearancePanel()
	: LLQuickPrefsPanel()
{
}

LLQuickPrefsAppearancePanel::~LLQuickPrefsAppearancePanel()
{
	if (m_ComplexityChangedSlot.connected())
		m_ComplexityChangedSlot.disconnect();
	if (m_HoverChangedSlot.connected())
		m_HoverChangedSlot.disconnect();
	if (m_VisibilityChangedSlot.connected())
		m_VisibilityChangedSlot.disconnect();
	if (m_MaxComplexityChangedSlot.connected())
		m_MaxComplexityChangedSlot.disconnect();
	if (m_MaxNonImpostorsChangedSlot.connected())
		m_MaxNonImpostorsChangedSlot.disconnect();
}

// virtual
BOOL LLQuickPrefsAppearancePanel::postBuild()
{
	m_ComplexityChangedSlot = LLAvatarRenderNotifier::instance().addComplexityChangedCallback(boost::bind(&LLQuickPrefsAppearancePanel::refreshComplexity, this));
	m_VisibilityChangedSlot = LLAvatarRenderNotifier::instance().addVisibilityChangedCallback(boost::bind(&LLQuickPrefsAppearancePanel::refreshComplexity, this));
	m_HoverChangedSlot = gSavedPerAccountSettings.getControl("AvatarHoverOffsetZ")->getCommitSignal()->connect(boost::bind(&LLQuickPrefsAppearancePanel::refreshHover, this));

	m_pHoverSlider = getChild<LLSliderCtrl>("appearance_hover_value");
	m_pHoverSlider->setMinValue(MIN_HOVER_Z);
	m_pHoverSlider->setMaxValue(MAX_HOVER_Z);
	m_pHoverSlider->setSliderMouseUpCallback(boost::bind(&LLQuickPrefsAppearancePanel::onHoverChange, this, true));
	m_pHoverSlider->setSliderEditorCommitCallback(boost::bind(&LLQuickPrefsAppearancePanel::onHoverChange, this, true));
	m_pHoverSlider->setCommitCallback(boost::bind(&LLQuickPrefsAppearancePanel::onHoverChange, this, false));

	m_pComplexityText = getChild<LLTextBox>("appearance_complexity_value");
	m_pVisibilityText = getChild<LLTextBox>("appearance_visibility_value");

	m_pShowNotificationsCheck = getChild<LLCheckBoxCtrl>("appearance_notifications_check");
	m_pShowNotificationsCheck->setCommitCallback(boost::bind(&LLQuickPrefsAppearancePanel::onShowNotificationsToggle, this));

	m_pMaxComplexitySlider = getChild<LLSliderCtrl>("appearance_maxcomplexity_slider");
	m_pMaxComplexityText = getChild<LLTextBox>("appearance_maxcomplexity_text");
	m_pMaxComplexitySlider->setCommitCallback(boost::bind(&LLQuickPrefsAppearancePanel::onMaxComplexityChange, this));
	m_MaxComplexityChangedSlot = gSavedSettings.getControl("RenderAvatarMaxComplexity")->getSignal()->connect(boost::bind(&LLQuickPrefsAppearancePanel::refreshMaxComplexity, this));

	m_pMaxNonImpostorsSlider = getChild<LLSliderCtrl>("appearance_nonimpostors_slider");
	m_pMaxNonImpostorsText = getChild<LLTextBox>("appearance_nonimpostors_value");
	m_pMaxNonImpostorsSlider->setCommitCallback(boost::bind(&LLQuickPrefsAppearancePanel::onMaxNonImpostorsChange, this));
	m_MaxNonImpostorsChangedSlot = m_pMaxNonImpostorsSlider->getControlVariable()->getSignal()->connect(boost::bind(&LLQuickPrefsAppearancePanel::refreshMaxNonImpostors, this));

	getChild<LLCheckBoxCtrl>("appearance_attachedlights_check")->setCommitCallback(boost::bind(&menu_toggle_attached_lights, nullptr));
	getChild<LLCheckBoxCtrl>("appearance_attachedparticles_check")->setCommitCallback(boost::bind(&menu_toggle_attached_particles, nullptr));

	// Calling LLAvatarComplexityControls::setIndirectControls repeatedly will continue to lower max complexity values due to direct<->indirect value conversions
	if (gSavedSettings.getU32("IndirectMaxComplexity") == 0)
		LLAvatarComplexityControls::setIndirectControls();

	return LLQuickPrefsPanel::postBuild();
}

// virtual
void LLQuickPrefsAppearancePanel::onVisibilityChange(BOOL fVisible)
{
	if (fVisible)
	{
		refreshComplexity();
		refreshMaxComplexity();
		refreshMaxNonImpostors();
		refreshNotifications();
	}
}

void LLQuickPrefsAppearancePanel::refreshComplexity()
{
	if (isInVisibleChain())
	{
		U32 nComplexity = (isAgentAvatarValid()) ? gAgentAvatarp->getVisualComplexity() : 0;
		m_pComplexityText->setText(llformat("%d", nComplexity));

		LLAvatarRenderNotifier* pAvRenderNotif = LLAvatarRenderNotifier::getInstance();
		if (pAvRenderNotif->getLatestAgentsCount() > 0)
		{
			m_pVisibilityText->setText(llformat("%d / %d (%.0f%%)", pAvRenderNotif->getLatestAgentsCount() - pAvRenderNotif->getLatestOverLimitAgents(),
																	pAvRenderNotif->getLatestAgentsCount(),
																	100.f - pAvRenderNotif->getLatestOverLimitPct()));
		}
		else
		{
			m_pVisibilityText->setText(llformat("- / -"));

		}
	}
}

void LLQuickPrefsAppearancePanel::refreshHover()
{
	if (isInVisibleChain())
	{
		F32 nHoverValue = gSavedPerAccountSettings.getF32("AvatarHoverOffsetZ");
		m_pHoverSlider->setValue(nHoverValue, false);
	}
}

void LLQuickPrefsAppearancePanel::refreshMaxComplexity()
{
	if (isInVisibleChain())
	{
		U32 nMaxComplexity = gSavedSettings.getU32("RenderAvatarMaxComplexity");
		m_pMaxComplexityText->setText( (0 != nMaxComplexity) ? llformat("%d", nMaxComplexity) : LLTrans::getString("no_limit") );
	}
}

void LLQuickPrefsAppearancePanel::refreshMaxNonImpostors()
{
	if (isInVisibleChain())
	{
		U32 nMaxNonImpostors = (U32)m_pMaxNonImpostorsSlider->getValue().asInteger();
		m_pMaxNonImpostorsText->setText( (0 != nMaxNonImpostors) ? llformat("%d", nMaxNonImpostors) : LLTrans::getString("no_limit") );
	}
}

void LLQuickPrefsAppearancePanel::refreshNotifications()
{
	bool fShowNotifications = false;
	for (int idxNotif = 0, cntNotif = sizeof(s_strNotifications) / sizeof(std::string); idxNotif < cntNotif; idxNotif++)
	{
		LLNotificationTemplatePtr notifPtr = LLNotifications::instance().getTemplate(s_strNotifications[idxNotif]);
		fShowNotifications |= !notifPtr->mForm->getIgnored();
	}

	m_pShowNotificationsCheck->set(fShowNotifications);
}

void LLQuickPrefsAppearancePanel::onHoverChange(bool fCommit)
{
	F32 nHoverValue = m_pHoverSlider->getValueF32();
	LLVector3 vecHoverOffset(0.0, 0.0, llclamp(nHoverValue, MIN_HOVER_Z, MAX_HOVER_Z));
	if (fCommit)
		gSavedPerAccountSettings.setF32("AvatarHoverOffsetZ", nHoverValue);
	gAgentAvatarp->setHoverOffset(vecHoverOffset, fCommit);
}

void LLQuickPrefsAppearancePanel::onMaxComplexityChange()
{
	LLAvatarComplexityControls::updateMax(m_pMaxComplexitySlider, m_pMaxComplexityText);
}

void LLQuickPrefsAppearancePanel::onMaxNonImpostorsChange()
{
	// Updates to the debug setting will trigger its commit signal
	LLFloaterPreferenceGraphicsAdvanced::updateMaxNonImpostors(m_pMaxNonImpostorsSlider->getValue().asInteger());
}

void LLQuickPrefsAppearancePanel::onShowNotificationsToggle()
{
	for (int idxNotif = 0, cntNotif = sizeof(s_strNotifications) / sizeof(std::string); idxNotif < cntNotif; idxNotif++)
	{
		LLNotificationTemplatePtr notifPtr = LLNotifications::instance().getTemplate(s_strNotifications[idxNotif]);
		notifPtr->mForm->setIgnored(!m_pShowNotificationsCheck->get());
	}

	LLFloaterPreference* pPrefsFloater = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if ( (pPrefsFloater) && (pPrefsFloater->isInVisibleChain()) )
	{
		pPrefsFloater->buildPopupLists();
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
