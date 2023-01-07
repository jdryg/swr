#include "macros.h"
#include "os.h"
#include "memory.h"
#include "string.h"
#include "allocator.h"
#include "error.h"
#include <stdbool.h>

#if CORE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include <Shlobj.h>
#include <VersionHelpers.h>

static core_os_timer* core_os_timerCreate();
static void core_os_timerDestroy(core_os_timer* timer);
static bool core_os_timerSleep(core_os_timer* timer, int64_t duration_us);
static int64_t core_os_timeNow(void);
static int64_t core_os_timeDiff(int64_t end, int64_t start);
static int64_t core_os_timeSince(int64_t start);
static int64_t core_os_timeLapTime(int64_t* timer);
static double core_os_timeConvertTo(int64_t delta, core_os_time_units units);
static uint64_t core_os_timestampNow(void);
static uint64_t core_os_timestampNow_precise(void);
static uint32_t core_os_timestampToString(uint64_t ts, char* buffer, uint32_t max);
static core_os_file* core_os_fileOpenRead(core_file_base_dir baseDir, const char* relPath);
static core_os_file* core_os_fileOpenWrite(core_file_base_dir baseDir, const char* relPath);
static void core_os_fileClose(core_os_file* f);
static uint32_t core_os_fileRead(core_os_file* f, void* buffer, uint32_t len);
static uint32_t core_os_fileWrite(core_os_file* f, const void* buffer, uint32_t len);
static uint64_t core_os_fileGetSize(core_os_file* f);
static void core_os_fileSeek(core_os_file* f, int64_t offset, core_file_seek_origin origin);
static uint64_t core_os_fileTell(core_os_file* f);
static int32_t core_os_fsSetBaseDir(core_file_base_dir whichDir, core_file_base_dir baseDir, const char* relPath);
static int32_t core_os_fsGetBaseDir(core_file_base_dir whichDir, char* absPath, uint32_t max);
static int32_t core_os_fsRemoveFile(core_file_base_dir baseDir, const char* relPath);
static int32_t core_os_fsCopyFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
static int32_t core_os_fsMoveFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
static int32_t core_os_fsCreateDirectory(core_file_base_dir baseDir, const char* relPath);

core_os_api* os_api = &(core_os_api){
	.timerCreate = core_os_timerCreate,
	.timerDestroy = core_os_timerDestroy,
	.timerSleep = core_os_timerSleep,
	.timeNow = core_os_timeNow,
	.timeDiff = core_os_timeDiff,
	.timeSince = core_os_timeSince,
	.timeLapTime = core_os_timeLapTime,
	.timeConvertTo = core_os_timeConvertTo,
	.timestampNow = core_os_timestampNow,
	.timestampToString = core_os_timestampToString,
	.fileOpenRead = core_os_fileOpenRead,
	.fileOpenWrite = core_os_fileOpenWrite,
	.fileClose = core_os_fileClose,
	.fileRead = core_os_fileRead,
	.fileWrite = core_os_fileWrite,
	.fileGetSize = core_os_fileGetSize,
	.fileSeek = core_os_fileSeek,
	.fileTell = core_os_fileTell,
	.fsSetBaseDir = core_os_fsSetBaseDir,
	.fsGetBaseDir = core_os_fsGetBaseDir,
	.fsRemoveFile = core_os_fsRemoveFile,
	.fsCopyFile = core_os_fsCopyFile,
	.fsMoveFile = core_os_fsMoveFile,
	.fsCreateDirectory = core_os_fsCreateDirectory,
};

typedef void (*pfnGetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);

typedef struct core_os_win32
{
	core_allocator_i* m_Allocator;
	pfnGetSystemTimePreciseAsFileTime GetSystemTimePreciseAsFileTime;
	int64_t m_TimerFreq;
	char m_InstallDir[512];
	char m_TempDir[512];
	char m_UserDataDir[512];
	char m_UserAppDataDir[512];
} core_os_win32;

static core_os_win32 s_OSContext = { 0 };

