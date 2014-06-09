/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llstatusbar.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llfloaterbuycurrency.h"
#include "llbuycurrencyhtml.h"
#include "llpanelnearbymedia.h"
// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
#include "llpaneltopinfobar.h"
// [/SL:KB]
#include "llpanelvolumepulldown.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhints.h"
#include "llhudicon.h"
#include "llnavigationbar.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llrootview.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewermedia.h"
#include "llviewermenu.h"	// for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "lltrans.h"

// library includes
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"

// system includes
#include <iomanip>


//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const S32 MENU_PARCEL_SPACING = 1;	// Distance from right of menu item to parcel information
const S32 SIM_STAT_WIDTH = 8;
const F32 SIM_WARN_FRACTION = 0.75f;
const F32 SIM_FULL_FRACTION = 0.98f;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY		= 3.f; // How long the balance and health icons should flash after a change.
const F32 ICON_FLASH_FREQUENCY	= 2.f;
const S32 TEXT_HEIGHT = 18;

static void onClickVolume(void*);

LLStatusBar::LLStatusBar(const LLRect& rect)
:	LLPanel(),
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	mViewStatus(NULL),
// [/SL:KB]
	mTextTime(NULL),
	mSGBandwidth(NULL),
	mSGPacketLoss(NULL),
//	mBtnStats(NULL),
	mBtnVolume(NULL),
	mBoxBalance(NULL),
	mBalance(0),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0)
{
	setRect(rect);
	
	// status bar can possible overlay menus?
	setMouseOpaque(FALSE);

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();

	buildFromFile("panel_status_bar.xml");
}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	refresh();
	LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	show_navbar_context_menu(this,x,y);
	return TRUE;
}

BOOL LLStatusBar::postBuild()
{
	gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	mViewStatus = findChildView("status_panel");
// [/SL:KB]

	mTextTime = getChild<LLTextBox>("TimeText" );
	
	getChild<LLUICtrl>("buyL")->setCommitCallback(
		boost::bind(&LLStatusBar::onClickBuyCurrency, this));

	getChild<LLUICtrl>("goShop")->setCommitCallback(boost::bind(&LLWeb::loadURLExternal, gSavedSettings.getString("MarketplaceURL")));

	mBoxBalance = getChild<LLTextBox>("balance");
	mBoxBalance->setClickedCallback( &LLStatusBar::onClickBalance, this );
	
//	mBtnStats = getChildView("stat_btn");

	mBtnVolume = getChild<LLButton>( "volume_btn" );
	mBtnVolume->setClickedCallback( onClickVolume, this );
	mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));

	mMediaToggle = getChild<LLButton>("media_toggle_btn");
	mMediaToggle->setClickedCallback( &LLStatusBar::onClickMediaToggle, this );
	mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	LLHints::registerHintTarget("linden_balance", mBoxBalance->getHandle());
// [/SL:KB]
//	LLHints::registerHintTarget("linden_balance", getChild<LLView>("balance_bg")->getHandle());

	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	LLControlVariable* pControl = gSavedSettings.getControl("ShowBuyCurrencyButton");
	pControl->getSignal()->connect(boost::bind(&LLStatusBar::onToggleBuyCurrencyButton, this, _2));
	onToggleBuyCurrencyButton(pControl->getValue());

	pControl = gSavedSettings.getControl("ShowMarketplaceButton");
	pControl->getSignal()->connect(boost::bind(&LLStatusBar::onToggleMarketplaceButton, this, _2));
	onToggleMarketplaceButton(pControl->getValue());

	pControl = gSavedSettings.getControl("ShowNetStats");
	pControl->getSignal()->connect(boost::bind(&LLStatusBar::onToggleNetStats, this, _2));
	onToggleNetStats(pControl->getValue());
// [/SL:KB]

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	LLPanel* pNetStats = findChild<LLPanel>("netstats_panel");
// [/SL:KB]

	// Adding Net Stat Graph
