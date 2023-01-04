
# ngx_stream_dumpdata_module

此模块需要修改nginx源代码，在nginx_stream_write_filter函数中插入此模块的接口

# Install

复制此文件到`nginx/src/core/stream`中

nginx版本是`(branches/stable-1.22)`

修改`nginx/src/core/stream/ngx_stream_write_filter_module.c`文件中的`ngx_stream_write_filter`函数

修改如下

添加头文件
![](.README.assest/2.png)
在`ngx_stream_write_filter`中插入test测试是否能编译成功
![](.README.assest/1.png)

# TODO

- [ ] 写一个stream模块来存储第三方接口配置信息读取
- [x] ~~如何将此模块编译进入nginx~~
- [ ] 需要一个循环体来存储第三方接口函数，并执行
- [ ] 多进程下，log_info获取文件fd失败