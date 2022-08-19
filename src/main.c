//
//  BlueCubeMod Firmware
//
//
//  Created by Nathan Reeves 2019
//

#include "esp_log.h"
// #include "esp_hidd_api.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"
// #include "esp_bt.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
// #include "esp_gap_bt_api.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
// #include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/rmt.h"
// #include "soc/rmt_reg.h"
#include "driver/periph_ctrl.h"

#include "ns_controller.h"
#include "buttons.h"
#include "lamp.h"
// #include "rmt_ws2812.h"
#include "uart_command.h"

static const char *TAG = "Main";

// #define LED_GPIO 25
// #define PIN_SEL (1ULL << LED_GPIO)

//for reading GameCube controller values
// #define RMT_TX_GPIO_NUM 23                               // GameCube TX GPIO ----
// #define RMT_RX_GPIO_NUM 18                               // GameCube RX GPIO ----
// #define RMT_TX_CHANNEL 2                                 /*!< RMT channel for transmitter */
// #define RMT_RX_CHANNEL 3                                 /*!< RMT channel for receiver */
// #define RMT_CLK_DIV 80                                   /*!< RMT counter clock divider */
// #define RMT_TICK_10_US (80000000 / RMT_CLK_DIV / 100000) /*!< RMT counter value for 10 us.(Source clock is APB clock) */
// #define rmt_item32_tIMEOUT_US 9500                       /*!< RMT receiver timeout value(us) */

// //Calibration
// static int lxcalib = 0;
// static int lycalib = 0;
// static int cxcalib = 0;
// static int cycalib = 0;
// static int lcalib = 0;
// static int rcalib = 0;
// //Buttons and sticks
// static uint8_t but1_send = 0;
// static uint8_t but2_send = 0;
// static uint8_t but3_send = 0;
// static uint8_t lx_send = 127;
// static uint8_t ly_send = 127;
// static uint8_t cx_send = 127;
// static uint8_t cy_send = 127;
// static uint8_t lt_send = 0;
// static uint8_t rt_send = 0;


// //RMT Transmitter Init - for reading GameCube controller
// rmt_item32_t items[25];
// rmt_config_t rmt_tx;

// TaskHandle_t BlinkHandle = NULL;


// static void rmt_tx_init()
// {

//     rmt_tx.channel = RMT_TX_CHANNEL;
//     rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
//     rmt_tx.mem_block_num = 1;
//     rmt_tx.clk_div = RMT_CLK_DIV;
//     rmt_tx.tx_config.loop_en = false;
//     rmt_tx.tx_config.carrier_freq_hz = 24000000;
//     rmt_tx.tx_config.carrier_level = 1;
//     rmt_tx.tx_config.carrier_en = 0;
//     rmt_tx.tx_config.idle_level = 1;
//     rmt_tx.tx_config.idle_output_en = true;
//     rmt_tx.rmt_mode = 0;
//     rmt_config(&rmt_tx);
//     rmt_driver_install(rmt_tx.channel, 0, 0);

//     //Fill items[] with console->controller command: 0100 0000 0000 0011 0000 0010

//     items[0].duration0 = 3;
//     items[0].level0 = 0;
//     items[0].duration1 = 1;
//     items[0].level1 = 1;
//     items[1].duration0 = 1;
//     items[1].level0 = 0;
//     items[1].duration1 = 3;
//     items[1].level1 = 1;
//     int j;
//     for (j = 0; j < 12; j++)
//     {
//         items[j + 2].duration0 = 3;
//         items[j + 2].level0 = 0;
//         items[j + 2].duration1 = 1;
//         items[j + 2].level1 = 1;
//     }
//     items[14].duration0 = 1;
//     items[14].level0 = 0;
//     items[14].duration1 = 3;
//     items[14].level1 = 1;
//     items[15].duration0 = 1;
//     items[15].level0 = 0;
//     items[15].duration1 = 3;
//     items[15].level1 = 1;
//     for (j = 0; j < 8; j++)
//     {
//         items[j + 16].duration0 = 3;
//         items[j + 16].level0 = 0;
//         items[j + 16].duration1 = 1;
//         items[j + 16].level1 = 1;
//     }
//     items[24].duration0 = 1;
//     items[24].level0 = 0;
//     items[24].duration1 = 3;
//     items[24].level1 = 1;
// }

