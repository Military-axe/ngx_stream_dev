/*
 * @Author: 张大炮 mi1itray.axe@gmail.com
 * @Date: 2023-01-07 04:30:01
 * @LastEditors: mi1itray.axe mi1itray.axe@gmail.com
 * @LastEditTime: 2023-01-15 17:51:13
 * @Description: 测试使用的nginx stream module
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <dlfcn.h>

typedef struct
{
	uint32_t src_addr; /* 来源ip */
	uint32_t dst_addr; /* 目的ip */
	uint16_t src_port; /* 来源端口 */
	uint16_t dst_port; /* 目的端口 */
} cs_info_s;

typedef struct
{
	cs_info_s *sockaddr; /* ip端口,4元组 */
	uint8_t protocol;	 /* 协议: TCP/UDP */
	uint8_t *data;		 /* 数据 */
	uint16_t data_len;	 /* 数据长度 */
} tran_s;

typedef struct
{
    char *name;
    ngx_uint_t on_off;
    int (*module_interface)(tran_s *);
} ngx_stream_test_on_off;

typedef struct
{
    ngx_array_t *rules;
} ngx_stream_test_srv_conf_t;

static char *
ngx_stream_test_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t
ngx_stream_test_init(ngx_conf_t *cf);
static void *
ngx_stream_test_create_srv_conf(ngx_conf_t *cf);

static ngx_command_t ngx_stream_test_commands[] = {

    {
        ngx_string("modules"),
        NGX_STREAM_MAIN_CONF | NGX_STREAM_SRV_CONF | NGX_CONF_TAKE2,
        ngx_stream_test_rule,
        NGX_STREAM_SRV_CONF_OFFSET,
        0,
        NULL,
    },

    ngx_null_command

};

static ngx_stream_module_t ngx_stream_test_module_ctx = {
    NULL,                 /* preconfiguration */
    ngx_stream_test_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    ngx_stream_test_create_srv_conf, /* create server configuration */
    NULL,                            /* merge server configuration */
};

ngx_module_t ngx_stream_test_module = {
    NGX_MODULE_V1,
    &ngx_stream_test_module_ctx, /* module context */
    ngx_stream_test_commands,    /* module directives */
    NGX_STREAM_MODULE,           /* module type */
    NULL,                        /* init master */
    NULL,                        /* init module */
    NULL,                        /* init process */
    NULL,                        /* init thread */
    NULL,                        /* exit thread */
    NULL,                        /* exit process */
    NULL,                        /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_int_t ngx_stream_test_init(ngx_conf_t *cf)
{
    ngx_stream_handler_pt *h;
    ngx_stream_core_main_conf_t *cmcf;

    cmcf = (ngx_stream_core_main_conf_t *)ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);

    return NGX_OK;
}

static char *ngx_stream_test_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_stream_test_srv_conf_t *ascf = conf;
    ngx_uint_t on_off = 0;
    ngx_str_t *value;
    ngx_stream_test_on_off *rule;

    value = cf->args->elts;
    if (value[2].len == 2 && ngx_strcmp(value[2].data, "on") == 0)
    {
        on_off = 1;
    }
    if (ascf->rules == NULL)
    {
        ascf->rules = ngx_array_create(cf->pool, 2, sizeof(ngx_stream_test_on_off));
    }
    rule = ngx_array_push(ascf->rules);
    if (rule == NULL)
    {
        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0,
                           "test mod get argu error");
        return NGX_CONF_ERROR;
    }
    rule->name = ngx_palloc(cf->pool, sizeof(ngx_str_t) * value[1].len);
    ngx_memcpy(rule->name, value[1].data, value[1].len);
    rule->on_off = on_off;

    /* open customize module */
    void* handle = dlopen("/root/Documents/nginx_stream_dev/customize_modules/test/libtest.so", RTLD_LAZY);
    if (handle == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0 ,"could not open the module error code: %s", dlerror());
        return NGX_CONF_ERROR;
    }
    int (*temp)(tran_s* a);
    temp = dlsym(handle, "test_modules_interface");
    rule->module_interface = temp;
    
    return NGX_CONF_OK;
}

static void *ngx_stream_test_create_srv_conf(ngx_conf_t *cf)
{
    ngx_stream_test_srv_conf_t *conf;

    conf = (ngx_stream_test_srv_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_test_srv_conf_t));
    if (conf == NULL)
    {
        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, "ngx_stream_test_create_srv_conf error");
        return NULL;
    }

    return conf;
}