static bool core_os_win32_getInstallFolder(char* path_utf8, uint32_t max);
static bool core_os_win32_getTempFolder(char* path_utf8, uint32_t max);
static bool core_os_win32_getUserDataFolder(char* path_utf8, uint32_t max);
static bool core_os_win32_getUserAppDataFolder(char* path_utf8, uint32_t max);
static const char* core_os_getBaseDirPathUTF8(core_file_base_dir baseDir);
static void core_os_win32_initKeycodes(void);
static void core_os_win32_initCursors(void);

bool core_os_initAPI(void)
{
	s_OSContext.m_Allocator = allocator_api->createAllocator("os");
	if (!s_OSContext.m_Allocator) {
		return false;
	}

	if (!core_os_win32_getInstallFolder(s_OSContext.m_InstallDir, CORE_COUNTOF(s_OSContext.m_InstallDir))) {
		return false;
	}

	if (!core_os_win32_getTempFolder(s_OSContext.m_TempDir, CORE_COUNTOF(s_OSContext.m_TempDir))) {
		return false;
	}

	if (!core_os_win32_getUserDataFolder(s_OSContext.m_UserDataDir, CORE_COUNTOF(s_OSContext.m_UserDataDir))) {
		return false;
	}

	if (!core_os_win32_getUserAppDataFolder(s_OSContext.m_UserAppDataDir, CORE_COUNTOF(s_OSContext.m_UserAppDataDir))) {
		return false;
	}

	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		s_OSContext.m_TimerFreq = (int64_t)freq.QuadPart;
	}

	{
		HMODULE kernel32 = LoadLibraryA("kernel32.dll");
		if (kernel32) {
			s_OSContext.GetSystemTimePreciseAsFileTime = (pfnGetSystemTimePreciseAsFileTime)GetProcAddress(kernel32, "GetSystemTimePreciseAsFileTime");
			FreeLibrary(kernel32);
		}

		const bool hasPreciseTimestamps = s_OSContext.GetSystemTimePreciseAsFileTime != NULL;
		if (hasPreciseTimestamps) {
			os_api->timestampNow = core_os_timestampNow_precise;
		}
	}

	return true;
}

void core_os_shutdownAPI(void)
{
	if (s_OSContext.m_Allocator) {
		allocator_api->destroyAllocator(s_OSContext.m_Allocator);
		s_OSContext.m_Allocator = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Init/common functions
//
static bool core_os_win32_getInstallFolder(char* path_utf8, uint32_t max)
{
	wchar_t exePathW[1024];
	if (!GetModuleFileNameW(NULL, &exePathW[0], CORE_COUNTOF(exePathW))) {
		return false;
	}

	char exePathA[1024];
	if (!core_utf8from_utf16(exePathA, CORE_COUNTOF(exePathA), exePathW, UINT32_MAX)) {
		return false;
	}

	char* lastSlash = core_strrchr(exePathA, '\\');
	if (!lastSlash) {
		return false;
	}
	*(lastSlash + 1) = '\0';

	const uint32_t len = (uint32_t)(lastSlash + 1 - exePathA);
	const uint32_t copyLen = max - 1 < len ? max - 1 : len;
	core_memCopy(path_utf8, exePathA, copyLen);
	path_utf8[copyLen] = '\0';

	return true;
}

static bool core_os_win32_getTempFolder(char* path_utf8, uint32_t max)
{
	wchar_t pathW[1024];
	if (!GetTempPathW(CORE_COUNTOF(pathW), pathW)) {
		return false;
	}

	if (!core_utf8from_utf16(path_utf8, max, pathW, UINT32_MAX)) {
		return false;
	}

	return true;
}

static bool core_os_win32_getUserDataFolder(char* path_utf8, uint32_t max)
{
	uint32_t pathLen = 0;
	wchar_t* folder = NULL;
	if (FAILED(SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &folder))) {
		goto error;
	}

	pathLen = core_utf8from_utf16(path_utf8, max, folder, UINT32_MAX);
	if (pathLen == 0) {
		goto error;
	}

	// Make sure the path ends with a forward slash
	if (path_utf8[pathLen - 1] != '\\') {
		if (pathLen == max) {
			goto error;
		}

		path_utf8[pathLen] = '\\';
		path_utf8[pathLen + 1] = '\0';
	}

	CoTaskMemFree(folder);
	return true;

error:
	CoTaskMemFree(folder);
	return false;
}

static bool core_os_win32_getUserAppDataFolder(char* path_utf8, uint32_t max)
{
	uint32_t pathLen = 0;
	wchar_t* folder = NULL;
	if (FAILED(SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &folder))) {
		goto error;
	}

	pathLen = core_utf8from_utf16(path_utf8, max, folder, UINT32_MAX);
	if (pathLen == 0) {
		goto error;
	}

	// Make sure the path ends with a forward slash
	if (path_utf8[pathLen - 1] != '\\') {
		if (pathLen == max) {
			goto error;
		}

		path_utf8[pathLen] = '\\';
		path_utf8[pathLen + 1] = '\0';
	}

	CoTaskMemFree(folder);
	return true;

error:
	CoTaskMemFree(folder);
	return false;
}