//RMT Receiver Init
// rmt_config_t rmt_rx;
// static void rmt_rx_init()
// {
//     rmt_rx.channel = RMT_RX_CHANNEL;
//    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
//     rmt_rx.clk_div = RMT_CLK_DIV;
//     rmt_rx.mem_block_num = 4;
//     rmt_rx.rmt_mode = RMT_MODE_RX;
//     rmt_rx.rx_config.idle_threshold = rmt_item32_tIMEOUT_US / 10 * (RMT_TICK_10_US);
//     rmt_config(&rmt_rx);
// }

// //Polls controller and formats response
// //GameCube Controller Protocol: http://www.int03.co.uk/crema/hardware/gamecube/gc-control.html
// static void get_buttons()
// {
//     ESP_LOGI("hi", "Hello world from core %d!\n", xPortGetCoreID());
//     //button init values
//     uint8_t but1 = 0;
//     uint8_t but2 = 0;
//     uint8_t but3 = 0;
//     uint8_t dpad = 0x08; //Released
//     uint8_t lx = 0;
//     uint8_t ly = 0;
//     uint8_t cx = 0;
//     uint8_t cy = 0;
//     uint8_t lt = 0;
//     uint8_t rt = 0;

//     //Sample and find calibration value for sticks
//     int calib_loop = 0;
//     int xsum = 0;
//     int ysum = 0;
//     int cxsum = 0;
//     int cysum = 0;
//     int lsum = 0;
//     int rsum = 0;
//     while (calib_loop < 5)
//     {
//         lx = 0;
//         ly = 0;
//         cx = 0;
//         cy = 0;
//         lt = 0;
//         rt = 0;
//         rmt_write_items(rmt_tx.channel, items, 25, 0);
//         rmt_rx_start(rmt_rx.channel, 1);

//         vTaskDelay(10);

//         rmt_item32_t *item = (rmt_item32_t *)(RMT_CHANNEL_MEM(rmt_rx.channel));
//         if (item[33].duration0 == 1 && item[27].duration0 == 1 && item[26].duration0 == 3 && item[25].duration0 == 3)
//         {

//             //LEFT STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 41].duration0 == 1))
//                 {
//                     lx += pow(2, 8 - x - 1);
//                 }
//             }

//             //LEFT STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 49].duration0 == 1))
//                 {
//                     ly += pow(2, 8 - x - 1);
//                 }
//             }
//             //C STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 57].duration0 == 1))
//                 {
//                     cx += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 65].duration0 == 1))
//                 {
//                     cy += pow(2, 8 - x - 1);
//                 }
//             }

//             //R AN
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 73].duration0 == 1))
//                 {
//                     rt += pow(2, 8 - x - 1);
//                 }
//             }

//             //L AN
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 81].duration0 == 1))
//                 {
//                     lt += pow(2, 8 - x - 1);
//                 }
//             }

//             xsum += lx;
//             ysum += ly;
//             cxsum += cx;
//             cysum += cy;
//             lsum += lt;
//             rsum += rt;
//             calib_loop++;
//         }
//     }

//     //Set Stick Calibration
//     lxcalib = 127 - (xsum / 5);
//     lycalib = 127 - (ysum / 5);
//     cxcalib = 127 - (cxsum / 5);
//     cycalib = 127 - (cysum / 5);
//     lcalib = 127 - (lsum / 5);
//     rcalib = 127 - (rsum / 5);

//     while (1)
//     {
//         but1 = 0;
//         but2 = 0;
//         but3 = 0;
//         dpad = 0x08;
//         lx = 0;
//         ly = 0;
//         cx = 0;
//         cy = 0;
//         lt = 0;
//         rt = 0;

//         //Write command to controller
//         rmt_write_items(rmt_tx.channel, items, 25, 0);
//         rmt_rx_start(rmt_rx.channel, 1);

//         vTaskDelay(6); //6ms between sample

//         rmt_item32_t *item = (rmt_item32_t *)(RMT_CHANNEL_MEM(rmt_rx.channel));

//         //Check first 3 bits and high bit at index 33
//         if (item[33].duration0 == 1 && item[27].duration0 == 1 && item[26].duration0 == 3 && item[25].duration0 == 3)
//         {

