#include <cellstatus.h>
#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/stat.h>
#include "Memory/Detour.hpp"
#include <vsh/newDelete.hpp>
#include <vsh/stdc.hpp>
#include <vshlib.hpp>

#include "../shared/GamePatchInfo.h"
#include "../shared/memory.h"
#include "../shared/macros.h"

#include "../data/game_plugins.yml.inc.h"
#include "../data/game_plugin_bootloader.sprx.inc.h"

// Macros defining our module informations as well as our entry/exit point
SYS_MODULE_INFO(game_patch_vsh_data, 0, 1, 1);
SYS_MODULE_START(module_start);
SYS_MODULE_STOP(module_stop);

// SNC is too old to constexpr?
static uint32_t stringid(const char* str, uint32_t base = 0x811c9dc5)
{
    while (*str)
    {
        base = 0x01000193 * (base ^ *str++);
    }
    return base;
}

// masked by ff
static uintptr_t PatternScan(sys_pid_t process, const uintptr_t module_base, const uint64_t module_size, const void* signature, const uint64_t signature_size, const uint64_t offset)
{
#define seek_size 256
    if (!module_base || !module_size || !signature_size || signature_size > seek_size)
    {
        return 0;
    }
    for (uintptr_t seek = module_base; seek < ((module_base + module_size) - signature_size); seek += seek_size)
    {
        uint8_t seek_buf[seek_size];
        memset(seek_buf, 0, sizeof(seek_buf));
        if (ReadProcessMemory(process, (void*)seek, seek_buf, sizeof(seek_buf)) == 0)
        {
            const uint8_t* scanBytes = seek_buf;
            const uint8_t* patternBytes = (uint8_t*)signature;
            for (uint64_t i = 0; i < sizeof(seek_buf); i++)
            {
                uint8_t found = 1;
                for (int32_t j = 0; j < signature_size; j++)
                {
                    if (scanBytes[i + j] != patternBytes[j] &&
                        patternBytes[j] != 0xff)
                    {
                        found = 0;
                        break;
                    }
                }
                if (found)
                {
                    const uintptr_t found_addr = (seek + (((uintptr_t)&scanBytes[i] - (uintptr_t)&scanBytes[0]) + offset));
                    // DPRINTF2("found_addr %lx\n", found_addr);
                    return found_addr;
                }
            }
        }
    }
#undef seek_size
    return 0;
}

static uint64_t GetVshSize()
{
    const uint64_t* memsz = reinterpret_cast<uint64_t*>(0x00010068);  // ElfHeader->Elf64[0].p_memsz
    return memsz[0];
}

#if !defined(MARK_AS_EXECUTABLE)
#define MARK_AS_EXECUTABLE __attribute__((section(".text")))
#endif

// Intellisense doesn't like asm compiler extensions
#if defined(__INTELLISENSE__)
#define asm
#endif

MARK_AS_EXECUTABLE static uint32_t m_JumpBuff[16] = {0};

// this will fool compiler to try to not obtain function pointer toc
#define STATIC_FUNCTION_PTR(ret_type, func_name, ...)                \
    asm __attribute__((naked)) ret_type func_name##_ptr(__VA_ARGS__) \
    {                                                                \
        trap;                                                        \
    }

STATIC_FUNCTION_PTR(void, load_plugin_main, void* p);

static void load_plugin_main(void* _p)
{
    struct my_Data
    {
        std::string plugin_name;
        std::string call_back_func;
        std::string resource_name;
        std::string plugin_path;
    }* p = (my_Data*)_p;
    static uint32_t hash_gp = 0;
    if (!hash_gp)
    {
        hash_gp = stringid("game_plugin");
    }
    paf::View* gamePlugin = paf::View::Find("game_plugin");
    const char* plugin_str = p->plugin_name.c_str();
    if (gamePlugin && stringid(plugin_str) == hash_gp)
    {
        vsh::printf("gamePlugin %p\n", gamePlugin);
        if (vsh::GamePluginInterface* gameInterface = gamePlugin->GetInterface<vsh::GamePluginInterface*>(1))
        {
            const uint32_t vsh_pid = sys_process_getpid();
            vsh::GamePluginInterface::gameInfo _gameInfo = {0};
            gameInterface->GameInfo(_gameInfo);
            vsh::printf("%s\n", _gameInfo.titleid);
            vsh::printf("%s\n", _gameInfo.titlename);
            vsh::printf("%s\n", _gameInfo.titlename2);
            vsh::GamePluginInterface::AppVerInfo app_ver = {0};
            gameInterface->GetAppVer(app_ver);
            vsh::printf("%s\n", app_ver.buf);
            GamePatchInfo write_info;
            vsh::memset(&write_info, 0, sizeof(write_info));
            vsh::memcpy(&write_info.titleid, &_gameInfo.titleid, sizeof(_gameInfo.titleid));
            vsh::memcpy(&write_info.app_ver, &app_ver.buf, sizeof(app_ver.buf));
            {
                FILE* fd = vsh::fopen(GAME_INFO_PATH, "wb");
                if (fd)
                {
                    vsh::fwrite(&write_info, sizeof(write_info), 1, fd);
                    vsh::fclose(fd);
                }
            }
            // need vsh to create this file first
            // because game from prx can't create it for some reason
            // or maybe i'm just setting up `sys_fs_open` wrong
            {
                FILE* fd = vsh::fopen(GAME_PATCH_NOTIFY_MSG_FILE, "wb");
                if (fd)
                {
                    char buf = CHECK_BYTE;
                    vsh::fwrite(&buf, sizeof(buf), 1, fd);
                    vsh::fclose(fd);
                }
            }
        }
    }
    load_plugin_main_ptr(p);
}


