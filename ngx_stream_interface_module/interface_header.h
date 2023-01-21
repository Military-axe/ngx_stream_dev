#include <stdint.h>
#define MODULE_FILE_ERR       0x00 /// 模块文件操作失败
#define MODULE_OK             0x01 /// 模块执行成功
#define MODULE_ERR            0x02 /// 模块执行失败
#define MODULE_EXCEPTION_END  0x03 /// 模块异常退出

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

typedef struct {
    uint8_t       len;
    unsigned char *data;
} str_t;

typedef struct {
	int argv_number;
	str_t *elts;
} module_argv_t;

typedef struct
{
    char *interface_name;
    module_argv_t *argv;
    int (*module_interface)(tran_t *, module_argv_t *);
} modules_switch;