//             //Button report: first item is item[25]
//             //0 0 1 S Y X B A
//             //1 L R Z U D R L
//             //Joystick X (8bit)
//             //Joystick Y (8bit)
//             //C-Stick X (8bit)
//             //C-Stick Y (8bit)
//             //L Trigger Analog (8/4bit)
//             //R Trigger Analog (8/4bit)

//             if (item[32].duration0 == 1)
//                 but1 += 0x08; // A
//             if (item[31].duration0 == 1)
//                 but1 += 0x04; // B
//             if (item[30].duration0 == 1)
//                 but1 += 0x02; // X
//             if (item[29].duration0 == 1)
//                 but1 += 0x01; // Y

//             if (item[28].duration0 == 1)
//                 but2 += 0x02; // START/PLUS

//             //DPAD
//             if (item[40].duration0 == 1)
//                 but3 += 0x08; // L
//             if (item[39].duration0 == 1)
//                 but3 += 0x04; // R
//             if (item[38].duration0 == 1)
//                 but3 += 0x01; // D
//             if (item[37].duration0 == 1)
//                 but3 += 0x02; // U

//             if (item[35].duration0 == 1)
//                 but1 += 0x80; // ZR
//             if (item[34].duration0 == 1)
//                 but3 += 0x80; // ZL
//             //Buttons
//             if (item[36].duration0 == 1)
//             {
//                 but1 += 0x40; // Z
//                               // if(but3 == 0x80) { but3 += 0x40;}
//                 if (but2 == 0x02)
//                 {
//                     but2 = 0x01;
//                 } // Minus =  Z + Start
//                 if (but3 == 0x02)
//                 {
//                     but2 = 0x10;
//                 } // Home =  Z + Up
//             }

//             //LEFT STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 41].duration0 == 1)
//                 {
//                     lx += pow(2, 8 - x - 1);
//                 }
//             }

//             //LEFT STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 49].duration0 == 1)
//                 {
//                     ly += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 57].duration0 == 1)
//                 {
//                     cx += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 65].duration0 == 1)
//                 {
//                     cy += pow(2, 8 - x - 1);
//                 }
//             }

//             /// Analog triggers here --  Ignore for Switch :/
//             /*
//             //R AN
//             for(int x = 8; x > -1; x--)
//             {
//                 if(item[x+73].duration0 == 1)
//                 {
//                     rt += pow(2, 8-x-1);
//                 }
//             }
            
//             //L AN
//             for(int x = 8; x > -1; x--)
//             {
//                 if(item[x+81].duration0 == 1)
//                 {
//                     lt += pow(2, 8-x-1);
//                 }
//             }*/
//             /////
//             but1_send = but1;
//             but2_send = but2;
//             but3_send = but3;
//             lx_send = lx + lxcalib;
//             ly_send = ly + lycalib;
//             cx_send = cx + cxcalib;
//             cy_send = cy + cycalib;
//             lt_send = 0; //lt;//left trigger analog
//             rt_send = 0; //rt;//right trigger analog
//         }
//         else
//         {
//             //log_info("GameCube controller read fail");
//         }
//     }
// }

///Switch Replies
// static uint8_t reply02[] = {0x01, 0x40, 0x00, 0x00, 0x00, 0xe6, 0x27, 0x78, 0xab, 0xd7, 0x76, 0x00, 0x82, 0x02, 0x03, 0x48, 0x03, 0x02, 0xD8, 0xA0, 0x1D, 0x40, 0x15, 0x66, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Reply for REQUEST_DEVICE_INFO

static NS_Controller_Type_t _joy_type;
static ns_button_status_t keys_data;


static Button_Handle_t _button_0_handle;

/// 大键触发事件回调
static void _button_key_Callback (Button_Event_t val)
{
    ESP_LOGI("BTN 0", "%d", val);
    
    // TODO: 脚本运行过程构想:
    //  Step 0: 重置全部按键值;
    //  Step 1: 区分JC(L)/JC(R)/Pro
    //  Step 2: 运行按下脚本
    //  Step 2 - 1: 设置按键值;
    //  Step 2 - 2: 延时;
    //  Step 2 - 3: 重复 Step2 - 1 - Step2 - 2;
    //  Step 2 - 4: 结束;
    //  Step 3 : 运行松开脚本:
    //  Step 3 - 1 ~ 4 同按下;
    //  Step 4 结束
    if (val == Button_Event_On) 
    {   
        keys_data.Capture = 1;
        NS_Set_Buttons(&keys_data);
        vTaskDelay(1000 / portTICK_RATE_MS);
        keys_data.Capture = 0;
        NS_Set_Buttons(&keys_data);
    } 
}

