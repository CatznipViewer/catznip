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
#include "llinventorymodel.h"
#include "lllandmark.h"
#include "lllandmarklist.h"
#include "llregionhandle.h"
#include "llresmgr.h"
#include "llqueryflags.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llslurl.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"

#include <boost/lexical_cast.hpp>

// ============================================================================
// LLPanelParcelInfo class
//

static LLRegisterPanelClassWrapper<LLPanelParcelInfo> t_panel_parcel_info("panel_parcel_info");

LLPanelParcelInfo::LLPanelParcelInfo()
	: m_eRequestType(REQUEST_NONE)
	, m_pParcelSnapshot(NULL)
	, m_pParcelName(NULL)
	, m_pRegionMaturityIcon(NULL)
	, m_pParcelLocation(NULL)
	, m_pParcelNumbers(NULL)
	, m_pParcelDescription(NULL)
{
}

LLPanelParcelInfo::~LLPanelParcelInfo()
{
	clearPendingRequest();
}

BOOL LLPanelParcelInfo::postBuild()
{
	m_pParcelSnapshot = getChild<LLTextureCtrl>("parcel_snapshot");
	m_pParcelName = getChild<LLTextBox>("parcel_name");
	m_pRegionMaturityIcon = getChild<LLIconCtrl>("region_maturity");
	m_pParcelLocation = getChild<LLTextBox>("parcel_location");
	m_pParcelNumbers = getChild<LLTextBox>("parcel_numbers");
	m_pParcelDescription = getChild<LLTextEditor>("parcel_description");

	clearControls(LLStringUtil::null, LLStringUtil::null);

	return TRUE;
}

void LLPanelParcelInfo::setParcelID(const LLUUID& idParcel)
{
	// Sanity check - don't do anything if we're already waiting for it or already have it
	if (idParcel.isNull())
	{
		clearPendingRequest();
		clearLocation();
		clearControls(LLStringUtil::null, LLStringUtil::null);
		return;
	}
	else if (m_idCurParcel == idParcel)
	{
		if (REQUEST_NONE == m_eRequestType)
		{
			updateFromParcelData();
			return;
		}
		else if (REQUEST_PARCEL_INFO == m_eRequestType)
		{
			return;
		}
	}

	clearPendingRequest();
	clearLocation();
	const std::string strLoading = LLTrans::getString("LoadingData");
	clearControls(strLoading, strLoading);

	m_eRequestType = REQUEST_PARCEL_INFO;
	m_idCurParcel = idParcel;
	LLRemoteParcelInfoProcessor::getInstance()->addObserver(m_idCurParcel, this);
	LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(m_idCurParcel);
}

void LLPanelParcelInfo::processParcelInfo(const LLParcelData& parcelData)
{
	// Sanity check - only process replies for the current parcel
	if ( (REQUEST_PARCEL_INFO != m_eRequestType) || (m_idCurParcel != parcelData.parcel_id) )
	{
		return;
	}
	clearPendingRequest();

	m_CurParcelData = parcelData;
	m_posCurGlobal.setVec(m_CurParcelData.global_x, m_CurParcelData.global_y, m_CurParcelData.global_z);
						  
	updateFromParcelData();
}

void LLPanelParcelInfo::setErrorStatus(U32 nStatus, const std::string& strReason)
{
	// We only really handle 404 and 499 errors
	std::string strNotAvailable = LLTrans::getString("parcel_not_available"), strErrorText;
	if (nStatus == 404)
		strErrorText = LLTrans::getString("parcel_server_error_text");
	else if (nStatus == 499)
		strErrorText = LLTrans::getString("parcel_server_forbidden_text");
	else
		strErrorText = LLTrans::getString("parcel_server_error_text");

	clearPendingRequest();
	clearLocation();
	clearControls(strNotAvailable, strErrorText);
}

void LLPanelParcelInfo::clearLocation()
{
	m_idCurParcel.setNull();
	m_posCurGlobal.setZero();
	m_CurParcelData.clear();
}

void LLPanelParcelInfo::clearControls(const std::string& strGeneral, const std::string& strDescription)
{
	m_pParcelSnapshot->setImageAssetID(LLUUID::null);
	m_pParcelName->setText(strGeneral);
	m_pRegionMaturityIcon->setValue("transparent.j2c");
	m_pParcelLocation->setText(LLStringUtil::null);
	m_pParcelNumbers->setText(LLStringUtil::null);
	m_pParcelDescription->setText(strDescription);
}

void LLPanelParcelInfo::clearPendingRequest()
{
	if ( (REQUEST_PARCEL_ID == m_eRequestType) && (m_idCurParcel.notNull()) )
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(m_idCurParcel, this);
	}
	m_eRequestType = REQUEST_NONE;
	m_posCurRequest.clearVec();
}

