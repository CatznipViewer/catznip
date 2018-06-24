/** 
 * @file lltoolfocus.cpp
 * @brief A tool to set the build focus point.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// File includes
#include "lltoolfocus.h" 

// Library includes
#include "v3math.h"
#include "llfontgl.h"
#include "llui.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "lltooltip.h"
#include "llhudmanager.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
#include "llviewerkeyboard.h"
// [/SL:KB]
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
// [SL:KB] - Patch: Settings-MouseCam | Checked: Catznip-5.2
#include "llvoiceclient.h"
// [/SL:KB]
#include "llmorphview.h"
#include "llfloaterreg.h"
#include "llfloatercamera.h"
#include "llmenugl.h"

// Globals
BOOL gCameraBtnZoom = TRUE;
BOOL gCameraBtnOrbit = FALSE;
BOOL gCameraBtnPan = FALSE;

const S32 SLOP_RANGE = 4;

//
// Camera - shared functionality
//

LLToolCamera::LLToolCamera()
:	LLTool(std::string("Camera")),
	mAccumX(0),
	mAccumY(0),
	mMouseDownX(0),
	mMouseDownY(0),
	mOutsideSlopX(FALSE),
	mOutsideSlopY(FALSE),
	mValidClickPoint(FALSE),
	mValidSelection(FALSE),
	mMouseSteering(FALSE),
	mMouseUpX(0),
	mMouseUpY(0),
	mMouseUpMask(MASK_NONE)
{ }


LLToolCamera::~LLToolCamera()
{ }

// virtual
void LLToolCamera::handleSelect()
{
	if (gFloaterTools)
	{
		gFloaterTools->setStatusText("camera");
		// in case we start from tools floater, we count any selection as valid
		mValidSelection = gFloaterTools->getVisible();
	}
}

// virtual
void LLToolCamera::handleDeselect()
{
//	gAgent.setLookingAtAvatar(FALSE);

	// Make sure that temporary selection won't pass anywhere except pie tool.
	MASK override_mask = gKeyboard ? gKeyboard->currentMask(TRUE) : 0;
	if (!mValidSelection && (override_mask != MASK_NONE || (gFloaterTools && gFloaterTools->getVisible())))
	{
		LLMenuGL::sMenuContainer->hideMenus();
		LLSelectMgr::getInstance()->validateSelection();
	}
}

BOOL LLToolCamera::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Ensure a mouseup
	setMouseCapture(TRUE);

	// call the base class to propogate info to sim
	LLTool::handleMouseDown(x, y, mask);

// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
	// Once we're in mouse steering, don't allow anything else to change
	if (mMouseSteering)
	{
		if ( (gViewerWindow->getLeftMouseDown()) && (gViewerWindow->getRightMouseDown()) )
		{
			gAgentCamera.setFocusOnAvatar(true, true);
			agent_push_forward(KEYSTATE_DOWN);
			mMouseWalking = true;
		}
		return TRUE;
	}
// [/SL:KB]

	mAccumX = 0;
	mAccumY = 0;

	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;

	mValidClickPoint = FALSE;

	// If mouse capture gets ripped away, claim we moused up
	// at the point we moused down. JC
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	gViewerWindow->hideCursor();

	gViewerWindow->pickAsync(x, y, mask, pickCallback, /*BOOL pick_transparent*/ FALSE, /*BOOL pick_rigged*/ FALSE, /*BOOL pick_unselectable*/ TRUE);

	return TRUE;
}

// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
BOOL LLToolCamera::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	// Do nothing if any of the other mouse buttons are down already
	if ( (gViewerWindow->getLeftMouseDown()) || (gViewerWindow->getRightMouseDown()) )
	{
		return LLTool::handleMiddleMouseDown(x, y, mask);
	}

	// Copy/pasted from the original handleMouseDown() above

	// Ensure a mouseup
	setMouseCapture(TRUE);

	// call the base class to propogate info to sim
	LLTool::handleMiddleMouseDown(x, y, mask);

	mAccumX = 0;
	mAccumY = 0;

	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;

	mValidClickPoint = FALSE;

	// If mouse capture gets ripped away, claim we moused up
	// at the point we moused down. JC
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	gViewerWindow->hideCursor();

	gViewerWindow->pickAsync(x, y, mask, pickCallback, /*BOOL pick_transparent*/ FALSE, /*BOOL pick_rigged*/ FALSE, /*BOOL pick_unselectable*/ TRUE);

	return TRUE;
}

