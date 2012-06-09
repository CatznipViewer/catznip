/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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
#include "llpanelparcelinfo.h"
#include "lliconctrl.h"
#include "llregionhandle.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llslurl.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewerregion.h"

// ============================================================================
// LLPanelParcelInfo class
//

static LLRegisterPanelClassWrapper<LLPanelParcelInfo> t_panel_parcel_info("panel_parcel_info");

LLPanelParcelInfo::LLPanelParcelInfo()
	: m_fRequestPending(false)
	, m_pParcelSnapshot(NULL), m_pParcelName(NULL), m_pRegionMaturityIcon(NULL), m_pParcelLocation(NULL), m_pParcelDescription(NULL)
{
}

LLPanelParcelInfo::~LLPanelParcelInfo()
{
	clearObserver();
}

BOOL LLPanelParcelInfo::postBuild()
{
	m_pParcelSnapshot = getChild<LLTextureCtrl>("parcel_snapshot");
	m_pParcelName = getChild<LLTextBox>("parcel_name");
	m_pRegionMaturityIcon = getChild<LLIconCtrl>("region_maturity");
	m_pParcelLocation = getChild<LLTextBox>("parcel_location");
	m_pParcelDescription = getChild<LLTextEditor>("parcel_description");

	clearControls("", "");

	return TRUE;
}

void LLPanelParcelInfo::setParcelID(const LLUUID& idParcel)
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

void LLPanelParcelInfo::processParcelInfo(const LLParcelData& parcelData)
{
	// Sanity check - only process replies for the current parcel
	if (m_idCurParcel != parcelData.parcel_id)
		return;
	m_CurParcelData = parcelData;

	clearObserver();
	updateFromParcelData();
}

void LLPanelParcelInfo::setErrorStatus(U32 nStatus, const std::string& strReason)
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

void LLPanelParcelInfo::clearLocation()
{
	const std::string strNoneFound = LLTrans::getString("NoneFound");
	clearControls(strNoneFound, strNoneFound);
	clearObserver();

	m_idCurParcel.setNull();
	m_posCurGlobal.setZero();
}

void LLPanelParcelInfo::clearControls(const std::string& strGeneral, const std::string& strDescription)
{
	m_pParcelSnapshot->setImageAssetID(LLUUID::null);
	m_pParcelName->setText(strGeneral);
	m_pRegionMaturityIcon->setValue(LLUUID::null);
	m_pParcelLocation->setText(LLStringUtil::null);
	m_pParcelDescription->setText(strDescription);
}

void LLPanelParcelInfo::clearObserver()
{
	if ( (m_fRequestPending) && (m_idCurParcel.notNull()) )
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(m_idCurParcel, this);
		m_fRequestPending = false;
	}
}

void LLPanelParcelInfo::requestRemoteParcel(const LLVector3d& posGlobal)
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

void LLPanelParcelInfo::updateFromParcelData()
{
	m_pParcelSnapshot->setImageAssetID(m_CurParcelData.snapshot_id);
	m_pParcelName->setText(m_CurParcelData.name);
	m_pParcelDescription->setText(m_CurParcelData.desc);

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
	m_pParcelLocation->setText(LLSLURL(m_CurParcelData.sim_name, LLVector3(posRegionX, posRegionY, posRegionZ)).getSLURLString());
}

// ============================================================================
