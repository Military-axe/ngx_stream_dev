/*
 * @Author: 张大炮 mi1itray.axe@gmail.com
 * @Date: 2022-12-12 02:45:23
 * @LastEditors: 张大炮 mi1itray.axe@gmail.com
 * @LastEditTime: 2022-12-13 05:37:45
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

#define TCP 0x01
#define UDP 0x02

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

#define new_cs_info_t(r) (cs_info_t *)ngx_palloc(r->pool, sizeof(cs_info_t))
#define new_tran_t(r) (tran_t *)ngx_palloc(r->pool,sizeof(tran_t))

void ip2str(uint32_t ip, char *s)
{
	/**
	 * 函数效果：将ip转化成可视ip地址
	 * 参数：ip -- uint32_t的一个ip值
	 *        s -- char* 指针指向一个空字符数组，用于存储转换结果
	 */
	sprintf(s, "%d.%d.%d.%d\x00", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, ip >> 24);
}

int log_info(ngx_stream_session_t *s, tran_t *info)
{
	/**
	 * 函数效果：将数据信息存储格式化打印到my.log文件中，存储格式如下
	 *     [时间]-[来源ip端口]-[目的ip端口]-[协议TCP/UDP]
	 *     [数据] 16进制存储
	 * 参数: s -- ngx_stream_session_t* 指针 
	 *      info -- tran_t* 类型存储结构
	 */
	ngx_pool_cleanup_t *cln;
	ngx_pool_cleanup_file_t *clfn;
	ngx_file_t *file;
	ngx_connection_t *r;
	pid_t pid;

	r = s->connection;
	file = (ngx_file_t *)ngx_palloc(r->pool, sizeof(ngx_file_t));
	if (file == NULL)
	{
		return NGX_ERROR;
	}
	/* format time */
	time_t curtime;
	struct tm *ltime;
	static char nowtime[20];
	time(&curtime);
	ltime = localtime(&curtime);
	strftime(nowtime, 20, "%Y-%m-%d %H:%M:%S", ltime);

	/* nginx file struct */
	char realfilename[20] = {0};
	sprintf(realfilename,"./runtime/logs/my_%d.log",getpid());
	file->fd = ngx_open_file(realfilename, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_APPEND, 777);
	if (file->fd == -1)
	{
		ngx_log_error(NGX_LOG_ERR, r->log, 0, "ngx_stream_dumpdata_module.h:77 file->fd = -1;");
		ngx_log_error(NGX_LOG_ERR, r->log, 0, "open file error code: %d",errno);
		return NGX_ERROR;
	}

	file->name.len = ngx_strlen(realfilename);
	file->name.data = (u_char *)realfilename;
	file->log = r->pool->log;
	cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
	if (cln == NULL)
	{
		return NGX_ERROR;
	}
	clfn = (ngx_pool_cleanup_file_t *)cln->data; // 此处要特别注意，让clfn指针指向上面ngx_pool_cleanup_add函数内分配的存放ngx_pool_cleanup_file_t的空间。
	clfn->fd = file->fd;
	clfn->name = (u_char *)realfilename;
	clfn->log = r->pool->log;
	cln->handler = ngx_pool_cleanup_file;

	// char *ip_addr = (char *)malloc(sizeof(char) * 4);
	// char *ip_add2 = (char *)malloc(sizeof(char) * 4);
	char *ip_addr = (char *)ngx_palloc(r->pool, sizeof(char) * 16);
	char *ip_add2 = (char *)ngx_palloc(r->pool, sizeof(char) * 16);
	char temp[80] = {0};
	ip2str(info->sockaddr->src_addr, ip_addr);
	ip2str(info->sockaddr->dst_addr, ip_add2);
	sprintf(temp, "[%s] %s:%d --> %s:%d", nowtime, ip_addr, info->sockaddr->src_port, ip_add2, info->sockaddr->dst_port);

	write(clfn->fd, temp, strlen(temp));
	if (info->protocol == TCP)
	{
		write(clfn->fd, " TCP", 4);
	}
	else if (info->protocol == UDP)
	{
		write(clfn->fd, " UDP", 4);
	}
	else
	{
		write(clfn->fd, " Unknow Protocal", 16);
	}
	/* output the data */
	/**
	 * 格式化数据如下
	 * [2022-12-30 04:18:26] 192.168.191.1:10159 --> 0.0.0.0:8070 TCP
			| 66 6c 61 67 7b 68 65 6c 6c 6f 77 6f 72 6c 64 7d |		| f l a g { h e l l o w o r l d } |
			| 0a 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |		| . . . . . . . . . . . . . . . . |
	   [2022-12-30 04:18:31] 192.168.191.1:10159 --> 0.0.0.0:8070 TCP
			| 79 65 70 3f 0a 00 00 00 00 00 00 00 00 00 00 00 |		| y e p ? . . . . . . . . . . . . |
	 */

	for (uint16_t i = 0; i < info->data_len; i += 16)
	{
		/* hex data */
		sprintf(temp, "\n\t|");
		write(clfn->fd, temp, 3);
		for (uint16_t j = i; j < i + 16; j++)
		{
			if (j >= info->data_len)
			{
				write(clfn->fd, " 00", 3);
				continue;
			}
			sprintf(temp, " %02x", info->data[j]);
			write(clfn->fd, temp, 3);
		}
		write(clfn->fd, " |", 2);
		/* ascii data */
		write(clfn->fd, "\t\t|", 3);
		for (uint16_t j = i; j < i + 16; j++)
		{
			if (j >= info->data_len)
			{
				write(clfn->fd, " .", 2);
				continue;
			}
			if (info->data[j] > 0x20 && info->data[j] < 0xff)
			{
				sprintf(temp, " %c", info->data[j]);
				write(clfn->fd, temp, 2);
			}
			else
			{
				write(clfn->fd, " .", 2);
			}
		}
		write(clfn->fd, " |", 2);
	}
	write(clfn->fd, "\n", 1);
	ngx_close_file(clfn->fd);
}

void get_data_from_nginx(ngx_stream_session_t *s, ngx_chain_t *in, ngx_uint_t from_upstream, tran_t *t)
{
	/**
	 * 函数效果: 从nginx中的结构体ngx_stream_session_t和ngx_chain_t中读取需要的数据
	 *          存储到tran_t* 和cs_info_t* 指针中。需要的数据有ip端口协议5元组
	 * 		    (来源ip,来源端口,目的ip,目的端口,协议类型)，还要读取传输数据与数据长度。
	 * 			来源ip端口从 s->connection->sockaddr提取
	 * 			目标ip端口从 s->connection->local_sockaddr提取
	 * 参数: s --  ngx_stream_session_t*
	 *      in -- ngx_chain_t*
	 * 		from_upstream -- 用于判断那个是来源那个是目的
	 *      t  -- tran_t*
	 */

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
	tran_t *t = new_tran_t(s->connection);
	t->sockaddr = new_cs_info_t(s->connection);
	get_data_from_nginx(s, in, from_upstream, t);
	log_info(s, t);
}