static const char* core_os_getBaseDirPathUTF8(core_file_base_dir baseDir)
{
	switch (baseDir) {
	case CORE_FILE_BASE_DIR_ABSOLUTE_PATH:
		return "";
	case CORE_FILE_BASE_DIR_INSTALL:
		return s_OSContext.m_InstallDir;
	case CORE_FILE_BASE_DIR_TEMP:
		return s_OSContext.m_TempDir;
	case CORE_FILE_BASE_DIR_USERDATA:
		return s_OSContext.m_UserDataDir;
	case CORE_FILE_BASE_DIR_USERAPPDATA:
		return s_OSContext.m_UserAppDataDir;
	default:
		break;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// Timers
//
static core_os_timer* core_os_timerCreate()
{
	HANDLE timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	return (core_os_timer*)timer;
}

static void core_os_timerDestroy(core_os_timer* timer)
{
	if (timer) {
		CloseHandle((HANDLE)timer);
	}
}

static bool core_os_timerSleep(core_os_timer* timer, int64_t duration_us)
{
	LARGE_INTEGER liDueTime;

	// Convert from microseconds to 100 of ns, and negative for relative time.
	liDueTime.QuadPart = -(duration_us * 10);

	if (!SetWaitableTimer((HANDLE)timer, &liDueTime, 0, NULL, NULL, 0)) {
		return false;
	}

	return WaitForSingleObject((HANDLE)timer, INFINITE) == WAIT_OBJECT_0;
}

//////////////////////////////////////////////////////////////////////////
// Time
//
static int64_t core_os_timeNow(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

static int64_t core_os_timeDiff(int64_t end, int64_t start)
{
	return end - start;
}

static int64_t core_os_timeSince(int64_t start)
{
	return core_os_timeNow() - start;
}

static int64_t core_os_timeLapTime(int64_t* timer)
{
	const int64_t now = core_os_timeNow();
	const int64_t dt = *timer == 0
		? 0
		: core_os_timeDiff(now, *timer)
		;
	*timer = now;
	return dt;
}

static double core_os_timeConvertTo(int64_t delta, core_os_time_units units)
{
	static const double kTimeConversionFactor[] = {
		1.0,          // JTIME_UNITS_SEC
		1000.0,       // JTIME_UNITS_MS
		1000000.0,    // JTIME_UNITS_US
		1000000000.0, // JTIME_UNITS_NS
	};
	return (double)delta * (kTimeConversionFactor[units] / (double)s_OSContext.m_TimerFreq);
}

//////////////////////////////////////////////////////////////////////////
// Timestamp
//
static uint64_t core_os_timestampNow(void)
{
	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);
	return (((uint64_t)fileTime.dwHighDateTime) << 32) | ((uint64_t)fileTime.dwLowDateTime);
}

static uint64_t core_os_timestampNow_precise(void)
{
	FILETIME fileTime;
	s_OSContext.GetSystemTimePreciseAsFileTime(&fileTime);
	return (((uint64_t)fileTime.dwHighDateTime) << 32) | ((uint64_t)fileTime.dwLowDateTime);
}

static uint32_t core_os_timestampToString(uint64_t ts, char* buffer, uint32_t max)
{
	FILETIME fileTime;
	fileTime.dwLowDateTime = (uint32_t)((ts & 0x00000000FFFFFFFFull) >> 0);
	fileTime.dwHighDateTime = (uint32_t)((ts & 0xFFFFFFFF00000000ull) >> 32);

	SYSTEMTIME systemTime;
	FileTimeToSystemTime(&fileTime, &systemTime);

	return core_snprintf(buffer, max, "%04u-%02u-%02u %02u:%02u:%02u.%03u "
		, systemTime.wYear
		, systemTime.wMonth
		, systemTime.wDay
		, systemTime.wHour
		, systemTime.wMinute
		, systemTime.wSecond
		, systemTime.wMilliseconds);
}

//////////////////////////////////////////////////////////////////////////
// File
//
#define JX_FILE_FLAGS_ACCESS_Pos   0
#define JX_FILE_FLAGS_ACCESS_Msk   (0x03u << JX_FILE_FLAGS_ACCESS_Pos)
#define JX_FILE_FLAGS_ACCESS_READ  ((0x01u << JX_FILE_FLAGS_ACCESS_Pos) & JX_FILE_FLAGS_ACCESS_Msk)
#define JX_FILE_FLAGS_ACCESS_WRITE ((0x02u << JX_FILE_FLAGS_ACCESS_Pos) & JX_FILE_FLAGS_ACCESS_Msk)
#define JX_FILE_FLAGS_BINARY_Pos   2
#define JX_FILE_FLAGS_BINARY_Msk   (0x01u << JX_FILE_FLAGS_BINARY_Pos)

typedef struct core_os_file
{
	HANDLE m_Handle;
	uint32_t m_Flags;
} core_os_file;

static core_os_file* core_os_fileOpenRead(core_file_base_dir baseDir, const char* relPath)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(baseDir);
	if (!baseDirPath) {
		return NULL;
	}

	char absPath[1024];
	core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, relPath);

	wchar_t absPathW[1024];
	if (!core_utf8to_utf16((uint16_t*)absPathW, CORE_COUNTOF(absPathW), absPath, SIZE_MAX)) {
		return NULL;
	}

	HANDLE handle = CreateFileW(absPathW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	core_os_file* file = (core_os_file*)CORE_ALLOC(s_OSContext.m_Allocator, sizeof(core_os_file));
	file->m_Handle = handle;
	file->m_Flags = JX_FILE_FLAGS_ACCESS_READ | JX_FILE_FLAGS_BINARY_Msk;

	return file;
}

static core_os_file* core_os_fileOpenWrite(core_file_base_dir baseDir, const char* relPath)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(baseDir);
	if (!baseDirPath) {
		return NULL;
	}

	char absPath[1024];
	core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, relPath);

	wchar_t absPathW[1024];
	if (!core_utf8to_utf16((uint16_t*)absPathW, CORE_COUNTOF(absPathW), absPath, SIZE_MAX)) {
		return NULL;
	}

	HANDLE handle = CreateFileW(absPathW, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	core_os_file* file = (core_os_file*)CORE_ALLOC(s_OSContext.m_Allocator, sizeof(core_os_file));
	file->m_Handle = handle;
	file->m_Flags = JX_FILE_FLAGS_ACCESS_WRITE | JX_FILE_FLAGS_BINARY_Msk;

	return file;
}

