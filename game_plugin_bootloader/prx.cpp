extern "C"
{
#include "../game_patch/lv2_stdio.h"
#include "../shared/macros.h"
#include "../game_patch/lib/file.h"
#include "../game_patch/plugins.h"
}

#include "../shared/GamePatchInfo.hpp"
#include "../shared/memory.h"

#include <cellstatus.h>
#include <sys/prx.h>
#include <sys/timer.h>

SYS_MODULE_INFO(game_plugin_bootloader, 0, 1, 1);
SYS_MODULE_START(module_start);
SYS_MODULE_STOP(module_stop);

static void load_plugin_callback(const PluginConfig* config, void* user_data)
{
    LoadPluginData* data = (LoadPluginData*)user_data;

    if (!config->app_id || !data->game_info)
    {
        return;
    }

    if (strncmp2(config->app_id, "all") == 0 || strcmp(config->app_id, data->game_info->titleid) == 0)
    {
        printf("app id match %s\n", config->app_id);

        bool version_match = false;
        for (size_t j = 0; j < config->app_ver_count; j++)
        {
            const char* curr_app_ver = config->app_versions[j];
            if (strncmp2(curr_app_ver, "all") == 0 || strcmp(curr_app_ver, data->game_info->app_ver) == 0)
            {
                printf("app ver match %s\n", curr_app_ver);
                version_match = true;
                break;
            }
        }

        if (!version_match)
        {
            return;
        }

        data->found_match = true;

        for (size_t j = 0; j < config->plugin_count; j++)
        {
            const char* path = config->plugins[j];
            printf("Loading: %s\n", path);
            const int prx = sys_prx_load_module(path, 0, 0);
            if (prx < 0)
            {
                printf("failed to load %s due to 0x%08x\n", path, prx);
                continue;
            }
            int res = 0;
            const int ret = sys_prx_start_module(prx, 1, data->main_arg, &res, 0, 0);
            if (ret < 0)
            {
                printf("failed to start %s due to 0x%08x\n", path, ret);
                continue;
            }
            printf("prx started, res %08x\n", res);
        }
    }
}

static int parse_load_file(const char* filename, program_args& args)
{
    if (!filename || !filename[0])
    {
        puts("invalid filename");
        return -1;
    }

    GamePatchInfo info;
    bzero(&info, sizeof(info));
    if (load_game_info(GAME_INFO_PATH, info) == 0)
    {
        puts("failed to retrive game info");
        return -1;
    }
    LoadPluginData data;
    bzero(&data, sizeof(data));
    data.game_info = &info;
    data.callback = load_plugin_callback;
    data.main_arg = &args;
    load_plugins(filename, &data);
    return 0;
}

static bool copy_main_args(program_args* pargs, int argc, bigint* argv)
{
    pargs->argc = argc;
    pargs->argv = argv;

    for (int i = 0; i < argc && i < max_args; i++)
    {
        if (!argv[i].c.lo)
        {
            continue;
        }
        pargs->new_argv[i] = argv[i];
    }

    for (int i = argc; i < max_args; i++)
    {
        bzero(&pargs->new_argv[i], sizeof(pargs->new_argv[i]));
    }

    return true;
}

static bool check_bytes(void* p, uint8_t num, size_t s, size_t& diff)
{
    diff = 0;
    uint8_t* pd = (uint8_t*)p;
    for (size_t i = 0; i < s; i++)
    {
        if (pd[i] != num)
        {
            diff += 1;
        }
    }
    return diff == 0;
}

extern "C" int module_start(unsigned int argc, program_args& arg)
{
    printf("program_args size %d\n", sizeof(program_args));
    printf("arg %p argc %d argv %p\n", &arg, arg.argc, arg.argv);
    printf("replace_arg_buf %ld\n", arg.replace_args_size);
    for (uint32_t i = 0; i < arg.argc; i++)
    {
        printf("arg[%d]: (%p) %s\n", i, &arg.argv[i].c.lo, arg.argv[i].c.lo ? arg.argv[i].c.lo : "");
    }

    size_t diff_bytes = 0;
    if (!check_bytes(arg.replace_args, 0, arg.replace_args_size, diff_bytes))
    {
        printf("WARNING: replacement arg buf contains %ld bytes that was not zero! was the stack ever truly cleared?\n", diff_bytes);
    }
    else
    {
        printf("checked replacement buf of %ld bytes and it was all clear!\n", arg.replace_args_size);
    }

    if (!copy_main_args(&arg, arg.argc, arg.argv))
    {
        printf("failed to copy main argc/argv\n");
    }

    parse_load_file(PLUGINS_PATH, arg);

    printf("argc now %d\n", arg.argc);
    hex_dump(arg.new_argv, sizeof(arg.new_argv), (uintptr_t)arg.new_argv);
    for (uint32_t i = 0; i < arg.argc; i++)
    {
        printf("arg[%d]: (%p) %s\n", i, arg.new_argv[i].c.lo, arg.new_argv[i].c.lo ? arg.new_argv[i].c.lo : "");
    }
    // so vsh can notify user
    sys_timer_sleep(2);
    return SYS_PRX_NO_RESIDENT;
}

extern "C" int module_stop(void)
{
    return 0;
}