// #define S_IFMT 0xf000
// #define S_IFREG 0x8000
#define S_ISREG(_m) (((_m) & S_IFMT) == S_IFREG)

static int file_exists(const char* path)
{
    stat sb;
    vsh::memset(&sb, 0, sizeof(sb));
    if ((vsh::stat(path, &sb) == 0) && S_ISREG(sb.st_mode))
    {
        return 0;
    }
    return -1;
}

char* read_file(const char* filename, int& file_size)
{
    FILE* file = vsh::fopen(filename, "rb");
    if (file == NULL)
    {
        vsh::perror("Error opening file");
        return NULL;
    }

    vsh::fseek(file, 0, SEEK_END);
    int size = vsh::ftell(file);
    vsh::fseek(file, 0, SEEK_SET);

    if (size < 0)
    {
        vsh::perror("Error getting file size");
        vsh::fclose(file);
        return NULL;
    }

    const size_t size_1 = size + 1;
    char* buffer = (char*)vsh::malloc(size_1);
    if (buffer == NULL)
    {
        vsh::perror("Memory allocation failed");
        vsh::fclose(file);
        return NULL;
    }

    vsh::memset(buffer, 0, size_1);

    size_t bytes_read = vsh::fread(buffer, 1, size, file);
    if (bytes_read != (size_t)size)
    {
        vsh::perror("Error reading file");
        vsh::free(buffer);
        vsh::fclose(file);
        return NULL;
    }

    vsh::fclose(file);

    file_size = size;

    return buffer;
}

static bool read_file(const char* filename, char* buffer, const size_t max_size, int& file_size)
{
    if (buffer == NULL)
    {
        vsh::printf("Error: Buffer is NULL\n");
        return false;
    }

    FILE* file = vsh::fopen(filename, "rb");
    if (file == NULL)
    {
        vsh::perror("Error opening file");
        return false;
    }

    vsh::fseek(file, 0, SEEK_END);
    int size = vsh::ftell(file);
    vsh::fseek(file, 0, SEEK_SET);

    if (size < 0)
    {
        vsh::perror("Error getting file size");
        vsh::fclose(file);
        return false;
    }

    if ((size_t)size >= max_size)
    {
        vsh::printf("Error: File size (%d bytes) exceeds maximum buffer size of %ld bytes\n", size, max_size);
        vsh::fclose(file);
        return false;
    }

    vsh::memset(buffer, 0, max_size);

    size_t bytes_read = vsh::fread(buffer, 1, size, file);
    if (bytes_read != (size_t)size)
    {
        vsh::perror("Error reading file");
        vsh::fclose(file);
        return false;
    }

    vsh::fclose(file);

    file_size = size;

    return true;
}

static void write_default_blob(const char* path, const void* data, const size_t data_sz)
{
    const int ret = file_exists(path);
    if (ret != 0)
    {
        vsh::printf("file %s does not exist, creating default.\n", path);
        FILE* fd = vsh::fopen(path, "wb");
        if (fd)
        {
            vsh::fwrite(data, data_sz, 1, fd);
            vsh::fclose(fd);
        }
    }
    else if (ret == 0)
    {
        vsh::printf("file %s exists. not creating default.\n", path);
    }
}

static void make_folders()
{
    // create data folders because only vsh has enough permissions to do it
    static const char* paths[] = {
        GAME_PATCH_DATA_PATH,
        GAME_PATCH_SETTINGS,
        GAME_PATCH_FILES_PATH,
        GAME_PATCH_WORK_PATH,
    };
    for (size_t i = 0; i < _countof(paths); i++)
    {
        vsh::mkdir(paths[i], 0777);
    }
    // delete temp info file
    vsh::unlink(GAME_PATCH_NOTIFY_MSG_FILE);
    vsh::unlink(GAME_INFO_PATH);
    write_default_blob(PLUGINS_PATH, game_plugins_yml_data, sizeof(game_plugins_yml_data));
    write_default_blob(BOOTLOADER_PATH, game_plugin_bootloader_sprx_data, sizeof(game_plugin_bootloader_sprx_data));
}

