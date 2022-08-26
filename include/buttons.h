/*
    按键

    2021-08-07  wing    创建.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#ifndef BUTTON_TASK_PRIORITY
#define BUTTON_TASK_PRIORITY        10
#endif

#ifndef BUTTON_TASK_CODE_ID      
#define BUTTON_TASK_CODE_ID         0
#endif

#ifndef BUTTON_DEBOUNCE        
// 消抖的时间   
#define BUTTON_DEBOUNCE             1000  /* us */
#endif

#ifndef BUTTON_COUNT_MAX
#define BUTTON_COUNT_MAX            2  /* 驱动允许的最大按键数量 */
#endif

typedef enum { 
    Button_On       = 0,
    Button_Off      = 1,
} Button_Level_t;


/**
 *  按键触发事件类型
 * 
 *  按键事件的触发顺序：
 *      按下按键 -> 
 *      Button_Event_On_Prepare -> 
 *      Button_Event_On_Task -> 
 *      松开按键 -> 
 *      Button_Event_Off_Prepare -> 
 *      Button_Event_Off_Task。
 */
typedef enum { 
    // 松开按键时，驱动会在单独的线程触发此事件，
    // 可用于执行需时较长的脚本。
    Button_Event_Off_Task       = 0,

    // 按下按键时，驱动会在单独的线程触发此事件，
    // 可用于执行需时较长的脚本。
    Button_Event_On_Task        = 1,

    // 在Button_Event_Off_Task 事件触发前会先触发此事件，
    // 主要用于控制Button_Event_Off_Task 阶段是否中止或路过。
    //
    // 此事件回调返回时：
    // 1. 若要让上一个Button_Event_Off_Task 中仍未结束的脚本即时中止，
    // 则需要将Param 的abort_task 成员设置为true；
    // 反之而若 abort_task 设置为false，并且上一个Button_Event_Off_Task 仍未结束，
    // 则会跳过一次Button_Event_Off_Task 事件。
    //
    // 2. 若要跳过一次Button_Event_Off_Task 事件，
    // 可设置skip_task 为true，
    // 建议若不需要Button_Event_Off_Task 事件则跳过它以减少线程切换压力。
    Button_Event_Off_Prepare    = 0x10,

    // 在Button_Event_On_Task 事件触发前会先触发此事件，
    // 主要用于控制Button_Event_On_Task 阶段是否中止或路过。
    // 
    // 此事件回调返回时的细则可参照Button_Event_Off_Prepare 的注释。
    Button_Event_On_Prepare     = 0x11,
} Button_Callback_Event_t;

/**
 *  按键事件回调的输入输出参数
 */
typedef struct {
    Button_Callback_Event_t event;

    union {
        struct {

            // 与上一个Button_Event_Off_Prepare 事件之间的时间差，单位ms。
            uint32_t        ms_passed;

            struct {
                // 输出参数：
                // 若设置为true，则跳过Button_Event_On_Task 事件。
                bool        skip_task   :1 ;

                // 输出参数：
                // 若设置为true，则中止上一个Button_Event_On_Task 事件中仍未结束的脚本。
                bool        abort_task  :1 ;
            };
        } button_on_prepare;

        struct {

            // 与上一个Button_Event_Off_Prepare 事件之间的时间差，单位ms。
            uint32_t        ms_passed;
            
        } button_on_task;

        struct {

            // 与Button_Event_On_Prepare 事件之间的时间差，单位ms。
            uint32_t        ms_passed;

            struct {
                // 输出参数：
                // 若设置为true，则跳过Button_Event_Off_Task 事件。
                bool        skip_task   :1 ;

                // 输出参数：
                // 若设置为true，则中止上一个Button_Event_Off_Task 事件中仍未结束的脚本。
                bool        abort_task  :1 ;
            };
        } button_off_prepare;

        struct {

            // 与Button_Event_On_Prepare 事件之间的时间差，单位ms。
            uint32_t        ms_passed;

        } button_off_task;
    };    
} Button_Callback_Param_t;

/**
 *  按键事件回调 
 *      
 * @param param
 */
typedef void (*Button_Callback) (Button_Callback_Param_t *param);

/**
 * 初始化按键配置
 */
typedef struct 
{
    gpio_num_t              gpio_num;       // 引脚号
    gpio_config_t           gpio_config;    // 引脚配置
    // Button_Callback_Event_t       click_mode;     // 捕捉哪些事件
    Button_Callback   callback;       // 事件回调
    // UBaseType_t             priority;       // 回调线程的优先级
    // const BaseType_t        core_id;        // 0 or 1, 回调线程在哪个核创建
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
        .pin_bit_mask           = (1ULL << (num)),          \
        .intr_type              = GPIO_INTR_NEGEDGE,        \
        .mode                   = GPIO_MODE_INPUT,          \
        .pull_up_en             = GPIO_PULLUP_ENABLE,       \
    },                                                      \
    .callback   = cb,                                       \
/*    .priority   = BUTTON_TASK_PRIORITY,                     \
    .core_id    = BUTTON_TASK_CODE_ID,                      \
*/ \
}

typedef void* Button_Handle_t;

/**
* @brief        启动按键
* @param[in]    config - 先用Button_IO_Default_Config() 载入默认配置
* @return       NULL  - : 按键启动失败;
*               other - : 有效的handle.
*/
Button_Handle_t Button_Enable(Button_Config_t *config);

#define Button_Disable_ALL 0xFFFFFFFF

/**
 * @brief   撤销按键
 *              撤销后该按键的handle 变成无效, 不能再利用,
 *              执行本函数后应将handle = NULL.
 * @param[in]   handle - 若要撤销指定的按键, 需要保留在启动按键Button_Enable()时返回的handle, 撤销时传入handle 以指定撤销哪个按键;
 *                     - 若要撤销全部按键, 传入Button_Disable_ALL.
 */
void Button_Disable(Button_Handle_t handle);

/**
 *  在启用按键后可以切换事件回调
 * @param[in]   handle - 要切换回调的按键的handle.
 * @param[in]   cb     - 回调函数
 * @return      ESP_OK - success, other - failed 
 */ 
esp_err_t Button_Set_Callback(Button_Handle_t handle, Button_Callback cb);

/**
 * 不启动按键检测线程，只读取按键电位
 * 
 * @param gpio_num 按键的IO
 * @return Button_Level_t
 */
Button_Level_t Button_Click(gpio_num_t gpio_num);

#endif
