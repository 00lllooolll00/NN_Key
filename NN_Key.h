/**
 * @file NN_Key.h
 * @brief 多功能按键处理模块头文件
 * @details 定义了按键处理相关的数据结构和函数接口
 *          支持按键消抖、长按、连按功能
 *          提供组合按键功能支持
 * @author N1ntyNine99
 * @date 2025-04-27
 * @version 1.0.0
 * @copyright Copyright (c) 2025
 */

#ifndef __NN_Key_H
#define __NN_Key_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

/* ========================= 宏定义 ========================= */
#define KEY_MAX_KEY_NUMBER     20 // 最大按键数量
#define KEY_MAX_COMBO_NUMBER   20 // 最大组合键数量

#define KEY_DEBOUNCE_TIME      20 // 默认消抖时间(ms)
#define KEY_LONG_PRESS_TIME    500 // 默认长按时间(ms)
#define KEY_LONG_PRESS_ALWS    1500 // 默认持续长按时间(ms)
#define KEY_MULTI_PRESS_TIME   300 // 默认连按间隔时间(ms)
#define KEY_LONG_PRESS_ALWS_CB 50 // 一直按住的回调函数处理间隔(ms)
#define KEY_MAX_COMBO_MEMBER   4 // 组合键最多组合成员
#define KEY_COMBO_WINDOW       300 // 组合键窗口时间(ms)

/* ========================= 类型定义声明 ========================= */
typedef struct nn_key_t nn_key_t;
typedef struct nn_comb_t nn_comb_t;

/* ========================= 快捷宏函数 ========================= */
/**
 * 仅设置按键消抖时间
 */
#define NN_Key_SetDebounceTime(key, time) NN_Key_SetPara(key, time, 0, 0, 0, 0)

/**
 * 仅设置按键长按时间
 */
#define NN_Key_SetLongPressTime(key, time) NN_Key_SetPara(key, 0, time, 0, 0, 0)

/**
 * 仅设置按键持续长按时间
 */
#define NN_Key_SetLongPressAlwsTime(key, time) NN_Key_SetPara(key, 0, 0, time, 0, 0)

/**
 * 仅设置按键连按间隔时间
 */
#define NN_Key_SetMultiPressTime(key, time) NN_Key_SetPara(key, 0, 0, 0, time, 0)

/**
 * 仅设置按键最大连按次数
 */
#define NN_Key_SetMultiPressMax(key, max) NN_Key_SetPara(key, 0, 0, 0, 0, max)

/**
 * @brief 简化按键回调函数定义的宏
 * @param func_name 回调函数名称
 * @details 使用此宏可以快速定义一个符合nn_key_callback_t类型的回调函数
 *          例如: NN_KEY_CALLBACK(MyCallback) { // 处理逻辑 }
 */
#define NN_KEY_CALLBACK(func_name) void func_name(nn_key_t *key, nn_key_event_t event, void *user_data)

/**
 * @brief 简化组合键回调函数定义的宏
 * @param func_name 回调函数名称
 * @details 使用此宏可以快速定义一个符合nn_comb_callback_t类型的回调函数
 *          例如: NN_COMB_CALLBACK(MyComboCallback) { // 处理逻辑 }
 */
#define NN_COMB_CALLBACK(func_name) void func_name(nn_comb_t *comb, void *user_data)

/**
 * @brief 快速注册单击事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnClick(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_PRESSED, cb, user_data)

/**
 * @brief 快速注册双击事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnDoubleClick(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_DOUBLE_PRESSED, cb, user_data)

/**
 * @brief 快速注册三击事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnTripleClick(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_TRIPLE_PRESSED, cb, user_data)

/**
 * @brief 快速注册多击事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnMultiClick(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_MULTI_PRESSED, cb, user_data)

/**
 * @brief 快速注册长按事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnLongPress(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_LONG_PRESSED, cb, user_data)

/**
 * @brief 快速注册持续长按事件回调
 * @param key 按键指针
 * @param cb 回调函数
 * @param user_data 用户数据
 */
#define NN_Key_OnContinuousPress(key, cb, user_data) NN_Key_SetCb(key, KEY_EVENT_LONG_PRESSED_ALWS, cb, user_data)

/* ========================= 枚举定义 ========================= */
/**
 * @brief 按键状态枚举
 */
typedef enum
{
    KEY_STATE_INIT = 0, // 初始状态
    KEY_STATE_RELEASED, // 释放状态
    KEY_STATE_PRESSED, // 按下状态
    KEY_STATE_LONG_PRESSED, // 长按状态
    KEY_STATE_LONG_PRESSED_ALWS, // 持续长按状态
    KEY_STATE_MULTI_PRESSED, // 连按状态
} nn_key_state_t;

/**
 * @brief 按键事件枚举
 */
