/**
 * @file llpaneltopinfobar.cpp
 * @brief Coordinates and Parcel Settings information panel definition
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llpaneltopinfobar.h"

#include "llagent.h"
#include "llagentui.h"
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
#include "llappviewer.h"
// [/SL:KB]
#include "llclipboard.h"
#include "llfloatersidepanelcontainer.h"
#include "lllandmarkactions.h"
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
#include "lllayoutstack.h"
// [/SL:KB]
#include "lllocationinputctrl.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

class LLPanelTopInfoBar::LLParcelChangeObserver : public LLParcelObserver
{
public:
	LLParcelChangeObserver(LLPanelTopInfoBar* topInfoBar) : mTopInfoBar(topInfoBar) {}

private:
	/*virtual*/ void changed()
	{
		if (mTopInfoBar)
		{
			mTopInfoBar->updateParcelIcons();
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
			mTopInfoBar->updateLayout();
// [/SL:KB]
		}
	}

	LLPanelTopInfoBar* mTopInfoBar;
};

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
static LLRegisterPanelClassWrapper<LLPanelTopInfoBar> t_topinfo_bar("topinfo_bar");
// [/SL:KB]

LLPanelTopInfoBar::LLPanelTopInfoBar(): mParcelChangedObserver(0)
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
	, mParcelInfoText(NULL)
	, mDamageText(NULL)
	, mMaturityIcon(NULL)
	, mIconsPanel(NULL)
// [/SL:KB]
//	, mInfoBtn(NULL)
{
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
	memset(&mParcelIcon, 0, sizeof(mParcelIcon));
// [/SL:KB]

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
	// set a listener function for LoginComplete event
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLPanelTopInfoBar::handleLoginComplete, this));
// [/SL:KB]
//	buildFromFile( "panel_topinfo_bar.xml");
}

LLPanelTopInfoBar::~LLPanelTopInfoBar()
{
	if (mParcelChangedObserver)
	{
		LLViewerParcelMgr::getInstance()->removeObserver(mParcelChangedObserver);
		delete mParcelChangedObserver;
	}

//	if (mParcelPropsCtrlConnection.connected())
//	{
//		mParcelPropsCtrlConnection.disconnect();
//	}

	if (mParcelMgrConnection.connected())
	{
		mParcelMgrConnection.disconnect();
	}

//	if (mShowCoordsCtrlConnection.connected())
//	{
//		mShowCoordsCtrlConnection.disconnect();
//	}
}

void LLPanelTopInfoBar::initParcelIcons()
{
	mParcelIcon[VOICE_ICON] = getChild<LLIconCtrl>("voice_icon");
	mParcelIcon[FLY_ICON] = getChild<LLIconCtrl>("fly_icon");
	mParcelIcon[PUSH_ICON] = getChild<LLIconCtrl>("push_icon");
	mParcelIcon[BUILD_ICON] = getChild<LLIconCtrl>("build_icon");
	mParcelIcon[SCRIPTS_ICON] = getChild<LLIconCtrl>("scripts_icon");
	mParcelIcon[DAMAGE_ICON] = getChild<LLIconCtrl>("damage_icon");
	mParcelIcon[SEE_AVATARS_ICON] = getChild<LLIconCtrl>("see_avatars_icon");

	mParcelIcon[VOICE_ICON]->setToolTip(LLTrans::getString("LocationCtrlVoiceTooltip"));
	mParcelIcon[FLY_ICON]->setToolTip(LLTrans::getString("LocationCtrlFlyTooltip"));
	mParcelIcon[PUSH_ICON]->setToolTip(LLTrans::getString("LocationCtrlPushTooltip"));
	mParcelIcon[BUILD_ICON]->setToolTip(LLTrans::getString("LocationCtrlBuildTooltip"));
	mParcelIcon[SCRIPTS_ICON]->setToolTip(LLTrans::getString("LocationCtrlScriptsTooltip"));
	mParcelIcon[DAMAGE_ICON]->setToolTip(LLTrans::getString("LocationCtrlDamageTooltip"));
	mParcelIcon[SEE_AVATARS_ICON]->setToolTip(LLTrans::getString("LocationCtrlSeeAVsTooltip"));

	mParcelIcon[VOICE_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, VOICE_ICON));
	mParcelIcon[FLY_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, FLY_ICON));
	mParcelIcon[PUSH_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, PUSH_ICON));
	mParcelIcon[BUILD_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, BUILD_ICON));
	mParcelIcon[SCRIPTS_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, SCRIPTS_ICON));
	mParcelIcon[DAMAGE_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, DAMAGE_ICON));
	mParcelIcon[SEE_AVATARS_ICON]->setMouseDownCallback(boost::bind(&LLPanelTopInfoBar::onParcelIconClick, this, SEE_AVATARS_ICON));

	mDamageText->setText(LLStringExplicit("100%"));
}

