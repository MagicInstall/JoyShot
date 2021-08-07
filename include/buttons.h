/*
    按键

    2021-08-07  wing    创建.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#ifndef BUTTON_DEFAULT_PRIORITY
#define BUTTON_DEFAULT_PRIORITY     10
#endif

#ifndef BUTTON_DEFAULT_CODE_ID      
#define BUTTON_DEFAULT_CODE_ID      1
#endif

#ifndef BUTTON_DEFAULT_DEBOUNCE           
#define BUTTON_DEFAULT_DEBOUNCE     1  // ms
#endif

#ifndef BUTTON_COUNT_MAX
#define BUTTON_COUNT_MAX            2
#endif

typedef enum { 
    // Button_Event_Click           = 1,        // 单击
    // Button_Event_LongPress       = 1 << 1,   // 长按
    // Button_Event_SuperLongPress  = 1 << 2    // 超长按
    Button_Event_Off    = 0,
    Button_Event_On     = 1,
} Button_Event_Mode;

/// 按键事件回调
///
/// \param val      表示长短按
typedef void (*Button_Event_Callback) (Button_Event_Mode val);

/**
 * 初始化按键配置
 */
typedef struct 
{
    gpio_num_t              gpio_num;       // 引脚号
    gpio_config_t           gpio_config;    // 引脚配置
    Button_Event_Mode       click_mode;     // 捕捉哪些事件
    Button_Event_Callback   callback;       // 事件回调
    UBaseType_t             priority;       // 回调线程的优先级
    const BaseType_t        core_id;        // 0 or 1, 回调线程在哪个核创建
} Button_Config_t;

/** 
 * @brief       载入默认按键配置,
 *              该配置使用内部上拉, 下降沿触发.
 * 
 * @param[in]   gpio_num - GPIO_NUM_x.
 * @param[in]   cb - 按键事件回调.
 */
#define Button_Default_Config(num, cb) {                    \
    .gpio_num = num,                                        \
    .gpio_config = {                                        \
        .pin_bit_mask   = (1ULL << num),                    \
        .intr_type              = GPIO_INTR_NEGEDGE,        \
        .mode                   = GPIO_MODE_INPUT,          \
        .pull_up_en             = GPIO_PULLUP_ENABLE,       \
    },                                                      \
    .callback   = cb,                                       \
    .priority   = BUTTON_DEFAULT_PRIORITY,                  \
    .core_id    = BUTTON_DEFAULT_CODE_ID,                   \
}

typedef void* Button_Handle_t;

/**
* @brief        启动按键
* @param[in]    config - 先用Button_IO_Default_Config() 载入默认配置
* @return       - NULL : 按键启动失败;
*               - !NULL : 有效的handle.
*/
Button_Handle_t Button_Enable(Button_Config_t *config);

#define Button_Disable_ALL 0xFFFFFFFF

/**
 * @brief   撤销按键
 * @param[in]   handle - 若要撤销指定的按键, 需要保留在启动按键Button_Enable()时返回的handle, 撤销时传入handle 以指定撤销哪个按键;
 *                     - 若要撤销全部按键, 传入Button_Disable_ALL.
 */
void Button_Disable(Button_Handle_t handle);

/**
 *      在启用按键后可以切换事件回调
 * @param[in]   handle - 要切换回调的按键的handle.
 * @param[in]   cb     - 回调函数
 */ 
void Button_Set_Callback(Button_Handle_t handle, Button_Event_Callback cb);

#endif
