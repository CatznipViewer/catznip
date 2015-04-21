/** 
 *
 * Copyright (c) 2011-2012, Kitty Barnett
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
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "lliconctrl.h"
#include "llinspect.h"
#include "llregionhandle.h"
#include "llremoteparcelrequest.h"
#include "llresmgr.h"
#include "llqueryflags.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltooltip.h"
#include "lltrans.h"
#include "llviewerregion.h"

#include "llinspectlocation.h"

#include <boost/lexical_cast.hpp>

// ============================================================================
// LLInspectLocation
//

class LLInspectLocation : public LLInspect, public LLRemoteParcelInfoObserver
{
	friend class LLFloaterReg;
public:
	LLInspectLocation(const LLSD& sdKey);
	/*virtual*/ ~LLInspectLocation();

	/*
	 * LLView/LLPanel overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& sdData);
	/*virtual*/ void onClose(bool fAppQuitting);

	/*
	 * LLRemoteParcelInfoObserver overrides
	 */
public:
	/*virtual*/ void setParcelID(const LLUUID& idParcel);
protected:
	/*virtual*/ void processParcelInfo(const LLParcelData& parcelData);
	/*virtual*/ void setErrorStatus(S32 nStatus, const std::string& strReason);

	/*
	 * Member functions
	 */
protected:
	void clearControls(const std::string& strGeneral, const std::string& strDescription);
	void clearObserver();
	void requestRemoteParcel(const LLVector3d& posGlobal);
	void updateFromParcelData();

	/*
	 * Event handlers
	 */
protected:
	void onClickTeleport();
	void onClickDetails();

	/*
	 * Member variables
	 */
protected:
	bool         m_fRequestPending;
	LLVector3d   m_posCurRequest;
	LLUUID       m_idCurParcel;
	LLVector3d   m_posCurGlobal;
	LLParcelData m_CurParcelData;

	LLTextBox*   m_pParcelName;
	LLIconCtrl*  m_pRegionMaturityIcon;
	LLTextBox*   m_pParcelLocation;
	LLTextBox*   m_pParcelDescription;
	LLButton*    m_pTeleportBtn;
	LLButton*    m_pMoreInfoBtn;
};

// ============================================================================

LLInspectLocation::LLInspectLocation(const LLSD& sdKey)
	: LLInspect(LLSD())	// single_instance, doesn't really need key
	, LLRemoteParcelInfoObserver()
	, m_fRequestPending(false)
	, m_pParcelName(NULL), m_pRegionMaturityIcon(NULL), m_pParcelLocation(NULL), m_pParcelDescription(NULL)
	, m_pTeleportBtn(NULL), m_pMoreInfoBtn(NULL)
{
	mCommitCallbackRegistrar.add("InspectLocation.Teleport", boost::bind(&LLInspectLocation::onClickTeleport, this));
	mCommitCallbackRegistrar.add("InspectLocation.ShowDetails", boost::bind(&LLInspectLocation::onClickDetails, this));	
}

LLInspectLocation::~LLInspectLocation()
{
	clearObserver();
}

BOOL LLInspectLocation::postBuild()
{
	m_pParcelName = getChild<LLTextBox>("parcel_name");
	m_pRegionMaturityIcon = getChild<LLIconCtrl>("region_maturity");
	m_pParcelLocation = getChild<LLTextBox>("parcel_slurl");
	m_pParcelDescription = getChild<LLTextBox>("parcel_description");

	m_pTeleportBtn = getChild<LLButton>("teleport_btn");
	m_pMoreInfoBtn = getChild<LLButton>("more_info_btn");

	clearControls("", "");

	return TRUE;
}

void LLInspectLocation::onOpen(const LLSD& sdData)
{
	LLVector3d posGlobal(sdData["global"]["x"].asReal(), sdData["global"]["y"].asReal(), sdData["global"]["z"].asReal());
	if (posGlobal.isExactlyZero())
		return;

	// Start fade animation
	LLInspect::onOpen(sdData);

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned [see LLToolTipMgr::createToolTip()]
	if (sdData.has("pos"))
		LLUI::positionViewNearMouse(this, sdData["pos"]["x"].asInteger(), sdData["pos"]["y"].asInteger());
	else
		LLUI::positionViewNearMouse(this);

	// Request the parcel id
	requestRemoteParcel(posGlobal);
}

void LLInspectLocation::onClose(bool fAppQuitting)
{
	clearObserver();

	LLTracker::stopTracking((void*)(intptr_t)TRUE);
}

void LLInspectLocation::setParcelID(const LLUUID& idParcel)
{
	// Sanity check - just refresh from existing data if it's the same parcel
	if (m_idCurParcel == idParcel)
	{
		updateFromParcelData();
		return;
	}

	const std::string strLoading = LLTrans::getString("LoadingData");
	clearControls(strLoading, strLoading);
	clearObserver();

	m_fRequestPending = true;
	m_idCurParcel = idParcel;
	m_posCurGlobal = m_posCurRequest;
	LLRemoteParcelInfoProcessor::getInstance()->addObserver(m_idCurParcel, this);
	LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(m_idCurParcel);
}

void LLInspectLocation::processParcelInfo(const LLParcelData& parcelData)
{
	// Sanity check - only process replies for the current parcel
	if (m_idCurParcel != parcelData.parcel_id)
		return;
	m_CurParcelData = parcelData;

	clearObserver();
	updateFromParcelData();
}

void LLInspectLocation::setErrorStatus(S32 nStatus, const std::string& strReason)
{
	// We only really handle 404 and 499 errors
	std::string strNotAvailable = getString("not_available"), strErrorText;
	if (nStatus == 404)
		strErrorText = getString("server_error_text");
	else if (nStatus == 499)
		strErrorText = getString("server_forbidden_text");
	else
		strErrorText = getString("server_error_text");

	clearControls(strNotAvailable, strErrorText);
	clearObserver();
}

