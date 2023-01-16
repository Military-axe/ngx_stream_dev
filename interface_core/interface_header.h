#include <stdint.h>

typedef struct client_server_info_struct
{
	uint32_t src_addr; /** 来源ip */
	uint32_t dst_addr; /** 目的ip */
	uint16_t src_port; /** 来源端口 */
	uint16_t dst_port; /** 目的端口 */
} cs_info_t;

typedef struct transport_struct
{
	cs_info_t *sockaddr; /** ip端口,4元组 */
	uint8_t protocol;	 /** 协议: TCP/UDP */
	uint8_t *data;		 /** 数据 */
	uint16_t data_len;	 /** 数据长度 */
} tran_t;

typedef struct
{
	char *name;
	uint8_t on_off;
	int (*module_interface)(tran_t *);
} modules_switch;