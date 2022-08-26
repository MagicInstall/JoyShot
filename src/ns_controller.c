/*
 *  Joy con & Switch pro
 * 
 *  非线程安全
 * 
 *  2021-08-16  wing    创建.
 */

#include "ns_controller.h"

#include <string.h>
#include "esp_log.h"
#include "esp_hidd_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "nvs.h"

#include "esp_gap_bt_api.h"
#include "freertos/semphr.h"

#define TAG "NS CONTROLLER"
#define SUB_DATA_MAX    45  // 要令report_data_t 结构对齐?

typedef struct 
{
    uint8_t             timer;          // x00 - xFF
    struct 
    {
        uint8_t         Connection_info : 4; // x0, x1, xE
        bool            Charging        : 1; // 充电中
        uint8_t         battery         : 3; // 电量(NS_Battery_Level_t) 8=full, 6=medium, 4=low, 2=critical, 0=empty.
    };
    ns_button_status_t  buttons;
    ns_analog_stick_t   left_stick;
    ns_analog_stick_t   rigth_stick;
    uint8_t             vibration;      // x70, xC0, xB0
    uint8_t             ask;            // x00, x80, x90, x82
} standard_data_t;

/// report 的开头standard 部分的内容是不断变化的,
/// 大部分reply 都需要更新这部分的内容,
/// 通过_send() 函数更新reply 的standard 部分.
standard_data_t _standard_buff;

typedef struct 
{
    uint8_t     sub_command;
    union
    {
        struct
        {
            uint16_t    firmware_version;
            uint8_t     controller_type;
            uint8_t     always_02;
            uint8_t     mac[6];
            uint8_t     always_01;
            uint8_t     color;              // 0x01 JC SPI; 0x02 Pro SPI
        }       reply_02;
        uint8_t sub_data[SUB_DATA_MAX];
    }; 
} sub_command_t;

// typedef struct 
// {
//     standard_data_t ;
//     sub_command_t;
// } report_buff_t;


static esp_hidd_callbacks_t callbacks;
static SemaphoreHandle_t xSemaphore;
static bool connected = false;
static int paired = 0;
static TaskHandle_t SendingHandle = NULL;
static NS_CONTROLLER_CALLBACK _callback = NULL;

