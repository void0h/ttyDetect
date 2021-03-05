#ifndef _HGL_USB_TTY_DETECT_H
#define _HGL_USB_TTY_DETECT_H

typedef struct usb_tty_detect usb_tty_detect_t;

usb_tty_detect_t *usb_tty_detect_init(int (*usb_attached_callback)(int vid, int pid, const char *port_num, const char *ttyName),
						int (*usb_detached_callback)(int vid, int pid, const char *port_num));

int usb_tty_detect_exit(usb_tty_detect_t * tty_handle);


#endif /*_HGL_USB_TTY_DETECT_H*/
