extern "C"
{
#include "GamePatchInfo.h"
#include "../game_patch/lib/file.h"
#include "../game_patch/lv2_stdio.h"
}

int load_game_info(const char* path, GamePatchInfo& info)
{
    FileHandle h = 0;
    FileStatus ret = fileOpen(&h, path, FILE_MODE_READ);
    uint64_t fsz = 0;
    int okay = 0;
    if (ret == FILE_STATUS_OK && fileSize(h, &fsz) == FILE_STATUS_OK && fsz == sizeof(info))
    {
        uint64_t readcount = 0;
        FileStatus read = fileRead(h, &info, sizeof(info), &readcount);
        if (read == FILE_STATUS_OK && readcount == sizeof(info))
        {
            printf("open %s %d %ld\n", path, ret, readcount);
            printf("read %d\n", read);
            printf("title %s\n", info.titleid);
            printf("app_ver %s\n", info.app_ver);
            okay = 1;
        }
    }
    fileClose(h);
    return okay;
}
