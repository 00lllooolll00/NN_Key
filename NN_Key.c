/**
 * @file NN_Key.c
 * @brief 多功能按键处理模块实现
 * @details 实现了按键的消抖、长按、连按功能
 *          支持多种按键事件检测和回调处理机制
 *          还支持组合按键功能的处理
 * @author N1ntyNine99
 * @date 2025-04-27
 * @version 1.0.0
 * @copyright Copyright (c) 2025
 */

#include "NN_Key.h"

/* ========================= 全局变量定义 ========================= */
static nn_key_t *_nn_key_list[KEY_MAX_KEY_NUMBER]; // 按键列表
static uint8_t _nn_key_num = 0; //按键数量

static nn_comb_t *_nn_combo_list[KEY_MAX_COMBO_NUMBER]; //组合键列表
static uint8_t _nn_combo_num = 0; //组合键数量

/* ========================= 内部函数声明 ========================= */
static bool _NN_Key_Event(nn_key_t *key, uint32_t tick);
static void _NN_Key_StateMachine(nn_key_t *key, uint32_t tick);
static void _NN_Combo_Process(uint32_t tick);

/* ========================= 基础按键函数实现 ========================= */
/**
 * @brief 初始化按键
 * @param key 按键指针
 * @param name 按键名称
 * @param pfunc 按键读取函数
 * @return 初始化是否成功
 * @note 此函数会设置按键的默认参数和状态
 */
bool NN_Key_Init(nn_key_t *key, const char *name, nn_key_read_t pfunc)
{
    if (key == NULL) return false;

    // 按键基础信息
    key->key_id = name; // 按键ID
    key->key_read = pfunc; // 读取按键函数
    key->key_last_time = 0; // 按键上一次事件时间

    // 初始化参数
    key->key_paras.debounce_time = KEY_DEBOUNCE_TIME; // 消抖时间
    key->key_paras.long_time = KEY_LONG_PRESS_TIME; // 长按时间
    key->key_paras.long_alws_time = KEY_LONG_PRESS_ALWS; // 持续长按时间
    key->key_paras.multi_time = KEY_MULTI_PRESS_TIME; // 连按时间

    // 初始化标志位
    key->key_flags.state = KEY_STATE_INIT; // 初始状态
    key->key_flags.event = KEY_EVENT_INIT; // 初始事件
    key->key_flags.lock_flag = false; // 锁定标志，用于组合键处理
    key->key_flags.is_member = false; // 是否为组合键成员

    //初始化多击相关
    key->key_multi_paras.multi_max = 4; // 最大连按次数
    key->key_multi_paras.multi_count = 0; // 连按计数

    // 初始化回调掩码和回调数组
    key->callback_mask = 0;

    // 初始化所有回调函数指针和用户数据
    for (uint8_t i = 0; i < KEY_EVENT_MAX; i++)
    {
        key->callbacks[i].func.callback_key = NULL;
        key->callbacks[i].user_data = NULL;
    }

    return true;
}

/**
 * @brief 添加按键到管理列表
 * @param key 按键指针
 * @param id 按键ID
 * @param read_func 按键读取函数
 * @return 添加是否成功
 * @note 此函数会初始化按键并添加到全局管理列表
 */
bool NN_Key_Add(nn_key_t *key, const char *id, nn_key_read_t read_func)
{
    // 参数检查
    if (key == NULL || read_func == NULL || _nn_key_num >= KEY_MAX_KEY_NUMBER) return false;

    // 初始化按键
    if (!NN_Key_Init(key, id, read_func)) return false;

    // 添加到按键列表
    _nn_key_list[_nn_key_num++] = key;

    return true;
}

/**
 * @brief 设置按键参数
 * @param key 按键指针
 * @param debounce_time 消抖时间(ms)
 * @param long_time 长按时间(ms)
 * @param long_alws_time 持续长按时间(ms)
 * @param multi_time 连按间隔时间(ms)
 * @param multi_max 最大连按次数
 * @return 设置是否成功
 * @note 传入0表示不修改该参数
 */
