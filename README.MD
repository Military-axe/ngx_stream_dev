
# ngx_stream_interface

此模块可以使得在nginx stream模块下反向代理，可以自定义模块来处理传输中的数据。

原本stream模块下的自定义模块存在一个问题，对于单次请求都可以用，但是对于downstream和upstream建立连接后，后续数据传输则不会经过stream中自定义的模块。

配置本模块后，对tcp握手这些不做处理，只处理tcp/udp等传输层中包体的数据

# Install

下载nginx源码，使本项目和nginx同目录，文件结构如下

```shell
$ tree -L 1
.
├── nginx/
└── stream_modules_interface/
```

使用git将目录下的`patch`文件patch到`branch/stable-1.22`版本mginx中

```shell
git apply --check patch
git apply patch
```

编译nginx

```sh
# 进入nginx项目目录中
cd ./nginx/
# 调用nginx中自定义的config生成Makefile
./auto/configure \
# 各种需要的模块
# ...
--with-stream \
--with-stream_ssl_module
--../stream_modules_interface/ngx_stream_interface_module
# 编译
make
```
编译成功后可执行文件路径在`./nginx/objs/nginx`,可以添加`make install`命令在上述指令后直接安装nginx

# modules

模块接口编写需要包含头文件`interface_header.h`

结构如下

```shell
$ tree
.
├── Library
│   ├── liblog.so
│   └── libreplace.so
├── log_module
│   ├── interface_header.h
│   └── log_module.c
└── replace_module
    ├── interface_header.h
    └── replace_module.cpp
```

代码需要设置一个接口函数，接口函数需要导出

```shell
$ nm -D liblog.so
...
00000000000012cd T log_module_interface
...
```
接口声明如下，形参是`tran_t *`和`module_argv_t *`，这两个数据类型包含在`interface_header.h`中，详细说明见项目中文件

```c
int log_module_interface(tran_t *t, module_argv_t *argv_t);
```

编译好自定义模块后，需要在`nginx.conf`中配置一行

```nginx
modules [模块接口函数名] [so文件路径] [on/off] [模块参数]
```
模块参数是可选配置，其他参数必须要有。模块参数数量最多不超过4个。

# TODO

- [x] ~~写一个stream模块来存储第三方接口配置信息读取~~
- [x] ~~如何将此模块编译进入nginx~~
- [x] ~~需要一个循环体来存储第三方接口函数，并执行~~
- [x] ~~多进程下，log_info获取文件fd失败~~
- [ ] 接口返回值对应不同的状态码，状态码的设计以及处理