//	S32 x = getRect().getWidth() - 2;
//	S32 y = 0;
	LLRect r;
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	r.set(0, MENU_BAR_HEIGHT - 1, SIM_STAT_WIDTH, 1);
// [/SL:KB]
//	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	LLStatGraph::Params sgp;
	sgp.name("BandwidthGraph");
	sgp.rect(r);
	sgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	sgp.mouse_opaque(false);
	sgp.stat.count_stat_float(&LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED);
	sgp.units("Kbps");
	sgp.precision(0);
	mSGBandwidth = LLUICtrlFactory::create<LLStatGraph>(sgp);
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	pNetStats->addChild(mSGBandwidth);
// [/SL:KB]
//	addChild(mSGBandwidth);
//	x -= SIM_STAT_WIDTH + 2;

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	r.set(SIM_STAT_WIDTH, MENU_BAR_HEIGHT - 1, SIM_STAT_WIDTH * 2, 1);
// [/SL:KB]
//	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	//these don't seem to like being reused
	LLStatGraph::Params pgp;
	pgp.name("PacketLossPercent");
	pgp.rect(r);
	pgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	pgp.mouse_opaque(false);
	pgp.stat.sample_stat_float(&LLStatViewer::PACKETS_LOST_PERCENT);
	pgp.units("%");
	pgp.min(0.f);
	pgp.max(5.f);
	pgp.precision(1);
	pgp.per_sec(false);
	LLStatGraph::Thresholds thresholds;
	thresholds.threshold.add(LLStatGraph::ThresholdParams().value(0.1).color(LLColor4::green))
						.add(LLStatGraph::ThresholdParams().value(0.25f).color(LLColor4::yellow))
						.add(LLStatGraph::ThresholdParams().value(0.6f).color(LLColor4::red));

	pgp.thresholds(thresholds);

	mSGPacketLoss = LLUICtrlFactory::create<LLStatGraph>(pgp);
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	pNetStats->addChild(mSGPacketLoss);
// [/SL:KB]
//	addChild(mSGPacketLoss);

	mPanelVolumePulldown = new LLPanelVolumePulldown();
	addChild(mPanelVolumePulldown);
	mPanelVolumePulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelVolumePulldown->setVisible(FALSE);

	mPanelNearByMedia = new LLPanelNearByMedia();
	addChild(mPanelNearByMedia);
	mPanelNearByMedia->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelNearByMedia->setVisible(FALSE);

//	mScriptOut = getChildView("scriptout");

	return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	static LLCachedControl<bool> show_net_stats(gSavedSettings, "ShowNetStats", false);
	bool net_stats_visible = show_net_stats;

	if (net_stats_visible)
	{
		// Adding Net Stat Meter back in
		F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
		mSGBandwidth->setMin(0.f);
		mSGBandwidth->setMax(bwtotal*1.25f);
		//mSGBandwidth->setThreshold(0, bwtotal*0.75f);
		//mSGBandwidth->setThreshold(1, bwtotal);
		//mSGBandwidth->setThreshold(2, bwtotal);
	}
	
	// update clock every 10 seconds
	if(mClockUpdateTimer.getElapsedTimeF32() > 10.f)
	{
		mClockUpdateTimer.reset();

		// Get current UTC time, adjusted for the user's clock
		// being off.
		time_t utc_time;
		utc_time = time_corrected();

		std::string timeStr = getString("time");
		LLSD substitution;
		substitution["datetime"] = (S32) utc_time;
		LLStringUtil::format (timeStr, substitution);
		mTextTime->setText(timeStr);

		// set the tooltip to have the date
		std::string dtStr = getString("timeTooltip");
		LLStringUtil::format (dtStr, substitution);
		mTextTime->setToolTip (dtStr);
	}

	LLRect r;
	const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();

	// reshape menu bar to its content's width
	if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
	{
		gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
	}

