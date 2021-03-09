// #include <jni.h>
#include <pthread.h>
#include <dirent.h>

#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "libusb.h"
#include "usb_tty_detect.h"


struct usb_tty_detect {
    int usb_handle_task_exit;
    libusb_context *usb_context;
    pthread_t usb_handle_thread;
    libusb_hotplug_callback_handle usb_attached_handle;
    libusb_hotplug_callback_handle usb_detached_handle;
    int (*g_usb_attached_callback)(int vid, int pid, const char *port_num, const char *ttyName);
    int (*g_usb_detached_callback)(int vid, int pid, const char *port_num);
};

void *usb_handle_events_task(void *param)
{
    usb_tty_detect_t *tty_handle = (usb_tty_detect_t *)param;

    while (tty_handle && tty_handle->usb_handle_task_exit != 1) {
        usleep(100*1000);
        libusb_handle_events_completed(tty_handle->usb_context, NULL);
    }

    return (void *)0;
}

int is_slink(char *file)
{
	struct stat file_stat= {0};

	if (lstat(file, &file_stat) == -1)
			return 0;

	return S_ISLNK(file_stat.st_mode) ? 1 : 0;
}

static int LIBUSB_CALL usb_arrived_callback(struct libusb_context *ctx,
                                            struct libusb_device *dev,
                                            libusb_hotplug_event event, void *userdata)
{
    DIR *dir;
    struct dirent *ptr;
    char file[PATH_MAX] = {0};
    char link_path[PATH_MAX] = {0};

	struct libusb_device_descriptor desc;
	int bus_num;
	uint8_t port_num[32] = {0};
	char port_num_path_key[128] = {0};
	uint8_t i, port_num_len = 0;
	static char port_num_path[32] = {0};
	static char ttyName[256] = {0};
	char *p_ttyName = NULL;

    usb_tty_detect_t *tty_handle = (usb_tty_detect_t *)userdata;

	libusb_get_device_descriptor(dev, &desc);
	bus_num = libusb_get_bus_number(dev);

	sprintf(port_num_path_key, "/%d", bus_num);

	memset(port_num_path, 0, sizeof(port_num_path));
	sprintf(port_num_path, "%d", bus_num);

	port_num_len = libusb_get_port_numbers(dev, port_num, 32);
	if (port_num_len) {
		sprintf(port_num_path, "%s-", port_num_path);
		sprintf(port_num_path_key, "%s-", port_num_path_key);
		for (i = 0; i < port_num_len; ++i) {
		//		LOG_D("port_num[%d]: %d\r\n", i, port_num[i]);
			sprintf(port_num_path_key, "%s%d", port_num_path_key, port_num[i]);
			sprintf(port_num_path, "%s%d", port_num_path, port_num[i]);
			if (i+1 < port_num_len) {
				sprintf(port_num_path_key, "%s.", port_num_path_key);
				sprintf(port_num_path, "%s.", port_num_path);
			}
		}

	}
	sprintf(port_num_path_key, "%s/", port_num_path_key);

	if (desc.bDeviceClass == LIBUSB_CLASS_HUB) {
		goto OUT;
	}

	if ((dir=opendir("/sys/class/tty")) == NULL)  {
		goto OUT;
	}
//	usleep(1000*500);	//等待生成软链接
	sleep(1);
	memset(ttyName, 0, sizeof(ttyName));
	while ((ptr=readdir(dir)) != NULL) {
		if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
			continue;
		sprintf(file, "/sys/class/tty/%s", ptr->d_name);

		memset(link_path, 0, PATH_MAX);
		if (is_slink(file) && readlink(file, link_path, PATH_MAX)) {
			if (strstr(link_path, port_num_path_key)) {
				printf("dev is tty: %s\r\n", ptr->d_name);
				if (p_ttyName == NULL) {
					sprintf(ttyName, "%s", ptr->d_name);
				} else {
					sprintf(ttyName, "%s %s", ttyName, ptr->d_name);
				}
				p_ttyName = ttyName;
			}
		}
	}
	closedir(dir);
	if (p_ttyName)
		printf("ttyName: %s\r\n", p_ttyName);
OUT:
	if (tty_handle->g_usb_attached_callback)
		tty_handle->g_usb_attached_callback(desc.idVendor, desc.idProduct, port_num_path, p_ttyName);
	return 0;
}