bool NN_Key_SetPara(nn_key_t *key,
                    uint16_t debounce_time,
                    uint16_t long_time,
                    uint16_t long_alws_time,
                    uint16_t multi_time,
                    uint8_t multi_max)
{
    if (key == NULL) return false;

    // 使用uint16_t，确保不溢出
    if (debounce_time) key->key_paras.debounce_time = debounce_time;
    if (long_time) key->key_paras.long_time = long_time;
    if (long_alws_time) key->key_paras.long_alws_time = long_alws_time;
    if (multi_time) key->key_paras.multi_time = multi_time;
    if (multi_max) key->key_multi_paras.multi_max = (multi_max > 15 ? 15 : multi_max); // 4位位域最大值为15

    return true;
}

/* ========================= 按键回调函数管理 ========================= */
/**
 * @brief 设置按键回调函数
 * @param key 按键指针
 * @param event 事件类型
 * @param cb 回调函数指针
 * @param user_data 用户数据
 * @return 设置是否成功
 * @note 每种事件类型可以设置独立的回调函数
 */
bool NN_Key_SetCb(nn_key_t *key, nn_key_event_t event, nn_key_callback_t cb, void *user_data)
{
    // 参数检查
    if (key == NULL || event >= KEY_EVENT_MAX || cb == NULL) return false;

    // 设置回调和用户数据
    key->callbacks[event].func.callback_key = cb;
    key->callbacks[event].user_data = user_data;

    // 设置有回调标志
    if (cb != NULL)
    {
        key->callback_mask |= (0x01 << event); // 置位对应事件的回调标志位
    }
    else
    {
        key->callback_mask &= ~(0x01 << event); // 清除对应事件的回调标志位
    }

    return true;
}

/**
 * @brief 删除按键回调函数
 * @param key 按键指针
 * @param event 事件类型
 * @return 删除是否成功
 * @note 删除后该事件不会再触发回调
 */
bool NN_Key_DeleteCb(nn_key_t *key, nn_key_event_t event)
{
    // 参数检查
    if (key == NULL || event >= KEY_EVENT_MAX) return false;

    // 删除回调函数
    key->callbacks[event].func.callback_key = NULL;
    key->callbacks[event].user_data = NULL;
    key->callback_mask &= ~(0x01 << event); // 清除对应事件的回调标志位

    return true;
}

/* ========================= 组合按键管理 ========================= */
/**
 * @brief 添加一个组合键
 * @param comb 组合键的结构体指针
 * @param id 组合键名称
 * @param mem_nbr 组合键的成员数量
 * @param member1 组合键的成员1
 * @param member2 组合键的成员2
 * @param ... 组合键的其他成员
 * @return 是否创建成功
 * @note 组合键需要至少两个成员按键，按键在组合键窗口时间内被按下才会触发
 */
