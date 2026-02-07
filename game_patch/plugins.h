#pragma once

#if !defined(PLUGIN_PARSER_H)
#define PLUGIN_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#include "../shared/GamePatchInfo.h"
#include "../game_patch/lv2_stdio.h"
#include "../shared/macros.h"

typedef struct
{
    char* app_id;
    char* app_versions[MAX_APP_VERS];
    size_t app_ver_count;
    bool is_app_ver_list;
    char** plugins;
    size_t plugin_count;
} PluginConfig;

typedef void (*PluginConfigCallback)(const PluginConfig* config, void* user_data);

typedef struct
{
    PluginConfigCallback callback;
    void* user_data;

    PluginConfig current_config;
    bool in_plugins_section : 1;
    bool has_current_config : 1;
    size_t current_indent;
} PluginParseContext;

#define reservered_buf 32
#define max_arg_buf (reservered_buf + 260)
#define max_args 32

#if defined(__PRX__)
typedef union bigint
{
    struct _c
    {
        char* hi;
        char* lo;
    } c;
} bigint;
#define pChar bigint
#define pChar2 bigint*
#else
#define pChar uint64_t
#define pChar2 uint32_t
#endif

typedef struct program_args
{
    uint32_t argc;
    pChar2 argv;
    uint32_t replace_args_size;
    uint32_t replace_args_per_buf_size;
    pChar new_argv[max_args];
    char replace_args[max_args][max_arg_buf + 1];
    struct change_flag
    {
        uint64_t changed : 1;
    } flag;
} program_args;
#if defined(__cplusplus)
static_assert(__builtin_offsetof(program_args, argc) == 0, "");
static_assert(__builtin_offsetof(program_args, argv) == 4, "");
static_assert(__builtin_offsetof(program_args, replace_args_size) == 8, "");
static_assert(__builtin_offsetof(program_args, replace_args_per_buf_size) == 12, "");
static_assert(__builtin_offsetof(program_args, new_argv) == 16, "");
static_assert(__builtin_offsetof(program_args, replace_args) == 272, "");
#endif

typedef struct
{
    const GamePatchInfo* game_info;
    PluginConfigCallback callback;
    program_args* main_arg;
    bool found_match;
} LoadPluginData;

int load_plugins(const char* filename, LoadPluginData* plugin_data);
bool append_arg(program_args* pargs, const char* arg);

#endif