//	mSGBandwidth->setVisible(net_stats_visible);
//	mSGPacketLoss->setVisible(net_stats_visible);
//	mBtnStats->setEnabled(net_stats_visible);

	// update the master volume button state
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	mBtnVolume->setToggleState(mute_audio);
	
	// Disable media toggle if there's no media, parcel media, and no parcel audio
	// (or if media is disabled)
	bool button_enabled = (gSavedSettings.getBOOL("AudioStreamingMusic")||gSavedSettings.getBOOL("AudioStreamingMedia")) && 
						  (LLViewerMedia::hasInWorldMedia() || LLViewerMedia::hasParcelMedia() || LLViewerMedia::hasParcelAudio());
	mMediaToggle->setEnabled(button_enabled);
	// Note the "sense" of the toggle is opposite whether media is playing or not
	bool any_media_playing = (LLViewerMedia::isAnyMediaShowing() || 
							  LLViewerMedia::isParcelMediaPlaying() ||
							  LLViewerMedia::isParcelAudioPlaying());
	mMediaToggle->setValue(!any_media_playing);
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
	mViewStatus->setVisible(visible);
// [/SL:KB]
//	mTextTime->setVisible(visible);
//	getChild<LLUICtrl>("balance_bg")->setVisible(visible);
//	mBoxBalance->setVisible(visible);
//	mBtnVolume->setVisible(visible);
//	mMediaToggle->setVisible(visible);
//	mSGBandwidth->setVisible(visible);
//	mSGPacketLoss->setVisible(visible);
	setBackgroundVisible(visible);
}

// [SL:KB] - Patch: UI-TopBarInfo | Checked: 2011-05-12 (Catznip-2.6)
LLPanelTopInfoBar* LLStatusBar::getTopInfoBarPanel() const
{
	return dynamic_cast<LLPanelTopInfoBar*>(getChildView("topinfo_bar"));
}

void LLStatusBar::showTopInfoBar(bool fVisible)
{
	LLPanel* pTopInfoBar = getTopInfoBarPanel();
	if (pTopInfoBar)
		pTopInfoBar->setVisible(fVisible);
}
// [/SL:KB]

void LLStatusBar::debitBalance(S32 debit)
{
	setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
	setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
	if (balance > getBalance() && getBalance() != 0)
	{
		LLFirstUse::receiveLindens();
	}

	std::string money_str = LLResMgr::getInstance()->getMonetaryString( balance );

	LLStringUtil::format_map_t string_args;
	string_args["[AMT]"] = llformat("%s", money_str.c_str());
	std::string label_str = getString("buycurrencylabel", string_args);
	mBoxBalance->setValue(label_str);

	// Resize the L$ balance background to be wide enough for your balance plus the buy button
	{
		const S32 HPAD = 24;
		LLRect balance_rect = mBoxBalance->getTextBoundingRect();
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
		LLView* balance_panel = findChildView("balance_panel");
		balance_panel->reshape(balance_rect.getWidth() + HPAD, balance_panel->getRect().getHeight());
// [/SL:KB]
//		LLRect buy_rect = getChildView("buyL")->getRect();
//		LLRect shop_rect = getChildView("goShop")->getRect();
//		LLView* balance_bg_view = getChildView("balance_bg");
//		LLRect balance_bg_rect = balance_bg_view->getRect();
//		balance_bg_rect.mLeft = balance_bg_rect.mRight - (buy_rect.getWidth() + shop_rect.getWidth() + balance_rect.getWidth() + HPAD);
//		balance_bg_view->setShape(balance_bg_rect);
	}

	if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
	{
		if (mBalance > balance)
			make_ui_sound("UISndMoneyChangeDown");
		else
			make_ui_sound("UISndMoneyChangeUp");
	}

	if( balance != mBalance )
	{
		mBalanceTimer->reset();
		mBalanceTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
		mBalance = balance;
	}
}


// static
void LLStatusBar::sendMoneyBalanceRequest()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
	gAgent.sendReliableMessage();
}


