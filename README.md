# JoyShot
开发环境基于macos + VScode + Platform IO，项目使用了[魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)。

# 笔记

* 不需要乐鑫那个各种设置的教程，运行menuconfig 只需直接在Pio 的终端运行：<br/>
pio run -t menuconfig<br/>
按S 保存后会更新工程目录下的sdkconfig.esp32dev，但是sdkconfig.h 不会同步更新，得先build 一次，在工程的".pio/build/esp32dev/config" 目录下便生成了新的sdkconfig.h，需要手动替换掉工程中旧的sdkconfig.h，以使语法正确着色。

* 在系统终端打印串口（也可以在Pio 的终端）<br/>
screen /dev/cu.usbserial-14401 115200,8,N,1<br/>

* 在系统终端向串口发送控制命令（也可以在Pio 的终端）<br/>
echo LR1 > /dev/cu.usbserial-14401<br/>
具体命令在main.c 里定义。<br/>

# 更新
* <s>2021-07-18  Wing<br/>
[魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)目前还未完成，虽然可以编译通过，但不能正常运行。</s>
* 2021-08-11 Wing<br/>
[魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)还有些问题未解决；目前可以连接上Switch，通过按键运行脚本。
* 2021-08-25 Wing<br/>
-- 特别感谢@elmagnificogi 的[解答](https://github.com/elmagnificogi/NS_joycon_auto_script_esp32/issues/1)!!!直接推进了进度!感谢!!!<br/>
-- [魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)还有些问题未解决，主动连接NS 只是通过BTA 层实现，若NS 在关机状态下也可以连上但报错：<br/>
(7147) BT_HIDD: hidd_l2cif_connect_cfm: connection failed, now disconnect<br/>
(7147) BT_HIDD: hidd_conn_disconnect: already disconnected<br/>
，感觉这个状况可能大概可以做home 键唤醒，但目前还没有头绪...；<br/>
-- 蓝牙部分拆分到另一个文件，再包装了一层。
* 2022-08-27 Wing<br/>
基本完成WS2812 和CPC4051 的驱动；封装了光污染。<br/>

# 目标（按顺序）
* OTA；
* 在OTA 的HTTPS Server 上添加脚本加载（甚至在线编辑）功能；
* Home 键唤醒Switch；
* ☑️P1-P4 LED；(一不小心解决了囧)
* ☑️手柄颜色；(一不小心解决了囧)

# 参考
[https://github.com/NathanReeves/BlueCubeMod](https://github.com/NathanReeves/BlueCubeMod)<br/>
[https://github.com/elmagnificogi/NS_joycon_auto_script_esp32](https://github.com/elmagnificogi/NS_joycon_auto_script_esp32)<br/>
[https://github.com/mizuyoukanao/UARTSwitchCon](https://github.com/mizuyoukanao/UARTSwitchCon)<br/>