void LLPanelTopInfoBar::handleLoginComplete()
{
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
	gMenuBarView->setResizeCallback(boost::bind(&LLPanelTopInfoBar::handleLayoutChange, this));
	handleLayoutChange();
// [/SL:KB]

	// An agent parcel update hasn't occurred yet, so
	// we have to manually set location and the icons.
	update();
}

BOOL LLPanelTopInfoBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(!LLUICtrl::CommitCallbackRegistry::getValue("TopInfoBar.Action"))
	{
		LLUICtrl::CommitCallbackRegistry::currentRegistrar()
				.add("TopInfoBar.Action", boost::bind(&LLPanelTopInfoBar::onContextMenuItemClicked, this, _2));
	}
	show_topinfobar_context_menu(this, x, y);
	return TRUE;
}

BOOL LLPanelTopInfoBar::postBuild()
{
//	mInfoBtn = getChild<LLButton>("place_info_btn");
//	mInfoBtn->setClickedCallback(boost::bind(&LLPanelTopInfoBar::onInfoButtonClicked, this));
//	mInfoBtn->setToolTip(LLTrans::getString("LocationCtrlInfoBtnTooltip"));

	mParcelInfoText = getChild<LLTextBox>("parcel_info_text");
	mDamageText = getChild<LLTextBox>("damage_text");
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	mMaturityIcon = getChild<LLIconCtrl>("maturity_icon");
	mIconsPanel = getChild<LLPanel>("icons_panel");
// [/SL:KB]

	initParcelIcons();

	mParcelChangedObserver = new LLParcelChangeObserver(this);
	LLViewerParcelMgr::getInstance()->addObserver(mParcelChangedObserver);

//	// Connecting signal for updating parcel icons on "Show Parcel Properties" setting change.
//	LLControlVariable* ctrl = gSavedSettings.getControl("NavBarShowParcelProperties").get();
//	if (ctrl)
//	{
//		mParcelPropsCtrlConnection = ctrl->getSignal()->connect(boost::bind(&LLPanelTopInfoBar::updateParcelIcons, this));
//	}

//	// Connecting signal for updating parcel text on "Show Coordinates" setting change.
//	ctrl = gSavedSettings.getControl("NavBarShowCoordinates").get();
//	if (ctrl)
//	{
//		mShowCoordsCtrlConnection = ctrl->getSignal()->connect(boost::bind(&LLPanelTopInfoBar::onNavBarShowParcelPropertiesCtrlChanged, this));
//	}

	mParcelMgrConnection = LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(
			boost::bind(&LLPanelTopInfoBar::onAgentParcelChange, this));

//	setVisibleCallback(boost::bind(&LLPanelTopInfoBar::onVisibilityChange, this, _2));

	return TRUE;
}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
void LLPanelTopInfoBar::handleLayoutChange()
{
	LLView* pStatus = gStatusBar->findChildView("status_panel");
	if (pStatus)
	{
		LLRect rctStatus = pStatus->getRect();
		rctStatus.mLeft = gMenuBarView->getRect().getWidth() + 10;
		rctStatus.mRight = rctStatus.mRight;
		pStatus->setShape(rctStatus);
	}
}

void LLPanelTopInfoBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	updateLayout();
}
// [/SL:KB]

//void LLPanelTopInfoBar::onNavBarShowParcelPropertiesCtrlChanged()
//{
//	std::string new_text;
//
//	// don't need to have separate show_coords variable; if user requested the coords to be shown
//	// they will be added during the next call to the draw() method.
//	buildLocationString(new_text, false);
//	setParcelInfoText(new_text);
//}

// when panel is shown, all minimized floaters should be shifted downwards to prevent overlapping of
// PanelTopInfoBar. See EXT-7951.
//void LLPanelTopInfoBar::onVisibilityChange(const LLSD& show)
//{
//	// this height is used as a vertical offset for ALREADY MINIMIZED floaters
//	// when PanelTopInfoBar visibility changes
//	S32 height = getRect().getHeight();
//
//	// this vertical offset is used for a start minimize position of floaters that
//	// are NOT MIMIMIZED YET
//	S32 minimize_pos_offset = 0;
//
//	if (show.asBoolean())
//	{
//		height = minimize_pos_offset = -height;
//	}
//
//	gFloaterView->shiftFloaters(0, height);
//	gFloaterView->setMinimizePositionVerticalOffset(minimize_pos_offset);
//}

