#include "file.h"

#include <stdbool.h>
#include <sys/syscall.h>
#include <sys/fs.h>

#include "../lv2_stdio.h"

#if defined(assert)
#undef assert
#endif
#define assert(x)

static int sys_fs_open(const char* path, int flags, int* fd, uint64_t mode, const void* arg, uint64_t size)
{
    system_call_6(801, (uint32_t)path, (uint64_t)flags, (uint32_t)fd, (uint64_t)mode, (uint32_t)arg, (uint64_t)size);
    return_to_user_prog(int);
}

static int sys_fs_read(int fd, void* buf, uint64_t nbytes, uint64_t* nread)
{
    system_call_4(802, (uint64_t)fd, (uint32_t)buf, (uint64_t)nbytes, (uint32_t)nread);
    return_to_user_prog(int);
}

static int sys_fs_write(int fd, const void* buf, uint64_t nbytes, uint64_t* nwrite)
{
    system_call_4(803, (uint64_t)fd, (uint32_t)buf, (uint64_t)nbytes, (uint32_t)nwrite);
    return_to_user_prog(int);
}

static int sys_fs_lseek(int fd, int64_t offset, int whence, uint64_t* pos)
{
    system_call_4(818, (uint64_t)fd, (uint64_t)offset, (uint64_t)whence, (uint32_t)pos);
    return_to_user_prog(int);
}

static int sys_fs_close(int fd)
{
    system_call_1(804, (uint64_t)fd);
    return_to_user_prog(int);
}

static int sys_fs_unlink(const char* path)
{
    system_call_1(814, (uint64_t)path);
    return_to_user_prog(int);
}

// syscall(SYS_FS_OPEN, "/dev_hdd0/game/NPEA00385/USRDIR/mod/multi/host.txt", O_RDONLY, &fd, 0, 0, 0);

static int fileModeToCellFs(FileMode mode)
{
    switch (mode)
    {
        case FILE_MODE_READ:
            return CELL_FS_O_RDONLY;
        case FILE_MODE_WRITE:
            return CELL_FS_O_WRONLY;
        case FILE_MODE_READ_WRITE:
            return CELL_FS_O_RDWR;
        case FILE_MODE_CREATE:
            return CELL_FS_O_CREAT;
        default:
            assert(false && "invalid file mode");
    }

    return 0;
}

static FileStatus cellFsErrorToFileStatus_(int err)
{
    switch (err)
    {
        case CELL_FS_OK:
            return FILE_STATUS_OK;
        case CELL_FS_ERROR_EEXIST:
            return FILE_STATUS_NOT_EXISTS;
        default:
            return FILE_STATUS_OPEN_FAILED;
    }
}

#define cellFsErrorToFileStatus(e) cellFsErrorToFileStatus_(e)
#define cellFsErrorToFileStatus0(s, e) \
    const FileStatus s = cellFsErrorToFileStatus_(e)
#define cellFsErrorToFileStatus1(s, e) \
    cellFsErrorToFileStatus0(s, e);    \
    printf("%s:%s:%d: err 0x%x status %d\n", __FILE__, __FUNCTION__, __LINE__, e, s)
#define cellFsErrorToFileStatus2(s, e) \
    cellFsErrorToFileStatus0(s, e);    \
    printf("%s:%s:%d: path %s err 0x%x status %d\n", __FILE__, __FUNCTION__, __LINE__, path, e, s)

static int fileSeekModeToCellSeekMode(FileSeekMode mode)
{
    switch (mode)
    {
        case FILE_SEEK_CURRENT:
            return CELL_FS_SEEK_CUR;
        case FILE_SEEK_START:
            return CELL_FS_SEEK_SET;
        case FILE_SEEK_END:
            return CELL_FS_SEEK_END;
        default:
            assert(false && "invalid seek mode");
    }

    return 0;
}

FileStatus fileOpen(FileHandle* handle, const char* path, FileMode mode)
{
    // TODO: open a create file properly
    int fd;
    const int cellmode = fileModeToCellFs(mode);
    const uint64_t omode = (((cellmode & CELL_FS_O_CREAT) != 0) ? 777 : 0);
    printf("mode %ld (%lx)\n", omode, omode);
    int err = sys_fs_open(path, cellmode, &fd, omode, 0, 0);
    cellFsErrorToFileStatus2(status, err);
    *handle = fd;
    return status;
}

