/** 
 * @file llviewermenufile.h
 * @brief "File" menu in the main menu bar.
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

#ifndef LLVIEWERMENUFILE_H
#define LLVIEWERMENUFILE_H

#include "llfoldertype.h"
#include "llassetstorage.h"
#include "llinventorytype.h"
#include "llfilepicker.h"
#include "llthread.h"
#include <queue>

#include "llviewerassetupload.h"

class LLTransactionID;


void init_menu_file();

// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
// NOTE: uploads the files without asking the user for confirmation
void upload_bulk(const std::vector<std::string>& files);
// [/SL:KB]

LLUUID upload_new_resource(
    const std::string& src_filename,
    std::string name,
    std::string desc,
    S32 compression_info,
    LLFolderType::EType destination_folder_type,
    LLInventoryType::EType inv_type,
    U32 next_owner_perms,
    U32 group_perms,
    U32 everyone_perms,
    const std::string& display_name,
    LLAssetStorage::LLStoreAssetCallback callback,
    S32 expected_upload_cost,
// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
	std::list<std::string>* pFileList = NULL);
// [/SL:KB]
//    void *userdata);

void upload_new_resource(
    LLResourceUploadInfo::ptr_t &uploadInfo,
    LLAssetStorage::LLStoreAssetCallback callback = NULL,
// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
	std::list<std::string>* pFileList = NULL);
// [/SL:KB]
//    void *userdata = NULL);


void assign_defaults_and_show_upload_message(
	LLAssetType::EType asset_type,
	LLInventoryType::EType& inventory_type,
	std::string& name,
	const std::string& display_name,
	std::string& description);

class LLFilePickerThread : public LLThread
{ //multi-threaded file picker (runs system specific file picker in background and calls "notify" from main thread)
public:

	static std::queue<LLFilePickerThread*> sDeadQ;
	static LLMutex* sMutex;

	static void initClass();
	static void cleanupClass();
	static void clearDead();

//	std::string mFile; 
//
//	LLFilePicker::ELoadFilter mFilter;
// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
	enum EPickerType { OPEN_SINGLE, OPEN_MULTIPLE, SAVE_SINGLE } mPickerType;
	std::vector<std::string> mFiles;
	S32 mFilter;
	std::string mInitialFile;
// [/SL:KB]

// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
	LLFilePickerThread(LLFilePicker::ELoadFilter filter, bool multiple = false)
		: LLThread("file picker")
		, mPickerType( (multiple) ? OPEN_MULTIPLE : OPEN_SINGLE )
		, mFilter(filter)
	{
	}

	LLFilePickerThread(LLFilePicker::ESaveFilter filter, const std::string& initial_file)
		: LLThread("file picker")
		, mPickerType(SAVE_SINGLE)
		, mFilter(filter)
		, mInitialFile(initial_file)
	{
	}
// [/SL:KB]
//	LLFilePickerThread(LLFilePicker::ELoadFilter filter)
//		: LLThread("file picker"), mFilter(filter)
//	{
//
//	}

	void getFile();

	virtual void run();

// [SL:KB] - Patch: Control-FilePicker | Checked: Catznip-3.3
	virtual void notify(const std::vector<std::string>& files) = 0;
// [/SL:KB]
//	virtual void notify(const std::string& filename) = 0;
};


#endif