//boost::signals2::connection LLPanelTopInfoBar::setResizeCallback( const resize_signal_t::slot_type& cb )
//{
//	return mResizeSignal.connect(cb);
//}

void LLPanelTopInfoBar::draw()
{
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
	if (getVisible())
	{
		updateParcelInfoText(true);
		updateHealth();
	}
// [/SL:KB]
//	updateParcelInfoText();
//	updateHealth();

	LLPanel::draw();
}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
void LLPanelTopInfoBar::buildLocationString(std::string& loc_str)
{
	if (!LLAgentUI::buildLocationString(loc_str, LLAgentUI::LOCATION_FORMAT_TOPBAR))
	{
		loc_str = "???";
	}
}
// [/SL:KB]
//void LLPanelTopInfoBar::buildLocationString(std::string& loc_str, bool show_coords)
//{
//	LLAgentUI::ELocationFormat format =
//		(show_coords ? LLAgentUI::LOCATION_FORMAT_FULL : LLAgentUI::LOCATION_FORMAT_NO_COORDS);
//
//	if (!LLAgentUI::buildLocationString(loc_str, format))
//	{
//		loc_str = "???";
//	}
//}

//void LLPanelTopInfoBar::setParcelInfoText(const std::string& new_text)
//{
//	LLRect old_rect = getRect();
//	const LLFontGL* font = mParcelInfoText->getFont();
//	S32 new_text_width = font->getWidth(new_text);
//
//	mParcelInfoText->setText(new_text);
//
//	LLRect rect = mParcelInfoText->getRect();
//	rect.setOriginAndSize(rect.mLeft, rect.mBottom, new_text_width, rect.getHeight());
//
//	mParcelInfoText->reshape(rect.getWidth(), rect.getHeight(), TRUE);
//	mParcelInfoText->setRect(rect);
//	layoutParcelIcons();
//
//	if (old_rect != getRect())
//	{
//		mResizeSignal();
//	}
//}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
void LLPanelTopInfoBar::update()
{
	updateParcelInfoText(false, true);
	updateHealth();
	updateParcelIcons();
	updateLayout();
}
// [/SL:KB]
//void LLPanelTopInfoBar::update()
//{
//	std::string new_text;
//
//	// don't need to have separate show_coords variable; if user requested the coords to be shown
//	// they will be added during the next call to the draw() method.
//	buildLocationString(new_text, false);
//	setParcelInfoText(new_text);
//
//	updateParcelIcons();
//}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2013-09-25 (Catznip-3.6)
void LLPanelTopInfoBar::updateParcelInfoText(bool fUpdateLayout, bool fForceUpdate)
{
	static LLVector3d sPrevPosGlobal;
	if ( (dist_vec_squared(sPrevPosGlobal, gAgent.getPositionGlobal()) >= 0.5f) || (fForceUpdate) )
	{
		std::string strLocation;
		buildLocationString(strLocation);

		mParcelInfoText->setText(strLocation);

		if (fUpdateLayout)
		{
			updateLayout();
		}

		sPrevPosGlobal = gAgent.getPositionGlobal();
	}
}
// [/SL:KB]
//void LLPanelTopInfoBar::updateParcelInfoText()
//{
//	static LLUICachedControl<bool> show_coords("NavBarShowCoordinates", false);
//
//	if (show_coords)
//	{
//		std::string new_text;
//
//		buildLocationString(new_text, show_coords);
//		setParcelInfoText(new_text);
//	}
//}

