/** 
 * @file llviewerfloaterreg.cpp
 * @brief LLViewerFloaterReg class registers floaters used in the viewer
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2016, Kitty Barnett
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

#include "llfloaterreg.h"
#include "llviewerfloaterreg.h"
// [SL:KB] - Patch UI-Floaters | Checked: Catznip-5.2
#include "llcallbacklist.h"
// [/SL:KB]
#include "llcommandhandler.h"
#include "llcompilequeue.h"
#include "llfasttimerview.h"
#include "llfloaterabout.h"
#include "llfloaterauction.h"
// [SL:KB] - Patch: Build-AssetRecovery | Checked: 2011-11-24 (Catznip-3.2)
#include "llfloaterassetrecovery.h"
// [/SL:KB]
//#include "llfloaterautoreplacesettings.h"
#include "llfloateravatar.h"
#include "llfloateravatarpicker.h"
#include "llfloateravatarrendersettings.h"
#include "llfloateravatartextures.h"
#include "llfloaterbanduration.h"
#include "llfloaterbigpreview.h"
#include "llfloaterbeacons.h"
// [SL:KB] - Patch: World-Derender | Checked: Catznip-3.2
#include "llfloaterblocked.h"
// [/SL:KB]
#include "llfloaterbuildoptions.h"
#include "llfloaterbulkpermission.h"
#include "llfloaterbump.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuycurrencyhtml.h"
#include "llfloaterbuyland.h"
#include "llfloaterbvhpreview.h"
#include "llfloatercamera.h"
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
#include "llfloatercamerapresetsex.h"
// [/SL:KB]
//#include "llfloatercamerapresets.h"
// [SL:KB] - Patch: Chat-Alerts | Checked: 2012-07-17 (Catznip-3.3)
#include "llfloaterchatalerts.h"
// [/SL:KB]
#include "llfloaterchatvoicevolume.h"
#include "llfloaterconversationlog.h"
#include "llfloaterconversationpreview.h"
#include "llfloaterdeleteprefpreset.h"
#include "llfloaterdestinations.h"
#include "llfloatereditextdaycycle.h"
#include "llfloaterenvironmentadjust.h"
#include "llfloaterexperienceprofile.h"
#include "llfloaterexperiences.h"
#include "llfloaterexperiencepicker.h"
#include "llfloaterevent.h"
// [SL:KB] - Patch: UI-Feedback | Checked: 2016-01-02 (Catznip-3.8)
#include "llfloaterfeedback.h"
// [/SL:KB]
#include "llfloaterfixedenvironment.h"
#include "llfloaterfonttest.h"
#include "llfloaterforgetuser.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloatergridstatus.h"
#include "llfloatergroups.h"
// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2)
#include "llfloatergroupactions.h"
// [/SL:KB]
#include "llfloaterhelpbrowser.h"
#include "llfloaterhoverheight.h"
#include "llfloaterhud.h"
#include "llfloaterimagepreview.h"
#include "llfloaterimsession.h"
#include "llfloaterinspect.h"
#include "llfloaterjoystick.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloaterlinkreplace.h"
#include "llfloaterloadprefpreset.h"
#include "llfloatermap.h"
#include "llfloatermarketplacelistings.h"
#include "llfloatermediasettings.h"
#include "llfloatermemleak.h"
#include "llfloatermodelpreview.h"
#include "llfloatermyscripts.h"
#include "llfloatermyenvironment.h"
#include "llfloaternamedesc.h"
#include "llfloaternotificationsconsole.h"
#include "llfloaternotificationstabbed.h"
#include "llfloaterobjectweights.h"
// [SL:KB] - Patch: Inventory-OfferToast | Checked: Catznip-5.2
#include "llfloaterofferinvfolderconfig.h"
// [/SL:KB]
#include "llfloateropenobject.h"
//#include "llfloateroutfitphotopreview.h"
//#include "llfloateroutfitsnapshot.h"
// [SL:KB] - Patch: World-Objects | Checked: 2013-12-18 (Catznip-3.6)
#include "llfloaterownedobjects.h"
// [/SL:KB]
// [SL:KB] - Patch: UI-ParcelInfoFloater | Checked: 2012-06-09 (Catznip-3.3)
#include "llfloaterparcelinfo.h"
// [/SL:KB]
#include "llfloaterpathfindingcharacters.h"
#include "llfloaterpathfindingconsole.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterpay.h"
#include "llfloaterperms.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterpreferenceviewadvanced.h"
#include "llfloaterpreviewtrash.h"
#include "llfloaterproperties.h"
// [SL:KB] - Patch: Settings-QuickPrefs | Checked: Catznip-4.2
#include "llfloaterquickprefs.h"
// [/SL:KB]
#include "llfloaterregiondebugconsole.h"
#include "llfloaterregioninfo.h"
#include "llfloaterregionrestarting.h"
#include "llfloaterreporter.h"
#include "llfloatersavecamerapreset.h"
#include "llfloatersaveprefpreset.h"
#include "llfloatersceneloadstats.h"
#include "llfloaterscriptdebug.h"
#include "llfloaterscriptedprefs.h"
#include "llfloaterscriptlimits.h"
#include "llfloatersearch.h"
// [SL:KB] - Patch: UI-Search | Checked: 2012-09-03 (Catznip-3.3)
#include "llfloatersearchcontainer.h"
// [/SL:KB]
// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-10-26 (Catznip-2.3)
#include "llfloatersearchreplace.h"
// [/SL:KB]
#include "llfloatersellland.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloatersnapshot.h"
#include "llfloatersounddevices.h"
#include "llfloaterspellchecksettings.h"
#include "llfloatertelehub.h"
#include "llfloatertestinspectors.h"
#include "llfloatertestlistview.h"
#include "llfloatertexturefetchdebugger.h"
#include "llfloatertools.h"
#include "llfloatertopobjects.h"
#include "llfloatertos.h"
#include "llfloatertoybox.h"
//#include "llfloatertranslationsettings.h"
#include "llfloateruipreview.h"
// [SL:KB] - Patch: Viewer-UpdateFloater | Catznip-3.1
#include "llfloaterupdate.h"
// [/SL:KB]
#include "llfloatervoiceeffect.h"
#include "llfloaterwebcontent.h"
#include "llfloaterwebprofile.h"
#include "llfloatervoicevolume.h"
#include "llfloaterwhitelistentry.h"
#include "llfloaterwindowsize.h"
#include "llfloaterworldmap.h"
#include "llfloaterimcontainer.h"
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-04-27 (Catznip-3.5)
#include "llfloaterimcontainerbase.h"
#include "llfloaterimcontainertab.h"
// [/SL:KB]
#include "llinspectavatar.h"
#include "llinspectgroup.h"
// [SL:KB] - Patch: Control-Inspectors | Checked: 2012-03-25 (Catznip-3.2)
#include "llinspectlocation.h"
// [/SL:KB]
#include "llinspectobject.h"
#include "llinspectremoteobject.h"
// [SL:KB] - Patch: Control-Inspectors | Checked: 2012-03-25 (Catznip-3.2)
#include "llinspecttexture.h"
// [/SL:KB]
#include "llinspecttoast.h"
#include "llmoveview.h"
#include "llfloaterimnearbychat.h"
#include "llpanelblockedlist.h"
#include "llpanelclassified.h"
// [SL:KB] - Patch: UI-ProfileGroupFloaters | Checked: 2011-01-23 (Catznip-2.5)
#include "llpanelgroup.h"
#include "llpanelprofileview.h"
// [/SL:KB]
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llscriptfloater.h"
#include "llsyswellwindow.h"

// *NOTE: Please add files in alphabetical order to keep merges easy.
// [RLVa:KB] - Checked: 2010-03-11
#include "rlvfloaters.h"
// [/RLVa:KB]
// handle secondlife:///app/openfloater/{NAME} URLs
class LLFloaterOpenHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLFloaterOpenHandler() : LLCommandHandler("openfloater", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (params.size() != 1)
		{
			return false;
		}

		const std::string floater_name = LLURI::unescape(params[0].asString());
		LLFloaterReg::showInstance(floater_name);

		return true;
	}
};

LLFloaterOpenHandler gFloaterOpenHandler;

void LLViewerFloaterReg::registerFloaters()
{
	// *NOTE: Please keep these alphabetized for easier merges

	LLFloaterAboutUtil::registerFloater();
	LLFloaterReg::add("block_timers", "floater_fast_timers.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFastTimerView>);
	LLFloaterReg::add("about_land", "floater_about_land.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLand>);
	LLFloaterReg::add("appearance", "floater_my_appearance.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelContainer>);
// [SL:KB] - Patch: Build-AssetRecovery | Checked: 2011-11-24 (Catznip-3.2)
	LLFloaterReg::add("asset_recovery", "floater_asset_recovery.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAssetRecovery>);
// [/SL:KB]
	LLFloaterReg::add("associate_listing", "floater_associate_listing.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAssociateListing>);
	LLFloaterReg::add("auction", "floater_auction.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAuction>);
	LLFloaterReg::add("avatar", "floater_avatar.xml",  (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatar>);
	LLFloaterReg::add("avatar_picker", "floater_avatar_picker.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarPicker>);
//	LLFloaterReg::add("avatar_render_settings", "floater_avatar_render_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarRenderSettings>);
	LLFloaterReg::add("avatar_textures", "floater_avatar_textures.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarTextures>);

	LLFloaterReg::add("ban_duration", "floater_ban_duration.xml", &LLFloaterReg::build<LLFloaterBanDuration>);
	LLFloaterReg::add("beacons", "floater_beacons.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBeacons>);
// [SL:KB] - Patch: World-Derender | Checked: Catznip-3.2
	LLFloaterReg::add("blocked", "floater_blocked.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBlocked>);
// [/SL:KB]
	LLFloaterReg::add("bulk_perms", "floater_bulk_perms.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBulkPermission>);
	LLFloaterReg::add("buy_currency", "floater_buy_currency.xml", &LLFloaterBuyCurrency::buildFloater);
	LLFloaterReg::add("buy_currency_html", "floater_buy_currency_html.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuyCurrencyHTML>);	
	LLFloaterReg::add("buy_land", "floater_buy_land.xml", &LLFloaterBuyLand::buildFloater);
	LLFloaterReg::add("buy_object", "floater_buy_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuy>);
	LLFloaterReg::add("buy_object_contents", "floater_buy_contents.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuyContents>);
	LLFloaterReg::add("build", "floater_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTools>);
	LLFloaterReg::add("build_options", "floater_build_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuildOptions>);
// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2)
	LLFloaterReg::add("build_options_axis", "floater_build_axis.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuildAxis>);
// [/SL:KB]
// [SL:KB] - Patch: Build-SelectionOptions | Checked: 2013-11-06 (Catznip-3.6)
	LLFloaterReg::add("build_options_selection", "floater_build_selection.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSelectionOptions>);
// [/SL:KB]
	LLFloaterReg::add("bumps", "floater_bumps.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBump>);

	LLFloaterReg::add("camera", "floater_camera.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCamera>);
// [SL:KB] - Patch: World-Camera | Checked: Catznip-6.4
	LLFloaterReg::add("camera_presets", "floater_camera_presets_ex.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCameraPresetsEx>);
// [/SL:KB]
//	LLFloaterReg::add("camera_presets", "floater_camera_presets.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCameraPresets>);
// [SL:KB] - Patch: Chat-Alerts | Checked: 2012-07-17 (Catznip-3.3)
	LLFloaterReg::add("chat_alerts", "floater_chat_alerts.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChatAlerts>);
// [/SL:KB]
	LLFloaterReg::add("chat_voice", "floater_voice_chat_volume.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChatVoiceVolume>);
	LLFloaterReg::add("nearby_chat", "floater_im_session.xml", (LLFloaterBuildFunc)&LLFloaterIMNearbyChat::buildFloater);
	LLFloaterReg::add("compile_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCompileQueue>);
	LLFloaterReg::add("conversation", "floater_conversation_log.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterConversationLog>);

	LLFloaterReg::add("delete_pref_preset", "floater_delete_pref_preset.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDeletePrefPreset>);
	LLFloaterReg::add("destinations", "floater_destinations.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDestinations>);

	LLFloaterReg::add("env_post_process", "floater_post_process.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPostProcess>);

    LLFloaterReg::add("env_fixed_environmentent_water", "floater_fixedenvironment.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFixedEnvironmentWater>);
    LLFloaterReg::add("env_fixed_environmentent_sky", "floater_fixedenvironment.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFixedEnvironmentSky>);

    LLFloaterReg::add("env_adjust_snapshot", "floater_adjust_environment.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterEnvironmentAdjust>);

    LLFloaterReg::add("env_edit_extdaycycle", "floater_edit_ext_day_cycle.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterEditExtDayCycle>);
    LLFloaterReg::add("my_environments", "floater_my_environments.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMyEnvironment>);

    LLFloaterReg::add("event", "floater_event.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterEvent>);
    LLFloaterReg::add("experiences", "floater_experiences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterExperiences>);
	LLFloaterReg::add("experience_profile", "floater_experienceprofile.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterExperienceProfile>);
	LLFloaterReg::add("experience_search", "floater_experience_search.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterExperiencePicker>);

// [SL:KB] - Patch: UI-ProfileGroupFloaters | Checked: 2011-01-23 (Catznip-2.5)
	LLFloaterReg::add("floater_group_info", "floater_group_info.xml", &LLFloaterReg::build<LLFloaterGroupInfo>);
	LLFloaterReg::add("floater_profile_view", "floater_profile_view.xml", &LLFloaterReg::build<LLFloaterProfileView>);
// [/SL:KB]

// [SL:KB] - Patch: UI-Feedback | Checked: 2016-01-02 (Catznip-3.8)
	LLFloaterReg::add("feedback", "floater_feedback.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFeedback>);
// [/SL:KB]
	LLFloaterReg::add("font_test", "floater_font_test.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFontTest>);
	LLFloaterReg::add("forget_username", "floater_forget_user.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterForgetUser>);

	LLFloaterReg::add("gestures", "floater_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGesture>);
	LLFloaterReg::add("god_tools", "floater_god_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGodTools>);
	LLFloaterReg::add("grid_status", "floater_grid_status.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGridStatus>);
// [SL:KB] - Patch: Notification-GroupCreateNotice | Checked: 2012-02-16 (Catznip-3.2)
	LLFloaterReg::add("group_create_notice", "floater_group_create_notice.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGroupCreateNotice>);
// [/SL:KB]
	LLFloaterReg::add("group_picker", "floater_choose_group.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGroupPicker>);

	LLFloaterReg::add("help_browser", "floater_help_browser.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHelpBrowser>);
	LLFloaterReg::add("edit_hover_height", "floater_edit_hover_height.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHoverHeight>);
	LLFloaterReg::add("hud", "floater_hud.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHUD>);

	LLFloaterReg::add("impanel", "floater_im_session.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterIMSession>);
// [SL:KB] - Patch: Chat-Tabs | Checked: 2013-05-05 (Catznip-3.5)
	LLFloaterReg::addWithFileCallback("im_container", &LLFloaterIMContainerBase::getFloaterXMLFile, &LLFloaterIMContainerBase::buildFloater);
// [/SL:KB]
//	LLFloaterReg::add("im_container", "floater_im_container.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterIMContainer>);
	LLFloaterReg::add("im_well_window", "floater_sys_well.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIMWellWindow>);
	LLFloaterReg::add("incoming_call", "floater_incoming_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIncomingCallDialog>);
//	LLFloaterReg::add("inventory", "floater_my_inventory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelContainer>);
// [SL:KB] - Patch: Inventory-ActivePanel | Checked: 2011-11-02 (Catznip-3.2)
	LLFloaterReg::add("inventory", "floater_my_inventory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelInventoryContainer>);
// [/SL:KB]
	LLFloaterReg::add("inspect", "floater_inspect.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInspect>);
	LLFloaterReg::add("item_properties", "floater_item_properties.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterItemProperties>);
	LLInspectAvatarUtil::registerFloater();
	LLInspectGroupUtil::registerFloater();
// [SL:KB] - Patch: Control-Inspectors | Checked: 2012-03-25 (Catznip-3.2)
	LLInspectLocationUtil::registerFloater();
// [/SL:KB]
	LLInspectObjectUtil::registerFloater();
// [SL:KB] - Patch: Control-Inspectors | Checked: 2012-03-25 (Catznip-3.2)
	LLInspectTextureUtil::registerFloater();
// [/SL:KB]
	LLInspectRemoteObjectUtil::registerFloater();
	LLFloaterVoiceVolumeUtil::registerFloater();
	LLNotificationsUI::registerFloater();
	
	LLFloaterReg::add("lagmeter", "floater_lagmeter.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLagMeter>);
	LLFloaterReg::add("land_holdings", "floater_land_holdings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLandHoldings>);
	LLFloaterReg::add("linkreplace", "floater_linkreplace.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLinkReplace>);
	LLFloaterReg::add("load_pref_preset", "floater_load_pref_preset.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLoadPrefPreset>);
	
	LLFloaterReg::add("mem_leaking", "floater_mem_leaking.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMemLeak>);

	if(gSavedSettings.getBOOL("TextureFetchDebuggerEnabled"))
	{
		LLFloaterReg::add("tex_fetch_debugger", "floater_texture_fetch_debugger.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTextureFetchDebugger>);
	}
	LLFloaterReg::add("media_settings", "floater_media_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaSettings>);	
	LLFloaterReg::add("marketplace_listings", "floater_marketplace_listings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMarketplaceListings>);
	LLFloaterReg::add("marketplace_validation", "floater_marketplace_validation.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMarketplaceValidation>);
	LLFloaterReg::add("message_critical", "floater_critical.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("message_tos", "floater_tos.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("moveview", "floater_moveview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMove>);
	LLFloaterReg::add("mute_object_by_name", "floater_mute_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGetBlockedObjectName>);
	LLFloaterReg::add("mini_map", "floater_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMap>);

	LLFloaterReg::add("notifications_console", "floater_notifications_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationConsole>);
	
// [SL:KB] - Patch: Notification-Filter | Checked: 2016-01-01 (Catznip-4.0)
	LLFloaterNotificationsUtil::registerFloater();
// [/SL:KB]
//	LLFloaterReg::add("notification_well_window", "floater_notifications_tabbed.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationsTabbed>);

	LLFloaterReg::add("object_weights", "floater_object_weights.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterObjectWeights>);
// [SL:KB] - Patch: Inventory-OfferToast | Checked: Catznip-5.2
	LLFloaterReg::add("offer_invfolder_configure", "floater_offer_invfolder_config.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInventoryOfferFolderConfig>);
// [/SL:KB]
	LLFloaterReg::add("openobject", "floater_openobject.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOpenObject>);
	LLFloaterReg::add("outgoing_call", "floater_outgoing_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLOutgoingCallDialog>);
//	LLFloaterReg::add("outfit_photo_preview", "floater_outfit_photo_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOutfitPhotoPreview>);
// [SL:KB] - Patch: World-Objects | Checked: 2013-12-18 (Catznip-3.6)
	LLFloaterReg::add("owned_objects", "floater_owned_objects.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOwnedObjects>);
// [/SL:KB]
	LLFloaterPayUtil::registerFloater();

// [SL:KB] - Patch: UI-ParcelInfoFloater | Checked: 2012-06-09 (Catznip-3.3)
	LLFloaterReg::add("parcel_info", "floater_parcel_info.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterParcelInfo>);
// [/SL:KB]
	LLFloaterReg::add("pathfinding_characters", "floater_pathfinding_characters.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPathfindingCharacters>);
	LLFloaterReg::add("pathfinding_linksets", "floater_pathfinding_linksets.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPathfindingLinksets>);
	LLFloaterReg::add("pathfinding_console", "floater_pathfinding_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPathfindingConsole>);
	LLFloaterReg::add("people", "floater_people.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelContainer>);
	LLFloaterReg::add("perms_default", "floater_perms_default.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPermsDefault>);
	LLFloaterReg::add("places", "floater_places.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelContainer>);
	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	LLFloaterReg::add("prefs_graphics_advanced", "floater_preferences_graphics_advanced.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreferenceGraphicsAdvanced>);
//	LLFloaterReg::add("prefs_view_advanced", "floater_preferences_view_advanced.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreferenceViewAdvanced>);
	LLFloaterReg::add("prefs_proxy", "floater_preferences_proxy.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreferenceProxy>);
	LLFloaterReg::add("prefs_spellchecker_import", "floater_spellcheck_import.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSpellCheckerImport>);
//	LLFloaterReg::add("prefs_translation", "floater_translation_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTranslationSettings>);
//	LLFloaterReg::add("prefs_spellchecker", "floater_spellcheck.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSpellCheckerSettings>);
//	LLFloaterReg::add("prefs_autoreplace", "floater_autoreplace.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAutoReplaceSettings>);
	LLFloaterReg::add("picks", "floater_picks.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSidePanelContainer>);
	LLFloaterReg::add("pref_joystick", "floater_joystick.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterJoystick>);
	LLFloaterReg::add("preview_anim", "floater_preview_animation.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewAnim>, "preview");
	LLFloaterReg::add("preview_conversation", "floater_conversation_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterConversationPreview>);
	LLFloaterReg::add("preview_gesture", "floater_preview_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewGesture>, "preview");
	LLFloaterReg::add("preview_notecard", "floater_preview_notecard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewNotecard>, "preview");
	LLFloaterReg::add("preview_script", "floater_script_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewLSL>, "preview");
	LLFloaterReg::add("preview_scriptedit", "floater_live_lsleditor.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLLiveLSLEditor>, "preview");
	LLFloaterReg::add("preview_sound", "floater_preview_sound.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewSound>, "preview");
	LLFloaterReg::add("preview_texture", "floater_preview_texture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewTexture>, "preview");
	LLFloaterReg::add("preview_trash", "floater_preview_trash.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreviewTrash>);
	LLFloaterReg::add("properties", "floater_inventory_item_properties.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterProperties>);
	LLFloaterReg::add("publish_classified", "floater_publish_classified.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPublishClassifiedFloater>);
	// [SL:KB] - Patch: Settings-QuickPrefs | Checked: Catznip-4.2
	LLFloaterReg::add("quickprefs", "floater_quickprefs.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterQuickPrefs>);
// [/SL:KB]
	LLFloaterReg::add("save_pref_preset", "floater_save_pref_preset.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSavePrefPreset>);
//	LLFloaterReg::add("save_camera_preset", "floater_save_camera_preset.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSaveCameraPreset>);
	LLFloaterReg::add("script_colors", "floater_script_ed_prefs.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptEdPrefs>);

	LLFloaterReg::add("telehubs", "floater_telehub.xml",&LLFloaterReg::build<LLFloaterTelehub>);
	LLFloaterReg::add("test_inspectors", "floater_test_inspectors.xml", &LLFloaterReg::build<LLFloaterTestInspectors>);
	//LLFloaterReg::add("test_list_view", "floater_test_list_view.xml",&LLFloaterReg::build<LLFloaterTestListView>);
	LLFloaterReg::add("test_textbox", "floater_test_textbox.xml", &LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("test_text_editor", "floater_test_text_editor.xml", &LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("test_widgets", "floater_test_widgets.xml", &LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("top_objects", "floater_top_objects.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTopObjects>);
	LLFloaterReg::add("toybox", "floater_toybox.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterToybox>);
	
	LLFloaterReg::add("reporter", "floater_report_abuse.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterReporter>);
	LLFloaterReg::add("reset_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterResetQueue>);
	LLFloaterReg::add("region_debug_console", "floater_region_debug_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionDebugConsole>);
	LLFloaterReg::add("region_info", "floater_region_info.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionInfo>);
	LLFloaterReg::add("region_restarting", "floater_region_restarting.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionRestarting>);
// [RLVa:KB] - Checked: 2010-03-11 (RLVa-1.2.0e) | Added: RLVa-1.2.0a
	LLFloaterReg::add("rlv_behaviours", "floater_rlv_behaviours.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<RlvFloaterBehaviours>);
	LLFloaterReg::add("rlv_console", "floater_rlv_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<RlvFloaterConsole>);
	LLFloaterReg::add("rlv_locks", "floater_rlv_locks.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<RlvFloaterLocks>);
	LLFloaterReg::add("rlv_strings", "floater_rlv_strings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<RlvFloaterStrings>);
// [/RLVa:KB]
	
	LLFloaterReg::add("script_debug", "floater_script_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebug>);
	LLFloaterReg::add("script_debug_output", "floater_script_debug_panel.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebugOutput>);
	LLFloaterReg::add("script_floater", "floater_script.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLScriptFloater>);
	LLFloaterReg::add("script_limits", "floater_script_limits.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptLimits>);
	LLFloaterReg::add("my_scripts", "floater_my_scripts.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMyScripts>);
	LLFloaterReg::add("sell_land", "floater_sell_land.xml", &LLFloaterSellLand::buildFloater);
	LLFloaterReg::add("settings_debug", "floater_settings_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSettingsDebug>);
	LLFloaterReg::add("sound_devices", "floater_sound_devices.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSoundDevices>);
	LLFloaterReg::add("stats", "floater_stats.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("start_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRunQueue>);
	LLFloaterReg::add("scene_load_stats", "floater_scene_load_stats.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSceneLoadStats>);
	LLFloaterReg::add("stop_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotRunQueue>);
	LLFloaterReg::add("snapshot", "floater_snapshot.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSnapshot>);
//    LLFloaterReg::add("outfit_snapshot", "floater_outfit_snapshot.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOutfitSnapshot>);
//	LLFloaterReg::add("search", "floater_search.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSearch>);
// [SL:KB] - Patch: UI-Search | Checked: 2012-09-03 (Catznip-3.3)
	LLFloaterReg::add("search", "floater_search_container.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSearchContainer>);
	LLFloaterReg::add("search_places", "floater_search_places.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("search_web", "floater_search.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSearch>);
// [/SL:KB]
// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-10-26 (Catznip-2.3)
	LLFloaterReg::add("search_replace", "floater_search_replace.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSearchReplace>);
// [/SL:KB]
	LLFloaterReg::add("my_profile", "floater_my_web_profile.xml", (LLFloaterBuildFunc)&LLFloaterWebProfile::create);
	LLFloaterReg::add("profile", "floater_web_profile.xml", (LLFloaterBuildFunc)&LLFloaterWebProfile::create);
	LLFloaterReg::add("how_to", "floater_how_to.xml", (LLFloaterBuildFunc)&LLFloaterWebContent::create);

	LLFloaterReg::add("big_preview", "floater_big_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBigPreview>);
	
	LLFloaterUIPreviewUtil::registerFloater();
// [SL:KB] - Patch: Viewer-UpdateFloater | Checked: Catznip-3.1
	LLFloaterReg::add("update", "floater_update.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterUpdate>);
	LLFloaterReg::add("update_progress", "floater_update_progress.xml", (LLFloaterBuildFunc)&LLFloaterReg::buildModal<LLFloaterUpdateProgress>);
// [/SL:KB]
	LLFloaterReg::add("upload_anim_bvh", "floater_animation_bvh_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBvhPreview>, "upload");
	LLFloaterReg::add("upload_anim_anim", "floater_animation_anim_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAnimPreview>, "upload");
	LLFloaterReg::add("upload_image", "floater_image_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterImagePreview>, "upload");
	LLFloaterReg::add("upload_model", "floater_model_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterModelPreview>, "upload");
	LLFloaterReg::add("upload_script", "floater_script_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptPreview>, "upload");
	LLFloaterReg::add("upload_sound", "floater_sound_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSoundPreview>, "upload");

	LLFloaterReg::add("voice_effect", "floater_voice_effect.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterVoiceEffect>);

	LLFloaterReg::add("web_content", "floater_web_content.xml", (LLFloaterBuildFunc)&LLFloaterWebContent::create);	
	LLFloaterReg::add("whitelist_entry", "floater_whitelist_entry.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWhiteListEntry>);	
	LLFloaterReg::add("window_size", "floater_window_size.xml", &LLFloaterReg::build<LLFloaterWindowSize>);
	LLFloaterReg::add("world_map", "floater_world_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWorldMap>);	

	// *NOTE: Please keep these alphabetized for easier merges
	
	LLFloaterReg::registerControlVariables(); // Make sure visibility and rect controls get preserved when saving
}

// [SL:KB] - Patch UI-Floaters | Checked: Catznip-5.2
// static
void LLFloaterReg::cycleFloaters(const std::string& floater_name)
{
	static std::map<std::string, LLSD> s_FloaterParams = {
		{ "delete_pref_preset", LLSD("graphic") },
		{ "env_delete_preset", LLSD("day_cycle") },
		{ "env_edit_sky", LLSD("edit") },
		{ "env_edit_water", LLSD("edit") },
		{ "env_edit_day_cycle", LLSD("edit") },
		{ "impanel", LLSD().with("show", false) },
		{ "im_container", LLSD().with("show", false) },
		{ "incoming_call", LLSD().with("notify_box_type", "VoiceInviteGroup") },
		{ "load_pref_preset", LLSD("graphic") },
		{ "save_pref_preset", LLSD("graphic") }
	};

	build_map_t::const_iterator itFloater = sBuildMap.begin();
	if (!floater_name.empty())
	{
		itFloater = sBuildMap.find(floater_name);
		if (sBuildMap.end() != itFloater)
			itFloater++;
	}

	while (sBuildMap.end() != itFloater)
	{
		LLSD sdParam;
		auto itFloaterParam = s_FloaterParams.find(itFloater->first);
		if (s_FloaterParams.end() != itFloaterParam)
			sdParam = itFloaterParam->second;
		if ( (sdParam.has("show")) && ((!sdParam["show"].asBoolean())) )
		{
			itFloater++;
			continue;
		}

		// Clean up the old floater
		if (LLFloater* pFloater = LLFloaterReg::findInstance(floater_name, sdParam))
		{
			pFloater->closeHostedFloater();
		}
		else
		{
			LLFloaterReg::const_instance_list_t floaters = LLFloaterReg::getFloaterList(floater_name);
			for (LLFloater* pFloater : floaters)
				if (pFloater)
					pFloater->closeHostedFloater();
		}


		// Open the new floater
		LLFloaterReg::showInstance(itFloater->first, sdParam, true);

		// Schedule a call back for the next one
		doAfterInterval(boost::bind(LLFloaterReg::cycleFloaters, itFloater->first), 5.f);
		break;
	}
}
// [/SL:KB]
