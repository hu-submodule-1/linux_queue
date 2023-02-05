/**
 * @file      : queue.h
 * @brief     : Linux平台队列驱动头文件
 * @author    : huenrong (huenrong1028@outlook.com)
 * @date      : 2023-01-18 13:59:21
 *
 * @copyright : Copyright (c) 2023 huenrong
 *
 * @history   : date       author          description
 *              2023-01-18 huenrong        创建文件
 *
 */

#ifndef __QUEUE_H
#define __QUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// 循环队列结构体
typedef struct
{
    uint8_t *data;               // 指向缓冲区的指针
    uint32_t head;               // 队列头指针(指向队列头元素)
    uint32_t tail;               // 队列尾指针(指向队列尾元素的下一个位置)
    uint32_t total_size;         // 队列缓冲区的总大小
    uint32_t current_size;       // 队列当前大小
    pthread_mutex_t queue_mutex; // 队列互斥锁
    pthread_cond_t queue_cond;   // 队列条件变量
} queue_t;

/**
 * @brief  初始化循环队列
 * @param  queue_name: 输出参数, 队列名
 * @param  queue_size: 输入参数, 队列缓冲区的总大小
 * @return true : 成功
 * @return false: 失败
 */
bool queue_init(queue_t *queue_name, const uint32_t queue_size);

/**
 * @brief  清空队列
 * @param  queue_name: 输出参数, 队列名
 * @return true : 成功
 * @return false: 失败
 */
bool queue_clear(queue_t *queue_name);

/**
 * @brief  获取队列当前元素个数
 * @param  queue_name: 输入参数, 队列名
 * @return 队列当前元素个数
 */
uint32_t queue_get_current_size(queue_t queue_name);

/**
 * @brief  写入数据到循环队列
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输入参数, 待插入数据
 * @param  data_len  : 输入参数, 待插入数据长度
 * @return 成功: 实际插入个数
 *         失败: -1
 */
int queue_put_data(queue_t *queue_name, const uint8_t *data, const uint32_t data_len);

/**
 * @brief  阻塞方式从循环队列中获取数据
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输出参数, 获取到的数据
 * @param  data_len  : 输入参数, 指定获取长度
 * @return 成功: 实际获取个数
 *         失败: -1
 */
int queue_get_data(queue_t *queue_name, uint8_t *data, const uint32_t data_len);

/**
 * @brief  超时方式从循环队列中获取数据(超时时间为0, 直接从队列获取数据)
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输出参数, 获取到的数据
 * @param  data_len  : 输入参数, 指定获取长度
 * @param  timeout   : 输入参数, 超时时间(单位: ms)
 * @return 成功: 实际获取个数
 *         失败: -1
 */
int queue_get_data_with_timeout(queue_t *queue_name, uint8_t *data, const uint32_t data_len, const uint32_t timeout);

/**
 * @brief  判断循环队列是否为空
 * @param  queue_name: 输入参数, 队列名
 * @return true : 队列为空
 * @return false: 队列非空
 */
bool queue_is_empty(const queue_t queue_name);

/**
 * @brief  销毁队列
 * @param  queue_name: 输出参数, 队列名
 * @return true : 成功
 * @return false: 失败
 */
bool queue_destroy(queue_t *queue_name);

#ifdef __cplusplus
}
#endif

#endif // __QUEUE_H
