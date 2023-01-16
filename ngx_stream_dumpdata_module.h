/*
 * @Author: 张大炮 mi1itray.axe@gmail.com
 * @Date: 2022-12-12 02:45:23
 * @LastEditors: mi1itray.axe mi1itray.axe@gmail.com
 * @LastEditTime: 2023-01-15 18:53:44
 * @Description:
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <errno.h> 

#define ON 0x01
#define OFF 0x00

typedef struct {
    ngx_array_t *rules;
} module_srv_conf_t;

typedef struct client_server_info_struct
{
	uint32_t src_addr; /* 来源ip */
	uint32_t dst_addr; /* 目的ip */
	uint16_t src_port; /* 来源端口 */
	uint16_t dst_port; /* 目的端口 */
} cs_info_t;

typedef struct transport_struct
{
	cs_info_t *sockaddr; /* ip端口,4元组 */
	uint8_t protocol;	 /* 协议: TCP/UDP */
	uint8_t *data;		 /* 数据 */
	uint16_t data_len;	 /* 数据长度 */
} tran_t;

typedef struct {
    char *name;
    ngx_uint_t on_off;
	int (*module_interface)(tran_t *);
} modules_switch;

#define new_cs_info_t(r) (cs_info_t *)ngx_palloc(r->pool, sizeof(cs_info_t))
#define new_tran_t(r) (tran_t *)ngx_palloc(r->pool,sizeof(tran_t))


/**
 * @brief 从nginx中的结构体ngx_stream_session_t和ngx_chain_t中读取需要的数据
 *          存储到tran_t* 和cs_info_t* 指针中。需要的数据有ip端口协议5元组
 * 		    (来源ip,来源端口,目的ip,目的端口,协议类型)，还要读取传输数据与数据长度。
 * 			来源ip端口从 s->connection->sockaddr提取
 * 			目标ip端口从 s->connection->local_sockaddr提取
 * 
 * @param s 是nginx stream session结构体指针，数据从这里取
 * @param in 每次交互存储的数据
 * @param from_upstream 用于判断数据是upstream来的还是downstream来的
 * @param t 自定义结构体用于存储信息
 */
void get_data_from_nginx(ngx_stream_session_t *s, ngx_chain_t *in, ngx_uint_t from_upstream, tran_t *t)
{

	cs_info_t *c = t->sockaddr;

	/* copy data */
	t->data = in->buf->pos;
	/* calculation data length */
	t->data_len = in->buf->last - in->buf->pos;
	/* Judge the protocol TCP/UDP */
	if (s->connection->udp)
	{
		t->protocol = UDP;
	}
	else
	{
		t->protocol = TCP;
	}
	/* copy src ip:port and dst ip:port */
	/* 根据ngx_stream_write_filter中的参数from_upstream来判断此时谁是来源谁是目的 */
	if (from_upstream)
	{
		memcpy(&c->src_addr, (char *)(s->connection->local_sockaddr) + 4, 4);
		memcpy(&c->src_port, (char *)(s->connection->local_sockaddr) + 2, 2);
		c->src_port = c->src_port >> 8 | (c->src_port & 0xff) << 8;
		memcpy(&c->dst_addr, (char *)(s->connection->sockaddr) + 4, 4);
		memcpy(&c->dst_port, (char *)(s->connection->sockaddr) + 2, 2);
		c->dst_port = c->dst_port >> 8 | (c->dst_port & 0xff) << 8;
	}
	else
	{
		memcpy(&c->src_addr, (char *)(s->connection->sockaddr) + 4, 4);
		memcpy(&c->src_port, (char *)(s->connection->sockaddr) + 2, 2);
		c->src_port = c->src_port >> 8 | (c->src_port & 0xff) << 8;
		memcpy(&c->dst_addr, (char *)(s->connection->local_sockaddr) + 4, 4);
		memcpy(&c->dst_port, (char *)(s->connection->local_sockaddr) + 2, 2);
		c->dst_port = c->dst_port >> 8 | (c->dst_port & 0xff) << 8;
	}
}

void test(ngx_stream_session_t *s, ngx_chain_t *in, ngx_uint_t from_upstream)
{
	module_srv_conf_t *ascf;
	modules_switch *switch_info;
	int ret_code;
	/* extern关键字可以链接已经编译好的ngx_stream_test_module */
	extern ngx_module_t ngx_stream_test_module;
	tran_t *t = new_tran_t(s->connection);
	t->sockaddr = new_cs_info_t(s->connection);
	get_data_from_nginx(s, in, from_upstream, t);
	// log_info(s, t);
	/* get the modules srv conf */
	ascf = (module_srv_conf_t *)ngx_stream_get_module_srv_conf(s,ngx_stream_test_module);
	if (ascf == NULL) {
		ngx_log_error(NGX_LOG_NOTICE, s->connection->log, 0,"Get modules srv conf error");
		return;
	}
	/* get the module name an on_off value */
	switch_info = (modules_switch *)ascf->rules->elts;
	/* test to run the customize module */
	for (int i = 0;i<ascf->rules->nelts;i++){
		if (strcmp(switch_info[i].name,"log") == 0 && switch_info->on_off == ON){
			ngx_log_debug0(NGX_LOG_DEBUG, s->connection->log, 0, "Use hello interface");
			ret_code = switch_info->module_interface(t);
			ngx_log_debug1(NGX_LOG_DEBUG, s->connection->log, 0,"hello_interface return code %d", ret_code);
		}
	}
}