static void run_patch(uint64_t)
{
    // wait at least 15 seconds so target manager can see the prints
#if defined(LAUNCHER_DEBUG)
    const int sleep_time = 15;
    for (int i = 0; i < sleep_time; i++)
    {
        vsh::printf("waiting to patch for %d/%d\n", i + 1, sleep_time);
        sys_timer_sleep(1);
    }
#endif
    make_folders();
    vsh::printf("patching now!\n");
    static const uint32_t start_plugin_search[] = {
        0x801f0008,
        0x2f800000,
        0x781f0020,
        0x7fe3fb78,
        0x419effff,
        0xffffffff,
        0x7fe3fb78,
        0xffffffff,
    };
    const uint32_t vsh_pid = sys_process_getpid();
    bool patch_okay = false;
    if (vsh_pid)
    {
        const uintptr_t start_plugin_search_addr = PatternScan(vsh_pid, 0x00010000, GetVshSize(), start_plugin_search, sizeof(start_plugin_search), 20);
        vsh::printf("start_plugin_search_addr %x\n", start_plugin_search_addr);
        if (start_plugin_search_addr)
        {
            patch_call(start_plugin_search_addr, load_plugin_main, load_plugin_main_ptr);
            patch_okay = true;
        }
    }
    if (patch_okay)
    {
        vsh::printf("patched okay!\n");
    }
    sys_ppu_thread_exit(0);
}

static void notify_thread(uint64_t arg)
{
    while (1)
    {
        if (file_exists(GAME_PATCH_NOTIFY_MSG_FILE) == 0)
        {
            // vsh::printf("%s exist\n", GAME_PATCH_NOTIFY_MSG_FILE);
            int notify_buf_sz = 0;
            char notify_buf[256 + 1];
            if (read_file(GAME_PATCH_NOTIFY_MSG_FILE, notify_buf, sizeof(notify_buf), notify_buf_sz) && notify_buf[0] != CHECK_BYTE && notify_buf_sz > 1)
            {
                wchar_t buf[_countof(notify_buf)];
                vsh::memset(buf, 0, sizeof(buf));
                vsh::swprintf(buf, _countof_1(buf), L"%hs", notify_buf);
                const int wait = 2;
                vsh::printf("waiting for %d seconds\n", wait);
                sys_timer_sleep(wait);
                vsh::printf("notify: %s\n", notify_buf);
                vsh::ShowNotificationWithIcon(buf, vsh::NotifyIcon::Info);
                vsh::unlink(GAME_PATCH_NOTIFY_MSG_FILE);
            }
        }
        sys_timer_sleep(1);
    }
}

CDECL_BEGIN
int module_start(unsigned int args, void* argp)
{
    sys_ppu_thread_t gVshMenuPpuThreadId = SYS_PPU_THREAD_ID_INVALID;
    sys_ppu_thread_create(&gVshMenuPpuThreadId, run_patch, 0, 1059, 4096, SYS_PPU_THREAD_CREATE_JOINABLE, "game_patch_vsh_data_start");
    sys_ppu_thread_create(&gVshMenuPpuThreadId, notify_thread, 0, 1059, 4096, SYS_PPU_THREAD_CREATE_JOINABLE, "game_patch_vsh_notify_thread");

    // Exit thread using directly the syscall and not the user mode library or else we will crash
    _sys_ppu_thread_exit(0);

    return 0;
}

int module_stop(unsigned int args, void* argp)
{
    sys_ppu_thread_t stopPpuThreadId;
    int ret = sys_ppu_thread_create(&stopPpuThreadId, [](uint64_t arg) -> void
                                    { sys_ppu_thread_exit(0); },
                                    0,
                                    2816,
                                    1024,
                                    SYS_PPU_THREAD_CREATE_JOINABLE,
                                    "game_patch_vsh_data_stop");

    if (ret == SUCCEEDED)
    {
        uint64_t exitCode;
        sys_ppu_thread_join(stopPpuThreadId, &exitCode);
    }

    sys_timer_sleep(5);

    // unloading prx from memory
    sys_prx_id_t prxId = _sys_prx_get_my_module_id();
    uint64_t meminfo[5]{0x28, 2, 0, 0, 0};
    _sys_prx_stop_module(prxId, 0, meminfo, NULL, 0, NULL);

    // Exit thread using directly the syscall and not the user mode library or else we will crash
    _sys_ppu_thread_exit(0);

    return 0;
}
CDECL_END
