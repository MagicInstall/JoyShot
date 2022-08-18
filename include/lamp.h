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
#include "freertos/semphr.h"

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
        gpio_num_t      output_io_num;
 
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
    } cpc405x;
    
} Lamp_Config_t;

// /**
//  * 色彩校正结构体
//  */
// typedef struct
// {
//     float b, g, r;
// } Lamp_Correction_t ;


/**
 * 灯带控制初始化
 * @param config 
 * @return 返回ESP_OK 表示完成初始化。
 */
esp_err_t Lamp_Init(Lamp_Config_t *config);

// /**
//  * 设置颜色校正值
//  * @param corr 若传入的某个分量的值大于1.0，则强制为1.0 。
//  */
// void Lamp_Set_Correction(Lamp_Correction_t *corr);

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
 * @param color 这里只使用第一个元素，其余led全部复制此颜色。
 * @return 返回ESP_OK 表示设置完成。
 */
esp_err_t Lamp_Set_Single_Color(WS2812_COLOR_t *color);


/**
 * 动态光效帧的计数模式
 */
typedef enum {
    LAMP_EFFECT_FRAME_MODE_FORWARD = 1,     // 递增：索引正向计数到最后一帧后，返回第一帧。
    LAMP_EFFECT_FRAME_MODE_TO_AND_FRO,      // 往复：索引正向计数到最后一帧后，再反向计数回到第一帧。
    LAMP_EFFECT_FRAME_MODE_SINGLE,          // 单次：到最后一帧后，灯光固定在最后一帧不再变化。frames 的最后建议添加一个值为LAMP_EFFECT_HOLDER_FRAME 的元素，以减少向ws2812输出波形，节约电量。
} Lamp_Effect_Frame_Mode_t;

/**
 * LAMP_EFFECT_STATE_MODE_PAUSE_ADD_ELAPSED 模式下播放的方向
 */
typedef enum {
    LAMP_EFFECT_FRAME_DIRECTION_FORWARD = 1,    // 正向
    LAMP_EFFECT_FRAME_DIRECTION_REVERSE = -1,   // 反向
} Lamp_Effect_Frame_Direction_t;

/**
 *  在进入Lamp_Effect_Start() 时从哪里开始
 */
typedef enum {
    LAMP_EFFECT_STATE_MODE_RESTART,             // 重新开始
    LAMP_EFFECT_STATE_MODE_PAUSE,               // 从之前暂停的帧开始
    LAMP_EFFECT_STATE_MODE_PAUSE_ADD_ELAPSED,   // 从之前暂停的位置再加上暂停后经过的时间来计算现在开始的帧
} Lamp_Effect_Start_Mode_t;

/**
 *  Lamp_Effect_t 结构体的frames 数组每个元素表示该帧图像的首地址偏移，
 *  某些帧有时不需要刷新（仍然显示上一帧的图像），
 *  这时可把该元素的值设置为LAMP_EFFECT_HOLDER_FRAME 表示这帧不需要
 *  向ws2812 发送波形。
 */
#define LAMP_EFFECT_HOLDER_FRAME    0xFFFFFFFF

/**
 *  动态光效数据对象结构体
 * 
 *  光效的动态主要由帧构成，
 *  frames 数组每个元素就是一每帧图像的首地址偏移，
 *  每帧的图像（每个ws2812可理解为像素）储存在table 中，
 *  光效线程以freq的频率逐帧刷新，
 *  按照mode 指定的模式，
 *  逐帧从frames 找到table 中对应的图像地址，
 *  交给ws2812驱动输出图像。
 * 
 *  注意：
 *      该对象虽然有一个互斥锁成员，
 *      但仍需要用户手动取锁和还锁来保证线程安全。
 *            
 */
typedef struct
{
    struct
    {
        uint16_t                    freq : 16;  // 刷新率(hz)
        bool                        clipping:1; // 是否启用裁剪模式   
        bool                        virginity:1;// 配合pauseTime成员使用，防止对象第一次使用时出现计算错误，一般初始化为true。
    };
    Lamp_Effect_Frame_Mode_t        frameMode;  // 帧计数模式
    Lamp_Effect_Frame_Direction_t   direction;  // 往复模式的当前方向
    uint32_t                        current;    // 当前播放帧
    uint32_t                        a;          // 裁剪帧开头，不能大于b。
    uint32_t                        b;          // 裁剪帧末尾，不能小于a。
    Lamp_Effect_Start_Mode_t        startMode;  // 进入Lamp_Effect_Start()时从哪帧开始
    TickType_t                      pauseTime;  // 暂停时的系统Tick
    uint32_t                       *frames;     // 每帧的图像在table 中的偏移量
    uint32_t                        length;     // frame的长度
    WS2812_COLOR_t                 *table;      // 每帧颜色(图像)索引表
    SemaphoreHandle_t               semaphore;  // 数据锁，初始化时若不分配锁对象，则必须设置为NULL（NULL的话Lamp_Effect_Start() 会给对象创建锁）；用户修改此对象时必须先xSemaphoreTake()取锁，修改完毕必须xSemaphoreGive()还锁。
} Lamp_Effect_t;

/**
 *  启动动态光效
 * 
 *  光效的制作可参照Lamp_Effect_t 结构体的说明。
 * 
 * @param effect    注意：
 *                      若之前用此方法启动某个光效对象，
 *                      中途改变对象的freq成员，是不会更新刷新率的，
 *                      要变更正在运行的光效的刷新率，
 *                      只能改变freq后再Lamp_Effect_Start()一次。
 * @return 返回ESP_OK 表示动态光效启动。
 */
esp_err_t Lamp_Effect_Start(Lamp_Effect_t *effect);

/**
 *  停止动态光效
 * @return 返回ESP_OK 表示动态光效停止。
 */
esp_err_t Lamp_Effect_Stop(void);

/**
 * 封装WS2812_Fill_Buffer()方法，
 * 在Lamp_Loop_Start()开启自动刷新后，
 * 向缓冲区填充数据。
 * 
 * @param color
 * @return 返回ESP_OK 表示填充完成。
 */
// esp_err_t Lamp_Fill_Buffer(WS2812_COLOR_t *color);

/**
 *  LEDC 占空比分辨率最大位宽计算工具
 * @param config 除duty_resolution成员外，
 *               cpc405x下其余的成员先设置好期待值后再调用本方法传入。
 * @return 若返回0 则表示
 */
ledc_timer_bit_t Lamp_LEDC_Duty_Resolution_Tool(Lamp_Config_t *config);

#endif