void LLPanelParcelInfo::requestRemoteParcel(const LLVector3d& posGlobal, const LLUUID& idRegion)
{
	// Sanity check - don't do anything if we're already waiting for it
	if ( (REQUEST_PARCEL_ID == m_eRequestType) && (posGlobal == m_posCurRequest) )
		return;

	clearPendingRequest();
	clearLocation();

	const LLViewerRegion* pRegion = gAgent.getRegion();
	const std::string strUrl = pRegion->getCapability("RemoteParcelRequest");
	if ( (pRegion) && (!strUrl.empty()) )
	{
		LLVector3 posRegion((F32)fmod(posGlobal.mdV[VX], (F64)REGION_WIDTH_METERS), 
							(F32)fmod(posGlobal.mdV[VY], (F64)REGION_WIDTH_METERS),
							(F32)posGlobal.mdV[VZ]);
		m_eRequestType = REQUEST_PARCEL_ID;
		m_posCurRequest = posGlobal;

		LLSD sdBody;
		if (idRegion.notNull())
			sdBody["region_id"] = idRegion;
		else
			sdBody["region_handle"] = ll_sd_from_U64(to_region_handle(posGlobal));
		sdBody["location"] = ll_sd_from_vector3(posRegion);
		LLHTTPClient::post(strUrl, sdBody, new LLRemoteParcelRequestResponder(getObserverHandle()));

		const std::string strLoading = LLTrans::getString("LoadingData");
		clearControls(strLoading, strLoading);
	}
	else
	{
		clearControls(LLTrans::getString("parcel_not_available"), LLTrans::getString("parcel_server_update_text"));
	}
}

void LLPanelParcelInfo::setParcelFromItem(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getLinkedItem(idItem);
	if ( (!pItem) || (LLAssetType::AT_LANDMARK != pItem->getType()) )
		return;

	clearPendingRequest();
	clearLocation();
	const std::string strLoading = LLTrans::getString("LoadingData");
	clearControls(strLoading, strLoading);

	LLLandmark* pLandmark = gLandmarkList.getAsset(pItem->getAssetUUID(), boost::bind(&onLandmarkLoaded, _1, getHandle()));
	if (pLandmark)
	{
		onLandmarkLoaded(pLandmark, getHandle());
	}
}

// static
void LLPanelParcelInfo::onLandmarkLoaded(LLLandmark* pLandmark, LLHandle<LLPanel> hPanel)
{
	LLPanelParcelInfo* pInstance = (!hPanel.isDead()) ? dynamic_cast<LLPanelParcelInfo*>(hPanel.get()) : NULL;
	if (pInstance)
	{
		LLVector3d posGlobal; LLUUID idRegion;
		if ( (!pLandmark) || (!pLandmark->getGlobalPos(posGlobal)) || (!pLandmark->getRegionID(idRegion)) )
		{
			pInstance->clearPendingRequest();
			pInstance->clearLocation();
			pInstance->clearControls(LLTrans::getString("parcel_not_available"), LLTrans::getString("parcel_server_error_text"));
			return;
		}
		pInstance->requestRemoteParcel(posGlobal, idRegion);
	}
}

void LLPanelParcelInfo::updateFromParcelData()
{
	m_pParcelSnapshot->setImageAssetID(m_CurParcelData.snapshot_id);
	m_pParcelName->setText(m_CurParcelData.name);
	m_pParcelDescription->setText( (!m_CurParcelData.desc.empty()) ? m_CurParcelData.desc : LLTrans::getString("parcel_no_description") );

	// HACK: Flag 0x2 == adult region; 0x1 == mature region; otherwise assume PG
	if (m_CurParcelData.flags & 0x2)
		m_pRegionMaturityIcon->setValue("Parcel_R_Dark");
	else if (m_CurParcelData.flags & 0x1)
		m_pRegionMaturityIcon->setValue("Parcel_M_Dark");
	else
		m_pRegionMaturityIcon->setValue("Parcel_PG_Dark");

	S32 posRegionX = llround(m_CurParcelData.global_x) % REGION_WIDTH_UNITS;
	S32 posRegionY = llround(m_CurParcelData.global_y) % REGION_WIDTH_UNITS;
	S32 posRegionZ = llround(m_CurParcelData.global_z);
	m_pParcelLocation->setText(LLSLURL(m_CurParcelData.sim_name, LLVector3(posRegionX, posRegionY, posRegionZ)).getSLURLString());

	LLStringUtil::format_map_t args;
	args["[AREA]"] = boost::lexical_cast<std::string>(m_CurParcelData.actual_area);
	args["[PRICE]"] = LLResMgr::getInstance()->getMonetaryString(m_CurParcelData.sale_price);
	args["[TRAFFIC]"] = boost::lexical_cast<std::string>(m_CurParcelData.dwell);
	m_pParcelNumbers->setText(getString( (m_CurParcelData.flags & DFQ_FOR_SALE) ? "area_sale_text" : "area_traffic_text", args));
}

// ============================================================================
