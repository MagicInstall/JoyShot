/*
 *  Joy con & Switch pro
 * 
 *  非线程安全
 * 
 *  2021-08-16  wing    创建.
 */

#ifndef NS_CONTROLLER_H
#define NS_CONTROLLER_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_bt_defs.h"

#ifndef NS_CONTROLLER_TASK_CORE_ID
#define NS_CONTROLLER_TASK_CORE_ID      0
#endif
#ifndef NS_CONTROLLER_TASK_PRIORITY
#define NS_CONTROLLER_TASK_PRIORITY     2
#endif

typedef enum 
{
    Left_Joycon     = 1,
    Right_Joycon    = 2,
    Switch_pro      = 3,
} NS_Controller_Type_t;

typedef enum
{
    Battery_Empty   = 0b000,
    Battery_Level_1 = 0b001,
    Battery_Level_2 = 0b010,
    Battery_Level_3 = 0b011,
    Battery_Full    = 0b100,
} NS_Battery_Level_t;

/// 按键结构
typedef struct {
    union {
        struct 
        {
            bool Y          : 1;
            bool X          : 1;
            bool B          : 1;
            bool A          : 1;
            bool Right_SR   : 1;
            bool Right_SL   : 1;
            bool R          : 1;
            bool ZR         : 1;
        };
        uint8_t button_status_1;
    };
    union {
        struct 
        {
            bool Minus  : 1;
            bool Plus   : 1;
            bool RStick : 1;
            bool LStick : 1;
            bool Home   : 1;
            bool Capture: 1;
            bool        : 1; // Reserve
            bool        : 1; // Charging Grip ?
        };
        uint8_t button_status_2;
    };
    union {
        struct 
        {
            bool Down       : 1;
            bool Up         : 1;
            bool Right      : 1;
            bool Left       : 1;
            bool Left_SR    : 1;
            bool Left_SL    : 1;
            bool L          : 1;
            bool ZL         : 1;
        };
        uint8_t button_status_3;
    };        
} ns_button_status_t;

/// 摇杆结构
typedef struct 
{
    union 
    {
        uint8_t stick_data_1;
    };
    
    union 
    {
        uint8_t stick_data_2;
    };
    
    union 
    {
        uint8_t stick_data_3;
    };
    
} ns_analog_stick_t;

typedef enum {
    /** 使用NS_Scan() 匹配到设备(通常已经连接上), 
     *  通过保存该地址, 可在以后直接使用NS_Open() 重连设备,
     *  无需再次扫描.
     */
    NS_CONTROLLER_SCANNED_DEVICE_EVT = 0,

    NS_CONTROLLER_CONNECTED_EVT,
    NS_CONTROLLER_DISCONNECTED_EVT,


} NS_CONTROLLER_EVT;

typedef union
{
    struct
    {
        esp_bd_addr_t   addr;        // 远程设备的地址
    } Scanned_device;
    
} NS_CONTROLLER_EVT_ARG_t;


/**
 *          使用NS_Scan() 匹配到设备(通常已经连接上), 
 *          通过保存该地址, 可使用.
 * @param   arg:    事件参数.
 */
typedef void (*NS_CONTROLLER_CALLBACK)(NS_CONTROLLER_EVT event, NS_CONTROLLER_EVT_ARG_t *arg);



/*
 *
 */
esp_err_t NS_Controller_init(NS_Controller_Type_t joy_type, NS_CONTROLLER_CALLBACK cb);

esp_err_t NS_Scan(void);
esp_err_t NS_Open(esp_bd_addr_t bd_addr);

void NS_Set_Battery(NS_Battery_Level_t level);
void NS_Set_Charging(bool charging);
void NS_Set_Buttons(ns_button_status_t *status);

#endif
