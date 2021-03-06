/** 
 * @file llfloatersearch.cpp
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llfloatersearch.h"
#include "llhttpconstants.h"
#include "llmediactrl.h"
#include "llnotificationsutil.h"
#include "lllogininstance.h"
#include "lluri.h"
#include "llagent.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llweb.h"

// support secondlife:///app/search/{CATEGORY}/{QUERY} SLapps
class LLSearchHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (!LLUI::getInstance()->mSettingGroups["config"]->getBOOL("EnableSearch"))
		{
			LLNotificationsUtil::add("NoSearch", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		const size_t parts = tokens.size();

		// get the (optional) category for the search
		std::string category;
		if (parts > 0)
		{
			category = tokens[0].asString();
		}

		// get the (optional) search string
		std::string search_text;
		if (parts > 1)
		{
			search_text = tokens[1].asString();
		}

		// create the LLSD arguments for the search floater
		LLFloaterSearch::Params p;
		p.search.category = category;
		p.search.query = LLURI::unescape(search_text);

		// open the search floater and perform the requested search
		LLFloaterReg::showInstance("search", p);
		return true;
	}
};
LLSearchHandler gSearchHandler;

LLFloaterSearch::SearchQuery::SearchQuery()
:	category("category", ""),
	query("query")
{}

LLFloaterSearch::LLFloaterSearch(const Params& key) :
	LLFloaterWebContent(key),
	mSearchGodLevel(0)
{
	// declare a map that transforms a category name into
	// the URL suffix that is used to search that category
	mCategoryPaths = LLSD::emptyMap();
	mCategoryPaths["all"]          = "search";
	mCategoryPaths["people"]       = "search/people";
	mCategoryPaths["places"]       = "search/places";
	mCategoryPaths["events"]       = "search/events";
	mCategoryPaths["groups"]       = "search/groups";
	mCategoryPaths["wiki"]         = "search/wiki";
	mCategoryPaths["land"]         = "land";
	mCategoryPaths["destinations"] = "destinations";
	mCategoryPaths["classifieds"]  = "classifieds";
}

BOOL LLFloaterSearch::postBuild()
{
	LLFloaterWebContent::postBuild();
	mWebBrowser->addObserver(this);
// [SL:KB] - Patch: UI-Search | Checked: Catznip-6.5
	mWebBrowser->navigateTo("about:blank");
// [/SL:KB]

	return TRUE;
}

// [SL:KB] - Patch: UI-Search | Checked: Catznip-6.5
void LLFloaterSearch::search(const LLSD& sdParams)
{
	// See LLFloaterSearch::onOpen(...) and LLFloaterWebContent::onOpen(...);
	Params p;
	p.trusted_content = true;
	p.allow_address_entry = false;
	if (!p.validateBlock())
	{
		return;
	}

	mWebBrowser->setTrustedContent(p.trusted_content);
	open_media(p);
	search(p.search);
}
// [/SL:KB]

void LLFloaterSearch::onOpen(const LLSD& key)
{
// [SL:KB] - Patch: UI-FloaterSearch | Checked: Catznip-2.1
	if ( (key.has("category")) || (mWebBrowser->getCurrentNavUrl().empty()))
	{
		// New search triggered - blank the page while loading, instead of temporarily showing stale results
		mWebBrowser->navigateTo("about:blank");

		search(key);
	}
// [/SL:KB]
//	Params p(key);
//	p.trusted_content = true;
//	p.allow_address_entry = false;
//
//	LLFloaterWebContent::onOpen(p);
	mWebBrowser->setFocus(TRUE);
//	search(p.search);
}

void LLFloaterSearch::onClose(bool app_quitting)
{
// [SL:KB] - Patch: UI-FloaterSearch | Checked: 2011-08-25 (Catznip-2.8)
	if (!app_quitting)
		setVisible(FALSE);
	else
		LLFloaterWebContent::onClose(app_quitting);
// [/SL:KB]
//	LLFloaterWebContent::onClose(app_quitting);
//	// tear down the web view so we don't show the previous search
//	// result when the floater is opened next time
//	destroy();
}

void LLFloaterSearch::godLevelChanged(U8 godlevel)
{
	// search results can change based upon god level - if the user
	// changes god level, then give them a warning (we don't refresh
	// the search as this might undo any page navigation or
	// AJAX-driven changes since the last search).
	
	//FIXME: set status bar text

	//getChildView("refresh_search")->setVisible( (godlevel != mSearchGodLevel));
}

void LLFloaterSearch::search(const SearchQuery &p)
{
	if (! mWebBrowser || !p.validateBlock())
	{
		return;
	}

	// reset the god level warning as we're sending the latest state
	getChildView("refresh_search")->setVisible(FALSE);
	mSearchGodLevel = gAgent.getGodLevel();

	// work out the subdir to use based on the requested category
	LLSD subs;
	if (mCategoryPaths.has(p.category))
	{
		subs["CATEGORY"] = mCategoryPaths[p.category].asString();
	}
	else
	{
		subs["CATEGORY"] = mCategoryPaths["all"].asString();
	}

	// add the search query string
	subs["QUERY"] = LLURI::escape(p.query);

	// add the permissions token that login.cgi gave us
	// We use "search_token", and fallback to "auth_token" if not present.
	LLSD search_token = LLLoginInstance::getInstance()->getResponse("search_token");
	if (search_token.asString().empty())
	{
		search_token = LLLoginInstance::getInstance()->getResponse("auth_token");
	}
	subs["AUTH_TOKEN"] = search_token.asString();

	// add the user's preferred maturity (can be changed via prefs)
	std::string maturity;
	if (gAgent.prefersAdult())
	{
		maturity = "42";  // PG,Mature,Adult
	}
	else if (gAgent.prefersMature())
	{
		maturity = "21";  // PG,Mature
	}
	else
	{
		maturity = "13";  // PG
	}
	subs["MATURITY"] = maturity;

	// add the user's god status
	subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";

	// get the search URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);

	// and load the URL in the web view
	mWebBrowser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
}
