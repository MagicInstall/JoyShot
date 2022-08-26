/**
 *  户分区数据读写
 * 
 *  2022-08-01  wing    创建.
 */

#ifndef DATA_H
#define DATA_H

#include "esp_partition.h"

#ifndef DATA_PARTITIONS_TYPE
#define DATA_PARTITIONS_TYPE        0x40
#endif

#ifndef DATA_PARTITIONS_SUBTYPE
#define DATA_PARTITIONS_SUBTYPE     0x00
#endif

#ifndef DATA_PARTITIONS_NAME
#define DATA_PARTITIONS_NAME        NULL
#endif


esp_err_t Data_Init(void);

#endif
