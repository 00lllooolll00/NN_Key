# NN_Key 多功能按键库使用文档

## 目录

- [简介](#简介)
- [特性](#特性)
- [快速开始](#快速开始)
- [API参考](#api参考)
  - [基础按键操作](#基础按键操作)
  - [按键回调函数管理](#按键回调函数管理)
  - [组合按键管理](#组合按键管理)
  - [便捷宏定义](#便捷宏定义)
- [实例示范](#实例示范)
  - [基本按键处理](#基本按键处理)
  - [多击和长按处理](#多击和长按处理)
  - [组合键使用](#组合键使用)
- [注意事项](#注意事项)

## 简介

NN_Key是一个功能丰富的嵌入式按键处理库，专为STM32等单片机系统设计，提供了完整的按键事件检测和处理能力。本库支持按键消抖、单击、双击、三击、多击、长按、持续长按等多种操作方式，并且提供了组合按键功能，满足复杂人机交互的需求。

库的设计遵循模块化思想，易于集成到各种嵌入式项目中，通过简单的回调机制，可以轻松实现各种按键事件的响应，无需编写复杂的状态机逻辑。

## 特性

- **多种按键事件**：支持单击、双击、三击、多击(>3)、长按、持续长按等事件类型
- **消抖处理**：内置消抖算法，避免按键抖动带来的误触发
- **组合按键**：支持多个按键的组合检测，最多支持4个按键组合
- **回调机制**：基于事件的回调函数注册，灵活响应各类按键事件
- **参数可调**：按键的消抖时间、长按时间、连击间隔等参数均可自定义
- **低资源占用**：代码高效精简，适合资源受限的嵌入式系统
- **便捷宏定义**：提供多种宏定义，简化库的使用

## 快速开始

1. 首先将`NN_Key.h`和`NN_Key.c`添加到你的项目中
2. 在项目中包含头文件：`#include "NN_Key.h"`
3. 定义按键读取函数，用于读取按键的物理状态，例如：

```c
bool Button1_Read(void)
{
    // 返回按键的物理状态，按下为true，释放为false
    // 这里根据实际硬件进行修改
    return (HAL_GPIO_ReadPin(BUTTON1_GPIO_Port, BUTTON1_Pin) == GPIO_PIN_RESET);
}
```

4. 创建按键实例并添加到按键管理列表

```c
// 方式一：使用便捷宏创建
nn_key_t *key1;
NN_Key_Create(key1, "KEY1", Button1_Read);

// 方式二：传统方式创建
nn_key_t key2;
NN_Key_Add(&key2, "KEY2", Button2_Read);
```

5. 注册按键事件回调函数

```c
// 定义回调函数
NN_KEY_CALLBACK(OnKey1Click)
{
    printf("按键 %s 被单击\n", key->key_id);
}

// 注册回调
NN_Key_OnClick(key1, OnKey1Click, NULL);
```

6. 在主循环中周期性调用按键处理函数

```c
uint32_t currentTick;
while (1)
{
    currentTick = HAL_GetTick(); // 获取系统时钟
    NN_Key_Handler(currentTick); // 调用按键处理函数
}
```

## API参考

### 基础按键操作

#### NN_Key_Init

```c
bool NN_Key_Init(nn_key_t *key, const char *name, nn_key_read_t pfunc);
```

**功能**：初始化按键结构体

**参数**：

- `key`: 按键结构体指针
- `name`: 按键名称标识符
- `pfunc`: 按键读取函数，返回按键物理状态(true为按下)

**返回值**：初始化是否成功

**示例**：

```c
nn_key_t myKey;
NN_Key_Init(&myKey, "Button1", Button1_Read);
```

#### NN_Key_Add

```c
bool NN_Key_Add(nn_key_t *key, const char *id, nn_key_read_t read_func);
```

**功能**：初始化按键并添加到全局管理列表，推荐使用此函数

**参数**：

- `key`: 按键结构体指针
- `id`: 按键名称标识符
- `read_func`: 按键读取函数

**返回值**：添加是否成功

**示例**：

```c
nn_key_t myKey;
NN_Key_Add(&myKey, "Button1", Button1_Read);
```

#### NN_Key_SetPara

```c
bool NN_Key_SetPara(nn_key_t *key, 
                   uint16_t debounce_time,
                   uint16_t long_time,
                   uint16_t long_alws_time,
                   uint16_t multi_time,
                   uint8_t multi_max);
```

**功能**：设置按键参数

**参数**：

- `key`: 按键结构体指针
- `debounce_time`: 消抖时间(ms)，传入0表示不修改
- `long_time`: 长按时间(ms)，传入0表示不修改
- `long_alws_time`: 持续长按时间(ms)，传入0表示不修改
- `multi_time`: 连按间隔时间(ms)，传入0表示不修改
- `multi_max`: 最大连按次数，传入0表示不修改

**返回值**：设置是否成功

**示例**：

```c
// 设置按键消抖时间为30ms，长按时间为800ms
NN_Key_SetPara(&myKey, 30, 800, 0, 0, 0);
```

#### NN_Key_Handler

```c
bool NN_Key_Handler(uint32_t tick);
```

**功能**：按键处理函数，需要在主循环中周期性调用

**参数**：

- `tick`: 当前系统时钟值(ms)

**返回值**：处理是否成功

**示例**：

```c
// 在主循环中调用
while (1)
{
    uint32_t currentTick = HAL_GetTick();
    NN_Key_Handler(currentTick);
}
```

### 按键回调函数管理

#### NN_Key_SetCb

```c
bool NN_Key_SetCb(nn_key_t *key, nn_key_event_t event, nn_key_callback_t cb, void *user_data);
```

**功能**：设置按键事件回调函数

**参数**：

- `key`: 按键结构体指针
- `event`: 事件类型，可以是以下值之一：
  - `KEY_EVENT_PRESSED`: 单击事件
  - `KEY_EVENT_LONG_PRESSED`: 长按事件
  - `KEY_EVENT_LONG_PRESSED_ALWS`: 持续长按事件
  - `KEY_EVENT_DOUBLE_PRESSED`: 双击事件
  - `KEY_EVENT_TRIPLE_PRESSED`: 三击事件
  - `KEY_EVENT_MULTI_PRESSED`: 多击事件（超过三次）
- `cb`: 回调函数
- `user_data`: 用户数据指针，会传递给回调函数，如果不需要可以传入"NULL"

**返回值**：设置是否成功

**示例**：

```c
// 定义回调函数
void OnButtonClick(nn_key_t *key, nn_key_event_t event, void *user_data)
{
    printf("按键 %s 被点击\n", key->key_id);
}

// 设置单击回调
NN_Key_SetCb(&myKey, KEY_EVENT_PRESSED, OnButtonClick, NULL);
```

#### NN_Key_DeleteCb

```c
bool NN_Key_DeleteCb(nn_key_t *key, nn_key_event_t event);
```

**功能**：删除按键事件回调函数

**参数**：

- `key`: 按键结构体指针
- `event`: 事件类型

**返回值**：删除是否成功

**示例**：

```c
// 删除单击回调
NN_Key_DeleteCb(&myKey, KEY_EVENT_PRESSED);
```

### 组合按键管理

#### NN_Combo_Add

```c
bool NN_Combo_Add(nn_comb_t *comb, const char *id, uint8_t mem_nbr, nn_key_t *member1, nn_key_t *member2, ...);
```

**功能**：添加组合键

**参数**：

- `comb`: 组合键结构体指针
- `id`: 组合键名称标识符
- `mem_nbr`: 组合键成员数量
- `member1`: 第一个成员按键
- `member2`: 第二个成员按键
- `...`: 可变参数，其他成员按键

**返回值**：添加是否成功

**示例**：

```c
// 创建一个两键组合
nn_comb_t myComb;
NN_Combo_Add(&myComb, "Combo1", 2, &key1, &key2);

// 创建一个三键组合
nn_comb_t myComb2;
NN_Combo_Add(&myComb2, "Combo2", 3, &key1, &key2, &key3);
```

#### NN_Combo_SetCb

```c
bool NN_Combo_SetCb(nn_comb_t *combo, nn_comb_callback_t cb, void *para);
```

**功能**：设置组合键回调函数

**参数**：

- `combo`: 组合键结构体指针
- `cb`: 组合键回调函数
- `para`: 用户数据指针

**返回值**：设置是否成功

**示例**：

```c
// 定义组合键回调函数
void OnComboTriggered(nn_comb_t *comb, void *user_data)
{
    printf("组合键 %s 被触发\n", comb->combo_id);
}

// 设置回调
NN_Combo_SetCb(&myComb, OnComboTriggered, NULL);
```

#### NN_Combo_SetWindowTime

```c
bool NN_Combo_SetWindowTime(nn_comb_t *combo, uint16_t time_ms);
```

**功能**：设置组合键窗口时间，即所有成员按键必须在这个时间内被按下才触发组合键

**参数**：

- `combo`: 组合键结构体指针
- `time_ms`: 窗口时间(ms)

**返回值**：设置是否成功

**示例**：

```c
// 设置组合键窗口时间为500ms
NN_Combo_SetWindowTime(&myComb, 500);
```

### 便捷宏定义

库提供了多种便捷宏定义，用于简化按键库的使用：

#### 参数设置宏

```c
// 仅设置按键消抖时间
NN_Key_SetDebounceTime(key, time)

// 仅设置按键长按时间
NN_Key_SetLongPressTime(key, time)

// 仅设置按键持续长按时间
NN_Key_SetLongPressAlwsTime(key, time)

// 仅设置按键连按间隔时间
NN_Key_SetMultiPressTime(key, time)

// 仅设置按键最大连按次数
NN_Key_SetMultiPressMax(key, max)
```

#### 回调函数定义宏

```c
// 简化按键回调函数定义
NN_KEY_CALLBACK(func_name) { /* 处理逻辑 */ }

// 简化组合键回调函数定义
NN_COMB_CALLBACK(func_name) { /* 处理逻辑 */ }
```

#### 事件注册宏

```c
// 快速注册单击事件回调
NN_Key_OnClick(key, cb, user_data)

// 快速注册双击事件回调
NN_Key_OnDoubleClick(key, cb, user_data)

// 快速注册三击事件回调
NN_Key_OnTripleClick(key, cb, user_data)

// 快速注册多击事件回调
NN_Key_OnMultiClick(key, cb, user_data)

// 快速注册长按事件回调
NN_Key_OnLongPress(key, cb, user_data)

// 快速注册持续长按事件回调
NN_Key_OnContinuousPress(key, cb, user_data)
```

#### 实例创建宏

```c
// 创建并初始化按键的简化宏
NN_Key_Create(key_ptr, key_name, read_func)

// 创建并初始化两键组合的简化宏
NN_Combo_Create2(combo_ptr, combo_name, key1, key2)

// 创建并初始化三键组合的简化宏
NN_Combo_Create3(combo_ptr, combo_name, key1, key2, key3)
```

## 实例示范

### 基本按键处理

```c
#include "NN_Key.h"

// 按键读取函数
bool Button1_Read(void)
{
    return (HAL_GPIO_ReadPin(BUTTON1_GPIO_Port, BUTTON1_Pin) == GPIO_PIN_RESET);
}

// 单击回调函数
NN_KEY_CALLBACK(OnButton1Click)
{
    printf("按键 %s 被单击\n", key->key_id);
    // 可以通过user_data访问用户数据
    if (user_data) {
        int *count = (int*)user_data;
        (*count)++;
        printf("点击次数: %d\n", *count);
    }
}

int main(void)
{
    // 系统初始化代码省略...
    
    // 创建按键
    nn_key_t *button1;
    static int clickCount = 0;
    
    // 使用宏创建按键
    NN_Key_Create(button1, "Button1", Button1_Read);
    
    // 设置消抖时间
    NN_Key_SetDebounceTime(button1, 30);
    
    // 注册单击回调
    NN_Key_OnClick(button1, OnButton1Click, &clickCount);
    
    // 主循环
    while (1)
    {
        uint32_t currentTick = HAL_GetTick();
        NN_Key_Handler(currentTick);
    }
}
```

### 多击和长按处理

```c
#include "NN_Key.h"

// 按键读取函数
bool Button1_Read(void) { /* 读取实现 */ }

// 各种回调函数
NN_KEY_CALLBACK(OnSingleClick) { printf("单击\n"); }
NN_KEY_CALLBACK(OnDoubleClick) { printf("双击\n"); }
NN_KEY_CALLBACK(OnTripleClick) { printf("三击\n"); }
NN_KEY_CALLBACK(OnLongPress) { printf("长按\n"); }
NN_KEY_CALLBACK(OnContinuousPress) { printf("持续长按中...\n"); }

int main(void)
{
    // 创建按键
    nn_key_t *button;
    NN_Key_Create(button, "MultiButton", Button1_Read);
    
    // 设置按键参数
    NN_Key_SetPara(button, 20, 800, 2000, 300, 5);
    
    // 注册各类事件回调
    NN_Key_OnClick(button, OnSingleClick, NULL);
    NN_Key_OnDoubleClick(button, OnDoubleClick, NULL);
    NN_Key_OnTripleClick(button, OnTripleClick, NULL);
    NN_Key_OnLongPress(button, OnLongPress, NULL);
    NN_Key_OnContinuousPress(button, OnContinuousPress, NULL);
    
    // 主循环
    while (1)
    {
        NN_Key_Handler(HAL_GetTick());
        HAL_Delay(10);
    }
}
```

### 组合键使用

```c
#include "NN_Key.h"

// 按键读取函数
bool Key1_Read(void) { /* 实现 */ }
bool Key2_Read(void) { /* 实现 */ }
bool Key3_Read(void) { /* 实现 */ }

// 组合键回调
NN_COMB_CALLBACK(OnCombo12Triggered)
{
    printf("按键组合 %s 被触发\n", comb->combo_id);
}

// 三键组合回调
NN_COMB_CALLBACK(OnCombo123Triggered)
{
    printf("三键组合 %s 被触发\n", comb->combo_id);
}

int main(void)
{
    // 创建三个按键
    nn_key_t *key1, *key2, *key3;
    NN_Key_Create(key1, "Key1", Key1_Read);
    NN_Key_Create(key2, "Key2", Key2_Read);
    NN_Key_Create(key3, "Key3", Key3_Read);
    
    // 创建两键组合
    nn_comb_t *combo12;
    NN_Combo_Create2(combo12, "Combo12", key1, key2);
    NN_Combo_SetCb(combo12, OnCombo12Triggered, NULL);
    
    // 创建三键组合
    nn_comb_t *combo123;
    NN_Combo_Create3(combo123, "Combo123", key1, key2, key3);
    NN_Combo_SetCb(combo123, OnCombo123Triggered, NULL);
    
    // 设置组合键窗口时间
    NN_Combo_SetWindowTime(combo12, 400);  // 400ms窗口
    NN_Combo_SetWindowTime(combo123, 600); // 600ms窗口
    
    // 主循环
    while (1)
    {
        NN_Key_Handler(HAL_GetTick());
        HAL_Delay(10);
    }
}
```

## 注意事项

2. **回调函数执行时间**：回调函数中应避免长时间执行或阻塞操作，以防影响按键库的正常工作。

2. **按键读取函数**：确保按键读取函数返回正确的物理状态（按下为true，释放为false）。如果硬件接口的逻辑与此相反，需要在读取函数中进行逻辑翻转。

3. **组合键限制**：组合键成员最多支持4个按键，且这些按键必须在窗口时间内被按下才能触发组合键事件。

4. **资源使用**：库内部维护了按键和组合键的全局列表，默认支持最多20个按键和20个组合键，可通过修改头文件中的宏定义进行调整。

5. **线程安全性**：本库设计用于单线程环境，如在多线程环境下使用，需考虑线程同步问题。
