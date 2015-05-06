/** 
 * @file llinspecttoast.cpp
 * @brief Toast inspector implementation.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h" // must be first include

// [SL:KB] - Patch: Chat-ChicletBarAligment | Checked: 2012-02-18 (Catznip-3.2)
#include "llchicletbar.h"
// [/SL:KB]
#include "llinspecttoast.h"
#include "llinspect.h"
#include "llfloaterreg.h"
#include "llscreenchannel.h"
#include "llchannelmanager.h"
#include "lltransientfloatermgr.h"

using namespace LLNotificationsUI;

/**
 * Represents inspectable toast .
 */
class LLInspectToast: public LLInspect
{
public:

	LLInspectToast(const LLSD& notification_idl);
	virtual ~LLInspectToast();

	/*virtual*/ void onOpen(const LLSD& notification_id);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);
private:
	void onToastDestroy(LLToast * toast);

	boost::signals2::scoped_connection mConnection;
	LLPanel* mPanel;
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLHandle<LLScreenChannelBase> mScreenChannel;
// [/SL:KB]
//	LLScreenChannel* mScreenChannel;
};

LLInspectToast::LLInspectToast(const LLSD& notification_id) :
	LLInspect(LLSD()), mPanel(NULL)
{
	LLScreenChannelBase* channel = LLChannelManager::getInstance()->findChannelByID(
																LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	if (channel)
		mScreenChannel = channel->getHandle();
	else
		LL_WARNS() << "Could not get requested screen channel." << LL_ENDL;
// [/SL:KB]
//	mScreenChannel = dynamic_cast<LLScreenChannel*>(channel);
//	if(NULL == mScreenChannel)
//	{
//		LL_WARNS() << "Could not get requested screen channel." << LL_ENDL;
//		return;
//	}

	LLTransientFloaterMgr::getInstance()->addControlView(this);
}
LLInspectToast::~LLInspectToast()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);

	mConnection.disconnect();
}

// virtual
void LLInspectToast::onOpen(const LLSD& notification_id)
{
	LLInspect::onOpen(notification_id);
//	LLToast* toast = mScreenChannel->getToastByNotificationID(notification_id);
// [SL:KB] - Patch: Chat-ScreenChannelHandle | Checked: 2013-08-23 (Catznip-3.6)
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(mScreenChannel.get());
	LLToast* toast = (channel) ? channel->getToastByNotificationID(notification_id) : NULL;
// [/SL:KB]
	if (toast == NULL)
	{
		LL_WARNS() << "Could not get requested toast  from screen channel." << LL_ENDL;
		return;
	}
	mConnection = toast->setOnToastDestroyedCallback(boost::bind(&LLInspectToast::onToastDestroy, this, _1));

	LLPanel * panel = toast->getPanel();
	if (panel == NULL)
	{
		LL_WARNS() << "Could not get toast's panel." << LL_ENDL;
		return;
	}
	panel->setVisible(TRUE);
	panel->setMouseOpaque(FALSE);
	if(mPanel != NULL && mPanel->getParent() == this)
	{
		removeChild(mPanel);
	}
	addChild(panel);
	panel->setFocus(TRUE);
	mPanel = panel;


	LLRect panel_rect;
	panel_rect = panel->getRect();
	reshape(panel_rect.getWidth(), panel_rect.getHeight());

// [SL:KB] - Patch: Chat-ChicletBarAligment | Checked: 2012-02-18 (Catznip-3.2)
	static LLView* pFloaterSnapRegion = NULL;
	if (!pFloaterSnapRegion)
		pFloaterSnapRegion = getRootView()->findChildView("floater_snap_region");

	// We want to constrain it to the floater snap region (minus the chiclet bar) rather than the entire available screen
	LLRect rctConstrain = getParent()->getLocalRect();
	if (pFloaterSnapRegion)
	{
		pFloaterSnapRegion->localRectToOtherView(pFloaterSnapRegion->getLocalRect(), &rctConstrain, getParent());
		if (LLChicletBar::instanceExists())
		{
			const LLChicletBar* pChicletBar = LLChicletBar::getInstance();
			if (LLChicletBar::ALIGN_TOP == pChicletBar->getAlignment())
				rctConstrain.mTop -= pChicletBar->getRect().getHeight();
			else
				rctConstrain.mBottom += pChicletBar->getRect().getHeight();
		}
	}
	LLUI::positionViewNearMouse(this, rctConstrain);
// [/SL:KB]
//	LLUI::positionViewNearMouse(this);
}

// virtual
BOOL LLInspectToast::handleToolTip(S32 x, S32 y, MASK mask)
{
	// We don't like the way LLInspect handles tooltips
	// (black tooltips look weird),
	// so force using the default implementation (STORM-511).
	return LLFloater::handleToolTip(x, y, mask);
}

void LLInspectToast::onToastDestroy(LLToast * toast)
{
	closeFloater(false);
}

void LLNotificationsUI::registerFloater()
{
	LLFloaterReg::add("inspect_toast", "inspect_toast.xml",
			&LLFloaterReg::build<LLInspectToast>);
}

