/** 
 * @file llfilepicker.h
 * @brief OS-specific file picker
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

// OS specific file selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. When you call getMultipleOpenFile(), it locks the picker
// until you iterate to the end of the list of selected files with
// getNextFile() or call reset().

#ifndef LL_LLFILEPICKER_H
#define LL_LLFILEPICKER_H

#include "stdtypes.h"

#if LL_DARWIN
#include <Carbon/Carbon.h>

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require

#include <vector>
#include "llstring.h"

#endif

// Need commdlg.h for OPENFILENAMEA
#ifdef LL_WINDOWS
#include "llwin32headers.h"
#include <commdlg.h>
#endif

extern "C" {
// mostly for Linux, possible on others
#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK
}

class LLFilePicker
{
#ifdef LL_GTK
	friend class LLDirPicker;
	friend void chooser_responder(GtkWidget *, gint, gpointer);
#elif LL_DARWIN
	friend class LLDirPicker;
#endif // LL_GTK
// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-08-21 (Catznip-3.3)
	friend class LLFilePickerThread;
// [/SL:KB]
public:
//	// calling this before main() is undefined
//	static LLFilePicker& instance( void ) { return sInstance; }

	enum ELoadFilter
	{
		FFLOAD_ALL = 1,
		FFLOAD_WAV = 2,
		FFLOAD_IMAGE = 3,
		FFLOAD_ANIM = 4,
#ifdef _CORY_TESTING
		FFLOAD_GEOMETRY = 5,
#endif
		FFLOAD_XML = 6,
		FFLOAD_SLOBJECT = 7,
		FFLOAD_RAW = 8,
		FFLOAD_MODEL = 9,
		FFLOAD_COLLADA = 10,
		FFLOAD_SCRIPT = 11,
		FFLOAD_DICTIONARY = 12,
        FFLOAD_DIRECTORY = 13,   // To call from lldirpicker.
        FFLOAD_EXE = 14          // Note: EXE will be treated as ALL on Windows and Linux but not on Darwin
	};

	enum ESaveFilter
	{
		FFSAVE_ALL = 1,
		FFSAVE_WAV = 3,
		FFSAVE_TGA = 4,
		FFSAVE_BMP = 5,
		FFSAVE_AVI = 6,
		FFSAVE_ANIM = 7,
#ifdef _CORY_TESTING
		FFSAVE_GEOMETRY = 8,
#endif
		FFSAVE_XML = 9,
		FFSAVE_COLLADA = 10,
		FFSAVE_RAW = 11,
		FFSAVE_J2C = 12,
		FFSAVE_PNG = 13,
		FFSAVE_JPEG = 14,
		FFSAVE_SCRIPT = 15,
		FFSAVE_TGAPNG = 16,
// [SL:KB] - Patch: Inventory-SaveTextureFormat | Checked: 2012-07-29 (Catznip-3.3)
#ifdef LL_WINDOWS
		FFSAVE_IMAGES = 20
#endif // LL_WINDOWS
// [/SL:KB]
	};

	// open the dialog. This is a modal operation
// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-08-21 (Catznip-3.3)
protected:
	BOOL getSaveFile(ESaveFilter filter, const std::string& filename, bool blocking);
	BOOL getOpenFile(ELoadFilter filter, bool blocking);
	BOOL getMultipleOpenFiles(ELoadFilter filter, bool blocking);
public:
	typedef boost::function<void()> picker_fail_callback_t;
	typedef boost::function<void(const std::string&)> picker_single_callback_t;
	typedef boost::function<void(const std::vector<std::string>&)> picker_multi_callback_t;

	static void getSaveFile(ESaveFilter filter, const std::string& filename, const picker_single_callback_t& success_cb, const picker_fail_callback_t& failure_cb = picker_fail_callback_t());
	static void getOpenFile(ELoadFilter filter, const picker_single_callback_t& success_cb, const picker_fail_callback_t& failure_cb = picker_fail_callback_t());
	static void getMultipleOpenFiles(ELoadFilter filter, const picker_multi_callback_t& success_cb, const picker_fail_callback_t& failure_cb = picker_fail_callback_t());

	static const std::string& getExtension(ESaveFilter filter);
	static bool               hasExtension(ESaveFilter filter);
// [/SL:KB]
//	BOOL getSaveFile( ESaveFilter filter = FFSAVE_ALL, const std::string& filename = LLStringUtil::null, bool blocking = true);
//	BOOL getOpenFile( ELoadFilter filter = FFLOAD_ALL, bool blocking = true  );
//	BOOL getMultipleOpenFiles( ELoadFilter filter = FFLOAD_ALL, bool blocking = true );

// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-08-21 (Catznip-3.3)
	// We refactored the entire class to work exclusively with callbacks so there isn't any need for outside access to these functions
protected:
// [/SL:KB]
	// Get the filename(s) found. getFirstFile() sets the pointer to
	// the start of the structure and allows the start of iteration.
	const std::string getFirstFile();

	// getNextFile() increments the internal representation and
	// returns the next file specified by the user. Returns NULL when
	// no more files are left. Further calls to getNextFile() are
	// undefined.
	const std::string getNextFile();

	// This utility function extracts the current file name without
	// doing any incrementing.
	const std::string getCurFile();

	// Returns the index of the current file.
	S32 getCurFileNum() const { return mCurrentFile; }

	S32 getFileCount() const { return (S32)mFiles.size(); }

	// See llvfs/lldir.h : getBaseFileName and getDirName to extract base or directory names
	
	// clear any lists of buffers or whatever, and make sure the file
	// picker isn't locked.
	void reset();

private:
	enum
	{
		SINGLE_FILENAME_BUFFER_SIZE = 1024,
		//FILENAME_BUFFER_SIZE = 65536
		FILENAME_BUFFER_SIZE = 65000
	};

	// utility function to check if access to local file system via file browser 
	// is enabled and if not, tidy up and indicate we're not allowed to do this.
	bool check_local_file_access_enabled();
	
#if LL_WINDOWS
	OPENFILENAMEW mOFN;				// for open and save dialogs
	WCHAR mFilesW[FILENAME_BUFFER_SIZE];

	BOOL setupFilter(ELoadFilter filter);
#endif

#if LL_DARWIN
    S32 mPickOptions;
	std::vector<std::string> mFileVector;
	
	bool doNavChooseDialog(ELoadFilter filter);
	bool doNavSaveDialog(ESaveFilter filter, const std::string& filename);
    std::vector<std::string>* navOpenFilterProc(ELoadFilter filter);
#endif

#if LL_GTK
	static void add_to_selectedfiles(gpointer data, gpointer user_data);
	static void chooser_responder(GtkWidget *widget, gint response, gpointer user_data);
	// we remember the last path that was accessed for a particular usage
	std::map <std::string, std::string> mContextToPathMap;
	std::string mCurContextName;
	// we also remember the extension of the last added file.
	std::string mCurrentExtension;
#endif

	std::vector<std::string> mFiles;
	S32 mCurrentFile;
	bool mLocked;

//	static LLFilePicker sInstance;

// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-09-25 (Catznip-3.3)
	static std::map<ESaveFilter, std::string> sSaveFilterExtensions;
// [/SL:KB]
	
protected:
#if LL_GTK
        GtkWindow* buildFilePicker(bool is_save, bool is_folder,
				   std::string context = "generic");
#endif

public:
	// don't call these directly please.
	LLFilePicker();
	~LLFilePicker();
};

// [SL:KB] - Patch: Control-FilePicker | Checked: 2012-08-21 (Catznip-3.3)
const void upload_single_file(const std::string& filename, LLFilePicker::ELoadFilter type);
// [/SL:KB]

#endif