BOOL LLToolCamera::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// Call the base class to propogate info to sim
	LLTool::handleRightMouseDown(x, y, mask);

	if ( (mMouseSteering) && (gViewerWindow->getLeftMouseDown()) && (gViewerWindow->getRightMouseDown()) )
	{
		gAgentCamera.setFocusOnAvatar(true, true);
		agent_push_forward(KEYSTATE_DOWN);
		mMouseWalking = true;
	}

	return TRUE;
}
// [/SL:KB]

void LLToolCamera::pickCallback(const LLPickInfo& pick_info)
{
	if (!LLToolCamera::getInstance()->hasMouseCapture())
	{
		return;
	}

	LLToolCamera::getInstance()->mMouseDownX = pick_info.mMousePt.mX;
	LLToolCamera::getInstance()->mMouseDownY = pick_info.mMousePt.mY;
// [SL:KB] - Patch: Settings-MouseCamming | Checked: Catznip-5.2
	LLToolCamera::getInstance()->mMouseCamming = false;
// [/SL:KB]

	gViewerWindow->moveCursorToCenter();

	// Potentially recenter if click outside rectangle
	LLViewerObject* hit_obj = pick_info.getObject();

	// Check for hit the sky, or some other invalid point
	if (!hit_obj && pick_info.mPosGlobal.isExactlyZero())
	{
		LLToolCamera::getInstance()->mValidClickPoint = FALSE;
		return;
	}

	// check for hud attachments
	if (hit_obj && hit_obj->isHUDAttachment())
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		if (!selection->getObjectCount() || selection->getSelectType() != SELECT_TYPE_HUD)
		{
			LLToolCamera::getInstance()->mValidClickPoint = FALSE;
			return;
		}
	}

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
	{
		BOOL good_customize_avatar_hit = FALSE;
		if( hit_obj )
		{
			if (isAgentAvatarValid() && (hit_obj == gAgentAvatarp))
			{
				// It's you
				good_customize_avatar_hit = TRUE;
			}
			else if (hit_obj->isAttachment() && hit_obj->permYouOwner())
			{
				// It's an attachment that you're wearing
				good_customize_avatar_hit = TRUE;
			}
		}

		if( !good_customize_avatar_hit )
		{
			LLToolCamera::getInstance()->mValidClickPoint = FALSE;
			return;
		}

		if( gMorphView )
		{
			gMorphView->setCameraDrivenByKeys( FALSE );
		}
	}
	//RN: check to see if this is mouse-driving as opposed to ALT-zoom or Focus tool
	else if (pick_info.mKeyMask & MASK_ALT || 
			(LLToolMgr::getInstance()->getCurrentTool()->getName() == "Camera")) 
	{
		LLViewerObject* hit_obj = pick_info.getObject();
		if (hit_obj)
		{
			// ...clicked on a world object, so focus at its position
			if (!hit_obj->isHUDAttachment())
			{
				gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
				gAgentCamera.setFocusGlobal(pick_info);
// [SL:KB] - Patch: Settings-MouseCam | Checked: Catznip-5.2
				if ( (!LLVoiceClient::instance().voiceEnabled()) || (!LLVoiceClient::instance().isPTTMiddleMouse()) )
				{
					LLToolCamera::getInstance()->mMouseCamming = gViewerWindow->getMiddleMouseDown();
				}
// [/SL:KB]
			}
		}
		else if (!pick_info.mPosGlobal.isExactlyZero())
		{
			// Hit the ground
			gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
			gAgentCamera.setFocusGlobal(pick_info);
// [SL:KB] - Patch: Settings-MouseCam | Checked: Catznip-5.2
			if ( (!LLVoiceClient::instance().voiceEnabled()) || (!LLVoiceClient::instance().isPTTMiddleMouse()) )
			{
				LLToolCamera::getInstance()->mMouseCamming = gViewerWindow->getMiddleMouseDown();
			}
// [/SL:KB]
		}

		BOOL zoom_tool = gCameraBtnZoom && (LLToolMgr::getInstance()->getBaseTool() == LLToolCamera::getInstance());
		if (!(pick_info.mKeyMask & MASK_ALT) &&
			!LLFloaterCamera::inFreeCameraMode() &&
			!zoom_tool &&
			gAgentCamera.cameraThirdPerson() &&
//			gViewerWindow->getLeftMouseDown() && 
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
			( (gViewerWindow->getLeftMouseDown()) || (gViewerWindow->getRightMouseDown()) ) &&
// [/SL:KB]
			!gSavedSettings.getBOOL("FreezeTime") &&
			(hit_obj == gAgentAvatarp || 
			 (hit_obj && hit_obj->isAttachment() && LLVOAvatar::findAvatarFromAttachment(hit_obj)->isSelf())))
		{
			LLToolCamera::getInstance()->mMouseSteering = TRUE;
		}

	}

	LLToolCamera::getInstance()->mValidClickPoint = TRUE;

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
	{
		gAgentCamera.setFocusOnAvatar(FALSE, FALSE);
		
		LLVector3d cam_pos = gAgentCamera.getCameraPositionGlobal();

		gAgentCamera.setCameraPosAndFocusGlobal( cam_pos, pick_info.mPosGlobal, pick_info.mObjectID);
	}
}


