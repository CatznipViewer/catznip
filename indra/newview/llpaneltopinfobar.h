/**
 * @file llpaneltopinfobar.h
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

#ifndef LLPANELTOPINFOBAR_H_
#define LLPANELTOPINFOBAR_H_

#include "llpanel.h"
#include "llinitdestroyclass.h"

class LLButton;
class LLTextBox;
class LLIconCtrl;
class LLParcelChangeObserver;

//class LLPanelTopInfoBar : public LLPanel, public LLSingleton<LLPanelTopInfoBar>, private LLDestroyClass<LLPanelTopInfoBar>
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
class LLPanelTopInfoBar : public LLPanel
// [/SL:KB]
{
//	LLSINGLETON(LLPanelTopInfoBar);
//	~LLPanelTopInfoBar();
	LOG_CLASS(LLPanelTopInfoBar);

//	friend class LLDestroyClass<LLPanelTopInfoBar>;

public:
	typedef boost::signals2::signal<void ()> resize_signal_t;

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
	LLPanelTopInfoBar();
	~LLPanelTopInfoBar();
// [/SL:KB]
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
// [/SL:KB]

	/**
	 * Updates location and parcel icons on login complete
	 */
	void handleLoginComplete();

//	/**
//	 * Called when the top info bar gets shown or hidden
//	 */
//	void onVisibilityChanged(const LLSD& show);

//	boost::signals2::connection setResizeCallback( const resize_signal_t::slot_type& cb );

// [RLVa:KB] - Checked: 2014-03-23 (RLVa-1.4.10)
	/**
	 * Shorthand to call updateParcelInfoText() and updateParcelIcons().
	 */
	void update();
// [/RLV:KB]
private:
	class LLParcelChangeObserver;

	friend class LLParcelChangeObserver;

	enum EParcelIcon
	{
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
		DAMAGE_ICON = 0,	// 0
		VOICE_ICON,			// 1
		FLY_ICON,			// 2
		PUSH_ICON,			// 3
		BUILD_ICON,			// 4
		SCRIPTS_ICON,		// 5
		SEE_AVATARS_ICON,	// 6
		ICON_COUNT			// 7 total
// [/SL:KB]
//		VOICE_ICON = 0,
//		FLY_ICON,			// 1
//		PUSH_ICON,			// 2
//		BUILD_ICON,			// 3
//		SCRIPTS_ICON,		// 4
//		DAMAGE_ICON,		// 5
//		SEE_AVATARS_ICON,	// 6
//		ICON_COUNT			// 7 total
	};

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-15 (Catznip-3.2)
	void handleLayoutChange();
// [/SL:KB]

	/**
	 * Initializes parcel icons controls. Called from the constructor.
	 */
	void initParcelIcons();

	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	/**
	 * Handles clicks on the parcel icons.
	 */
	void onParcelIconClick(EParcelIcon icon);

	/**
	 * Handles clicks on the info buttons.
	 */
	void onInfoButtonClicked();

	/**
	 * Called when agent changes the parcel.
	 */
	void onAgentParcelChange();

	/**
	 * Called when context menu item is clicked.
	 */
	void onContextMenuItemClicked(const LLSD::String& userdata);

//	/**
//	 * Called when user checks/unchecks Show Coordinates menu item.
//	 */
//	void onNavBarShowParcelPropertiesCtrlChanged();

//	/**
//	 * Shorthand to call updateParcelInfoText() and updateParcelIcons().
//	 */
//	void update();

	/**
	 * Updates parcel info text (mParcelInfoText).
	 */
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2013-09-25 (Catznip-3.6)
	void updateParcelInfoText(bool fUpdateLayout = false, bool fForceUpdate = false);
// [/SL:KB]
//	void updateParcelInfoText();

	/**
	 * Updates parcel icons (mParcelIcon[]).
	 */
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2013-09-25 (Catznip-3.6)
	void updateParcelIcons(bool fUpdateLayout = false);
// [/SL:KB]
//	void updateParcelIcons();

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	/**
	 * Updates region maturity (mMaturityButton).
	 */
	void updateMaturity();
// [/SL:KB]

	/**
	 * Updates health information (mDamageText).
	 */
	void updateHealth();

	/**
	 * Lays out all parcel icons starting from right edge of the mParcelInfoText + 11px
	 * (see screenshots in EXT-5808 for details).
	 */
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	void updateLayout();
// [/SL:KB]
//	void layoutParcelIcons();

	/**
	 * Lays out a widget. Widget's rect mLeft becomes equal to the 'left' argument.
	 */
	S32 layoutWidget(LLUICtrl* ctrl, S32 left);

	/**
	 * Generates location string and returns it in the loc_str parameter.
	 */
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	void buildLocationString(std::string& loc_str);
// [/SL:KB]
//	void buildLocationString(std::string& loc_str, bool show_coords);

//	/**
//	 * Sets new value to the mParcelInfoText and updates the size of the top bar.
//	 */
//	void setParcelInfoText(const std::string& new_text);

//	/**
//	 *  Implementation of LLDestroyClass<T>
//	 */
//	static void destroyClass()
//	{
//		if (LLPanelTopInfoBar::instanceExists())
//		{
//			LLPanelTopInfoBar::getInstance()->setEnabled(FALSE);
//		}
//	}

//	LLButton* 				mInfoBtn;
	LLTextBox* 				mParcelInfoText;
	LLTextBox* 				mDamageText;
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2012-01-16 (Catznip-3.2)
	LLIconCtrl*				mMaturityIcon;
	LLPanel*				mIconsPanel;
// [/SL:KB]
	LLIconCtrl*				mParcelIcon[ICON_COUNT];
	LLParcelChangeObserver*	mParcelChangedObserver;

//	boost::signals2::connection	mParcelPropsCtrlConnection;
//	boost::signals2::connection	mShowCoordsCtrlConnection;
	boost::signals2::connection	mParcelMgrConnection;

//	resize_signal_t mResizeSignal;
};

#endif /* LLPANELTOPINFOBAR_H_ */
