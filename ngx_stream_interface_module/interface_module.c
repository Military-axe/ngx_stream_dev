/**
 * @file interface_module.c
 * @author mi1itray.axe (mi1itray.axe@gmail.com)
 * @brief nginx stream的模块，用于收集nginx.conf配置信息
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023 mi1itray.axe
 *
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <dlfcn.h>
#include "interface_header.h"

typedef struct
{
    ngx_array_t *rules;
} ngx_stream_interface_srv_conf_t;

static char *
ngx_stream_interface_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t
ngx_stream_interface_init(ngx_conf_t *cf);
static void *
ngx_stream_interface_create_srv_conf(ngx_conf_t *cf);
static char *
ngx_stream_interface_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_command_t ngx_stream_interface_commands[] = {

    {
        ngx_string("modules"),
        NGX_STREAM_MAIN_CONF | NGX_STREAM_SRV_CONF | NGX_CONF_TAKE3 |
            NGX_CONF_TAKE4 | NGX_CONF_TAKE5 | NGX_CONF_TAKE6 | NGX_CONF_TAKE7,
        ngx_stream_interface_rule,
        NGX_STREAM_SRV_CONF_OFFSET,
        0,
        NULL,
    },

    ngx_null_command

};

static ngx_stream_module_t ngx_stream_interface_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    ngx_stream_interface_create_srv_conf, /* create server configuration */
    ngx_stream_interface_merge_srv_conf,  /* merge server configuration */
};

ngx_module_t ngx_stream_interface_module = {
    NGX_MODULE_V1,
    &ngx_stream_interface_module_ctx, /* module context */
    ngx_stream_interface_commands,    /* module directives */
    NGX_STREAM_MODULE,                /* module type */
    NULL,                             /* init master */
    NULL,                             /* init module */
    NULL,                             /* init process */
    NULL,                             /* init thread */
    NULL,                             /* exit thread */
    NULL,                             /* exit process */
    NULL,                             /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_int_t ngx_stream_interface_init(ngx_conf_t *cf)
{
    ngx_stream_handler_pt *h;
    ngx_stream_core_main_conf_t *cmcf;

    cmcf = (ngx_stream_core_main_conf_t *)ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);

    return NGX_OK;
}

/// @brief 读取nginx.conf中modules后跟随的的配置信息，并将信息存储到ngx_stream_interface_on_off结构体
/// @param cf 各种内存，log等信息
/// @param cmd 自定义的配置命令，这里是modules
/// @param conf 已经解析过的配置信息
/// @return 返回配置是否成功
static char *ngx_stream_interface_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_stream_interface_srv_conf_t *ascf = conf;
    ngx_str_t *value;
    modules_switch *rule;

    value = cf->args->elts;
    /**
     * 模块格式 modules [接口名] [so文件路径] [on/off] [模块参数]
     */
    if (value[3].len == 3 && ngx_strcmp(value[3].data, "off") == 0)
    {
        return NGX_CONF_OK;
    }
    if (ascf->rules == NULL)
    {
        ascf->rules = ngx_array_create(cf->pool, 2, sizeof(modules_switch));
    }
    rule = ngx_array_push(ascf->rules);
    if (rule == NULL)
    {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "ngx_array_push error");
        return NGX_CONF_ERROR;
    }
    rule->argv = ngx_palloc(cf->pool, sizeof(module_argv_t));
    rule->argv->argv_number = cf->args->nelts - 4;
    rule->interface_name = ngx_palloc(cf->pool, sizeof(char) * (value[2].len + 1));
    memcpy(rule->interface_name, value[1].data, value[1].len);
    rule->interface_name[value[1].len] = 0;
    /* copy the modules argument */
    // rule->argv->elts = value + 4;
    rule->argv->elts = ngx_palloc(cf->pool, sizeof(str_t) * rule->argv->argv_number);
    for (int i = 0; i < rule->argv->argv_number; i++){
        rule->argv->elts[i].len = value[4].len;
        /* copy the data */
        rule->argv->elts[i].data = ngx_palloc(cf->pool, sizeof(char) * (value[4].len + 1));
        memcpy(rule->argv->elts[i].data, value[4].data, value[4].len);
    }
    /* open customize module */
    void *handle = dlopen(value[2].data, RTLD_LAZY);
    if (handle == 0)
    {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "could not open the module error code: %s", dlerror());
        return NGX_CONF_ERROR;
    }
    int (*temp)(tran_t * a);
    temp = dlsym(handle, value[1].data);
    rule->module_interface = temp;

    return NGX_CONF_OK;
}

static void *ngx_stream_interface_create_srv_conf(ngx_conf_t *cf)
{
    ngx_stream_interface_srv_conf_t *conf;

    conf = (ngx_stream_interface_srv_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_interface_srv_conf_t));
    if (conf == NULL)
    {
        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, "ngx_stream_interface_create_srv_conf error");
        return NULL;
    }

    return conf;
}

static char *
ngx_stream_interface_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_stream_interface_srv_conf_t *prev = parent;
    ngx_stream_interface_srv_conf_t *conf = child;
    if (conf->rules == NULL)
    {
        conf->rules = prev->rules;
    }

    return NGX_CONF_OK;
}
