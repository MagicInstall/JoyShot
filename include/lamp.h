/**
 *  使用WS2812 LED 显示各种信息,
 *  封装了WS2812 驱动以及CPC405x 电源管理。
 * 
 *  2021-09-05    wing    创建.
 */

#ifndef LAMP_H
#define LAMP_H

#include "rmt_ws2812.h"
#include "driver/ledc.h"

#ifndef CPC405x_EN_GPIO_NUM
#error "CPC405x_EN_GPIO_NUM undefine!"
#endif

#ifndef CPC405X_PWM_FREQ
#define CPC405X_PWM_FREQ        1000     
#endif
#ifndef CPC405X_PWM_RESOLUTION
#define CPC405X_PWM_RESOLUTION  LEDC_TIMER_13_BIT
#endif
#ifndef CPC405X_PWM_SPEED_MODE
#define CPC405X_PWM_SPEED_MODE  LEDC_LOW_SPEED_MODE
#endif
#ifndef CPC405X_PWM_TIMER
#define CPC405X_PWM_TIMER       LEDC_TIMER_3
#endif
// #ifndef CPC405X_PWM_CLK_CFG
// #define CPC405X_PWM_CLK_CFG     LEDC_AUTO_CLK
// #endif
#ifndef CPC405X_PWM_CHANNEL
#define CPC405X_PWM_CHANNEL     LEDC_CHANNEL_7
#endif

// 若CPC405X_PWM_FREQ设置为1000(hz)，最少需要8ms 才能上升至VCC(3.3v) ，
// 再要加上WS2812 的启动时间约2ms
#define CPC405X_LDO_UP_DELAY    10

typedef struct 
{
    WS2812_CONFIG_t ws2812;

    struct
    {
        /// 除了34及以上的只读Pin, 其它都可以
        gpio_num_t      Output_IO_Num;
 
    } cpc405x;
    
} Lamp_Config_t;

/**
 * 灯带控制初始化
 * @param config 
 * @return 返回ESP_OK 表示完成初始化。
 */
esp_err_t Lamp_Init(Lamp_Config_t *config);

/**
 * 通过电源管理IC为LED供电
 * 
 * 该方法非线程安全，多线程必须手动协调！
 * 
 * @param delay CPC405x需要多个脉冲逐级抬升输出电压，
 *              该参数会让本方法在开启LEDC脉冲输出后，
 *              让线程暂停若干毫秒才返回。
 *              实测在LEDC频率为1000hz的时候，
 *              需要约8ms 将输出电压抬升到VCC（3.3v)；
 *              可直接使用CPC405X_LDO_UP_DELAY 宏。
 * @return 返回ESP_OK 表示电压输出完成。
 */
esp_err_t Lamp_On(uint32_t delay);

/**
 * 通过电源管理IC断开LED供电
 * 
 * @return 返回ESP_OK 表示已让CPC405x停止供电；
 *         CPC405x不会立即断电，而是在5ms后才停止供电（
 *         加上大电容储能大约需要100ms才降到0v）。
 */
esp_err_t Lamp_Off(void);

/**
 * 让全部ws2812显示同一个颜色
 *  
 * @param color
 * @return 返回ESP_OK 表示设置完成。
 */
esp_err_t Lamp_Set_Color(WS2812_COLOR_t *color);

#endif
