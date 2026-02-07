#ifndef FILE_H
#define FILE_H

#include <stdint.h>

/**
 * @brief Describes the status of a file operation.
 *
 */
typedef enum
{
    FILE_STATUS_OK,
    FILE_STATUS_NOT_EXISTS,
    FILE_STATUS_OPEN_FAILED,
} FileStatus;

/**
 * @brief The mode in which a file is opened.
 *
 */
typedef enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_READ_WRITE,
    FILE_MODE_CREATE,
} FileMode;

/**
 * @brief Describes the different seek modes used to calculate the file position.
 *
 */
typedef enum
{
    FILE_SEEK_CURRENT,
    FILE_SEEK_START,
    FILE_SEEK_END,
} FileSeekMode;

typedef int FileHandle;

/**
 * @brief Opens a file at the given path using the specified mode.
 *
 * @param handle The returned file handle.
 * @param path The file path.
 * @param mode The mode in which the file is to be opened.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileOpen(FileHandle* handle, const char* path, FileMode mode);

/**
 * @brief Reads bytes from the file into a given buffer.
 *
 * @param handle The file handle.
 * @param buffer The buffer to write to.
 * @param count The number of bytes to read from the file.
 * @param readCount The actual number of bytes read from the file.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileRead(FileHandle handle, void* buffer, uint64_t count, uint64_t* readCount);

/**
 * @brief Writes a buffer to the specified file.
 *
 * @param handle The file handle.
 * @param buffer The buffer to write to the file.
 * @param count The number of bytes to write to the file.
 * @param writeCount The number of bytes actually written to the file.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileWrite(FileHandle handle, const void* buffer, uint64_t count, uint64_t* writeCount);
FileStatus fileWrite2(const char* path, const void* buffer, uint64_t count, uint64_t* writeCount);
    /**
 * @brief Seeks a specified file to a given position.
 *
 * @param handle The file handle.
 * @param mode The mode used to determine the new absolute file position.
 * @param offset The offset in the specified mode.
 * @param position The new file position after seeking.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileSeek(FileHandle handle, FileSeekMode mode, uint64_t offset, uint64_t* position);

/**
 * @brief Returns the file size of an opened file.
 *
 * @param handle The file handle.
 * @param size The returned file size.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileSize(FileHandle handle, uint64_t* size);

/**
 * @brief Returns the current file position.
 *
 * @param handle The file handle.
 * @param position The current file position.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileTell(FileHandle handle, uint64_t* position);

/**
 * @brief Closes the specified file handle.
 *
 * @param handle The file handle.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileClose(FileHandle handle);

/**
 * @brief Reads an ASCII line from the specified file.
 *
 * @param handle The file handle.
 * @param buffer The buffer to write the line to.
 * @param bufferSize The total size of the write buffer.
 * @param bytesRead The number of bytes read from the file.
 * @param bytesWritten The number of bytes writting into the write buffer.
 * @return FileStatus indicating whether the operation succeeded.
 */
FileStatus fileReadLine(FileHandle handle, char* buffer, uint32_t bufferSize, uint64_t* bytesRead, uint64_t* bytesWritten);

void fileDelete(const char* path);

#endif