static void core_os_fileClose(core_os_file* f)
{
	if (!f) {
		return;
	}

	CloseHandle(f->m_Handle);
	CORE_FREE(s_OSContext.m_Allocator, f);
}

static uint32_t core_os_fileRead(core_os_file* f, void* buffer, uint32_t len)
{
	if (!f || f->m_Handle == INVALID_HANDLE_VALUE) {
		return 0;
	}

	DWORD numBytesRead = 0;
	if (!ReadFile(f->m_Handle, buffer, len, &numBytesRead, NULL)) {
		return 0;
	}

	return numBytesRead;
}

static uint32_t core_os_fileWrite(core_os_file* f, const void* buffer, uint32_t len)
{
	if (!f || f->m_Handle == INVALID_HANDLE_VALUE) {
		return 0;
	}

	DWORD numBytesWritten = 0;
	if (!WriteFile(f->m_Handle, buffer, len, &numBytesWritten, NULL)) {
		return 0;
	}

	return numBytesWritten;
}

static uint64_t core_os_fileGetSize(core_os_file* f)
{
	if (!f || f->m_Handle == INVALID_HANDLE_VALUE) {
		return 0ull;
	}

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(f->m_Handle, &fileSize)) {
		return 0;
	}

	return (uint64_t)fileSize.QuadPart;
}

