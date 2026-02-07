#include "plugins.h"
#include "patch.h"
#include "my_string.h"
#include "../game_patch/lib/file.h"
#include "../shared/GamePatchInfo.h"

static void free_plugin_config(PluginConfig* config)
{
    if (!config)
    {
        return;
    }

    if (config->app_id)
    {
        free(config->app_id);
        config->app_id = NULL;
    }

    for (size_t i = 0; i < config->app_ver_count; i++)
    {
        if (config->app_versions[i])
        {
            free(config->app_versions[i]);
            config->app_versions[i] = NULL;
        }
    }

    if (config->plugins)
    {
        for (size_t i = 0; i < config->plugin_count; i++)
        {
            if (config->plugins[i])
            {
                free(config->plugins[i]);
                config->plugins[i] = NULL;
            }
        }
        free(config->plugins);
        config->plugins = NULL;
    }

    config->app_ver_count = 0;
    config->plugin_count = 0;
}

static void init_plugin_parse_context(PluginParseContext* ctx)
{
    if (!ctx)
    {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
}

static void free_plugin_parse_context(PluginParseContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    if (ctx->has_current_config)
    {
        free_plugin_config(&ctx->current_config);
    }
}

static void emit_current_config(PluginParseContext* ctx)
{
    if (!ctx->has_current_config)
    {
        return;
    }

    if (ctx->callback)
    {
        ctx->callback(&ctx->current_config, ctx->user_data);
    }

    free_plugin_config(&ctx->current_config);
    memset(&ctx->current_config, 0, sizeof(ctx->current_config));
    ctx->has_current_config = false;
    ctx->in_plugins_section = false;
}



static bool add_plugin(PluginConfig* config, const char* plugin_path)
{
    if (!config || !plugin_path)
    {
        return false;
    }

    char* plugin_str = parse_quoted_string(plugin_path);
    if (!plugin_str)
    {
        return false;
    }

    size_t new_count = config->plugin_count + 1;
    char** new_plugins = malloc(sizeof(char*) * new_count);
    if (!new_plugins)
    {
        free(plugin_str);
        return false;
    }

    if (config->plugins && config->plugin_count > 0)
    {
        memcpy(new_plugins, config->plugins, sizeof(char*) * config->plugin_count);
        free(config->plugins);
    }

    new_plugins[config->plugin_count] = plugin_str;
    config->plugins = new_plugins;
    config->plugin_count = new_count;

    return true;
}


static void process_line(const char* line, PluginParseContext* ctx)
{
    if (is_comment_or_empty(line))
    {
        return;
    }

    size_t indent = get_indent_level(line);
    const char* content = line;
    while (*content && isspace(*content))
    {
        content++;
    }

    if (strncmp2(content, "- app:") == 0)
    {
        emit_current_config(ctx);

        ctx->has_current_config = true;
        ctx->in_plugins_section = false;
        ctx->current_indent = indent;

        char* app_str = parse_quoted_string(content + 6);
        if (app_str)
        {
            ctx->current_config.app_id = app_str;
        }
    }
    else if (strstr(content, "app_ver:") && ctx->has_current_config)
    {
        const char* ver_start = strstr(content, "app_ver:");
        ver_start += 8;

        if (is_list_value(content))
        {
            ctx->current_config.is_app_ver_list = true;
            char* versions[MAX_APP_VERS];
            bzero(versions, sizeof(versions));
            size_t count = parse_string_list(content, versions, _countof(versions));

            if (count > 0)
            {
                for (size_t i = 0; i < count && i < _countof(versions); i++)
                {
                    ctx->current_config.app_versions[i] = versions[i];
                }
                ctx->current_config.app_ver_count = count;
            }
        }
        else
        {
            ctx->current_config.is_app_ver_list = false;
            char* version = parse_quoted_string(ver_start);
            if (version)
            {
                ctx->current_config.app_versions[0] = version;
                ctx->current_config.app_ver_count = 1;
            }
        }
    }
    else if (strstr(content, "plugins:") && ctx->has_current_config)
    {
        ctx->in_plugins_section = true;
        ctx->current_indent = indent;
    }
    else if (ctx->in_plugins_section && ctx->has_current_config && *content == '-')
    {
        content++;
        while (*content && isspace(*content))
        {
            content++;
        }
        add_plugin(&ctx->current_config, content);
    }
}

static int parse_plugin(PluginParseContext* ctx, const char* filename, PluginConfigCallback callback, void* user_data)
{
    if (!ctx || !filename || !filename[0])
    {
        here();
        return -1;
    }

    ctx->callback = callback;
    ctx->user_data = user_data;

    FileHandle h = 0;
    FileStatus ret = fileOpen(&h, filename, FILE_MODE_READ);
    printf("ret %d\n", ret);
    if (ret != FILE_STATUS_OK)
    {
        here();
        return -1;
    }

    uint64_t fsz = 0;
    if (fileSize(h, &fsz) != FILE_STATUS_OK)
    {
        here();
        fileClose(h);
        return -1;
    }

    uint64_t bufread_add = 0;
    uint64_t bufread = 0, bufread2 = 0;

    while (bufread_add < fsz)
    {
        char line[MAX_LINE_LENGTH + 1];
        memset(line, 0, sizeof(line));
        FileStatus readret = fileReadLine(h, line, _countof_1(line), &bufread, &bufread2);
        if (readret != FILE_STATUS_OK || bufread == 0)
        {
            break;
        }

        bufread_add += bufread;
        process_line(line, ctx);
    }
    emit_current_config(ctx);
    fileClose(h);
    return 0;
}

int load_plugins(const char* filename, LoadPluginData* plugin_data)
{
    if (!filename || !plugin_data)
    {
        return -1;
    }

    PluginParseContext ctx;
    init_plugin_parse_context(&ctx);

    int result = parse_plugin(&ctx, filename, plugin_data->callback, plugin_data);

    free_plugin_parse_context(&ctx);

    if (result != 0)
    {
        return -1;
    }

    return plugin_data->found_match ? 0 : -1;
}

bool append_arg(program_args* pargs, const char* arg)
{
    if (pargs->argc >= max_args)
    {
        return false;
    }
    const int idx = pargs->argc;
    const size_t count = _countof_1(pargs->replace_args[idx]);
    strncpy(pargs->replace_args[idx], arg, count);
    pargs->new_argv[idx].c.lo = pargs->replace_args[idx];
    pargs->argc += 1;
    if (pargs->argc < max_args)
    {
        bzero(&pargs->new_argv[pargs->argc], sizeof(pargs->new_argv[pargs->argc]));
    }
    pargs->flag.changed = true;
    return true;
}
