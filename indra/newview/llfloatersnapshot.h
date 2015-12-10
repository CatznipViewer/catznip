/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llfloater.h"

class LLSpinCtrl;

class LLFloaterSnapshot : public LLFloater
{
	LOG_CLASS(LLFloaterSnapshot);

public:
	typedef enum e_snapshot_format
	{
		SNAPSHOT_FORMAT_PNG,
		SNAPSHOT_FORMAT_JPEG,
		SNAPSHOT_FORMAT_BMP
	} ESnapshotFormat;

	LLFloaterSnapshot(const LLSD& key);
	virtual ~LLFloaterSnapshot();
    
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ S32 notify(const LLSD& info);
	
	static void update();

	// TODO: create a snapshot model instead
	static LLFloaterSnapshot* getInstance();
	static LLFloaterSnapshot* findInstance();
	static void saveTexture();
// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-08-21 (Catznip-3.3)
	typedef boost::function<void(bool)> save_image_callback_t;
// [SL:KB] - Patch: Settings-Snapshot | Checked: 2011-11-15 (Catznip-3.2)
	static void saveLocal(bool prompt_path, const save_image_callback_t& cb);
// [/SL:KB]
//	static void saveLocal(const save_image_callback_t& cb);
// [/SL:KB]
//	static BOOL saveLocal();
	static void postSave();
	static void postPanelSwitch();
	static LLPointer<LLImageFormatted> getImageData();
	static const LLVector3d& getPosTakenGlobal();
	static void setAgentEmail(const std::string& email);

	static const LLRect& getThumbnailPlaceholderRect() { return sThumbnailPlaceholder->getRect(); }

private:
	static LLUICtrl* sThumbnailPlaceholder;
	LLUICtrl *mRefreshBtn, *mRefreshLabel;
	LLUICtrl *mSucceessLblPanel, *mFailureLblPanel;

	class Impl;
	Impl& impl;
};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLFloaterView::Params>
	{
	};

protected:
	LLSnapshotFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;

#endif // LL_LLFLOATERSNAPSHOT_H