void LLInspectLocation::clearControls(const std::string& strGeneral, const std::string& strDescription)
{
	m_pParcelName->setText(strGeneral);
	m_pRegionMaturityIcon->setValue("transparent.j2c");
	m_pParcelLocation->setText(LLStringUtil::null);
	m_pParcelDescription->setText(strDescription);

	m_pTeleportBtn->setEnabled(false);
	m_pMoreInfoBtn->setEnabled(false);
}

void LLInspectLocation::clearObserver()
{
	if ( (m_fRequestPending) && (m_idCurParcel.notNull()) )
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(m_idCurParcel, this);
		m_fRequestPending = false;
	}
}

void LLInspectLocation::requestRemoteParcel(const LLVector3d& posGlobal)
{
	// Sanity check - need a region and a position different from the last one
	const LLViewerRegion* pRegion = gAgent.getRegion();
	if ( (!pRegion) || (posGlobal.isExactlyZero()) || (dist_vec_squared(m_posCurGlobal, posGlobal) < 9.0f) )
	{
		clearObserver();
		updateFromParcelData();
		return;
	}

	const std::string strLoading = LLTrans::getString("LoadingData");
	clearControls(strLoading, strLoading);
	clearObserver();

	const std::string strUrl = pRegion->getCapability("RemoteParcelRequest");
	if (!strUrl.empty())
	{
		U64 hRegion = to_region_handle(posGlobal);
		LLVector3 posRegion((F32)fmod(posGlobal.mdV[VX], (F64)REGION_WIDTH_METERS), 
							(F32)fmod(posGlobal.mdV[VY], (F64)REGION_WIDTH_METERS),
							(F32)posGlobal.mdV[VZ]);
		m_posCurRequest = posGlobal;

		LLSD sdBody;
		sdBody["region_handle"] = ll_sd_from_U64(hRegion);
		sdBody["location"] = ll_sd_from_vector3(posRegion);
		LLHTTPClient::post(strUrl, sdBody, new LLRemoteParcelRequestResponder(getObserverHandle()));
	}
	else
	{
		clearControls(getString("not_available"), getString("server_update_text"));
	}
}

void LLInspectLocation::updateFromParcelData()
{
	m_pParcelName->setText(m_CurParcelData.name);

	m_pParcelDescription->setText(LLStringUtil::null);
	if (m_CurParcelData.flags & DFQ_FOR_SALE)
	{
		LLStringUtil::format_map_t args;
		args["[AMOUNT]"] = LLResMgr::getInstance()->getMonetaryString(m_CurParcelData.sale_price);
		std::string strForSale = LLTrans::getString("TooltipForSaleL$", args);
		strForSale.push_back('\n');

		LLStyle::Params params;
		params.font.style = "BOLD";
		m_pParcelDescription->appendText(strForSale, false, params);
	}
	m_pParcelDescription->appendText(m_CurParcelData.desc, false);

	// HACK: Flag 0x2 == adult region; 0x1 == mature region; otherwise assume PG
	if (m_CurParcelData.flags & 0x2)
		m_pRegionMaturityIcon->setValue(getString("icon_R"));
	else if (m_CurParcelData.flags & 0x1)
		m_pRegionMaturityIcon->setValue(getString("icon_M"));
	else
		m_pRegionMaturityIcon->setValue(getString("icon_PG"));

	S32 posRegionX = llround(m_CurParcelData.global_x) % REGION_WIDTH_UNITS;
	S32 posRegionY = llround(m_CurParcelData.global_y) % REGION_WIDTH_UNITS;
	S32 posRegionZ = llround(m_CurParcelData.global_z);

	LLStringUtil::format_map_t args;
	args["[SLURL]"] = LLSLURL(m_CurParcelData.sim_name, LLVector3(posRegionX, posRegionY, posRegionZ)).getSLURLString();
	args["[AREA]"] = boost::lexical_cast<std::string>(m_CurParcelData.actual_area);
	m_pParcelLocation->setText(getString("location_text", args));

	m_pTeleportBtn->setEnabled(true);
	m_pMoreInfoBtn->setEnabled(true);
}

void LLInspectLocation::onClickTeleport()
{
	gAgent.teleportViaLocation(m_posCurGlobal);

	closeFloater();
}

void LLInspectLocation::onClickDetails()
{
	LLSD sdParams;
	sdParams["id"] = m_idCurParcel;
	sdParams["type"] = "remote_place";
	LLFloaterSidePanelContainer::showPanel("places", sdParams);

	closeFloater();
}

// ============================================================================

void LLInspectLocationUtil::registerFloater()
{
	LLFloaterReg::add("inspect_location", "inspect_location.xml", &LLFloaterReg::build<LLInspectLocation>);
}

void LLInspectLocationUtil::showInspector(const LLVector3d& posGlobal)
{
	LLTracker::trackLocation(posGlobal, LLStringUtil::null, LLStringUtil::null);

	LLSD sdParams;
	sdParams["global"]["x"] = posGlobal.mdV[VX];
	sdParams["global"]["y"] = posGlobal.mdV[VY];
	sdParams["global"]["z"] = posGlobal.mdV[VZ];
	if (LLToolTipMgr::instance().toolTipVisible())
	{
		LLRect rct = LLToolTipMgr::instance().getToolTipRect();
		sdParams["pos"]["x"] = rct.mLeft;
		sdParams["pos"]["y"] = rct.mTop;
	}
	LLFloaterReg::showInstance("inspect_location", sdParams);
}

// ============================================================================
