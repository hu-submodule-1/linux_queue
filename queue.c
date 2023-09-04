/**
 * @file      : queue.c
 * @brief     : Linux平台队列驱动源文件
 * @author    : huenrong (huenrong1028@outlook.com)
 * @date      : 2023-01-18 14:02:03
 *
 * @copyright : Copyright (c) 2023 huenrong
 *
 * @history   : date       author          description
 *              2023-01-18 huenrong        创建文件
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "./queue.h"

/**
 * @brief  初始化循环队列
 * @param  queue_name: 输出参数, 队列名
 * @param  queue_size: 输入参数, 队列缓冲区的总大小
 * @return true : 成功
 * @return false: 失败
 */
bool queue_init(queue_t *queue_name, const uint32_t queue_size)
{
    if ((!queue_name) || (!queue_size))
    {
        return false;
    }

    // 计算需要分配的内存空间
    // 申请时, 需要多加一个间隔元素
    uint32_t len = (queue_size * sizeof(uint8_t) + 1);

    // 分配内存空间
    queue_name->data = (uint8_t *)malloc(len);
    if (!queue_name->data)
    {
        return false;
    }

    queue_name->head = queue_name->tail = 0;
    queue_name->total_size = len;
    queue_name->current_size = 0;

    // 初始化互斥锁
    pthread_mutex_init(&queue_name->queue_mutex, NULL);

    // 初始化条件变量
    pthread_cond_init(&queue_name->queue_cond, NULL);

    return true;
}

/**
 * @brief  清空队列
 * @param  queue_name: 输出参数, 队列名
 * @return true : 成功
 * @return false: 失败
 */
bool queue_clear(queue_t *queue_name)
{
    if (!queue_name)
    {
        return false;
    }

    pthread_mutex_lock(&queue_name->queue_mutex);

    queue_name->head = queue_name->tail = 0;
    queue_name->current_size = 0;

    pthread_mutex_unlock(&queue_name->queue_mutex);

    return true;
}

/**
 * @brief  获取队列当前元素个数
 * @param  queue_name: 输入参数, 队列名
 * @return 队列当前元素个数
 */
uint32_t queue_get_current_size(queue_t queue_name)
{
    return queue_name.current_size;
}

/**
 * @brief  写入数据到循环队列
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输入参数, 待插入数据
 * @param  data_len  : 输入参数, 待插入数据长度
 * @return 成功: 实际插入个数
 *         失败: -1
 */
int queue_put_data(queue_t *queue_name, const uint8_t *data, const uint32_t data_len)
{
    // 实际插入个数
    uint32_t put_num = 0;

    if ((!queue_name) || (!data) || (!data_len))
    {
        return -1;
    }

    pthread_mutex_lock(&queue_name->queue_mutex);

    // 数据循环插入队列, 并修改队尾指针
    for (put_num = 0; put_num < data_len; put_num++)
    {
        // 每次插入数据前, 先判断队列是否已满
        if ((queue_name->tail + 1) % queue_name->total_size == queue_name->head)
        {
            pthread_cond_signal(&queue_name->queue_cond);

            pthread_mutex_unlock(&queue_name->queue_mutex);

            return put_num;
        }

        // 数据插入队列
        queue_name->data[queue_name->tail] = data[put_num];

        // 修改队尾指针
        queue_name->tail = ((queue_name->tail + 1) % queue_name->total_size);

        // 元素个数增加
        queue_name->current_size++;
    }

    pthread_cond_signal(&queue_name->queue_cond);

    pthread_mutex_unlock(&queue_name->queue_mutex);

    return put_num;
}

/**
 * @brief  阻塞方式从循环队列中获取数据
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输出参数, 获取到的数据
 * @param  data_len  : 输入参数, 指定获取长度
 * @return 成功: 实际获取个数
 *         失败: -1
 */
