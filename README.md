# JoyShot
开发环境基于macos + VScode + Platform IO，项目使用了[魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)。

# 笔记

* 不需要乐鑫那个各种设置的教程，运行menuconfig 只需直接在Pio 的终端运行：<br/>
pio run -t menuconfig<br/>
按S 保存后会更新工程目录下的sdkconfig.esp32dev，但是sdkconfig.h 不会同步更新，得先build 一次，在工程的".pio/build/esp32dev/config" 目录下便生成了新的sdkconfig.h，需要手动替换掉工程中旧的sdkconfig.h，以使语法正确着色。

* 在系统终端打印串口（也可以在Pio 的终端）<br/>
screen /dev/cu.usbserial-14401 115200,8,N,1<br/>

# 更新
* 2021-07-18  Wing<br/>
[魔改再魔改的IDF](https://github.com/MagicInstall/esp-idf-NathanReeves-pio.git)目前还未完成，虽然可以编译通过，但不能正常运行。