FileStatus fileRead(FileHandle handle, void* buffer, uint64_t count, uint64_t* readCount)
{
    int err = sys_fs_read(handle, buffer, count, readCount);
    cellFsErrorToFileStatus0(status, err);
    return status;
}

FileStatus fileWrite(FileHandle handle, const void* buffer, uint64_t count, uint64_t* writeCount)
{
    int err = sys_fs_write(handle, buffer, count, writeCount);
    cellFsErrorToFileStatus1(status, err);
    return status;
}

FileStatus fileWrite2(const char* path, const void* buffer, uint64_t count, uint64_t* writeCount)
{
    FileHandle handle;
    FileStatus status;

    status = fileOpen(&handle, path, FILE_MODE_READ_WRITE);
    printf("%s fileOpen status %d handle %d\n", __FUNCTION__, status, handle);
    if (status != FILE_STATUS_OK)
    {
        status = fileOpen(&handle, path, FILE_MODE_CREATE);
        printf("%s fileOpen FILE_MODE_CREATE status %d handle %d\n", __FUNCTION__, status, handle);
        if (status != FILE_STATUS_OK)
        {
            return status;
        }
    }

    status = fileWrite(handle, buffer, count, writeCount);
    printf("%s fileWrite status %d\n", __FUNCTION__, status);
    FileStatus closeStatus = fileClose(handle);

    if (status != FILE_STATUS_OK)
    {
        return status;
    }

    return closeStatus;
}

FileStatus fileSeek(FileHandle handle, FileSeekMode mode, uint64_t offset, uint64_t* position)
{
    int err = sys_fs_lseek(handle, offset, fileSeekModeToCellSeekMode(mode), position);
    FileStatus status = cellFsErrorToFileStatus(err);
    return status;
}

FileStatus fileTell(FileHandle handle, uint64_t* position)
{
    int err = fileSeek(handle, FILE_SEEK_CURRENT, 0, position);
    FileStatus status = cellFsErrorToFileStatus(err);
    return status;
}

FileStatus fileSize(FileHandle handle, uint64_t* size)
{
    FileStatus status;

    // Save current pos
    uint64_t tmp;
    status = fileTell(handle, &tmp);
    if (status != FILE_STATUS_OK)
    {
        return status;
    }

    // Seek to end
    uint64_t len;
    status = fileSeek(handle, FILE_SEEK_END, 0, &len);
    if (status != FILE_STATUS_OK)
    {
        return status;
    }

    // Seek back to old pos
    status = fileSeek(handle, FILE_SEEK_START, tmp, &tmp);
    if (status != FILE_STATUS_OK)
    {
        return status;
    }

    *size = len;
    return status;
}

FileStatus fileClose(FileHandle handle)
{
    int err = sys_fs_close(handle);
    FileStatus status = cellFsErrorToFileStatus(err);
    return status;
}

FileStatus fileReadLine(FileHandle handle, char* buffer, uint32_t bufferSize, uint64_t* bytesRead, uint64_t* bytesWritten)
{
    char* bufferCursor = buffer;

    *bytesRead = 0;
    while (true)
    {
        uint64_t curReadSize;
        FileStatus status = fileRead(handle, bufferCursor, 1, &curReadSize);
        if (status != FILE_STATUS_OK)
        {
            return status;
        }

        *bytesRead += curReadSize;

        if (*bufferCursor == '\r')
        {
            // skip return
            continue;
        }
        else if (*bufferCursor == '\n')
        {
            // overwrite newline with end of string
            *bufferCursor = 0;
            break;
        }
        else if (curReadSize == 0)
        {
            // write end of string
            bufferCursor++;
            *bufferCursor = 0;
            break;
        }

        *bytesWritten += curReadSize;
        bufferCursor += curReadSize;
    }

    return FILE_STATUS_OK;
}

void fileDelete(const char* path)
{
    sys_fs_unlink(path);
}