uint8_t hid_descriptor[] = 
{
    0x05, 0x01,                             // Usage Page (Generic Desktop) 
    0x15, 0x00,                             // Logical Minimum (0) 
    0x09, 0x04,                             // Usage (Joystick) 
    0xA1, 0x01,                             // Collection (Application) 

        0x85, 0x30,                         // Report ID (48)   
        0x05, 0x01,                         // Usage Page (Generic Desktop)  
        0x05, 0x09,                         // Usage Page (Button)  
        0x19, 0x01,                         // Usage Minimum (Button 1)   
        0x29, 0x0A,                         // Usage Maximum (Button 10) 
        0x15, 0x00,                         // Logical Minimum (0)  
        0x25, 0x01,                         // Logical Maximum (1)   
        0x75, 0x01,                         // Report Size (1)   
        0x95, 0x0A,                         // Report Count (10)
        0x55, 0x00,                         // Unit Exponent (0)   
        0x65, 0x00,                         // Unit (None)   
        0x81, 0x02,                         // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)   

        0x05, 0x09,                         // Usage Page (Button) 
        0x19, 0x0B,                         // Usage Minimum (Button 11)   
        0x29, 0x0E,                         // Usage Maximum (Button 14)  
        0x15, 0x00,                         // Logical Minimum (0)   
        0x25, 0x01,                         // Logical Maximum (1)   
        0x75, 0x01,                         // Report Size (1) 
        0x95, 0x04,                         // Report Count (4)   
        0x81, 0x02,                         // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)  

        0x75, 0x01,                         // Report Size (1)   
        0x95, 0x02,                         // Report Count (2) 
        0x81, 0x03,                         // Input (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

        0x0B, 0x01, 0x00,0x01, 0x00,        // Usage (Generic Desktop:Pointer) 
        0xA1, 0x00,                         // Collection (Physical)   
            0x0B, 0x30, 0x00, 0x01, 0x00,   // Usage (Generic Desktop:X)   
            0x0B, 0x31, 0x00, 0x01, 0x00,   // Usage (Generic Desktop:Y)
            0x0B, 0x32, 0x00, 0x01, 0x00,   // Usage (Generic Desktop:Z) 
            0x0B, 0x35, 0x00, 0x01, 0x00,   // Usage (Generic Desktop:Rz) 
            0x15, 0x00,                     // Logical Minimum (0)  
            0x27, 0xFF, 0xFF, 0x00, 0x00,   // Logical Maximum (65535)  
            0x75, 0x10,                     // Report Size (16) 
            0x95, 0x04,                     // Report Count (4) 
            0x81, 0x02,                     // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit) 
        0xC0,                               // End Collection

        0x0B, 0x39, 0x00, 0x01, 0x00,       // Usage (Generic Desktop:Hat Switch)   
        0x15, 0x00,                         // Logical Minimum (0) 
        0x25, 0x07,                         // Logical Maximum (7)   
        0x35, 0x00,                         // Physical Minimum (0)   
        0x46, 0x3B, 0x01,                   // Physical Maximum (315) 
        0x65, 0x14,                         // Unit (Eng Rot: Degree) 
        0x75, 0x04,                         // Report Size (4)  
        0x95, 0x01,                         // Report Count (1) 
        0x81, 0x02,                         // Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)  

        0x05, 0x09,                         // Usage Page (Button) 
        0x19, 0x0F,// Usage Minimum (Button 15)  
        0x29, 0x12,// Usage Maximum (Button 18) 
        0x15, 0x00,// Logical Minimum (0)  
        0x25, 0x01,// Logical Maximum (1)  
        0x75, 0x01,// Report Size (1)  
        0x95, 0x04,// Report Count (4)  
        0x81, 0x02,// Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

        0x75, 0x08,// Report Size (8) 
        0x95, 0x34,// Report Count (52) 
        0x81, 0x03,// Input (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

        0x06, 0x00, 0xFF,// Usage Page (Vendor-Defined 1) 
        0x85, 0x21,// Report ID (33)
        0x09, 0x01,// Usage (Vendor-Defined 1) 
        0x75, 0x08,// Report Size (8) 
        0x95, 0x3F,// Report Count (63) 
        0x81, 0x03,// Input (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

        0x85, 0x81,// Report ID (129) 
        0x09, 0x02,// Usage (Vendor-Defined 2) 
        0x75, 0x08,// Report Size (8) 
        0x95, 0x3F,// Report Count (63) 
        0x81, 0x03,// Input (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

        0x85, 0x01,// Report ID (1) 
        0x09, 0x03,// Usage (Vendor-Defined 3) 
        0x75, 0x08,// Report Size (8) 
        0x95, 0x3F,// Report Count (63) 
        0x91, 0x83,// Output (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Vol,Bit)

        0x85, 0x10,// Report ID (16)
        0x09, 0x04,// Usage (Vendor-Defined 4) 
        0x75, 0x08,// Report Size (8) 
        0x95, 0x3F,// Report Count (63) 
        0x91, 0x83,// Output (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Vol,Bit) 

        0x85, 0x80,// Report ID (128) 
        0x09, 0x05,// Usage (Vendor-Defined 5) 
        0x75, 0x08,// Report Size (8) 
        0x95, 0x3F,// Report Count (63) 
        0x91, 0x83,// Output (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Vol,Bit)

        0x85, 0x82,// Report ID (130) 
        0x09, 0x06,// Usage (Vendor-Defined 6)   
        0x75, 0x08,// Report Size (8)  
        0x95, 0x3F,// Report Count (63) 
        0x91, 0x83,// Output (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Vol,Bit) 
    0xC0,// End Collection 



    // 0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    // 0x09, 0x05, // Usage (Game Pad)
    // 0xA1, 0x01, // Collection (Application)
    // //Padding
    // 0x95, 0x03, //     REPORT_COUNT = 3
    // 0x75, 0x08, //     REPORT_SIZE = 8
    // 0x81, 0x03, //     INPUT = Cnst,Var,Abs
    // //Sticks
    // 0x09, 0x30,       //   Usage (X)
    // 0x09, 0x31,       //   Usage (Y)
    // 0x09, 0x32,       //   Usage (Z)
    // 0x09, 0x35,       //   Usage (Rz)
    // 0x15, 0x00,       //   Logical Minimum (0)
    // 0x26, 0xFF, 0x00, //   Logical Maximum (255)
    // 0x75, 0x08,       //   Report Size (8)
    // 0x95, 0x04,       //   Report Count (4)
    // 0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // //DPAD
    // 0x09, 0x39,       //   Usage (Hat switch)
    // 0x15, 0x00,       //   Logical Minimum (0)
    // 0x25, 0x07,       //   Logical Maximum (7)
    // 0x35, 0x00,       //   Physical Minimum (0)
    // 0x46, 0x3B, 0x01, //   Physical Maximum (315)
    // 0x65, 0x14,       //   Unit (System: English Rotation, Length: Centimeter)
    // 0x75, 0x04,       //   Report Size (4)
    // 0x95, 0x01,       //   Report Count (1)
    // 0x81, 0x42,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    // //Buttons
    // 0x65, 0x00, //   Unit (None)
    // 0x05, 0x09, //   Usage Page (Button)
    // 0x19, 0x01, //   Usage Minimum (0x01)
    // 0x29, 0x0E, //   Usage Maximum (0x0E)
    // 0x15, 0x00, //   Logical Minimum (0)
    // 0x25, 0x01, //   Logical Maximum (1)
    // 0x75, 0x01, //   Report Size (1)
    // 0x95, 0x0E, //   Report Count (14)
    // 0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // //Padding
    // 0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    // 0x09, 0x20,       //   Usage (0x20)
    // 0x75, 0x06,       //   Report Size (6)
    // 0x95, 0x01,       //   Report Count (1)
    // 0x15, 0x00,       //   Logical Minimum (0)
    // 0x25, 0x7F,       //   Logical Maximum (127)
    // 0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // //Triggers
    // 0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    // 0x09, 0x33,       //   Usage (Rx)
    // 0x09, 0x34,       //   Usage (Ry)
    // 0x15, 0x00,       //   Logical Minimum (0)
    // 0x26, 0xFF, 0x00, //   Logical Maximum (255)
    // 0x75, 0x08,       //   Report Size (8)
    // 0x95, 0x02,       //   Report Count (2)
    // 0x81, 0x02,

    // // 0x06, 0x01, 0xff, 
    // 0x85, 0x01, 
    // 0x09, 0x01, 0x75, 0x08, 0x95, 0x30, 
    // 0x91, 0x02, 
    
    // 0x85, 0x10, 
    // 0x09, 0x10, 0x75, 0x08, 0x95, 0x30, 
    // 0x91, 0x02, 
    
    // 0x85, 0x11, 
    // 0x09, 0x11, 0x75, 0x08, 0x95, 0x30, 
    // 0x91, 0x02, 
    
    // 0x85, 0x12, 
    // 0x09, 0x12, 0x75, 0x08, 0x95, 0x30, 
    // 0x91, 0x02, 
    
    // 0xc0
};
static int hid_descriptor_len = sizeof(hid_descriptor);

