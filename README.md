## linux_queue

### 介绍

该仓库代码适用于Linux平台下的循环队列驱动

### 使用说明

- 系统初始化时, 调用`queue_init()`函数, 初始化循环队列
- 生产者线程, 调用`queue_put_data()`函数, 插入数据到队列
- 消费者线程, 调用`queue_get_data()`函数, 阻塞方式从队列中获取数据
- 消费者线程, 调用`queue_get_data_with_timeout()`函数, 超时方式从队列中获取数据
- 调用`queue_get_current_size()`函数, 获取队列中元素个数
- 调用`queue_is_empty()`函数, 判断队列是否为空
- 具体使用方式参考[示例代码](https://github.com/hu-submodule-demo/linux_queue_demo)