bool NN_Combo_Add(nn_comb_t *comb, const char *id, uint8_t mem_nbr, nn_key_t *member1, nn_key_t *member2, ...)
{
    // 参数检查
    if (mem_nbr > KEY_MAX_COMBO_MEMBER || _nn_combo_num > KEY_MAX_COMBO_NUMBER) return false;
    if (comb == NULL || member1 == NULL || member2 == NULL) return false;

    // 初始化组合键基础属性
    comb->combo_id = id;
    comb->combo_mem_first = 0;
    memset(comb->combo_member, 0, sizeof(nn_key_t *) * KEY_MAX_COMBO_MEMBER);
    comb->combo_window = KEY_COMBO_WINDOW;
    comb->combo_member_nbr = mem_nbr;
    comb->combo_value.combo_value_excepted = 0;
    comb->combo_value.combo_value_now = 0;
    comb->combo_trigger = false;

    // 设置期望的组合键值掩码
    for (uint8_t i = 0; i < mem_nbr; i++)
    {
        comb->combo_value.combo_value_excepted |= (0x01 << i);
    }

    // 处理可变参数
    va_list args;
    nn_key_t *temp = NULL;

    va_start(args, member2);

    // 将成员添加到列表
    comb->combo_member[0] = member1;
    comb->combo_member[1] = member2;
    member1->key_flags.is_member = true; // 标记为组合键成员
    member2->key_flags.is_member = true; // 标记为组合键成员

    // 处理剩余成员
    for (uint8_t i = 0; i < mem_nbr - 2; i++)
    {
        temp = va_arg(args, nn_key_t *);
        if (temp != NULL)
        {
            temp->key_flags.is_member = true; // 标记为组合键成员
            comb->combo_member[2 + i] = temp;
        }
    }
    va_end(args);

    // 添加到组合键列表
    _nn_combo_list[_nn_combo_num++] = comb;

    return true;
}

/**
 * @brief 设置组合键的回调函数
 * @param combo 组合键的结构体指针
 * @param cb 回调函数
 * @param para 参数
 * @return 是否添加成功
 * @note 当组合键触发时会调用此回调函数
 */
bool NN_Combo_SetCb(nn_comb_t *combo, nn_comb_callback_t cb, void *para)
{
    // 参数检查
    if (combo == NULL || cb == NULL) return false;

    // 设置回调函数和用户数据
    combo->combo_cb.func.callback_comb = cb;
    combo->combo_cb.user_data = para;

    return true;
}

/**
 * @brief 设置组合键按下的窗口时间
 * @param combo 组合键的结构体指针
 * @param time_ms 窗口时间(ms)
 * @return 是否设置成功
 * @note 窗口时间决定了组合键成员按下的有效时间范围
 */
bool NN_Combo_SetWindowTime(nn_comb_t *combo, uint16_t time_ms)
{
    // 参数检查
    if (combo == NULL || time_ms > UINT16_MAX) return false;

    // 设置窗口时间
    combo->combo_window = time_ms;

    return true;
}

/* ========================= 组合键内部处理函数 ========================= */
/**
 * @brief 组合键处理函数
 * @param tick 当前系统时钟值(ms)
 * @note 内部函数，处理所有组合键的识别和触发
 */
static void _NN_Combo_Process(uint32_t tick)
{
    // 处理所有组合键
    for (uint8_t i = 0; i < _nn_combo_num; i++)
    {
        nn_comb_t *comb = _nn_combo_list[i];
        bool combo_active = false; // 标记组合键是否处于活跃状态

        // 如果组合键已经开始形成，标记其成员为锁定状态
        if (comb->combo_mem_first > 0)
        {
            combo_active = true;
        }

        // 检查所有组合键成员的状态
        for (uint8_t k = 0; k < comb->combo_member_nbr; k++)
        {
            nn_key_t *mem_key = comb->combo_member[k];

            // 只处理处于PRESSED事件的按键
            if (mem_key->key_flags.event != KEY_EVENT_PRESSED) continue;

            if (!comb->combo_mem_first)
            {
                // 如果是第一个按下的成员，记录时间戳
                comb->combo_mem_first = tick;
                comb->combo_value.combo_value_now = (1 << k);
                combo_active = true;
            }
            else if (tick - comb->combo_mem_first <= comb->combo_window)
            {
                // 如果在窗口时间内，更新组合键状态
                comb->combo_value.combo_value_now |= (1 << k);
            }

            // 检查组合键是否已完全匹配
            if (comb->combo_value.combo_value_now == comb->combo_value.combo_value_excepted)
            {
                comb->combo_trigger = true;
                comb->combo_value.combo_value_now = 0;
                comb->combo_mem_first = 0;
            }
        }

        // 如果组合键处于活跃状态，锁定所有成员按键
        if (combo_active)
        {
            for (uint8_t j = 0; j < comb->combo_member_nbr; j++)
            {
                comb->combo_member[j]->key_flags.lock_flag = true; // 设置锁定标志
            }
        }

        // 处理已触发的组合键
        if (comb->combo_trigger == true)
        {
            comb->combo_trigger = false;

            // 重置所有成员按键事件
            for (uint8_t j = 0; j < comb->combo_member_nbr; j++)
            {
                comb->combo_member[j]->key_flags.event = KEY_EVENT_INIT;
                comb->combo_member[j]->key_flags.lock_flag = false; // 解除锁定
            }

            // 触发组合键回调
            if (comb->combo_cb.func.callback_comb)
            {
                comb->combo_cb.func.callback_comb(comb, comb->combo_cb.user_data);
            }
        }

        // 窗口时间超时处理
        if (comb->combo_mem_first && tick - comb->combo_mem_first > comb->combo_window)
        {
            comb->combo_mem_first = 0;
            comb->combo_value.combo_value_now = 0;

            // 解除所有成员锁定
            for (uint8_t j = 0; j < comb->combo_member_nbr; j++)
            {
                comb->combo_member[j]->key_flags.lock_flag = false; // 解除锁定
            }
        }
    }
}

