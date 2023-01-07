#ifndef CORE_OS_H
#define CORE_OS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque type for timers
typedef struct core_os_timer core_os_timer;

// Opaque type for files
typedef struct core_os_file core_os_file;

typedef enum core_file_base_dir
{
	CORE_FILE_BASE_DIR_ABSOLUTE_PATH = 0,
	CORE_FILE_BASE_DIR_INSTALL,
	CORE_FILE_BASE_DIR_USERDATA,
	CORE_FILE_BASE_DIR_USERAPPDATA,
	CORE_FILE_BASE_DIR_TEMP
} core_file_base_dir;

typedef enum core_file_seek_origin
{
	CORE_FILE_SEEK_ORIGIN_BEGIN = 0,
	CORE_FILE_SEEK_ORIGIN_CURRENT,
	CORE_FILE_SEEK_ORIGIN_END
} core_file_seek_origin;

typedef enum core_os_time_units
{
	CORE_TIME_UNITS_SEC = 0,
	CORE_TIME_UNITS_MS,
	CORE_TIME_UNITS_US,
	CORE_TIME_UNITS_NS
} core_os_time_units;

typedef struct core_os_api
{
	// Waitable timers
	core_os_timer*  (*timerCreate)();
	void            (*timerDestroy)(core_os_timer* timer);
	bool            (*timerSleep)(core_os_timer* timer, int64_t duration_us);

	// High-resolution timer for time interval measurements
	int64_t         (*timeNow)(void);
	int64_t         (*timeDiff)(int64_t end, int64_t start);
	int64_t         (*timeSince)(int64_t start);
	int64_t         (*timeLapTime)(int64_t* timer);
	double          (*timeConvertTo)(int64_t delta, core_os_time_units units);

	// Timestamps for high-resolution time-of-day measurements
	uint64_t        (*timestampNow)(void);
	uint32_t        (*timestampToString)(uint64_t ts, char* buffer, uint32_t max);

	core_os_file*   (*fileOpenRead)(core_file_base_dir baseDir, const char* relPath);
	core_os_file*   (*fileOpenWrite)(core_file_base_dir baseDir, const char* relPath);
	void            (*fileClose)(core_os_file* f);
	uint32_t        (*fileRead)(core_os_file* f, void* buffer, uint32_t len);
	uint32_t        (*fileWrite)(core_os_file* f, const void* buffer, uint32_t len);
	uint64_t        (*fileGetSize)(core_os_file* f);
	void            (*fileSeek)(core_os_file* f, int64_t offset, core_file_seek_origin origin);
	uint64_t        (*fileTell)(core_os_file* f);

	int32_t         (*fsSetBaseDir)(core_file_base_dir whichDir, core_file_base_dir baseDir, const char* relPath);
	int32_t         (*fsGetBaseDir)(core_file_base_dir whichDir, char* absPath, uint32_t max);
	int32_t         (*fsRemoveFile)(core_file_base_dir baseDir, const char* relPath);
	int32_t         (*fsCopyFile)(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
	int32_t         (*fsMoveFile)(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
	int32_t         (*fsCreateDirectory)(core_file_base_dir baseDir, const char* relPath);
} core_os_api;

extern core_os_api* os_api;

static core_os_timer* core_osTimerCreate();
static void core_osTimerDestroy(core_os_timer* timer);
static bool core_osTimerSleep(core_os_timer* timer, int64_t duration_us);

static int64_t core_osTimeNow(void);
static int64_t core_osTimeDiff(int64_t end, int64_t start);
static int64_t core_osTimeSince(int64_t start);
static int64_t core_osTimeLapTime(int64_t* timer);
static double core_osTimeConvertTo(int64_t delta, core_os_time_units units);

static uint64_t core_osTimestampNow(void);
static uint32_t core_osTimestampToString(uint64_t ts, char* buffer, uint32_t max);

static core_os_file* core_osFileOpenRead(core_file_base_dir baseDir, const char* relPath);
static core_os_file* core_osFileOpenWrite(core_file_base_dir baseDir, const char* relPath);
static void core_osFileClose(core_os_file* f);
static uint32_t core_osFileRead(core_os_file* f, void* buffer, uint32_t len);
static uint32_t core_osFileWrite(core_os_file* f, const void* buffer, uint32_t len);
static uint64_t core_osFileGetSize(core_os_file* f);
static void core_osFileSeek(core_os_file* f, int64_t offset, core_file_seek_origin origin);
static uint64_t core_osFileTell(core_os_file* f);

static int32_t core_osFsSetBaseDir(core_file_base_dir whichDir, core_file_base_dir baseDir, const char* relPath);
static int32_t core_osFsGetBaseDir(core_file_base_dir whichDir, char* absPath, uint32_t max);
static int32_t core_osFsRemoveFile(core_file_base_dir baseDir, const char* relPath);
static int32_t core_osFsCopyFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
static int32_t core_osFsMoveFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath);
static int32_t core_osFsCreateDirectory(core_file_base_dir baseDir, const char* relPath);

#ifdef __cplusplus
}
#endif

#endif // CORE_OS_H

#include "inline/os.inl"
