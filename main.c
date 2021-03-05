#include <stdio.h>
#include "usb_tty_detect.h"

int usb_attached_callback(int vid, int pid, const char *port_num, const char *ttyName)
{
    printf("usb dev attached. vid: %x, pid: %x, port: %s\r\n", vid, pid, port_num);
    if (ttyName)
        printf("tty: %s\r\n", ttyName);
    return 0;
}

int usb_detached_callback(int vid, int pid, const char *port_num)
{
    printf("usb dev detached. vid: %x, pid: %x, port: %s\r\n", vid, pid, port_num);
    return 0;
}

int main(int agrc, char *agrv[])
{
    usb_tty_detect_t *tty_handle = usb_tty_detect_init(usb_attached_callback, 
                                                        usb_detached_callback);
    
    getchar();
    
    usb_tty_detect_exit(tty_handle);
    return 0;
}