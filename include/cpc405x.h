/**
 *  CPC405x 电源管理驱动
 * 
 *  该电源管理IC的部分LDO虽然支持阶梯调压，
 *  但本驱动目前只实现最大输出，
 *  不使用调压。
 * 
 *  2022-08-18    wing    创建.
 *  2022-08-18    wing    目前只实现了CPC4051, 其余型号有待更新。
 */

#ifndef CPC405X_H
#define CPC405X_H

#include "driver/ledc.h"

/**
 *  载入默认LEDC配置
 * 
 * @param[in]   io      GPIO_NUM_x
 * @param[in]   ldo     CPC405x_LDO_x
 */
#define CPC405x_LDO_Default_Config(io, ldo) {       \
    .output_io_num      = io,                       \
    .ldo_num            = ldo,                      \
    .speed_mode         = LEDC_LOW_SPEED_MODE,      \
    .timer_num          = LEDC_TIMER_3,             \
    .channel            = LEDC_CHANNEL_7,           \
    .clk_cfg            = LEDC_AUTO_CLK,            \
    .freq_hz            = (1000),                   \
    .duty_resolution    = LEDC_TIMER_13_BIT         \
}

// 可调电压的LDO通道号
typedef enum {
    CPC405x_LDO_None    = 0,
    CPC405x_LDO_1       = 1,
    CPC405x_LDO_2,
    CPC405x_LDO_Max
} CPC405x_LDO_num_t;

typedef struct {
    /// 除了34及以上的只读Pin, 其它都可以
    gpio_num_t      output_io_num;

    /// LDO 通道号
    CPC405x_LDO_num_t ldo_num; 

    /// LEDC 使用高速或低速通道
    ledc_mode_t     speed_mode;

    /// LEDC 高速和低速各有4个定时器(0 - 3)
    ledc_timer_t    timer_num;

    /// LEDC 输出通道
    ledc_channel_t  channel;

    /// 使用哪个时钟源；只有LEDC_USE_RTC8M_CLK 可以在休眠的时候继续运行；可以简单使用LEDC_AUTO_CLK。
    ledc_clk_cfg_t  clk_cfg;

    /// PWM 信号的频率
    uint32_t        freq_hz;

    /// 占空比分辨率位宽
    ledc_timer_bit_t duty_resolution;
} CPC405x_LDO_Config_t;

typedef struct {
    // LDO 配置
    CPC405x_LDO_Config_t ldo[2];

    /// 充电状态读取引脚
    gpio_num_t      status_io_num;

    /// 电池电量ADC引脚
    gpio_num_t      adc_io_num;


} CPC405x_Config_t;


/**
 *  初始化一个可调LDO通道
 * 
 *  本驱动只实现通道开关，
 *  不使用阶梯调压。
 * 
 * @param config
 * @return 返回ESP_OK 表示初始化完成。
 */
esp_err_t CPC405x_LDO_Init(CPC405x_LDO_Config_t *config);

/**
 *  开启可调LDO 供电输出
 * 
 *  该方法非线程安全，多线程必须手动协调！
 * 
 * @param   delay   CPC405x需要多个脉冲逐级抬升输出电压，
 *                  该参数会让本方法在开启LEDC脉冲输出后，
 *                  让线程暂停若干毫秒才返回。
 *                  实测在LEDC频率为1000hz的时候，
 *                  需要约8ms 将输出电压抬升到VCC（3.3v)；
 *                  可直接使用CPC405X_LDO_UP_DELAY 宏。
 * 
 * @param   num     要开启的LDO通道号
 * 
 * @return 返回ESP_OK 表示电压输出完成。
 */
esp_err_t CPC405x_LDO_On(uint32_t delay, CPC405x_LDO_num_t num);

/**
 *  关闭可调LDO 供电输出
 * 
 *  该方法非线程安全，多线程必须手动协调！
 * 
 * @param   num 要关闭的LDO通道号
 * 
 * @return 返回ESP_OK 表示已让CPC405x停止供电；
 *         CPC405x不会立即断电，而是在5ms后才停止供电（
 *         加上大电容储能大约需要100ms才降到0v）。
 */
esp_err_t CPC405x_LDO_Off(CPC405x_LDO_num_t num);


/**
 *  LEDC 占空比分辨率最大位宽计算工具
 * 
 *  官方文档没有很明确给出所有范围，
 *  此方法是参照IDF 源码内容制作，
 *  便于确定duty_resolution 可用的最大位宽。
 * 
 * @param config 除duty_resolution成员外，
 *               cpc405x下其余的成员先设置好期待值后再调用本方法传入。
 * @return 若返回0 则表示config 中的设置没有可用的占空比分辨率，
 *         需尝试降低频率(freq_hz) 或更换时钟源(clk_cfg)。
 */
ledc_timer_bit_t LEDC_Duty_Resolution_Tool(CPC405x_LDO_Config_t *config);

#endif