// "Let go" of the mouse, for example on mouse up or when
// we lose mouse capture.  This ensures that cursor becomes visible
// if a modal dialog pops up during Alt-Zoom. JC
void LLToolCamera::releaseMouse()
{
	// Need to tell the sim that the mouse button is up, since this
	// tool is no longer working and cursor is visible (despite actual
	// mouse button status).
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
	if (gViewerWindow->getLeftMouseDown())
		LLTool::handleMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);
	if (gViewerWindow->getMiddleMouseDown())
		LLTool::handleMiddleMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);
	if (gViewerWindow->getRightMouseDown())
		LLTool::handleRightMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);
// [/SL:KB]
//	LLTool::handleMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);

	gViewerWindow->showCursor();

	//for the situation when left click was performed on the Agent
	if (!LLFloaterCamera::inFreeCameraMode())
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}

	mMouseSteering = FALSE;
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
	if (mMouseWalking)
	{
		agent_push_forward(KEYSTATE_UP);
		mMouseWalking = false;
	}
	mMouseCamming = false;
// [/SL:KB]
	mValidClickPoint = FALSE;
	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;
}


BOOL LLToolCamera::handleMouseUp(S32 x, S32 y, MASK mask)
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
{
	// In mouse steering mode only the last mouse up releases everything; otherwise call base class only
	if ( ((!mMouseSteering) || (!gViewerWindow->getRightMouseDown())) && (!mMouseCamming) )
		return handleMouseUpInternal(x, y, mask);

	if ( (mMouseSteering) && (!mMouseCamming) )
	{
		if (mMouseWalking)
		{
			agent_push_forward(KEYSTATE_UP);
			mMouseWalking = false;
		}

		// Right mouse is down - so orbit around the avatar without turning
		gAgentCamera.setFocusOnAvatar(false, true);
	}

	return LLTool::handleMouseUp(x, y, mask);
}

BOOL LLToolCamera::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	// In mouse steering mode only the last mouse up releases everything; otherwise call base class only
	if ( (!mMouseSteering) || ((!gViewerWindow->getLeftMouseDown()) && (!gViewerWindow->getRightMouseDown())) )
	{
		return handleMouseUpInternal(x, y, mask);
	}

	return LLTool::handleMiddleMouseDown(x, y, mask);
}

BOOL LLToolCamera::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// In mouse steering mode only the last mouse up releases everything; otherwise call base class only
	if ( (mMouseSteering) && ((!gViewerWindow->getLeftMouseDown()) && (!gViewerWindow->getMiddleMouseDown())) )
		return handleMouseUpInternal(x, y, mask);

	if (mMouseSteering)
	{
		if (mMouseWalking)
		{
			agent_push_forward(KEYSTATE_UP);
			mMouseWalking = false;
		}

		// Left mouse is down - so turn the avatar while orbiting
		if (gViewerWindow->getLeftMouseDown())
		{
			gAgentCamera.setFocusOnAvatar(true, true);
		}
	}

	return LLTool::handleMouseUp(x, y, mask);
}

BOOL LLToolCamera::handleMouseUpInternal(S32 x, S32 y, MASK mask)
// [/SL:KB]
{
	// Claim that we're mousing up somewhere
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	if (hasMouseCapture())
	{
		if (mValidClickPoint)
		{
			if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
			{
				LLCoordGL mouse_pos;
				LLVector3 focus_pos = gAgent.getPosAgentFromGlobal(gAgentCamera.getFocusGlobal());
				BOOL success = LLViewerCamera::getInstance()->projectPosAgentToScreen(focus_pos, mouse_pos);
				if (success)
				{
					LLUI::setMousePositionScreen(mouse_pos.mX, mouse_pos.mY);
				}
			}
//			else if (mMouseSteering)
// [SL:KB] - Patch: Settings-MouseCam | Checked: Catznip-5.2
			else if ((mMouseSteering) || (mMouseCamming) )
// [/SL:KB]
			{
				LLUI::setMousePositionScreen(mMouseDownX, mMouseDownY);
			}
			else
			{
				gViewerWindow->moveCursorToCenter();
			}
		}
		else
		{
			// not a valid zoomable object
			LLUI::setMousePositionScreen(mMouseDownX, mMouseDownY);
		}

		// calls releaseMouse() internally
		setMouseCapture(FALSE);
	}
	else
	{
		releaseMouse();
	}

	return TRUE;
}