static void core_os_fileSeek(core_os_file* f, int64_t offset, core_file_seek_origin origin)
{
	if (!f || f->m_Handle == INVALID_HANDLE_VALUE) {
		return;
	}

	LONG offsetLow = (LONG)((offset & 0x00000000FFFFFFFFull) >> 0);
	LONG offsetHigh = (LONG)((offset & 0xFFFFFFFF00000000ull) >> 32);
	DWORD moveMethod = origin == CORE_FILE_SEEK_ORIGIN_BEGIN
		? FILE_BEGIN
		: (origin == CORE_FILE_SEEK_ORIGIN_CURRENT ? FILE_CURRENT : FILE_END)
		;
	const DWORD result = SetFilePointer(f->m_Handle, offsetLow, &offsetHigh, moveMethod);
}

static uint64_t core_os_fileTell(core_os_file* f)
{
	if (!f || f->m_Handle == INVALID_HANDLE_VALUE) {
		return 0ull;
	}

	LONG distanceHigh = 0;
	DWORD offsetLow = SetFilePointer(f->m_Handle, 0, &distanceHigh, FILE_CURRENT);

	return ((uint64_t)offsetLow | ((uint64_t)distanceHigh << 32));
}

//////////////////////////////////////////////////////////////////////////
// File system
//
static int32_t core_os_fsSetBaseDir(core_file_base_dir whichDir, core_file_base_dir baseDir, const char* relPath)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(baseDir);
	if (!baseDirPath) {
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	char absPath[512];
	const uint32_t absPathLen = core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, relPath);

	// Make sure path ends with a slash.
	if (absPath[absPathLen - 1] == '/') {
		absPath[absPathLen - 1] = '\\';
	} else if (absPath[absPathLen - 1] != '\\') {
		absPath[absPathLen] = '\\';
		absPath[absPathLen + 1] = '\0';
	}

	switch (whichDir) {
	case CORE_FILE_BASE_DIR_ABSOLUTE_PATH:
	case CORE_FILE_BASE_DIR_INSTALL:
		return CORE_ERROR_INVALID_ARGUMENT;
	case CORE_FILE_BASE_DIR_USERDATA:
		core_os_fsCreateDirectory(CORE_FILE_BASE_DIR_ABSOLUTE_PATH, absPath);
		core_snprintf(s_OSContext.m_UserDataDir, CORE_COUNTOF(s_OSContext.m_UserDataDir), "%s", absPath);
		break;
	case CORE_FILE_BASE_DIR_USERAPPDATA:
		core_os_fsCreateDirectory(CORE_FILE_BASE_DIR_ABSOLUTE_PATH, absPath);
		core_snprintf(s_OSContext.m_UserAppDataDir, CORE_COUNTOF(s_OSContext.m_UserAppDataDir), "%s", absPath);
		break;
	case CORE_FILE_BASE_DIR_TEMP:
		core_os_fsCreateDirectory(CORE_FILE_BASE_DIR_ABSOLUTE_PATH, absPath);
		core_snprintf(s_OSContext.m_TempDir, CORE_COUNTOF(s_OSContext.m_TempDir), "%s", absPath);
		break;
	default:
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	return CORE_ERROR_NONE;
}

static int32_t core_os_fsGetBaseDir(core_file_base_dir whichDir, char* absPath, uint32_t max)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(whichDir);
	if (!baseDirPath) {
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	core_snprintf(absPath, max, "%s", baseDirPath);

	return CORE_ERROR_NONE;
}

static int32_t core_os_fsRemoveFile(core_file_base_dir baseDir, const char* relPath)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(baseDir);
	if (!baseDirPath) {
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	char absPath[1024];
	core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, relPath);

	wchar_t absPathW[1024];
	if (!core_utf8to_utf16((uint16_t*)absPathW, CORE_COUNTOF(absPathW), absPath, SIZE_MAX)) {
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	return DeleteFileW(absPathW) != 0
		? CORE_ERROR_NONE
		: CORE_ERROR_OPERATION_FAILED
		;
}

