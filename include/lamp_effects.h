/*
 * @Author: wing zlcnkkm@21cn.com
 * @Date: 2022-08-19 17:53:14
 * @LastEditors: wing zlcnkkm@21cn.com
 * @LastEditTime: 2022-08-20 20:47:50
 * @FilePath: /JoyShot/include/ lamp_effects.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 *  预定义的动态光效
 * 
 *  2022-08-19    wing    创建.
 */

#ifndef LAMP_EFFECTS_H
#define LAMP_EFFECTS_H

#include "lamp.h"

/*-----------------------------------------------------------------*/ 

uint32_t Gradient_Red_Frames[] = {
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

WS2812_COLOR_t Gradient_Red_Table[] = {
    // 全部半帧
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

//  红色渐亮
Lamp_Effect_t Gradient_Red_Effect = {
    {
    .freq       = 25,
    .clipping   = false,
    .virginity  = true,
    },
    .frameMode  = LAMP_EFFECT_FRAME_MODE_SINGLE,
    // .direction  = LAMP_EFFECT_FRAME_DIRECTION_FORWARD,
    .current    = 0,
    .startMode  = LAMP_EFFECT_STATE_MODE_RESTART,
    .frames     = Gradient_Red_Frames,
    .length     = sizeof(Gradient_Red_Frames) / sizeof(uint32_t),
    .table      = Gradient_Red_Table,
    .semaphore  = NULL,
};

/*-----------------------------------------------------------------*/

uint32_t Green_Running_Frames[] = {
     0,  2,  4,  6,  8, 10, 12, 14, 
    16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 
};
    

WS2812_COLOR_t Green_Running_Table[] = {
    {.rgb = 0x001400}, {.rgb = 0x000000},
    {.rgb = 0x003000}, {.rgb = 0x000000},
    {.rgb = 0x004C00}, {.rgb = 0x000000},
    {.rgb = 0x006800}, {.rgb = 0x000000},
    {.rgb = 0x008400}, {.rgb = 0x001400}, 
    {.rgb = 0x00A000}, {.rgb = 0x003000}, 
    {.rgb = 0x00BC00}, {.rgb = 0x004C00}, 
    {.rgb = 0x00D800}, {.rgb = 0x006800}, 
    {.rgb = 0x00BC00}, {.rgb = 0x008400},
    {.rgb = 0x00A000}, {.rgb = 0x00A000},
    {.rgb = 0x008400}, {.rgb = 0x00BC00}, 
    {.rgb = 0x006800}, {.rgb = 0x00D800},
    {.rgb = 0x004C00}, {.rgb = 0x00BC00},
    {.rgb = 0x003000}, {.rgb = 0x00A000},
    {.rgb = 0x000000}, {.rgb = 0x008400},
    {.rgb = 0x000000}, {.rgb = 0x006800},
    {.rgb = 0x000000}, {.rgb = 0x004C00},
    {.rgb = 0x000000}, {.rgb = 0x003000},
};

//  绿色流水
Lamp_Effect_t Green_Running_Effect = {
    {
    .freq       = 25,
    .clipping   = false,
    .virginity  = true,
    },
    .frameMode  = LAMP_EFFECT_FRAME_MODE_TO_AND_FRO,
    .direction  = LAMP_EFFECT_FRAME_DIRECTION_FORWARD,
    .current    = 0,
    .startMode  = LAMP_EFFECT_STATE_MODE_RESTART,
    .frames     = Green_Running_Frames,
    .length     = sizeof(Green_Running_Frames) / sizeof(uint32_t),
    .table      = Green_Running_Table,
    .semaphore  = NULL,
};

/*-----------------------------------------------------------------*/
uint32_t Player_Light_Frames[] = {
    0,
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    LAMP_EFFECT_HOLDER_FRAME, 
    0, 2, 0, 2, 0, 2, 0, 2, 0, 
};
    

WS2812_COLOR_t Player_Light_Table[] = {
    {.rgb = 0x000000}, {.rgb = 0x000000},
    {.rgb = 0x00FF00}, {.rgb = 0x00FF00},
};

//  手柄号
Lamp_Effect_t Player_Light_Effect = {
    {
    .freq       = 6,
    .clipping   = true,
    .virginity  = true,
    },
    .frameMode  = LAMP_EFFECT_FRAME_MODE_FORWARD,
    // .direction  = LAMP_EFFECT_FRAME_DIRECTION_FORWARD,
    .current    = 0,
    .startMode  = LAMP_EFFECT_STATE_MODE_RESTART,
    .frames     = Player_Light_Frames,
    .length     = sizeof(Player_Light_Frames) / sizeof(uint32_t),
    .a          = 0,
    .b          = (sizeof(Player_Light_Frames) / sizeof(uint32_t)) - 1,
    .table      = Player_Light_Table,
    .semaphore  = NULL,
};

/*-----------------------------------------------------------------*/

uint32_t Heartbeat_Blue_Frames[] = {
    // 第一下
    12, 8, 2,  0,   
    // 第二下开始渐弱
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 
    10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  
     0, 
};

WS2812_COLOR_t Heartbeat_Blue_Table[] = {
    // 整帧
    /*  0  */{.rgb = 0x000000}, {.rgb = 0x000000}, 
    // 后边渐弱是半帧
    /*  2  */{.rgb = 0x000003}, {.rgb = 0x000011}, {.rgb = 0x00001F}, {.rgb = 0x00002D}, {.rgb = 0x00003B}, {.rgb = 0x000049}, {.rgb = 0x000057}, {.rgb = 0x000065}, {.rgb = 0x000073}, {.rgb = 0x000081}, 
    /*  12 */{.rgb = 0x00008F}, {.rgb = 0x00009D}, {.rgb = 0x0000AB}, {.rgb = 0x0000B9}, {.rgb = 0x0000C7}, {.rgb = 0x0000D5}, {.rgb = 0x0000E3}, {.rgb = 0x0000F1},
    /*  20 */{.rgb = 0x0000FF}, {.rgb = 0x0000FF}/*最后一帧补够两个像素*/, 
};

//  蓝色心跳
Lamp_Effect_t Heartbeat_Blue_Effect = {
    {
    .freq       = 25,
    .clipping   = false,
    .virginity  = true,
    },
    .frameMode  = LAMP_EFFECT_FRAME_MODE_FORWARD,
    // .direction  = LAMP_EFFECT_FRAME_DIRECTION_FORWARD,
    .current    = 0,
    .startMode  = LAMP_EFFECT_STATE_MODE_RESTART,
    .frames     = Heartbeat_Blue_Frames,
    .length     = sizeof(Heartbeat_Blue_Frames) / sizeof(uint32_t),
    .table      = Heartbeat_Blue_Table,
    .semaphore  = NULL,
};
/*-----------------------------------------------------------------*/

#endif