/// 给NS_Open() 做状态标记
static bool conneted = false;

// 前置声明
void _ns_controller_cb(NS_CONTROLLER_EVT event, NS_CONTROLLER_EVT_ARG_t *arg);

void _wait_connet_task(void *param)
{
    if ((!conneted) && (param != NULL))
    {
        esp_bd_addr_t addr;
        memcpy(addr, param, sizeof(esp_bd_addr_t));
        ESP_ERROR_CHECK(NS_Open(addr));

        vTaskDelay(5000 / portTICK_RATE_MS);
        if (!conneted)
            // 触发事件至关机
            _ns_controller_cb(NS_CONTROLLER_DISCONNECTED_EVT, NULL);
    }

    ESP_LOGI("_wait_connet_task", "heap free: %d", xPortGetFreeHeapSize());
    vTaskDelete(NULL);
}

void _ns_controller_cb(NS_CONTROLLER_EVT event, NS_CONTROLLER_EVT_ARG_t *arg)
{
    switch (event)
    {
    case NS_CONTROLLER_SCANNED_DEVICE_EVT:
        // TODO: 将地址保存在flash...
        // TODO: 是否需要新开一个线程?
        break;
    case NS_CONTROLLER_CONNECTED_EVT:
        conneted = true;
        break;
    case NS_CONTROLLER_DISCONNECTED_EVT:
        conneted = false;
        // TODO: 关机...
        printf("To sleep...\n");
        break;
    
    default:
        break;
    }
}


// static WS2812_COLOR_t color[] = {
//     {0xFF0000}, 
//     {0x00FF00}, 
//     {0x0000FF}, 
// };

// // 测试
// uint8_t w[] = {
//             0, 2, 4, 6, 
//             9, 12, 15, 18,
//             22, 26, 30, 34,
//             39, 44, 49, 54,
//             60, 66, 72, 78,
//             85, 92, 99, 106,
//             114, 122
// };

// void _ws2812_task(void *param)
// {
//     ESP_ERROR_CHECK(WS2812_Loop_Start(25));

//     // 测试
//     int i = 0;
//     int a = 1;
//     while (1)
//     {
//         color[0].r = w[i];
//         color[0].g = w[i];
//         color[0].b = w[i];
//         color[1].r = w[i];
//         color[1].g = w[i];
//         color[1].b = w[i];
//         i += a;
//         if (i == sizeof(w) - 1) a = -1;
//         if (i == 0)    a = 1;
//         ESP_ERROR_CHECK(WS2812_Fill_Buffer(color, 2));
//         vTaskDelay(40 / portTICK_RATE_MS);
//     }
// }

uint32_t _gradient_red_frames[] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15, 
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31, 
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47, 
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63, 
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79, 
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95, 
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 
    LAMP_EFFECT_HOLDER_FRAME,
};

