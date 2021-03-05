# ttyDetect
自动获取tty usb串口设备名,需要使用libusb库

在/sys/class/tty目录下可以看到tty设备名.并且这个软连接到了具体的设备上,软连接路径中包含了usb的端口号,可以通过libusb获取到usb的端口号,再由此获取tty设备名.当然,用了libusb可以直接使用vid pid方式读写设备

```
/sys/class/tty # ls -lh
total 0
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 console -> ../../devices/virtual/tty/console
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 ptmx -> ../../devices/virtual/tty/ptmx
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 tty -> ../../devices/virtual/tty/tty
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 tty0 -> ../../devices/virtual/tty/tty0
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 tty1 -> ../../devices/virtual/tty/tty1
lrwxrwxrwx    1 root     root           0 Jan  2 05:56 ttyAMA4 -> ../../devices/platform/soc/soc:amba/120a4000.uart/tty/ttyAMA4
...
lrwxrwxrwx    1 root     root           0 Jan  2 06:35 ttyUSB0 -> ../../devices/platform/soc/100e0000.xhci_0/usb1/1-1/1-1.4/1-1.4:1.0/ttyUSB0/tty/ttyUSB0
```

调用usb_tty_detect_init注册回调即可监听tty设备,如果不是tty设备,ttyName将返回null.