//                           | Timer|nibble |     button     |     L stick     |     R stick    | Vib   ACK  subId |
//                           |                                   Report 0x30                                |subId |
static uint8_t reply02[]     = {0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x3C, 0xB8, 0x6F, 0x0B, 0x82, 0x02, 
                          //               | 01 Left Joycon
                          //               | 02 Right Joycon
                          //               | 03 Pro Controller |
                          // |    FW Ver.  | Joy type | 
                                0x04, 0x00, 0x02,            
                          // | always |            MAC ADDRESS           | always
                                0x02,  0xD8, 0xA0, 0x1D, 0x40, 0x15, 0x66, 0x03,
                          // | 手柄颜色来源: 0=default黑; 1=Body + button; 2=1+grip(pro)
                                0x01, 
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply08[]     = {0x02, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//                                                                                                     |    ASK    |      SPI address      | len |
static uint8_t reply106000[] = {0x03, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x00, 0x60, 0x00, 0x00, 0x10, 
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00, 0x00 , 0x00 , 0x00 , 0x00  , 0x00 , 0x00 };
                            // |                                          SN                                                  | 自己的最后两位是21改成32
                            //  0x00, 0x00, 0x58, 0x42, 0x57, 0x32, 0x30, 0x30, 0x31, 0x31, 0x34, 0x36, 0x35, 0x34, 0x33, 0x32, 0x32, 0x31, 0x00, 0x00}; // , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply106050[] = {0x03, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x50, 0x60, 0x00, 0x00, 0x0D, 
                            // |   Body color    |   button color  |  grip color     |  grip color     |  ?  |
                                0x00, 0x28, 0x00, 0x1E, 0xDC, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // , 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t reply106080[] = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x80, 0x60, 0x00, 0x00, 0x18, 
                                0x5E, 0x01, 0x00, 0x00, 0x0F, 0xF0,  // 6-Axis Horizontal Offsets. (JC sideways)
                                0x19, 0xD0, 0x4C, 0xAE, 0x40, 0xE1,  //-|
                                0xEE, 0xE2, 0x2E, 0xEE, 0xE2, 0x2E,  // |- Stick device parameters 1
                                0xB4, 0x4A, 0xAB, 0x96, 0x64, 0x49,  //-|
                                0x00, 0x00};
static uint8_t reply106098[] = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x98, 0x60, 0x00, 0x00, 0x12, 
                                0x19, 0xD0, 0x4C, 0xAE, 0x40, 0xE1,  //-|
                                0xEE, 0xE2, 0x2E, 0xEE, 0xE2, 0x2E,  // |- same with parameters 1
                                0xB4, 0x4A, 0xAB, 0x96, 0x64, 0x49,  //-|
                                0x00, 0x00};
