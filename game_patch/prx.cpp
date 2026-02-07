// #include "stdafx.h"

#include <sysutil/sysutil_gamecontent.h>

#include <cellstatus.h>
#include <sys/prx.h>
#include <stdarg.h>
#include <sys/timer.h>
#include <sys/process.h>

#include <unistd.h>

extern "C"
{
#include "lib/file.h"
#include "lv2_stdio.h"
}

#include "../shared/GamePatchInfo.hpp"
#include "plugins.h"
#include "../shared/memory.h"

SYS_MODULE_INFO(PPU_Game_Patch, 0, 1, 1);
SYS_MODULE_START(module_start);
SYS_MODULE_STOP(module_stop);

extern "C" int run_patch(GamePatchInfo&);

static int init_list(const char* path)
{
    GamePatchInfo info;
    bzero(&info, sizeof(info));
    if (load_game_info(path, info) && run_patch(info) == 0)
    {
        puts("patch hopefully okay!");
    }
    else
    {
        puts("patch failed?");
    }
#if 0 && defined(_DEBUG)
    sys_timer_sleep(2);
    if (1)
    {
        __builtin_trap();
    }
#endif
    return 0;
}

extern "C" program_args* g_args = 0;

extern "C" int module_start(unsigned int argc, program_args& arg)
{
    g_args = &arg;
    for (uint32_t i = 0; i < arg.argc; i++)
    {
        printf("arg[%d]: %s\n", i, arg.argv[i].c.lo ? arg.argv[i].c.lo : "");
    }
    init_list(GAME_INFO_PATH);
    return SYS_PRX_NO_RESIDENT;
}

extern "C" int module_stop(void)
{
    return 0;
}
