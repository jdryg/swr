#ifndef CORE_OS_H
#error "Must be included from os.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline core_os_timer* core_osTimerCreate()
{
	return os_api->timerCreate();
}

static inline void core_osTimerDestroy(core_os_timer* timer)
{
	os_api->timerDestroy(timer);
}

static inline bool core_osTimerSleep(core_os_timer* timer, int64_t duration_us)
{
	return os_api->timerSleep(timer, duration_us);
}

static inline int64_t core_osTimeNow(void)
{
	return os_api->timeNow();
}

static inline int64_t core_osTimeDiff(int64_t end, int64_t start)
{
	return os_api->timeDiff(end, start);
}

static inline int64_t core_osTimeSince(int64_t start)
{
	return os_api->timeSince(start);
}

static inline int64_t core_osTimeLapTime(int64_t* timer)
{
	return os_api->timeLapTime(timer);
}

static inline double core_osTimeConvertTo(int64_t delta, core_os_time_units units)
{
	return os_api->timeConvertTo(delta, units);
}

static uint64_t core_osTimestampNow(void)
{
	return os_api->timestampNow();
}

static uint32_t core_osTimestampToString(uint64_t ts, char* buffer, uint32_t max)
{
	return os_api->timestampToString(ts, buffer, max);
}

static inline core_os_file* core_osFileOpenRead(core_file_base_dir baseDir, const char* relPath)
{
	return os_api->fileOpenRead(baseDir, relPath);
}

static inline core_os_file* core_osFileOpenWrite(core_file_base_dir baseDir, const char* relPath)
{
	return os_api->fileOpenWrite(baseDir, relPath);
}

static inline void core_osFileClose(core_os_file* f)
{
	os_api->fileClose(f);
}

static inline uint32_t core_osFileRead(core_os_file* f, void* buffer, uint32_t len)
{
	return os_api->fileRead(f, buffer, len);
}

static inline uint32_t core_osFileWrite(core_os_file* f, const void* buffer, uint32_t len)
{
	return os_api->fileWrite(f, buffer, len);
}

static inline uint64_t core_osFileGetSize(core_os_file* f)
{
	return os_api->fileGetSize(f);
}

static inline void core_osFileSeek(core_os_file* f, int64_t offset, core_file_seek_origin origin)
{
	os_api->fileSeek(f, offset, origin);
}

static inline uint64_t core_osFileTell(core_os_file* f)
{
	return os_api->fileTell(f);
}

static inline int32_t core_osFsSetBaseDir(core_file_base_dir whichDir, core_file_base_dir baseDir, const char* relPath)
{
	return os_api->fsSetBaseDir(whichDir, baseDir, relPath);
}

static inline int32_t core_osFsGetBaseDir(core_file_base_dir whichDir, char* absPath, uint32_t max)
{
	return os_api->fsGetBaseDir(whichDir, absPath, max);
}

static inline int32_t core_osFsRemoveFile(core_file_base_dir baseDir, const char* relPath)
{
	return os_api->fsRemoveFile(baseDir, relPath);
}

static inline int32_t core_osFsCopyFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath)
{
	return os_api->fsCopyFile(srcBaseDir, srcRelPath, dstBaseDir, dstRelPath);
}

static inline int32_t core_osFsMoveFile(core_file_base_dir srcBaseDir, const char* srcRelPath, core_file_base_dir dstBaseDir, const char* dstRelPath)
{
	return os_api->fsMoveFile(srcBaseDir, srcRelPath, dstBaseDir, dstRelPath);
}

static inline int32_t core_osFsCreateDirectory(core_file_base_dir baseDir, const char* relPath)
{
	return os_api->fsCreateDirectory(baseDir, relPath);
}

#ifdef __cplusplus
}
#endif