WS2812_COLOR_t _gradient_red_table[] = {
    {.rgb = 0x000000,}, {.rgb = 0x010000,}, {.rgb = 0x020000,}, {.rgb = 0x030000,}, {.rgb = 0x040000,}, {.rgb = 0x050000,}, {.rgb = 0x060000,}, {.rgb = 0x070000,}, {.rgb = 0x080000,}, {.rgb = 0x090000,}, {.rgb = 0x0A0000,}, {.rgb = 0x0B0000,}, {.rgb = 0x0C0000,}, {.rgb = 0x0D0000,}, {.rgb = 0x0E0000,}, {.rgb = 0x0F0000,}, 
    {.rgb = 0x100000,}, {.rgb = 0x110000,}, {.rgb = 0x120000,}, {.rgb = 0x130000,}, {.rgb = 0x140000,}, {.rgb = 0x150000,}, {.rgb = 0x160000,}, {.rgb = 0x170000,}, {.rgb = 0x180000,}, {.rgb = 0x190000,}, {.rgb = 0x1A0000,}, {.rgb = 0x1B0000,}, {.rgb = 0x1C0000,}, {.rgb = 0x1D0000,}, {.rgb = 0x1E0000,}, {.rgb = 0x1F0000,}, 
    {.rgb = 0x200000,}, {.rgb = 0x210000,}, {.rgb = 0x220000,}, {.rgb = 0x230000,}, {.rgb = 0x240000,}, {.rgb = 0x250000,}, {.rgb = 0x260000,}, {.rgb = 0x270000,}, {.rgb = 0x280000,}, {.rgb = 0x290000,}, {.rgb = 0x2A0000,}, {.rgb = 0x2B0000,}, {.rgb = 0x2C0000,}, {.rgb = 0x2D0000,}, {.rgb = 0x2E0000,}, {.rgb = 0x2F0000,}, 
    {.rgb = 0x300000,}, {.rgb = 0x310000,}, {.rgb = 0x320000,}, {.rgb = 0x330000,}, {.rgb = 0x340000,}, {.rgb = 0x350000,}, {.rgb = 0x360000,}, {.rgb = 0x370000,}, {.rgb = 0x380000,}, {.rgb = 0x390000,}, {.rgb = 0x3A0000,}, {.rgb = 0x3B0000,}, {.rgb = 0x3C0000,}, {.rgb = 0x3D0000,}, {.rgb = 0x3E0000,}, {.rgb = 0x3F0000,}, 
    {.rgb = 0x400000,}, {.rgb = 0x410000,}, {.rgb = 0x420000,}, {.rgb = 0x430000,}, {.rgb = 0x440000,}, {.rgb = 0x450000,}, {.rgb = 0x460000,}, {.rgb = 0x470000,}, {.rgb = 0x480000,}, {.rgb = 0x490000,}, {.rgb = 0x4A0000,}, {.rgb = 0x4B0000,}, {.rgb = 0x4C0000,}, {.rgb = 0x4D0000,}, {.rgb = 0x4E0000,}, {.rgb = 0x4F0000,}, 
    {.rgb = 0x500000,}, {.rgb = 0x510000,}, {.rgb = 0x520000,}, {.rgb = 0x530000,}, {.rgb = 0x540000,}, {.rgb = 0x550000,}, {.rgb = 0x560000,}, {.rgb = 0x570000,}, {.rgb = 0x580000,}, {.rgb = 0x590000,}, {.rgb = 0x5A0000,}, {.rgb = 0x5B0000,}, {.rgb = 0x5C0000,}, {.rgb = 0x5D0000,}, {.rgb = 0x5E0000,}, {.rgb = 0x5F0000,}, 
    {.rgb = 0x600000,}, {.rgb = 0x610000,}, {.rgb = 0x620000,}, {.rgb = 0x630000,}, {.rgb = 0x640000,}, {.rgb = 0x650000,}, {.rgb = 0x660000,}, {.rgb = 0x670000,}, {.rgb = 0x680000,}, {.rgb = 0x690000,}, {.rgb = 0x6A0000,}, {.rgb = 0x6B0000,}, {.rgb = 0x6C0000,}, {.rgb = 0x6D0000,}, {.rgb = 0x6E0000,}, {.rgb = 0x6F0000,}, 
    {.rgb = 0x700000,}, {.rgb = 0x710000,}, {.rgb = 0x720000,}, {.rgb = 0x730000,}, {.rgb = 0x740000,}, {.rgb = 0x750000,}, {.rgb = 0x760000,}, {.rgb = 0x770000,}, {.rgb = 0x780000,}, {.rgb = 0x790000,}, {.rgb = 0x7A0000,}, {.rgb = 0x7B0000,}, {.rgb = 0x7C0000,}, {.rgb = 0x7D0000,}, {.rgb = 0x7E0000,}, {.rgb = 0x7F0000,}, 
    {.rgb = 0x800000,}, {.rgb = 0x810000,}, {.rgb = 0x820000,}, {.rgb = 0x830000,}, {.rgb = 0x840000,}, {.rgb = 0x850000,}, {.rgb = 0x860000,}, {.rgb = 0x870000,}, {.rgb = 0x880000,}, {.rgb = 0x890000,}, {.rgb = 0x8A0000,}, {.rgb = 0x8B0000,}, {.rgb = 0x8C0000,}, {.rgb = 0x8D0000,}, {.rgb = 0x8E0000,}, {.rgb = 0x8F0000,}, 
    {.rgb = 0x900000,}, {.rgb = 0x910000,}, {.rgb = 0x920000,}, {.rgb = 0x930000,}, {.rgb = 0x940000,}, {.rgb = 0x950000,}, {.rgb = 0x960000,}, {.rgb = 0x970000,}, {.rgb = 0x980000,}, {.rgb = 0x990000,}, {.rgb = 0x9A0000,}, {.rgb = 0x9B0000,}, {.rgb = 0x9C0000,}, {.rgb = 0x9D0000,}, {.rgb = 0x9E0000,}, {.rgb = 0x9F0000,}, 
    {.rgb = 0xA00000,}, {.rgb = 0xA10000,}, {.rgb = 0xA20000,}, {.rgb = 0xA30000,}, {.rgb = 0xA40000,}, {.rgb = 0xA50000,}, {.rgb = 0xA60000,}, {.rgb = 0xA70000,}, {.rgb = 0xA80000,}, {.rgb = 0xA90000,}, {.rgb = 0xAA0000,}, {.rgb = 0xAB0000,}, {.rgb = 0xAC0000,}, {.rgb = 0xAD0000,}, {.rgb = 0xAE0000,}, {.rgb = 0xAF0000,}, 
    {.rgb = 0xB00000,}, {.rgb = 0xB10000,}, {.rgb = 0xB20000,}, {.rgb = 0xB30000,}, {.rgb = 0xB40000,}, {.rgb = 0xB50000,}, {.rgb = 0xB60000,}, {.rgb = 0xB70000,}, {.rgb = 0xB80000,}, {.rgb = 0xB90000,}, {.rgb = 0xBA0000,}, {.rgb = 0xBB0000,}, {.rgb = 0xBC0000,}, {.rgb = 0xBD0000,}, {.rgb = 0xBE0000,}, {.rgb = 0xBF0000,}, 
    {.rgb = 0xC00000,}, {.rgb = 0xC10000,}, {.rgb = 0xC20000,}, {.rgb = 0xC30000,}, {.rgb = 0xC40000,}, {.rgb = 0xC50000,}, {.rgb = 0xC60000,}, {.rgb = 0xC70000,}, {.rgb = 0xC80000,}, {.rgb = 0xC90000,}, {.rgb = 0xCA0000,}, {.rgb = 0xCB0000,}, {.rgb = 0xCC0000,}, {.rgb = 0xCD0000,}, {.rgb = 0xCE0000,}, {.rgb = 0xCF0000,}, 
    {.rgb = 0xD00000,}, {.rgb = 0xD10000,}, {.rgb = 0xD20000,}, {.rgb = 0xD30000,}, {.rgb = 0xD40000,}, {.rgb = 0xD50000,}, {.rgb = 0xD60000,}, {.rgb = 0xD70000,}, {.rgb = 0xD80000,}, {.rgb = 0xD90000,}, {.rgb = 0xDA0000,}, {.rgb = 0xDB0000,}, {.rgb = 0xDC0000,}, {.rgb = 0xDD0000,}, {.rgb = 0xDE0000,}, {.rgb = 0xDF0000,}, 
    {.rgb = 0xE00000,}, {.rgb = 0xE10000,}, {.rgb = 0xE20000,}, {.rgb = 0xE30000,}, {.rgb = 0xE40000,}, {.rgb = 0xE50000,}, {.rgb = 0xE60000,}, {.rgb = 0xE70000,}, {.rgb = 0xE80000,}, {.rgb = 0xE90000,}, {.rgb = 0xEA0000,}, {.rgb = 0xEB0000,}, {.rgb = 0xEC0000,}, {.rgb = 0xED0000,}, {.rgb = 0xEE0000,}, {.rgb = 0xEF0000,}, 
    {.rgb = 0xF00000,}, {.rgb = 0xF10000,}, {.rgb = 0xF20000,}, {.rgb = 0xF30000,}, {.rgb = 0xF40000,}, {.rgb = 0xF50000,}, {.rgb = 0xF60000,}, {.rgb = 0xF70000,}, {.rgb = 0xF80000,}, {.rgb = 0xF90000,}, {.rgb = 0xFA0000,}, {.rgb = 0xFB0000,}, {.rgb = 0xFC0000,}, {.rgb = 0xFD0000,}, {.rgb = 0xFE0000,}, {.rgb = 0xFF0000,}, 
    {.rgb = 0xFF0000,}/*最后一帧补够两个像素*/, 
};