//void LLPanelTopInfoBar::updateParcelIcons()
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2013-09-25 (Catznip-3.6)
void LLPanelTopInfoBar::updateParcelIcons(bool fUpdateLayout)
// [/SL:KB]
{
	LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();

	LLViewerRegion* agent_region = gAgent.getRegion();
	LLParcel* agent_parcel = vpm->getAgentParcel();
	if (!agent_region || !agent_parcel)
		return;

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	updateMaturity();
// [/SL:KB]

//	if (gSavedSettings.getBOOL("NavBarShowParcelProperties"))
	{
		LLParcel* current_parcel;
		LLViewerRegion* selection_region = vpm->getSelectionRegion();
		LLParcel* selected_parcel = vpm->getParcelSelection()->getParcel();

		// If agent is in selected parcel we use its properties because
		// they are updated more often by LLViewerParcelMgr than agent parcel properties.
		// See LLViewerParcelMgr::processParcelProperties().
		// This is needed to reflect parcel restrictions changes without having to leave
		// the parcel and then enter it again. See EXT-2987
		if (selected_parcel && selected_parcel->getLocalID() == agent_parcel->getLocalID()
				&& selection_region == agent_region)
		{
			current_parcel = selected_parcel;
		}
		else
		{
			current_parcel = agent_parcel;
		}

		bool allow_voice	= vpm->allowAgentVoice(agent_region, current_parcel);
		bool allow_fly		= vpm->allowAgentFly(agent_region, current_parcel);
		bool allow_push		= vpm->allowAgentPush(agent_region, current_parcel);
		bool allow_build	= vpm->allowAgentBuild(current_parcel); // true when anyone is allowed to build. See EXT-4610.
		bool allow_scripts	= vpm->allowAgentScripts(agent_region, current_parcel);
		bool allow_damage	= vpm->allowAgentDamage(agent_region, current_parcel);
		bool see_avs        = current_parcel->getSeeAVs();

		// Most icons are "block this ability"
		mParcelIcon[VOICE_ICON]->setVisible(   !allow_voice );
		mParcelIcon[FLY_ICON]->setVisible(     !allow_fly );
		mParcelIcon[PUSH_ICON]->setVisible(    !allow_push );
		mParcelIcon[BUILD_ICON]->setVisible(   !allow_build );
		mParcelIcon[SCRIPTS_ICON]->setVisible( !allow_scripts );
		mParcelIcon[DAMAGE_ICON]->setVisible(  allow_damage );
		mDamageText->setVisible(allow_damage);
		mParcelIcon[SEE_AVATARS_ICON]->setVisible( !see_avs );

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
		if (fUpdateLayout)
		{
			updateLayout();
		}
	}
// [/SL:KB]
//		layoutParcelIcons();
//	}
//	else
//	{
//		for (S32 i = 0; i < ICON_COUNT; ++i)
//		{
//			mParcelIcon[i]->setVisible(false);
//		}
//		mDamageText->setVisible(false);
//	}
}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
void LLPanelTopInfoBar::updateMaturity()
{
	// Updating maturity rating icon.
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
		return;

	bool fVisible = true;
	std::string strIconName, strTooltip;
	switch (region->getSimAccess())
	{
		case SIM_ACCESS_PG:
			strIconName = "Parcel_PG_Light";
			strTooltip = LLTrans::getString("LocationCtrlGeneralIconTooltip");
			break;
		case SIM_ACCESS_ADULT:
			strIconName = "Parcel_R_Light";
			strTooltip = LLTrans::getString("LocationCtrlAdultIconTooltip");
			break;
		case SIM_ACCESS_MATURE:
			strIconName = "Parcel_M_Light";
			strTooltip = LLTrans::getString("LocationCtrlModerateIconTooltip");
			break;
		default:
			fVisible = false;
			break;
	}

	mMaturityIcon->setVisible(fVisible);
	mMaturityIcon->setToolTip(strTooltip);
	mMaturityIcon->setImage(LLUI::getUIImage(strIconName));
}
// [/SL:KB]