static uint8_t reply108010[] = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x10, 0x80, 0x00, 0x00, 0x18, //  0x00, 0x00};
                                //User analog stick calib
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t reply10603D[] = {0x05, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x3D, 0x60, 0x00, 0x00, 0x19, 
                            //   JC R dump 数据
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                                0x69, 0x48, 0x6F, 0xD0, 0xD4, 0x43, 0xBD, 0x04, 0x49, 
                                0xFF, 0xE6, 0xFF, 0x00, 0x14, 0x28, 0x00}; //, 0x07, 0x7f, 0xF0, 0x07, 0x7f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply106020[] = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x90, 0x10, 0x20, 0x60, 0x00, 0x00, 0x18,  
                            //   JC R dump 数据
                                0xB8, 0x00, 0xB0, 0xFF, 0xB7, 0x00, 0x00, 0x40, 0x00, 
                                0x40, 0x00, 0x40, 0x07, 0x00, 0xEA, 0xFF, 0x02, 0x00, 
                                0xE7, 0x3B, 0xE7, 0x3B, 0xE7, 0x3B,
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t reply03[]     = {0x05, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x80, 0x03, 
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply04[]     = {0x06, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x83, 0x04, 
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply40[]     = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply48[]     = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x80, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply30[]     = {0x04, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0x80, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Reply for SubCommand.SET_NFC_IR_MCU_STATE
static uint8_t reply22[]     = {0x12, 0x8e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x80, 0x22, 
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t reply21[]     = {0x03, 0x8E, 0x84, 0x00, 0x12, 0x01, 0x18, 0x80, 0x01, 0x18, 0x80, 0x80, 0xA0, 0x21, 
                                0x01, 0x00, 0xFF, 0x00, 0x03, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C};
                                // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t emptyReport[] = {0x00, 0x00};



// //Switch button report example //         batlvl       Buttons              Lstick           Rstick
// static uint8_t report30[] = {
//     // 0x30,
//     0x00,
//     0x70, // 高4位: Battery level;(LSB最低位=Charging, 1-3位 8=full, 6=medium, 4=low, 2=critical, 0=empty.) 
//     0, //but1
//     0, //but2
//     0, //but3
//     0, //Ls
//     0, //Ls
//     0, //Ls
//     0, //Rs
//     0, //Rs
//     0, //Rs
//     // 0, // 振动
//     0x08};


// void send_buttons()
// {
//     static uint8_t timer = 0;

//     xSemaphoreTake(xSemaphore, portMAX_DELAY);
//     if (timer > 0x0F) timer = 0;
//     report30[1] = timer ++;
//     //buttons
//     report30[3] = keys_data.button_status_1; // but1_send;
//     report30[4] = keys_data.button_status_2; // but2_send;
//     report30[5] = keys_data.button_status_3; // but3_send;
//     //encode left stick
//     report30[6] = (lx_send << 4) & 0xF0;
//     report30[7] = (lx_send & 0xF0) >> 4;
//     report30[8] = ly_send;
//     //encode right stick
//     report30[9] = (cx_send << 4) & 0xF0;
//     report30[10] = (cx_send & 0xF0) >> 4;
//     report30[11] = cy_send;
//     xSemaphoreGive(xSemaphore);
//     timer += 1;
//     if (timer == 255)
//         timer = 0;

//     const uint8_t REPLT_ID = 0x30;
//     if (!paired)
//     {
//         emptyReport[1] = timer;
//         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(emptyReport), emptyReport);
//         vTaskDelay(100);
//     }
//     else
//     {
//         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(report30), report30);
//         vTaskDelay(15);
//     }
// }

// sending bluetooth values every 15ms
static void _send_task(void *pvParameters)
{
    ESP_LOGI("send_task", "Sending hid reports on core %d\n", xPortGetCoreID());
    // const uint8_t SEND_ID = 0xA1;//0x30;

    while (1)
    {
        // 在connection_cb 里的ESP_HIDD_CONN_STATE_CONNECTED 事件中设置已连接状态
        // 断开状态在ESP_HIDD_CONN_STATE_DISCONNECTED 事件里设置
        if (connected)
        {
            // 在intr_data_cb() 的LED 命令中设置已配对状态
            // 在ESP_HIDD_CONN_STATE_DISCONNECTED 事件里变回未配对状态
            if (!paired)
            {
                // 在paired = true 之前需要不断发送一个0x00 到report 0 ?
                // 不然后面的intr_data_cb 事件不会触发!
                esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, 0xA1, sizeof(emptyReport), emptyReport);
                vTaskDelay(100);
            }
            else 
            {
                xSemaphoreTake(xSemaphore, portMAX_DELAY);
                // _standard_buff.ask = 0x08;
                esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, 0x30, sizeof(standard_data_t), (uint8_t *)&_standard_buff);
                _standard_buff.timer ++;
                xSemaphoreGive(xSemaphore);
                vTaskDelay(15);            
            } 
        }
        else vTaskDelay(100);
    }
}

// //LED blink
// void startBlink()
// {
//     while (1)
//     {
//         gpio_set_level(LED_GPIO, 0);
//         vTaskDelay(150);
//         gpio_set_level(LED_GPIO, 1);
//         vTaskDelay(150);
//         gpio_set_level(LED_GPIO, 0);
//         vTaskDelay(150);
//         gpio_set_level(LED_GPIO, 1);
//         vTaskDelay(1000);
//     }
//     vTaskDelete(NULL);
// }

// callback for hidd connection changes
static void _connection_cb(esp_bd_addr_t bd_addr, esp_hidd_connection_state_t state)
{
    const char *CONN_TAG = "connection_cb";

    switch (state)
    {
    case ESP_HIDD_CONN_STATE_CONNECTED:
        ESP_LOGI(CONN_TAG, "connected to %02x:%02x:%02x:%02x:%02x:%02x",
                 bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        ESP_LOGI(CONN_TAG, "setting bluetooth non connectable");
        esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

        //start solid
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        connected = true;
        xSemaphoreGive(xSemaphore);
        //restart send_task
        if (SendingHandle != NULL)
        {
            vTaskDelete(SendingHandle);
            SendingHandle = NULL;
        }
        xTaskCreatePinnedToCore(_send_task, "send_task", 2048, NULL, NS_CONTROLLER_TASK_PRIORITY, &SendingHandle, NS_CONTROLLER_TASK_CORE_ID);
        _callback(NS_CONTROLLER_CONNECTED_EVT, NULL);
        break;
    case ESP_HIDD_CONN_STATE_CONNECTING:
        ESP_LOGI(CONN_TAG, "connecting");
        break;
    case ESP_HIDD_CONN_STATE_DISCONNECTED:
        // //start blink
        // xTaskCreate(startBlink, "blink_task", 1024, NULL, 1, &BlinkHandle);
        ESP_LOGI(CONN_TAG, "disconnected from %02x:%02x:%02x:%02x:%02x:%02x",
                 bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        ESP_LOGI(CONN_TAG, "making self discoverable");
        paired = 0;
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        connected = false;
        xSemaphoreGive(xSemaphore);
        // if (SendingHandle != NULL)
        // {
        //     vTaskDelete(SendingHandle);
        //     SendingHandle = NULL;
        // }
        // esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        _callback(NS_CONTROLLER_DISCONNECTED_EVT, NULL);
        break;
    case ESP_HIDD_CONN_STATE_DISCONNECTING:
        ESP_LOGI(CONN_TAG, "disconnecting");
        break;
    default:
        ESP_LOGI(CONN_TAG, "unknown connection status");
        break;
    }
}

//callback for discovering
// static void _get_device_cb()
// {
//     ESP_LOGI("hi", "found a device");
// }

// callback for when hid host requests a report
static void _get_report_cb(uint8_t type, uint8_t id, uint16_t buffer_size)
{
    // const char *TAG = "get_report_cb";
    ESP_LOGI("get_report_cb", "got a get_report request from host");
}

// callback for when hid host sends a report
static void _set_report_cb(uint8_t type, uint8_t id, uint16_t len, uint8_t *p_data)
{
    ESP_LOGI("set_report_cb", "got a report from host");
}

// callback for when hid host requests a protocol change
static void _set_protocol_cb(uint8_t protocol)
{
    ESP_LOGI("set_protocol_cb", "got a set_protocol request from host");
}

/** 封装esp_hid_device_send_report 函数,
 *  所有发送都使用此函数, 以便统一更新relpy[xx]的standard 部分的内容.
 *  
 *  send 前一定要先上锁再send, send 完再解锁!
 * 
 *  @param[in]  reply:  reply[xx], 实际发送的数据;
 *                      若不需要发送sub command 部分, 传入NULL,
 *                      则只发送_standard_buff.
 *  @param[in]  len:    sizeof(reply[xx])
 */
// static void _send(esp_hidd_report_type_t type, uint8_t id, uint8_t* reply, uint16_t len)
// {
//     if (reply) 
//     {
//         // xSemaphoreTake(xSemaphore, portMAX_DELAY);
//         memcpy(
//             reply, 
//             &_standard_buff, 
//             // 不更改relpy 的ask 成员
//             sizeof(standard_data_t) - 1/* standard_data_t.ask */); 

//         esp_hid_device_send_report(type, id, len, reply);
//         // xSemaphoreGive(xSemaphore);
//     }
//     else
//     {
//         esp_hid_device_send_report(type, id, sizeof(_standard_buff), &_standard_buff);
//     }
//     _standard_buff.timer++;
// }

void _send_reply(uint8_t *reply, uint16_t len) {      
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    memcpy( reply, 
            &_standard_buff,                                  
            /* 不更改relpy 的ask 成员 */                              
            sizeof(standard_data_t) - 1/* standard_data_t.ask */);  
    esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, 0x21, len, reply);                                      
    _standard_buff.timer++;    
    xSemaphoreGive(xSemaphore);                                     
}

void _update_rumble(void)
{
    // TODO: Rumble 在哪?
}

// callback for when hid host sends interrupt data
static void _intr_data_cb(uint8_t report_id, uint16_t len, uint8_t *p_data)
{
    const char *INTR_TAG = "intr_data_cb";
    // const uint8_t REPLT_ID = 0x21;
    if (report_id == 0x10)
    {
        // ESP_LOGI(INTR_TAG, "id: 0x10 Rumble only");
        return; // TODO: 暂时不响应Rumble     wing
    }

    ESP_LOGI(INTR_TAG, "id:0x%02X len:%d", report_id, len);
    // esp_log_buffer_hex(INTR_TAG, p_data, len);

    switch (report_id)
    {
    case 0x10:
        // Rumble only
        _update_rumble();
        // 测试用
        // xSemaphoreTake(xSemaphore, portMAX_DELAY);
        // _standard_buff.ask = 0x08;
        // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, id, sizeof(_standard_buff), &_standard_buff);
        // _send(ESP_HIDD_REPORT_TYPE_INTRDATA, 0x30, NULL, 0);
        // xSemaphoreGive(xSemaphore);
        return;
    case 0x01:
    //     // 回复0x01 或者 0x10 命令?
    //     esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply02), reply02);
        ESP_LOGI(INTR_TAG, "sub_comm:0x%02X len:%d", p_data[9], len);
        // return;
        switch (p_data[9]) // sub comm
        {
        // Step 1
        case 0x02:
            _send_reply(reply02, sizeof(reply02));
            // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply02), reply02);
            return;
        // Step 2
        case 0x08: 
            _send_reply(reply08, sizeof(reply08)); 
            // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply08), reply08);
            return;
        case 0x10:
            {
                uint32_t *spi_addr = (uint32_t *)&p_data[10];
                ESP_LOGI(INTR_TAG, "read spi 0x%08X", *spi_addr);
                switch (*spi_addr)
                {
                // Step 3
                case 0x6000:
                    _send_reply(reply106000, sizeof(reply106000)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106000), reply106000);
                    return;
                // Step 4
                case 0x6050:
                    _send_reply(reply106050, sizeof(reply106050)); 
                    return;
                // Step 7
                case 0x6080:
                    _send_reply(reply106080, sizeof(reply106080)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106080), reply106080);
                    return;                  
                // Step 8
                case 0x6098:
                    _send_reply(reply106098, sizeof(reply106098)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106098), reply106098);
                    return;
                // Step 9
                case 0x8010:
                    _send_reply(reply108010, sizeof(reply108010)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply108010), reply108010);
                    return;
                // Step 10
                case 0x603D:
                    _send_reply(reply10603D, sizeof(reply10603D)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply10603D), reply10603D);
                    return;
                // Step 11
                case 0x6020:
                    _send_reply(reply106020, sizeof(reply106020)); 
                    // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106020), reply106020);
                    return;

                default:
                    ESP_LOGI(INTR_TAG, "Undefine spi data with addr:0x%08X len:%d", *spi_addr, p_data[14]);
                    return;
                }
            } 
            break;
        // Step 5
        case 0x03:
            ESP_LOGI(INTR_TAG, "id:%d sub:%d", report_id, p_data[9]);
            // esp_log_buffer_hex(INTR_TAG, p_data, len);
            _send_reply(reply03, sizeof(reply03)); 
            // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply03), reply03);
            return;
        // Step 6
        case 0x04:
            _send_reply(reply04, sizeof(reply04)); 
            // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply04), reply04);
            return;
        // Step 12
        case 0x40:
            // if (p_data[10] == 1)
            //     esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply40), reply40);
            // if (p_data[10] == 2)
                _send_reply(reply40, sizeof(reply40)); 
                // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply40), reply40);
            return;
        // Step 13
        case 0x48:
            // if (p_data[10] == 1)
                _send_reply(reply48, sizeof(reply48)); 
                // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply48), reply48);
            // else break;
            return;
        // Step 14
        case 0x22:
            _send_reply(reply22, sizeof(reply22)); 
            // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply22), reply22);        
            return;
        // Step 15
        case 0x30: // player lights
            // if (p_data[10] == 1)
                ESP_LOGI(INTR_TAG, "Set player lights:0x%02X", p_data[10]);
                reply30[14] = p_data[10];
                _send_reply(reply30, sizeof(reply30)); 
                // 设置已配对, 开始发送按键
                paired = 1;
                {
                    static NS_CONTROLLER_EVT_ARG_t arg;
                    arg.Player_Lights.player = reply30[14];
                    _callback(NS_CONTROLLER_PLAYER_LIGHTS_EVT, &arg);
                }
                // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply30), reply30);
            // else break;
            return;
        // Step 16 jc 没收到此命令
        case 0x21:
            if (p_data[10] == 33)
            {
                _send_reply(reply21, sizeof(reply21)); 
                // esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply21), reply21);

            }
            else break;
            return;


        default:
            ESP_LOGI(INTR_TAG, "Unknow sub_comm:0x%02X len:%d", p_data[9], len);
            return;
        }
        break;

    default:
        break;
    }


    // //switch pairing sequence
    // if (len == 49)
    // {
    //     if (p_data[10] == 2)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply02), reply02);
    //     }
    //     if (p_data[10] == 8)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply08), reply08);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 0 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106000), reply106000);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 80 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply1050), reply1050);
    //     }
    //     if (p_data[10] == 3)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply03), reply03);
    //     }
    //     if (p_data[10] == 4)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply04), reply04);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 128 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106080), reply106080);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 152 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106098), reply106098);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 16 && p_data[12] == 128)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply108010), reply108010);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 61 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply10603D), reply10603D);
    //     }
    //     if (p_data[10] == 16 && p_data[11] == 32 && p_data[12] == 96)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply106020), reply106020);
    //     }
    //     if (p_data[10] == 64 && p_data[11] == 1)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply40), reply40);
    //     }
    //     if (p_data[10] == 72 && p_data[11] == 1)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply48), reply48);
    //     }
    //     if (p_data[10] == 48 && p_data[11] == 1)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply30), reply30);
    //     }

    //     if (p_data[10] == 33 && p_data[11] == 33)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply21), reply21);
    //         paired = 1;
    //     }
    //     if (p_data[10] == 64 && p_data[11] == 2)
    //     {
    //         esp_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, REPLT_ID, sizeof(reply40), reply40);
    //     }
    //     //ESP_LOGI(TAG, "got an interrupt report from host, subcommand: %d  %d  %d Length: %d", p_data[10], p_data[11], p_data[12], len);
    //     else
    //     {

            ESP_LOGI(INTR_TAG, "Unknow report id:0x%02X len:%d", report_id, len);
    //     //break;
    //     }
    //     ESP_LOGI("heap size:", "%d", xPortGetFreeHeapSize());
    //     ESP_LOGI(INTR_TAG, "pairing packet size != 49, subcommand: %d  %d  %d  Length: %d", p_data[10], p_data[11], p_data[12], len);
    // }
}

