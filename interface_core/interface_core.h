///@file interface_core.h
///@author mi1itray.axe (mi1itray.axe@gmail.com)
///@brief 接口文件核心部分
///@version 0.1
///@date 2023-01-16
///
///@copyright Copyright (c) 2023 mi1itray.axe
///
///
#include <stdlib.h>
#include <stdio.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include "interface_header.h"
#define TCP 0x01
#define UDP 0x02
#define ON 0x01
#define OFF 0x00

typedef struct
{
	ngx_array_t *rules;
} module_srv_conf_t;

#define new_cs_info_t(r) (cs_info_t *)ngx_palloc(r->pool, sizeof(cs_info_t)) /// 生成一个cs_info_t结构体内存
#define new_tran_t(r) (tran_t *)ngx_palloc(r->pool, sizeof(tran_t))			 /// 生成一个tran_t结构体内存


///@brief 从nginx中的结构体ngx_stream_session_t和ngx_chain_t中读取需要的数据
///存储到tran_t* 和cs_info_t* 指针中。\n
///需要的数据有ip端口协议5元组\n
///(来源ip,来源端口,目的ip,目的端口,协议类型)，还要读取传输数据与数据长度。
///来源ip端口从 s->connection->sockaddr提取\n
///目标ip端口从 s->connection->local_sockaddr提取
///
///@param s 是nginx stream session结构体指针，数据从这里取
///@param in 每次交互存储的数据
///@param from_upstream 用于判断数据是upstream来的还是downstream来的
///@param t 自定义结构体用于存储信息
static void get_data_from_nginx(ngx_stream_session_t *s, ngx_chain_t *in, ngx_uint_t from_upstream, tran_t *t)
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


///@brief 接口核心函数，
///获取nginx运行中的数据\n
///获取配置中的参数，在一个循环体中循环遍历哪一个模块接口函数被调用\n
///
///@param s ngx_stream_session_t*
///@param in ngx_chain_t *
///@param from_upstream ngx_uint_t
///@todo 对于模块返回值ret_code的各种对应处理
void interface_core(ngx_stream_session_t *s, ngx_chain_t *in, ngx_uint_t from_upstream)
{
	module_srv_conf_t *ascf;
	modules_switch *switch_info;
	int ret_code;

	extern ngx_module_t ngx_stream_interface_module;

	ascf = (module_srv_conf_t* )ngx_stream_get_module_srv_conf(s, ngx_stream_interface_module);
	if (ascf == NULL)
	{
		ngx_log_error(NGX_LOG_NOTICE, s->connection->log, 0, "Get modules srv conf error");
	} 
	else if (ascf->rules == NULL) 
	{
		ngx_log_debug0(NGX_LOG_DEBUG, s->connection->log, 0, "Not run modules");
	}
	else
	{

		/* get data from nginx */

		tran_t *t = new_tran_t(s->connection);
		t->sockaddr = new_cs_info_t(s->connection);
		get_data_from_nginx(s, in, from_upstream, t);

		/* get the module arguments */
		
		switch_info = (modules_switch* )ascf->rules->elts;
		
		/* run the customize module */
		
		for (int i = 0; i < ascf->rules->nelts; i++)
		{
			if (switch_info[i].module_interface != NULL)
			{
				ret_code = switch_info[i].module_interface(t, switch_info[i].argv);
				ngx_log_debug2(NGX_LOG_DEBUG, s->connection->log, 0, "%s return code %d", switch_info[i].interface_name, ret_code);
				/* exception of ret_code */
				/* TODO */
				/* 处理需要细化,考虑数据可能损坏，需不需要复制一下数据 */
				switch (ret_code) {
					case MODULE_FILE_ERR:
						ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "Module: %s; module file manipulation error", switch_info[i].interface_name);
						break;
					case MODULE_OK:
						ngx_log_error(NGX_LOG_DEBUG, s->connection->log, 0, "Module: %s; module complete", switch_info[i].interface_name);
						break;
					case MODULE_ERR:
						ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "Module: %s; module run error", switch_info[i].interface_name);
						break;
					case MODULE_EXCEPTION_END:
						ngx_log_error(NGX_LOG_ERR, s->connection->log, 0, "Module: %s; module run exception to end", switch_info[i].interface_name);
						break;
				}
			}
		}

		ngx_pfree(s->connection->pool, t->sockaddr);
		ngx_pfree(s->connection->pool, t);

	}
}