void LLStatusBar::setHealth(S32 health)
{
	//LL_INFOS() << "Setting health to: " << buffer << LL_ENDL;
	if( mHealth > health )
	{
		if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
		{
			if (isAgentAvatarValid())
			{
				if (gAgentAvatarp->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}

		mHealthTimer->reset();
		mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
	}

	mHealth = health;
}

S32 LLStatusBar::getBalance() const
{
	return mBalance;
}


S32 LLStatusBar::getHealth() const
{
	return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}

// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-15 (Catznip-3.2)
void LLStatusBar::onToggleBuyCurrencyButton(const LLSD& sdValue)
{
	getChildView("buyL_panel")->setVisible(sdValue.asBoolean());
}

void LLStatusBar::onToggleMarketplaceButton(const LLSD& sdValue)
{
	getChildView("shop_panel")->setVisible(sdValue.asBoolean());
}

void LLStatusBar::onToggleNetStats(const LLSD& sdValue)
{
	getChildView("netstats_panel")->setVisible(sdValue.asBoolean());
}
// [/SL:KB]

void LLStatusBar::onClickBuyCurrency()
{
	// open a currency floater - actual one open depends on 
	// value specified in settings.xml
	LLBuyCurrencyHTML::openCurrencyFloater();
	LLFirstUse::receiveLindens(false);
}

void LLStatusBar::onMouseEnterVolume()
{
	LLButton* volbtn =  getChild<LLButton>( "volume_btn" );
//	LLRect vol_btn_rect = volbtn->getRect();
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-17 (Catznip-3.2)
	LLRect vol_btn_rect;
	volbtn->localRectToOtherView(volbtn->getLocalRect(), &vol_btn_rect, this);
// [/SL:KB]
	LLRect volume_pulldown_rect = mPanelVolumePulldown->getRect();
	volume_pulldown_rect.setLeftTopAndSize(vol_btn_rect.mLeft -
	     (volume_pulldown_rect.getWidth() - vol_btn_rect.getWidth()),
			       vol_btn_rect.mBottom,
			       volume_pulldown_rect.getWidth(),
			       volume_pulldown_rect.getHeight());

	mPanelVolumePulldown->setShape(volume_pulldown_rect);


	// show the master volume pull-down
	LLUI::clearPopups();
	LLUI::addPopup(mPanelVolumePulldown);
	mPanelNearByMedia->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterNearbyMedia()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLRect nearby_media_rect = mPanelNearByMedia->getRect();
	LLButton* nearby_media_btn =  getChild<LLButton>( "media_toggle_btn" );
//	LLRect nearby_media_btn_rect = nearby_media_btn->getRect();
// [SL:KB] - Patch: UI-StatusBar | Checked: 2012-01-17 (Catznip-3.2)
	LLRect nearby_media_btn_rect;
	nearby_media_btn->localRectToOtherView(nearby_media_btn->getLocalRect(), &nearby_media_btn_rect, this);
// [/SL:KB]
	nearby_media_rect.setLeftTopAndSize(nearby_media_btn_rect.mLeft - 
										(nearby_media_rect.getWidth() - nearby_media_btn_rect.getWidth())/2,
										nearby_media_btn_rect.mBottom,
										nearby_media_rect.getWidth(),
										nearby_media_rect.getHeight());
	// force onscreen
	nearby_media_rect.translate(popup_holder->getRect().getWidth() - nearby_media_rect.mRight, 0);
	
	// show the master volume pull-down
	mPanelNearByMedia->setShape(nearby_media_rect);
	LLUI::clearPopups();
	LLUI::addPopup(mPanelNearByMedia);

	mPanelVolumePulldown->setVisible(FALSE);
	mPanelNearByMedia->setVisible(TRUE);
}


static void onClickVolume(void* data)
{
	// toggle the master mute setting
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	LLAppViewer::instance()->setMasterSystemAudioMute(!mute_audio);	
}

//static 
void LLStatusBar::onClickBalance(void* )
{
	// Force a balance request message:
	LLStatusBar::sendMoneyBalanceRequest();
	// The refresh of the display (call to setBalance()) will be done by process_money_balance_reply()
}

//static 
void LLStatusBar::onClickMediaToggle(void* data)
{
	LLStatusBar *status_bar = (LLStatusBar*)data;
	// "Selected" means it was showing the "play" icon (so media was playing), and now it shows "pause", so turn off media
	bool enable = ! status_bar->mMediaToggle->getValue();
	LLViewerMedia::setAllMediaEnabled(enable);
}

BOOL can_afford_transaction(S32 cost)
{
	return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
	refresh();
}

// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
	// Requires "trusted" browser/URL source
	LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1
			&& tokens[0].asString() == "request")
		{
			LLStatusBar::sendMoneyBalanceRequest();
			return true;
		}
		return false;
	}
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