/* ========================= 按键处理主函数 ========================= */
/**
 * @brief 按键处理函数
 * @param tick 当前系统时钟值(ms)
 * @return 处理是否成功
 * @note 此函数需要由主循环周期性调用，用于刷新所有按键状态和处理事件
 *       建议调用频率不低于10ms一次
 */
bool NN_Key_Handler(uint32_t tick)
{
    bool result = true;

    // 首先重置所有组合键成员的锁定状态
    for (uint8_t i = 0; i < _nn_key_num; i++)
    {
        nn_key_t *key = _nn_key_list[i];
        if (key->key_flags.is_member)
        {
            key->key_flags.lock_flag = false; // 重置组合键锁定状态
        }
    }

    // 更新所有按键的状态
    for (uint8_t i = 0; i < _nn_key_num; i++)
    {
        nn_key_t *key = _nn_key_list[i];
        // 运行按键状态机
        _NN_Key_StateMachine(key, tick);
    }

    // 处理组合键
    _NN_Combo_Process(tick);

    // 处理单个按键事件
    for (uint8_t i = 0; i < _nn_key_num; i++)
    {
        nn_key_t *key = _nn_key_list[i];

        // 如果按键被组合键锁定，跳过处理
        if (key->key_flags.lock_flag)
        {
            continue;
        }

        // 处理按键事件
        result &= _NN_Key_Event(key, tick);
    }

    return result;
}

/* ========================= 内部函数实现 ========================= */
/**
 * @brief 处理按键事件并执行对应回调
 * @param key 按键指针
 * @param tick 当前系统时钟值(ms)
 * @return 事件是否成功处理
 * @note 内部函数，处理事件的回调触发
 */
static bool _NN_Key_Event(nn_key_t *key, uint32_t tick)
{
    static uint32_t LastTick = 0; // 静态变量，记录上次长按持续回调的时间

    // 参数检查
    if (key == NULL) return false;

    // 初始化状态不需要处理
    if (key->key_flags.event == KEY_EVENT_INIT) return true;

    nn_key_event_t event = (nn_key_event_t)key->key_flags.event;

    // 检查此事件是否有回调函数
    if ((key->callback_mask & (0x01 << event)) && key->callbacks[event].func.callback_key != NULL)
    {
        // 对于持续长按状态，需要持续触发回调
        if (event == KEY_EVENT_LONG_PRESSED_ALWS)
        {
            // 为长按持续状态，每KEY_LONG_PRESS_ALWS_CB毫秒触发一次回调
            if ((tick - LastTick) >= KEY_LONG_PRESS_ALWS_CB)
            {
                LastTick = tick; // 更新上次触发时间
                key->callbacks[event].func.callback_key(key, event, key->callbacks[event].user_data);
            }
            return true;
        }

        // 调用回调函数
        key->callbacks[event].func.callback_key(key, event, key->callbacks[event].user_data);

        // 非持续性事件触发一次后重置为初始事件，防止重复触发
        if (event != KEY_EVENT_LONG_PRESSED_ALWS)
        {
            key->key_flags.event = KEY_EVENT_INIT;
        }

        return true;
    }

    // 没有回调函数但有事件，也重置为初始状态防止重复处理
    if (key->key_flags.event != KEY_EVENT_LONG_PRESSED_ALWS)
    {
        key->key_flags.event = KEY_EVENT_INIT;
    }

    return true;
}