typedef enum
{
    KEY_EVENT_INIT = 0, // 初始事件
    KEY_EVENT_PRESSED, // 按下事件
    KEY_EVENT_LONG_PRESSED, // 长按事件
    KEY_EVENT_LONG_PRESSED_ALWS, // 持续长按事件
    KEY_EVENT_DOUBLE_PRESSED, // 双击事件
    KEY_EVENT_TRIPLE_PRESSED, // 三击事件
    KEY_EVENT_MULTI_PRESSED, // 多击事件
    KEY_EVENT_MAX // 最大事件数
} nn_key_event_t;

/* ========================= 函数定义 ========================= */
/**
 * @brief 按键读取函数类型定义
 * @return 按键是否被按下 (true: 按下, false: 释放)
 */
typedef bool (*nn_key_read_t)(void);

/**
 * @brief 按键回调函数类型定义
 * @param key 触发事件的按键指针
 * @param event 按键事件类型
 * @param user_data 用户数据指针
 */
typedef void (*nn_key_callback_t)(nn_key_t *key, nn_key_event_t event, void *user_data);

/**
 * @brief 组合键回调函数类型定义
 * @param comb 触发事件的组合键指针
 * @param user_data 用户数据指针
 */
typedef void (*nn_comb_callback_t)(nn_comb_t *comb, void *user_data);

/* ========================= 数据结构定义 ========================= */
/**
 * @brief 按键回调函数结构体
 */
typedef struct
{
    union
    {
        nn_key_callback_t callback_key; // 普通按键回调函数指针
        nn_comb_callback_t callback_comb; // 组合键回调函数指针
    } func;
    void *user_data; // 用户数据指针
} nn_key_callback_item_t;

/**
 * @brief 按键数据结构定义
 */
typedef struct nn_key_t
{
    const char *key_id; // 按键标识符
    nn_key_read_t key_read; // 按键读取函数
    uint32_t key_last_time; // 上次处理时间

    struct
    {
        uint16_t debounce_time; // 消抖时间
        uint16_t long_time; // 长按时间阈值
        uint16_t long_alws_time; // 持续长按时间阈值
        uint16_t multi_time; // 连按间隔时间
    } key_paras; // 参数结构体

    struct
    {
        nn_key_state_t state:3; // 当前按键状态 (使用位域)
        nn_key_event_t event:3; // 当前按键事件 (使用位域)
        bool is_member:1; // 是一个组合键的成员
        bool lock_flag:1; // 保留位
    } key_flags; // 标志位结构体

    struct
    {
        uint8_t multi_max:4; // 最大连按次数 (使用位域)
        uint8_t multi_count:4; // 当前连按次数 (使用位域)
    } key_multi_paras; // 多击相关

    // 回调位掩码，每位表示一个事件是否有回调函数
    uint8_t callback_mask;

    // 为每个事件类型分配独立的回调函数和用户数据
    nn_key_callback_item_t callbacks[KEY_EVENT_MAX];
} nn_key_t;

/**
 * @brief 组合键数据结构定义
 */
typedef struct nn_comb_t
{
    const char *combo_id; // 组合键标识符

#if KEY_MAX_COMBO_MEMBER <= 4
    struct
    {
        uint8_t combo_value_excepted:4; // 预期的组合键按下位模式
        uint8_t combo_value_now:4; // 当前组合键按下位状态
    } combo_value;
#endif

#if KEY_MAX_COMBO_MEMBER >= 5
    struct
    {
        uint16_t combo_value_excepted:8; // 预期的组合键按下位模式
        uint16_t combo_value_now:8; // 当前组合键按下位状态
    } combo_value;
#endif

    uint32_t combo_mem_first; // 成员第一次按下的时间
    uint16_t combo_window; // 窗口时间
    uint8_t combo_member_nbr; // 成员数目
    bool combo_trigger; // 是否触发
    nn_key_t *combo_member[KEY_MAX_COMBO_MEMBER]; // 组合键成员指针数组
    nn_key_callback_item_t combo_cb; // 组合键的回调函数
} nn_comb_t;

/* ========================= 函数声明 ========================= */
/* --- 基础按键操作函数 --- */
bool NN_Key_Init(nn_key_t *key, const char *name, nn_key_read_t pfunc);
bool NN_Key_Add(nn_key_t *key, const char *id, nn_key_read_t read_func);
bool NN_Key_SetPara(nn_key_t *key,
                    uint16_t debounce_time,
                    uint16_t long_time,
                    uint16_t long_alws_time,
                    uint16_t multi_time,
                    uint8_t multi_max);
bool NN_Key_Handler(uint32_t tick);

/* --- 按键回调函数管理 --- */
bool NN_Key_SetCb(nn_key_t *key, nn_key_event_t event, nn_key_callback_t cb, void *user_data);
bool NN_Key_DeleteCb(nn_key_t *key, nn_key_event_t event);

/* --- 组合按键管理函数 --- */
bool NN_Combo_Add(nn_comb_t *comb, const char *id, uint8_t mem_nbr, nn_key_t *member1, nn_key_t *member2, ...);
bool NN_Combo_SetCb(nn_comb_t *combo, nn_comb_callback_t cb, void *para);
bool NN_Combo_SetWindowTime(nn_comb_t *combo, uint16_t time_ms);

#endif