// callback for when hid host does a virtual cable unplug
static void _vc_unplug_cb(void)
{
    ESP_LOGI("vc_unplug_cb", "host did a virtual cable unplug");
}

static esp_err_t set_bt_address()
{
    //store a random mac address in flash
    nvs_handle my_handle;
    esp_err_t err;
    uint8_t esp_addr[8];
    uint8_t bt_addr[6];

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    size_t addr_size = 0;
    err = nvs_get_blob(my_handle, "mac_addr", NULL, &addr_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    if (addr_size > 0)
    {
        err = nvs_get_blob(my_handle, "mac_addr", esp_addr, &addr_size);
    }
    else
    {
        for (int i = 0; i < 8; i++)
            esp_addr[i] = esp_random() % 255;
        size_t addr_size = sizeof(esp_addr);
        err = nvs_set_blob(my_handle, "mac_addr", esp_addr, addr_size);
    }

    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    esp_base_mac_addr_set(esp_addr);
    // bt_addr[5]++;

    // base地址不等于蓝牙地址,
    // esp的蓝牙地址通常最后一字节加2
    esp_read_mac(bt_addr, ESP_MAC_BT);
    // ESP_LOGI(TAG, "read MAC :");
    // esp_log_buffer_hex(TAG, bt_addr, 6);

    // 更新reply02 包中的MAC 地址
    for (int z = 0; z < 6; z++)
        reply02[z + 18] = bt_addr[z];


    return err;
}

static void print_bt_address()
{
    const uint8_t *bd_addr;

    bd_addr = esp_bt_dev_get_address();
    ESP_LOGI(TAG, "my bluetooth address is %02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
}

#define GAP_TAG "gap_cb"
static void _esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
        ESP_LOGI(GAP_TAG, "ESP_BT_GAP_DISC_RES_EVT");
        esp_log_buffer_hex(GAP_TAG, param->disc_res.bda, ESP_BD_ADDR_LEN);
        break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        // ESP_LOGE(GAP_TAG, "%d", esp_bt_gap_dis);
        // if(p_dev->state == APP_GAP_STATE_IDLE){
        //     ESP_LOGE(GAP_TAG, "discovery start ...");
        //     p_dev->state = APP_GAP_STATE_DEVICE_DISCOVERING; 
        // }else if(p_dev->state == APP_GAP_STATE_DEVICE_DISCOVERING){
        //     ESP_LOGE(GAP_TAG, "discovery timeout ...");
        //     p_dev->state = APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE;
        //     esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        // }else{
        //     ESP_LOGE(GAP_TAG, "discovery again ...");
        //     p_dev->state = APP_GAP_STATE_IDLE;
        ESP_LOGI(GAP_TAG, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT %d", param->disc_st_chg.state);
        break;
    case ESP_BT_GAP_RMT_SRVCS_EVT:
        ESP_LOGI(GAP_TAG, "ESP_BT_GAP_RMT_SRVCS_EVT");
        ESP_LOGI(GAP_TAG, "%d", param->rmt_srvcs.num_uuids);
        break;
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
        ESP_LOGI(GAP_TAG, "ESP_BT_GAP_RMT_SRVC_REC_EVT");
        break;
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(GAP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(GAP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);

            NS_CONTROLLER_EVT_ARG_t arg;
            memcpy(&(arg.Scanned_device.addr), &(param->auth_cmpl.bda), sizeof(esp_bd_addr_t));
            _callback(NS_CONTROLLER_SCANNED_DEVICE_EVT, &arg);
        }
        else
        {
            ESP_LOGE(GAP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
            /*******************************
             * 
             * TODO: 哩度进入深度睡眠 ?
             * 
             ********************************/
        }
        break;
    }
    case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
        ESP_LOGI(GAP_TAG, "? ESP_BT_GAP_CONFIG_EIR_DATA_EVT ?");
        break;

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(GAP_TAG, "ESP_BT_GAP_MODE_CHG_EVT PM Mode:%d", param->mode_chg.mode);
        esp_log_buffer_hex(GAP_TAG, param->mode_chg.bda, ESP_BD_ADDR_LEN);

        if (param->mode_chg.mode == ESP_BT_PM_MD_ACTIVE) 
        {

        }
        break;

    default:
        ESP_LOGE(GAP_TAG, "Unknow event:%d", event);
        break;
    }
}

// callback for notifying when hidd application is registered or not registered
static void _application_cb(esp_bd_addr_t bd_addr, esp_hidd_application_state_t state)
{
    const char *APP_TAG = "application_cb";

    switch (state)
    {
    case ESP_HIDD_APP_STATE_NOT_REGISTERED:
        ESP_LOGI(APP_TAG, "app not registered");
        break;
    case ESP_HIDD_APP_STATE_REGISTERED:
        ESP_LOGI(APP_TAG, "app is now registered!");
        if (bd_addr == NULL)
        {
            ESP_LOGI(APP_TAG, "bd_addr is null...");
            break;
        }
        break;
    default:
        ESP_LOGW(APP_TAG, "unknown app state %i", state);
        break;
    }
}

esp_err_t NS_Controller_init(NS_Controller_Type_t joy_type, NS_CONTROLLER_CALLBACK cb) 
{
    if (cb == NULL) return ESP_ERR_INVALID_ARG;
    _callback = cb;
    esp_err_t ret;

    if (xSemaphore) return ESP_FAIL;
    xSemaphore = xSemaphoreCreateMutex();

    set_bt_address();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_mem_release(ESP_BT_MODE_BLE);
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        ESP_LOGE(TAG, "initialize controller failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK)
    {
        ESP_LOGE(TAG, "enable controller failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK)
    {
        ESP_LOGE(TAG, "initialize bluedroid failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(TAG, "enable bluedroid failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    if (esp_bt_gap_register_callback(_esp_bt_gap_cb) != ESP_OK)
        return ESP_FAIL;

    ESP_LOGI(TAG, "setting hid parameters");
    static esp_hidd_app_param_t app_param;
    app_param.name = "Wireless Gamepad";
    app_param.description = "Gamepad";
    app_param.provider = "Nintendo";
    app_param.subclass = 0x08;
    app_param.desc_list = hid_descriptor;
    app_param.desc_list_len = hid_descriptor_len;
    static esp_hidd_qos_param_t both_qos;
    memset(&both_qos, 0, sizeof(esp_hidd_qos_param_t));
    if (esp_hid_device_register_app(&app_param, &both_qos, &both_qos) != ESP_OK)
        return ESP_FAIL;

    ESP_LOGI(TAG, "starting hid device");
    callbacks.application_state_cb = _application_cb;
    callbacks.connection_state_cb = _connection_cb;
    callbacks.get_report_cb = _get_report_cb;
    callbacks.set_report_cb = _set_report_cb;
    callbacks.set_protocol_cb = _set_protocol_cb;
    callbacks.intr_data_cb = _intr_data_cb;
    callbacks.vc_unplug_cb = _vc_unplug_cb;
    if (esp_hid_device_init(&callbacks) != ESP_OK)
        return ESP_FAIL;

    _standard_buff.Connection_info  = 0x0E;
    // _standard_buff.battery          = Battery_Full;
    // _standard_buff.Charging         = false;
    // _standard_buff.vibration        = 0x0B;
    _standard_buff.ask              = 0x00;
    _standard_buff.left_stick.stick_data_1  = 0x00;
    _standard_buff.left_stick.stick_data_2  = 0x08;
    _standard_buff.left_stick.stick_data_3  = 0x80;
    _standard_buff.rigth_stick.stick_data_1 = 0x00;
    _standard_buff.rigth_stick.stick_data_2 = 0x08;
    _standard_buff.rigth_stick.stick_data_3 = 0x80;

    ESP_LOGI(TAG, "setting device name");
    switch (joy_type)
    {
    case Left_Joycon:
        if (esp_bt_dev_set_device_name("Joy-Con (L)") != ESP_OK)
            return ESP_FAIL;
        reply02[16] = 1;
        break;
    case Right_Joycon:
        if (esp_bt_dev_set_device_name("Joy-Con (R)") != ESP_OK)
            return ESP_FAIL;
        reply02[16] = 2;
        break;
    
    default:
        if (esp_bt_dev_set_device_name("Pro Controller") != ESP_OK)
            return ESP_FAIL;
        reply02[16] = 3;
        break;
    }

    ESP_LOGI(TAG, "setting hid device class");
    static esp_bt_cod_t class =
    {
        .minor = 2,
        .major = 5,
        .service = 1,
    };
    if (esp_bt_gap_set_cod(class, ESP_BT_SET_COD_ALL) != ESP_OK)
        return ESP_FAIL;

    return ESP_OK;
}

esp_err_t NS_Scan(void)
{
    ESP_LOGI(TAG, "setting to connectable, discoverable");
    
    return  
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE)
        
        == ESP_OK ? ESP_OK : ESP_FAIL;
}

// #include "bta_hd_int.h"

esp_err_t NS_Open(esp_bd_addr_t bd_addr)
{
    ESP_LOGI(TAG, "BTA Open...");
    // tBTA_HD_CBACK_DATA data =
    // {
    //     .addr = {0x58, 0x2F, 0x40, 0xDA, 0xAF, 0x01},
    // };
    // bta_hd_open_act((tBTA_HD_DATA*)&data);
    esp_hid_open_device(bd_addr);
    return ESP_OK;
}
// {
//     // = {
//         // .addr = {0x58, 0x2F, 0x40, 0xDA, 0xAF, 0x01};
//     //};
//     bta_hd_open_act((tBTA_HD_DATA*)&data);
// //     return esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 48, 0);
// }

void NS_Set_Battery(NS_Battery_Level_t level)
{
    // 允许在锁初始化前设置
    bool have_lock = xSemaphore == NULL ? false : true;
    if (have_lock) xSemaphoreTake(xSemaphore, portMAX_DELAY);
    _standard_buff.battery = level;
    if (have_lock) xSemaphoreGive(xSemaphore);
}

void NS_Set_Charging(bool charging)
{
    // 允许在锁初始化前设置
    bool have_lock = xSemaphore == NULL ? false : true;
    if (have_lock) xSemaphoreTake(xSemaphore, portMAX_DELAY);
    _standard_buff.Charging = charging;
    if (have_lock) xSemaphoreGive(xSemaphore);
}

void NS_Set_Buttons(ns_button_status_t *status)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    memcpy(&(_standard_buff.buttons), status, sizeof(ns_button_status_t));
    xSemaphoreGive(xSemaphore);
}