void LLPanelTopInfoBar::updateHealth()
{
//	static LLUICachedControl<bool> show_icons("NavBarShowParcelProperties", false);

	// *FIXME: Status bar owns health information, should be in agent
//	if (show_icons && gStatusBar)
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	if (gStatusBar)
// [/SL:KB]
	{
		static S32 last_health = -1;
		S32 health = gStatusBar->getHealth();
		if (health != last_health)
		{
			std::string text = llformat("%d%%", health);
			mDamageText->setText(text);
			last_health = health;
		}
	}
}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
void LLPanelTopInfoBar::updateLayout()
{
	static const int FIRST_ICON_HPAD = 10;
	static const int LAST_ICON_HPAD = 5;

	// Layout the icons panel
	S32	left = layoutWidget(mParcelIcon[0], FIRST_ICON_HPAD);
	left = layoutWidget(mDamageText, left);
	for (int i = 1; i < ICON_COUNT; i++)
		left = layoutWidget(mParcelIcon[i], left);
	left += LAST_ICON_HPAD;

	// Resize parcel info text to fill any remaining space
	if (mParcelInfoText)
	{
		S32 max_text_width = getRect().getWidth() - left - mParcelInfoText->getRect().mLeft;
		LLRect rect = mParcelInfoText->getRect();
//		rect.mRight = rect.mLeft + llmin(mParcelInfoText->getFont()->getWidth(mParcelInfoText->getWText().c_str()), max_text_width);
		rect.mRight = rect.mLeft + llmin(mParcelInfoText->getTextPixelWidth(), max_text_width);
		mParcelInfoText->reshape(rect.getWidth(), rect.getHeight());
		mParcelInfoText->setRect(rect);
	}

	// Move the icons panel up behind the parcel text
	if ((mIconsPanel) && (mParcelInfoText))
	{
		LLRect rect = mIconsPanel->getRect();
		rect.mLeft = mParcelInfoText->getRect().mRight;
		rect.mRight = rect.mLeft + left;
		mIconsPanel->setRect(rect);
	}
}
// [/SL:KB]
//void LLPanelTopInfoBar::layoutParcelIcons()
//{
//	LLRect old_rect = getRect();
//
//	// TODO: remove hard-coded values and read them as xml parameters
//	static const int FIRST_ICON_HPAD = 32;
//	static const int LAST_ICON_HPAD = 11;
//
//	S32 left = mParcelInfoText->getRect().mRight + FIRST_ICON_HPAD;
//
//	left = layoutWidget(mDamageText, left);
//
//	for (int i = ICON_COUNT - 1; i >= 0; --i)
//	{
//		left = layoutWidget(mParcelIcon[i], left);
//	}
//
//	LLRect rect = getRect();
//	rect.set(rect.mLeft, rect.mTop, left + LAST_ICON_HPAD, rect.mBottom);
//	setRect(rect);
//
//	if (old_rect != getRect())
//	{
//		mResizeSignal();
//	}
//}

S32 LLPanelTopInfoBar::layoutWidget(LLUICtrl* ctrl, S32 left)
{
	// TODO: remove hard-coded values and read them as xml parameters
	static const int ICON_HPAD = 2;

//	if (ctrl->getVisible())
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	if ( (ctrl) && (ctrl->getVisible()) )
// [/SL:KB]
	{
		LLRect rect = ctrl->getRect();
		rect.mRight = left + rect.getWidth();
		rect.mLeft = left;

		ctrl->setRect(rect);
		left += rect.getWidth() + ICON_HPAD;
	}

	return left;
}

void LLPanelTopInfoBar::onParcelIconClick(EParcelIcon icon)
{
	switch (icon)
	{
	case VOICE_ICON:
		LLNotificationsUtil::add("NoVoice");
		break;
	case FLY_ICON:
		LLNotificationsUtil::add("NoFly");
		break;
	case PUSH_ICON:
		LLNotificationsUtil::add("PushRestricted");
		break;
	case BUILD_ICON:
		LLNotificationsUtil::add("NoBuild");
		break;
	case SCRIPTS_ICON:
	{
		LLViewerRegion* region = gAgent.getRegion();
		if(region && region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS))
		{
			LLNotificationsUtil::add("ScriptsStopped");
		}
		else if(region && region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS))
		{
			LLNotificationsUtil::add("ScriptsNotRunning");
		}
		else
		{
			LLNotificationsUtil::add("NoOutsideScripts");
		}
		break;
	}
	case DAMAGE_ICON:
		LLNotificationsUtil::add("NotSafe");
		break;
	case SEE_AVATARS_ICON:
		LLNotificationsUtil::add("SeeAvatars");
		break;
	case ICON_COUNT:
		break;
	// no default to get compiler warning when a new icon gets added
	}
}

void LLPanelTopInfoBar::onAgentParcelChange()
{
	update();
}

void LLPanelTopInfoBar::onContextMenuItemClicked(const LLSD::String& item)
{
	if (item == "landmark")
	{
		LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();

		if(landmark == NULL)
		{
			LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "create_landmark"));
		}
		else
		{
			LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "landmark").with("id",landmark->getUUID()));
		}
	}
	else if (item == "copy")
	{
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl, false);
		LLUIString location_str(slurl.getSLURLString());

		LLClipboard::instance().copyToClipboard(location_str,0,location_str.length());
	}
}

void LLPanelTopInfoBar::onInfoButtonClicked()
{
	LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "agent"));
}