int queue_get_data(queue_t *queue_name, uint8_t *data, const uint32_t data_len)
{
    // 实际获取个数
    uint32_t get_num = 0;

    if ((!queue_name) || (!data) || (!data_len))
    {
        return -1;
    }

    // 没有数据才超时等待信号
    // 使用while而不使用if, 防止该线程进入睡眠时, 被其他信号打断, 而过早的退出睡眠
    while (0 == queue_get_current_size(*queue_name))
    {
        pthread_mutex_lock(&queue_name->queue_mutex);

        pthread_cond_wait(&queue_name->queue_cond, &queue_name->queue_mutex);

        pthread_mutex_unlock(&queue_name->queue_mutex);
    }

    pthread_mutex_lock(&queue_name->queue_mutex);

    for (get_num = 0; get_num < data_len; get_num++)
    {
        // 队列为空, 直接退出
        if (queue_name->head == queue_name->tail)
        {
            pthread_mutex_unlock(&queue_name->queue_mutex);

            return get_num;
        }

        // 取队列头数据
        data[get_num] = queue_name->data[queue_name->head];

        // 修改队头指针
        queue_name->head = ((queue_name->head + 1) % queue_name->total_size);

        // 元素个数减小
        queue_name->current_size--;
    }

    pthread_mutex_unlock(&queue_name->queue_mutex);

    return get_num;
}

/**
 * @brief  超时方式从循环队列中获取数据(超时时间为0, 直接从队列获取数据)
 * @param  queue_name: 输出参数, 队列名
 * @param  data      : 输出参数, 获取到的数据
 * @param  data_len  : 输入参数, 指定获取长度
 * @param  timeout   : 输入参数, 超时时间(单位: ms)
 * @return 成功: 实际获取个数
 *         失败: -1
 */
int queue_get_data_with_timeout(queue_t *queue_name, uint8_t *data, const uint32_t data_len, const uint32_t timeout)
{
    // 实际获取个数
    uint32_t get_num = 0;

    if ((!queue_name) || (!data) || (!data_len))
    {
        return -1;
    }

    if (timeout > 0)
    {
        // 等待信号的开始时间
        struct timespec start_time = {0};
        clock_gettime(CLOCK_REALTIME, &start_time);

        // 等待信号的结束时间
        struct timespec end_time = {0};
        end_time.tv_sec = (start_time.tv_sec + (timeout / 1000));
        end_time.tv_nsec = ((start_time.tv_nsec + ((timeout % 1000) * 1000000)));

        // tv_nsec必须小于1S
        if (end_time.tv_nsec >= 1000000000)
        {
            end_time.tv_sec++;
            end_time.tv_nsec -= 1000000000;
        }

        // 没有数据才超时等待信号
        // 使用while而不使用if, 防止该线程进入睡眠时, 被其他信号打断, 而过早的退出睡眠
        while (0 == queue_get_current_size(*queue_name))
        {
            pthread_mutex_lock(&queue_name->queue_mutex);

            // 超时方式等待一个信号
            int ret = pthread_cond_timedwait(&queue_name->queue_cond, &queue_name->queue_mutex, &end_time);

            // 等待过程中被信号中断, 继续等待
            if (EINTR == ret)
            {
                pthread_mutex_unlock(&queue_name->queue_mutex);

                continue;
            }
            // 超时或其它错误, 直接退出, 防止一直等待
            else
            {
                pthread_mutex_unlock(&queue_name->queue_mutex);

                return -1;
            }
        }
    }

    pthread_mutex_lock(&queue_name->queue_mutex);

    for (get_num = 0; get_num < data_len; get_num++)
    {
        // 队列为空, 直接退出
        if (queue_name->head == queue_name->tail)
        {
            pthread_mutex_unlock(&queue_name->queue_mutex);

            return get_num;
        }

        // 取队列头数据
        data[get_num] = queue_name->data[queue_name->head];

        // 修改队头指针
        queue_name->head = ((queue_name->head + 1) % queue_name->total_size);

        // 元素个数减小
        queue_name->current_size--;
    }

    pthread_mutex_unlock(&queue_name->queue_mutex);

    return get_num;
}

/**
 * @brief  判断循环队列是否为空
 * @param  queue_name: 输入参数, 队列名
 * @return true : 队列为空
 * @return false: 队列非空
 */
bool queue_is_empty(const queue_t queue_name)
{
    return ((!queue_name.current_size) ? true : false);
}

/**
 * @brief  销毁队列
 * @param  queue_name: 输出参数, 队列名
 * @return true : 成功
 * @return false: 失败
 */
bool queue_destroy(queue_t *queue_name)
{
    int ret = -1;

    if (!queue_name)
    {
        return false;
    }

    free(queue_name->data);

    ret = pthread_mutex_destroy(&queue_name->queue_mutex);
    if (0 != ret)
    {
        return false;
    }

    ret = pthread_cond_destroy(&queue_name->queue_cond);
    if (0 != ret)
    {
        return false;
    }

    queue_name->head = queue_name->tail = 0;

    queue_name->current_size = 0;

    queue_name->total_size = 0;

    return true;
}