BOOL LLToolCamera::handleHover(S32 x, S32 y, MASK mask)
{
// [SL:KB] - Patch: Settings-MouseWalk | Checked: Catznip-5.2
	if ( (mMouseSteering) && (gViewerWindow->getLeftMouseDown()) && (gViewerWindow->getRightMouseDown()) )
	{
		agent_push_forward(KEYSTATE_LEVEL);
	}
// [/SL:KB]

	S32 dx = gViewerWindow->getCurrentMouseDX();
	S32 dy = gViewerWindow->getCurrentMouseDY();
	
	if (hasMouseCapture() && mValidClickPoint)
	{
		mAccumX += llabs(dx);
		mAccumY += llabs(dy);

		if (mAccumX >= SLOP_RANGE)
		{
			mOutsideSlopX = TRUE;
		}

		if (mAccumY >= SLOP_RANGE)
		{
			mOutsideSlopY = TRUE;
		}
	}

	if (mOutsideSlopX || mOutsideSlopY)
	{
// [SL:KB] - Patch: Settings-MouseCam | Checked: Catznip-5.2
		if (mMouseCamming)
		{
			if (gViewerWindow->getLeftMouseDown())
				mask |= MASK_ORBIT;
			if (gViewerWindow->getRightMouseDown())
				mask |= MASK_PAN;
		}
// [/SL:KB]

		if (!mValidClickPoint)
		{
			LL_DEBUGS("UserInput") << "hover handled by LLToolFocus [invalid point]" << LL_ENDL;
			gViewerWindow->setCursor(UI_CURSOR_NO);
			gViewerWindow->showCursor();
			return TRUE;
		}

		if (gCameraBtnOrbit ||
			mask == MASK_ORBIT || 
			mask == (MASK_ALT | MASK_ORBIT))
		{
			// Orbit tool
			if (hasMouseCapture())
			{
				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				if (dy != 0)
				{
					gAgentCamera.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
				}

				gViewerWindow->moveCursorToCenter();
			}
			LL_DEBUGS("UserInput") << "hover handled by LLToolFocus [active]" << LL_ENDL;
		}
		else if (	gCameraBtnPan ||
					mask == MASK_PAN ||
					mask == (MASK_PAN | MASK_ALT) )
		{
			// Pan tool
			if (hasMouseCapture())
			{
				LLVector3d camera_to_focus = gAgentCamera.getCameraPositionGlobal();
				camera_to_focus -= gAgentCamera.getFocusGlobal();
				F32 dist = (F32) camera_to_focus.normVec();

				// Fudge factor for pan
				F32 meters_per_pixel = 3.f * dist / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraPanLeft( dx * meters_per_pixel );
				}

				if (dy != 0)
				{
					gAgentCamera.cameraPanUp( -dy * meters_per_pixel );
				}

				gViewerWindow->moveCursorToCenter();
			}
			LL_DEBUGS("UserInput") << "hover handled by LLToolPan" << LL_ENDL;
		}
		else if (gCameraBtnZoom)
		{
			// Zoom tool
			if (hasMouseCapture())
			{

				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				const F32 IN_FACTOR = 0.99f;

				if (dy != 0 && mOutsideSlopY )
				{
					if (mMouseSteering)
					{
						gAgentCamera.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
					}
					else
					{
						gAgentCamera.cameraZoomIn( pow( IN_FACTOR, dy ) );
					}
				}

				gViewerWindow->moveCursorToCenter();
			}

			LL_DEBUGS("UserInput") << "hover handled by LLToolZoom" << LL_ENDL;		
		}
	}

	if (gCameraBtnOrbit ||
		mask == MASK_ORBIT || 
		mask == (MASK_ALT | MASK_ORBIT))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (	gCameraBtnPan ||
				mask == MASK_PAN ||
				mask == (MASK_PAN | MASK_ALT) )
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}
	
	return TRUE;
}


void LLToolCamera::onMouseCaptureLost()
{
	releaseMouse();
}