/**
 * @brief 按键状态机处理函数
 * @param key 按键指针
 * @param tick 当前系统时钟值(ms)
 * @details 该函数实现按键状态转换的核心逻辑，包括:
 *          - 消抖处理
 *          - 短按/长按/持续长按识别
 *          - 多次连击检测
 *          - 各种事件状态的切换与生成
 * @note 内部函数，由NN_Key_Handler调用
 */
static void _NN_Key_StateMachine(nn_key_t *key, uint32_t tick)
{
    uint32_t now_tick = tick; // 当前系统时钟值
    uint32_t diff_tick = now_tick - key->key_last_time; // 计算时间差，用于判断按键状态变化时间
    bool key_val = key->key_read(); // 读取当前按键物理状态（按下为true，释放为false）

    // 按键状态机
    switch (key->key_flags.state)
    {
        case KEY_STATE_INIT:
            // 初始化状态：根据按键初始状态决定下一个状态
            if (key_val)
            {
                // 如果按键被按下，转为PRESSED状态
                key->key_flags.state = KEY_STATE_PRESSED;
                key->key_last_time = now_tick; // 更新时间戳
            }
            else
            {
                // 如果按键未被按下，转为RELEASED状态
                key->key_flags.state = KEY_STATE_RELEASED;
                key->key_last_time = now_tick; // 更新时间戳
                key->key_flags.event = KEY_EVENT_INIT; // 设置初始化事件
            }
            break;

        case KEY_STATE_RELEASED:
            // 释放状态：检测是否有新的按键按下事件
            if (key_val && diff_tick >= key->key_paras.debounce_time)
            {
                // 检测到按键按下且已超过消抖时间，转为按下状态
                key->key_flags.state = KEY_STATE_PRESSED;
                key->key_last_time = now_tick; // 更新时间戳
                key->key_flags.event = KEY_EVENT_INIT; // 重置事件状态
            }
            else if (!key_val)
            {
                // 按键持续保持释放状态
                key->key_flags.state = KEY_STATE_RELEASED;
            }
            break;

        case KEY_STATE_PRESSED:
            if (!key_val)
            {
                // 按键释放
                uint32_t press_duration = now_tick - key->key_last_time;

                // 根据按下持续时间判断是短按还是长按
                if (press_duration >= key->key_paras.long_time)
                {
                    // 按下时间超过长按阈值，判定为长按
                    key->key_flags.event = KEY_EVENT_LONG_PRESSED;
                    key->key_flags.state = KEY_STATE_RELEASED;
                    key->key_last_time = now_tick;
                    key->key_multi_paras.multi_count = 0; // 重置多击计数
                }
                else
                {
                    // 短按，进入多击检测状态
                    key->key_flags.state = KEY_STATE_MULTI_PRESSED;
                    key->key_multi_paras.multi_count++; // 增加点击计数
                    key->key_last_time = now_tick; // 更新时间戳
                }
            }
            else if (diff_tick >= key->key_paras.long_time && diff_tick < key->key_paras.long_alws_time &&
                     key->key_paras.long_alws_time > 0)
            {
                // 按键持续按下超过长按阈值但尚未达到持续长按阈值
                // 这个状态用于检测长按后是否进入持续长按状态
                key->key_flags.state = KEY_STATE_LONG_PRESSED;
            }
            else if (diff_tick >= key->key_paras.long_alws_time && key->key_paras.long_alws_time > 0)
            {
                // 只有在长按持续时间大于0时才进入持续长按状态
                // 按键持续按下超过持续长按阈值时间
                key->key_flags.state = KEY_STATE_LONG_PRESSED_ALWS;
                key->key_flags.event = KEY_EVENT_LONG_PRESSED_ALWS;
                key->key_last_time = now_tick; // 更新时间戳
            }
            break;

        case KEY_STATE_LONG_PRESSED:
            // 长按状态，如果释放则触发长按事件
            if (!key_val)
            {
                key->key_flags.event = KEY_EVENT_LONG_PRESSED;
                key->key_flags.state = KEY_STATE_RELEASED;
                key->key_last_time = now_tick; // 更新时间戳
                key->key_multi_paras.multi_count = 0; // 重置多击计数
            }
            else if (diff_tick >= key->key_paras.long_alws_time && key->key_paras.long_alws_time > 0)
            {
                // 长按状态下继续按住达到持续长按阈值，转入持续长按状态
                key->key_flags.state = KEY_STATE_LONG_PRESSED_ALWS;
                key->key_flags.event = KEY_EVENT_LONG_PRESSED_ALWS;
                key->key_last_time = now_tick; // 更新时间戳
            }
            break;

        case KEY_STATE_LONG_PRESSED_ALWS:
            if (!key_val)
            {
                // 持续长按后按键被释放
                key->key_last_time = now_tick; // 更新时间戳
                key->key_flags.state = KEY_STATE_RELEASED; // 回到释放状态
                key->key_flags.event = KEY_EVENT_INIT; // 重置事件为初始状态，确保不会继续触发
                key->key_multi_paras.multi_count = 0; // 重置多击计数
            }
            else
            {
                // 按键仍然保持按下，持续触发持续长按事件
                key->key_flags.event = KEY_EVENT_LONG_PRESSED_ALWS; // 持续产生长按事件
            }
            break;

        case KEY_STATE_MULTI_PRESSED:
            if (key_val && diff_tick >= key->key_paras.debounce_time)
            {
                // 在多击等待期间检测到新的按下
                key->key_flags.state = KEY_STATE_PRESSED; // 回到按下状态
                key->key_last_time = now_tick; // 更新时间戳
            }
            else if (!key_val && diff_tick >= key->key_paras.multi_time)
            {
                // 超过多击等待时间，多击序列结束

                // 根据累计的点击次数设置对应的事件类型
                if (key->key_multi_paras.multi_count == 1)
                {
                    key->key_flags.event = KEY_EVENT_PRESSED; // 单击
                }
                else if (key->key_multi_paras.multi_count == 2)
                {
                    key->key_flags.event = KEY_EVENT_DOUBLE_PRESSED; // 双击
                }
                else if (key->key_multi_paras.multi_count == 3)
                {
                    key->key_flags.event = KEY_EVENT_TRIPLE_PRESSED; // 三击
                }
                else if (key->key_multi_paras.multi_count > 3)
                {
                    key->key_flags.event = KEY_EVENT_MULTI_PRESSED; // 多击（超过三次）
                }

                key->key_flags.state = KEY_STATE_RELEASED; // 回到释放状态
                key->key_last_time = now_tick; // 更新时间戳
                key->key_multi_paras.multi_count = 0; // 重置多击计数器
            }
            break;

        default:
            // 未知状态处理，重置到初始状态
            key->key_flags.state = KEY_STATE_INIT; // 回到初始状态
            key->key_last_time = now_tick; // 更新时间戳
            key->key_multi_paras.multi_count = 0; // 重置多击计数
            key->key_flags.event = KEY_EVENT_INIT; // 重置事件类型
            break;
    }
}