static int32_t core_os_fsCopyFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath)
{
	wchar_t existingFilenameW[512];
	wchar_t newFilenameW[512];

	{
		const char* baseDirPath = core_os_getBaseDirPathUTF8(srcBaseDir);
		if (!baseDirPath) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}

		char absPath[1024];
		core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, srcRelPath);

		if (!core_utf8to_utf16((uint16_t*)existingFilenameW, CORE_COUNTOF(existingFilenameW), absPath, SIZE_MAX)) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}
	}

	{
		const char* baseDirPath = core_os_getBaseDirPathUTF8(dstBaseDir);
		if (!baseDirPath) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}

		char absPath[1024];
		core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, dstRelPath);

		if (!core_utf8to_utf16((uint16_t*)newFilenameW, CORE_COUNTOF(newFilenameW), absPath, SIZE_MAX)) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}
	}

	return CopyFileW(existingFilenameW, newFilenameW, FALSE) != 0
		? CORE_ERROR_NONE
		: CORE_ERROR_OPERATION_FAILED
		;
}

static int32_t core_os_fsMoveFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath)
{
	wchar_t existingFilenameW[512];
	wchar_t newFilenameW[512];

	{
		const char* baseDirPath = core_os_getBaseDirPathUTF8(srcBaseDir);
		if (!baseDirPath) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}

		char absPath[1024];
		core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, srcRelPath);

		if (!core_utf8to_utf16((uint16_t*)existingFilenameW, CORE_COUNTOF(existingFilenameW), absPath, SIZE_MAX)) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}
	}

	{
		const char* baseDirPath = core_os_getBaseDirPathUTF8(dstBaseDir);
		if (!baseDirPath) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}

		char absPath[1024];
		core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, dstRelPath);

		if (!core_utf8to_utf16((uint16_t*)newFilenameW, CORE_COUNTOF(newFilenameW), absPath, SIZE_MAX)) {
			return CORE_ERROR_INVALID_ARGUMENT;
		}
	}

	return MoveFileW(existingFilenameW, newFilenameW) != 0
		? CORE_ERROR_NONE
		: CORE_ERROR_OPERATION_FAILED
		;
}

static bool core_os_createDirectory_internal(const char* path)
{
	if (!path) {
		return false;
	}

	wchar_t pathW[1024];
	if (!core_utf8to_utf16((uint16_t*)pathW, CORE_COUNTOF(pathW), path, UINT32_MAX)) {
		return false;
	}

	bool success = true;

	const DWORD fileAttributes = GetFileAttributesW(pathW);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		success = CreateDirectoryW(pathW, NULL) != 0;
	} else {
		// Make sure this is a directory.
		success = false
			|| ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			|| ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
			;
	}

	return success;
}

static int32_t core_os_fsCreateDirectory(core_file_base_dir baseDir, const char* relPath)
{
	const char* baseDirPath = core_os_getBaseDirPathUTF8(baseDir);
	if (!baseDirPath) {
		return CORE_ERROR_INVALID_ARGUMENT;
	}

	char absPath[1024];
	core_snprintf(absPath, CORE_COUNTOF(absPath), "%s%s", baseDirPath, relPath);

	char partialPath[1024];

	char* slash = core_strchr(absPath, '\\');
	while (slash) {
		const uint32_t partialPathLen = (uint32_t)(slash - absPath);
		const uint32_t copyLen = partialPathLen < (CORE_COUNTOF(partialPath) - 1)
			? partialPathLen
			: (CORE_COUNTOF(partialPath) - 1)
			;
		core_memCopy(partialPath, absPath, copyLen);
		partialPath[copyLen] = '\0';

		if (!core_os_createDirectory_internal(partialPath)) {
			return CORE_ERROR_OPERATION_FAILED;
		}

		slash = core_strchr(slash + 1, '\\');
	}

	return core_os_createDirectory_internal(absPath);
}

#endif // JX_PLATFORM_WINDOWS