Lamp_Effect_t _gradient_red_effect = {
    .freq       = 25,
    .clipping   = false,
    .virginity  = true,
    .frameMode  = LAMP_EFFECT_FRAME_MODE_SINGLE,
    // .direction  = LAMP_EFFECT_FRAME_DIRECTION_FORWARD,
    .current    = 0,
    .startMode  = LAMP_EFFECT_STATE_MODE_RESTART,
    .frames     = _gradient_red_frames,
    .length     = sizeof(_gradient_red_frames) / sizeof(uint32_t),
    .table      = _gradient_red_table,
    .semaphore  = NULL,
};

void app_main()
{
    // 等待外设上电
    vTaskDelay(100 / portTICK_RATE_MS);

    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 灯带
	WS2812_CONFIG_t      ws2812_congif = WS2812_Default_Config(WS2812_DIN_GPIO_NUM, WS2812_COUNT);
	CPC405x_LDO_Config_t ldo_config    = CPC405x_LDO_Default_Config(CPC405x_EN_GPIO_NUM, CPC405x_LDO_NUM);
    ESP_ERROR_CHECK(Lamp_Init(&ws2812_congif, &ldo_config));
    Lamp_Gamma_On(NULL);

    // 判断重置键是否长按
    uint8_t press = 0;
    for (size_t c = 0; c < 3; c++) {
        if (Button_Click(BTN_RES_GPIO_NUM) == Button_On) press++;
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    // 开始十秒倒数
    if (press > 0) {
        press = 1;
        ESP_ERROR_CHECK(Lamp_Effect_Start(&_gradient_red_effect));
        for (size_t k = 0; k < 20; k++) {
            if (Button_Click(BTN_KEY_GPIO_NUM) == Button_Off) {
                press = 0;
                break;
            }

            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }
    // 进入DFU模式
    if (press > 0) {
        ESP_LOGI(TAG, "Enter DFU...\n");
    }
    else 
        ESP_ERROR_CHECK(Lamp_Effect_Stop());




    //GameCube Contoller reading init
    // rmt_tx_init();
    // rmt_rx_init();
    // xTaskCreatePinnedToCore(get_buttons, "gbuttons", 2048, NULL, 1, NULL, 1);


    memset(&keys_data, 0, sizeof(keys_data));
    // TODO: 读取脚本设置中的手柄类型
    _joy_type = Left_Joycon; // Switch_pro));

    // 启动蓝牙
    ESP_ERROR_CHECK(NS_Controller_init(_joy_type, _ns_controller_cb)); 
    NS_Set_Battery(Battery_Full);
    // NS_Set_Battery(Battery_Level_3);
    // NS_Set_Charging(true);


    // TODO: 从flash 读取匹配过的地址
    esp_bd_addr_t bd_addr = {0x58, 0x2F, 0x40, 0xDA, 0xAF, 0x01};
 
    if (bd_addr == NULL)

        ESP_ERROR_CHECK(NS_Scan());
    else
    {
        xTaskCreatePinnedToCore(_wait_connet_task, "wait_connet_task", 2048, bd_addr, 10, NULL, NS_CONTROLLER_TASK_CORE_ID);

    }


    Button_Config_t btn_config = Button_Default_Config(BTN_KEY_GPIO_NUM, _button_key_Callback);
    _button_0_handle = Button_Enable(&btn_config);
    if ( _button_0_handle == NULL)
        ESP_LOGI(TAG, "BUTTON Enable failed!");

    // 启动串口0命令
    UART_0_Rx_Start();

    // gpio_config_t io_conf;
    // //disable interrupt
    // io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);


    // //start blinking
    // xTaskCreate(startBlink, "blink_task", 1024, NULL, 1, &BlinkHandle);
}