static int LIBUSB_CALL usb_left_callback(struct libusb_context *ctx,
                                         struct libusb_device *dev,
                                         libusb_hotplug_event event, void *userdata)
{
	struct libusb_device_descriptor desc;
	int bus_num;
	uint8_t port_num[32] = {0};
	static char port_num_path[32] = {0};
	uint8_t i, port_num_len = 0;

    usb_tty_detect_t *tty_handle = (usb_tty_detect_t *)userdata;

	libusb_get_device_descriptor(dev, &desc);
	bus_num = libusb_get_bus_number(dev);


	sprintf(port_num_path, "%d", bus_num);

	port_num_len = libusb_get_port_numbers(dev, port_num, 32);
	if (port_num_len) {
		sprintf(port_num_path, "%s-", port_num_path);
		for (i = 0; i < port_num_len; ++i) {
			sprintf(port_num_path, "%s%d", port_num_path, port_num[i]);
			if (i+1 < port_num_len)
				sprintf(port_num_path, "%s.", port_num_path);
		}
	}
	if (tty_handle->g_usb_detached_callback)
		tty_handle->g_usb_detached_callback(desc.idVendor, desc.idProduct, port_num_path);
	return 0;
}

usb_tty_detect_t *usb_tty_detect_init(int (*usb_attached_callback)(int vid, int pid, const char *port_num, const char *ttyName),
						int (*usb_detached_callback)(int vid, int pid, const char *port_num))
{
	int ret = -1;
    usb_tty_detect_t *tty_handle;

	tty_handle = (usb_tty_detect_t *)malloc(sizeof(usb_tty_detect_t));
    if (NULL == tty_handle) {
        goto ERR1;
    }
    memset(tty_handle, 0x0, sizeof(usb_tty_detect_t));

	tty_handle->g_usb_attached_callback = usb_attached_callback;

	tty_handle->g_usb_detached_callback = usb_detached_callback;

	if (libusb_init(&tty_handle->usb_context) != 0) {
		printf("libusb_init failed!");
		goto ERR2;
	}

	if (pthread_create(&tty_handle->usb_handle_thread, NULL, usb_handle_events_task, tty_handle) != 0) {
		printf("pthread_create failed!");
		goto ERR3;
	}
	/*flag需要设置为LIBUSB_HOTPLUG_ENUMERATE才能检测到已插入的设备*/
	ret = libusb_hotplug_register_callback(tty_handle->usb_context,
										   LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
										   LIBUSB_HOTPLUG_ENUMERATE,
										   LIBUSB_HOTPLUG_MATCH_ANY,
										   LIBUSB_HOTPLUG_MATCH_ANY,
										   LIBUSB_HOTPLUG_MATCH_ANY,
										   usb_arrived_callback,
										   tty_handle,
										   &tty_handle->usb_attached_handle);
	if (LIBUSB_SUCCESS != ret) {
		printf("Error to register usb arrived callback");
		goto ERR4;
	}

	ret = libusb_hotplug_register_callback(tty_handle->usb_context,
											LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
											LIBUSB_HOTPLUG_NO_FLAGS,
											LIBUSB_HOTPLUG_MATCH_ANY,
											LIBUSB_HOTPLUG_MATCH_ANY,
											LIBUSB_HOTPLUG_MATCH_ANY,
											usb_left_callback,
											tty_handle,
											&tty_handle->usb_detached_handle);
	if (LIBUSB_SUCCESS != ret) {
		printf("Error to register usb left callback");
		goto ERR5;
	}

	return tty_handle;
ERR5:
	libusb_hotplug_deregister_callback(tty_handle->usb_context, tty_handle->usb_attached_handle);
ERR4:
	tty_handle->usb_handle_task_exit = 1;
	pthread_join(tty_handle->usb_handle_thread, NULL);
ERR3:
	libusb_exit(tty_handle->usb_context);
ERR2:
    free(tty_handle);
ERR1:
	return NULL;
}

int usb_tty_detect_exit(usb_tty_detect_t * tty_handle)
{
	if (tty_handle) {
		libusb_hotplug_deregister_callback(tty_handle->usb_context, tty_handle->usb_attached_handle);
		tty_handle->usb_handle_task_exit = 1;
		pthread_join(tty_handle->usb_handle_thread, NULL);
		libusb_exit(tty_handle->usb_context);

		tty_handle->g_usb_attached_callback = NULL;
		tty_handle->g_usb_detached_callback = NULL;
        free(tty_handle);
	    return 0;
	}
    return -1;
}
