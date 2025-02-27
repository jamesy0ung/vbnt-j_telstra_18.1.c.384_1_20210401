/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>

#include <linux/usbdevice_fs.h>
#include <linux/usbdevice_fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif
#include <asm/byteorder.h>

#include "usb.h"

#define MAX_RETRIES 5
//#define TRACE_USB
#ifdef TRACE_USB
#define DBG1(x...) fprintf(stderr, x)
#define DBG(x...) fprintf(stderr, x)
#else
#define DBG(x...)
#define DBG1(x...)
#endif

#ifndef makedev
#define makedev MKDEV
#endif

#ifndef MKDEV
#define MKDEV(ma,mi) ((ma)<<8 | (mi))
#endif
typedef  unsigned short    U16;  

/* The max bulk size for linux is 16384 which is defined
 * in drivers/usb/core/devio.c.
 */
#define MAX_USBFS_BULK_SIZE (16 * 1024)

struct usb_handle 
{
    char fname[64];
    int desc;
    unsigned char ep_in;
    unsigned char ep_out;
};

static inline int badname(const char *name)
{
    if (!isdigit(*name))
      return 1;
    while(*++name) {
        if(!isdigit(*name) && *name != '.' && *name != '-')
            return 1;
    }
    return 0;
}


static int check(void *_desc, int len, unsigned type, int size)
{
    unsigned char *desc = _desc;
    
    if(len < size) return -1;
    if(desc[0] < size) return -1;
    if(desc[0] > len) return -1;
    if(desc[1] != type) return -1;
    
    return 0;
}
U16 le16_to_cpu ( U16 v16) {
    U16 tmp = v16;
    unsigned char *s = (unsigned char *)(&v16);
    unsigned char *d = (unsigned char *)(&tmp);
    d[0] = s[1];
    d[1] = s[0];
    
    return tmp;
}
static int filter_usb_device(char* sysfs_name,
                             char *ptr, int len, int writable,
                             ifc_match_func callback,
                             int *ept_in_id, int *ept_out_id, int *ifc_id)
{
    struct usb_device_descriptor *dev;
    struct usb_config_descriptor *cfg;
    struct usb_interface_descriptor *ifc;
    struct usb_endpoint_descriptor *ept;
    struct usb_ifc_info info;

    int in, out;
    unsigned i;
    unsigned e;
    if (check(ptr, len, USB_DT_DEVICE, USB_DT_DEVICE_SIZE))
        return -1;
    dev = (struct usb_device_descriptor *)ptr;
    len -= dev->bLength;
    ptr += dev->bLength;

    if (check(ptr, len, USB_DT_CONFIG, USB_DT_CONFIG_SIZE))
        return -1;
    cfg = (struct usb_config_descriptor *)ptr;
    len -= cfg->bLength;
    ptr += cfg->bLength;

    info.dev_vendor = dev->idVendor;
    info.dev_product = dev->idProduct;
    info.dev_class = dev->bDeviceClass;
    info.dev_subclass = dev->bDeviceSubClass;
    info.dev_protocol = dev->bDeviceProtocol;
    info.writable = writable;

    snprintf(info.device_path, sizeof(info.device_path), "usb:%s", sysfs_name);

    /* Read device serial number (if there is one).
     * We read the serial number from sysfs, since it's faster and more
     * reliable than issuing a control pipe read, and also won't
     * cause problems for devices which don't like getting descriptor
     * requests while they're in the middle of flashing.
     */
    info.serial_number[0] = '\0';
    if (dev->iSerialNumber) {
        char path[80];
        int fd;

        snprintf(path, sizeof(path),
                 "/sys/bus/usb/devices/%s/serial", sysfs_name);
        path[sizeof(path) - 1] = '\0';

        fd = open(path, O_RDONLY);
        if (fd >= 0) {
            int chars_read = read(fd, info.serial_number,
                                  sizeof(info.serial_number) - 1);
            close(fd);

            if (chars_read <= 0)
                info.serial_number[0] = '\0';
            else if (info.serial_number[chars_read - 1] == '\n') {
                // strip trailing newline
                info.serial_number[chars_read - 1] = '\0';
            }
        }
    }

    for(i = 0; i < cfg->bNumInterfaces; i++) {

        while (len > 0) {
	        struct usb_descriptor_header *hdr = (struct usb_descriptor_header *)ptr;
            if (check(hdr, len, USB_DT_INTERFACE, USB_DT_INTERFACE_SIZE) == 0)
                break;
            len -= hdr->bLength;
            ptr += hdr->bLength;
        }

        if (len <= 0)
            return -1;

        ifc = (struct usb_interface_descriptor *)ptr;
        len -= ifc->bLength;
        ptr += ifc->bLength;

        in = -1;
        out = -1;
        info.ifc_class = ifc->bInterfaceClass;
        info.ifc_subclass = ifc->bInterfaceSubClass;
        info.ifc_protocol = ifc->bInterfaceProtocol;

        for(e = 0; e < ifc->bNumEndpoints; e++) {
            while (len > 0) {
	            struct usb_descriptor_header *hdr = (struct usb_descriptor_header *)ptr;
                if (check(hdr, len, USB_DT_ENDPOINT, USB_DT_ENDPOINT_SIZE) == 0)
                    break;
                len -= hdr->bLength;
                ptr += hdr->bLength;
            }
            if (len < 0) {
                break;
            }

            ept = (struct usb_endpoint_descriptor *)ptr;
            len -= ept->bLength;
            ptr += ept->bLength;

            if((ept->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_BULK)
                continue;

            if(ept->bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
                in = ept->bEndpointAddress;
            } else {
                out = ept->bEndpointAddress;
            }

            // For USB 3.0 devices skip the SS Endpoint Companion descriptor
            if (check((struct usb_descriptor_hdr *)ptr, len,
                      USB_DT_SS_ENDPOINT_COMP, USB_DT_SS_EP_COMP_SIZE) == 0) {
                len -= USB_DT_SS_EP_COMP_SIZE;
                ptr += USB_DT_SS_EP_COMP_SIZE;
            }
        }

        info.has_bulk_in = (in != -1);
        info.has_bulk_out = (out != -1);

        if(callback(&info) == 0) {
            *ept_in_id = in;
            *ept_out_id = out;
            *ifc_id = ifc->bInterfaceNumber;
            return 0;
        }
    }

    return -1;
}
static int read_sysfs_string(const char *sysfs_name, const char *sysfs_node,
                             char* buf, int bufsize)
{
    char path[80];
    int fd, n;

    snprintf(path, sizeof(path),
             "/sys/bus/usb/devices/%s/%s", sysfs_name, sysfs_node);
    path[sizeof(path) - 1] = '\0';

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    n = read(fd, buf, bufsize - 1);
    close(fd);

    if (n < 0)
        return -1;

    buf[n] = '\0';

    return n;
}

static int read_sysfs_number(const char *sysfs_name, const char *sysfs_node)
{
    char buf[16];
    int value;

    if (read_sysfs_string(sysfs_name, sysfs_node, buf, sizeof(buf)) < 0)
        return -1;

    if (sscanf(buf, "%d", &value) != 1)
        return -1;

    return value;
}

/* Given the name of a USB device in sysfs, get the name for the same
 * device in devfs. Returns 0 for success, -1 for failure.
 */
static int convert_to_devfs_name(const char* sysfs_name,
                                 char* devname, int devname_size)
{
    int busnum, devnum;

    busnum = read_sysfs_number(sysfs_name, "busnum");
    if (busnum < 0)
        return -1;

    devnum = read_sysfs_number(sysfs_name, "devnum");
    if (devnum < 0)
        return -1;

    snprintf(devname, devname_size, "/dev/bus/usb/%03d/%03d", busnum, devnum);
    return 0;
}

static usb_handle *find_usb_device(const char *base, ifc_match_func callback)
{
    usb_handle *usb = 0;
    char devname[64];
    char desc[1024];
    int n, in, out, ifc;

    DIR *busdir;
    struct dirent *de;
    int fd;
    int writable;

    busdir = opendir(base);
    if(busdir == 0) return 0;

    while((de = readdir(busdir)) && (usb == 0)) {
        if(badname(de->d_name)) continue;

        if(!convert_to_devfs_name(de->d_name, devname, sizeof(devname))) {

            writable = 1;
            if((fd = open(devname, O_RDWR)) < 0) {
                // Check if we have read-only access, so we can give a helpful
                // diagnostic like "adb devices" does.
                writable = 0;
                if((fd = open(devname, O_RDONLY)) < 0) {
                    continue;
                }
            }

            n = read(fd, desc, sizeof(desc));

            if(filter_usb_device(de->d_name, desc, n, writable, callback,
                                 &in, &out, &ifc) == 0) {
                usb = calloc(1, sizeof(usb_handle));
                strcpy(usb->fname, devname);
                usb->ep_in = in;
                usb->ep_out = out;
                usb->desc = fd;

                n = ioctl(fd, USBDEVFS_CLAIMINTERFACE, &ifc);
                if(n != 0) {
                    close(fd);
                    free(usb);
                    usb = 0;
                    continue;
                }
            } else {
                close(fd);
            }
        }
    }
    closedir(busdir);

    return usb;
}

int usb_write(usb_handle *h, const void *_data, int len, int fd)
{
    unsigned char *data = (unsigned char*) _data;
    unsigned count = 0;
    struct usbdevfs_bulktransfer bulk;
    int n;
    int use_fd = 0;

    if(_data == 0 && fd != 0)
    {
#define     BULK_SIZE	1024
    	use_fd = 1;
    	data = (char*)malloc(BULK_SIZE);
    	if(data == 0)
    	{
    		return -1;
    	}
    }

    if(h->ep_out == 0) {
        return -1;
    }
    
    if(len == 0) {
        bulk.ep = h->ep_out;
        bulk.len = 0;
        bulk.data = data;
        bulk.timeout = 0;
        
        n = ioctl(h->desc, USBDEVFS_BULK, &bulk);
        if(n != 0) {
            fprintf(stderr,"ERROR: n = %d, errno = %d (%s)\n",
                    n, errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    
    while(len > 0) {
        int xfer;

        if(use_fd)
        {
        	xfer = (len > BULK_SIZE) ? BULK_SIZE : len;
        }else
        {
        	xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;
        }
        
        bulk.ep = h->ep_out;
        bulk.len = xfer;
        if(use_fd)
        {
        	n = read(fd, data, xfer);
        }
        bulk.data = data;
        bulk.timeout = 0;
        
        n = ioctl(h->desc, USBDEVFS_BULK, &bulk);
        if(n != xfer) {
            DBG("ERROR: n = %d, errno = %d (%s)\n",
                n, errno, strerror(errno));
            return -1;
        }
        count += xfer;
        len -= xfer;
        if(!use_fd)
        	data += xfer;
    }

	if(use_fd)
	{
		free(data);
		data = 0;
	}
    return count;
}

int usb_read(usb_handle *h, void *_data, int len)
{
    unsigned char *data = (unsigned char*) _data;
    unsigned count = 0;
    struct usbdevfs_bulktransfer bulk;
    int n, retry;

    if(h->ep_in == 0) {
        return -1;
    }
    
    while(len > 0) {
        int xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;
        
        bulk.ep = h->ep_in;
        bulk.len = xfer;
        bulk.data = data;
        bulk.timeout = 0;
        retry = 0;

        do{
           DBG("[ usb read %d fd = %d], fname=%s\n", xfer, h->desc, h->fname);
           n = ioctl(h->desc, USBDEVFS_BULK, &bulk);
           DBG("[ usb read %d ] = %d, fname=%s, Retry %d \n", xfer, n, h->fname, retry);

           if( n < 0 ) {
            DBG1("ERROR: n = %d, errno = %d (%s)\n",n, errno, strerror(errno));
            if ( ++retry > MAX_RETRIES ) return -1;
            sleep( 1 );
           }
        }
        while( n < 0 );

        count += n;
        len -= n;
        data += n;
        
        if(n < xfer) {
            break;
        }
    }
    
    return count;
}

void usb_kick(usb_handle *h)
{
    int fd;

    fd = h->desc;
    h->desc = -1;
    if(fd >= 0) {
        close(fd);
        DBG("[ usb closed %d ]\n", fd);
    }
}

int usb_close(usb_handle *h)
{
    int fd;
    
    fd = h->desc;
    h->desc = -1;
    if(fd >= 0) {
        close(fd);
        DBG("[ usb closed %d ]\n", fd);
    }

    return 0;
}
int charsplit(const char *src,char* desc,int n,const char* splitStr)
{
	char* p;
	char*p1;
	int len;
	
	len=strlen(splitStr);
	p=strstr(src,splitStr);
	if(p==NULL)
		return -1;
	p1=strstr(p,"\n");
	if(p1==NULL)
		return -1;
	memset(desc,0,n);
	memcpy(desc,p+len,p1-p-len);
	
	return 0;
}
int  CreateDir(const   char   *sPathName)  
{  
	char DirName[256];  
	strcpy(DirName,sPathName);  
	int  i,len  = strlen(DirName);  
	if(DirName[len-1]!='/')  
	strcat(DirName,   "/");  
	len=strlen(DirName);  
	for(i=1; i<len;i++)  
	{  
		if(DirName[i]=='/')  
		{  
			DirName[i]   =   0;  
			if(opendir(DirName)==0)  
			{  
				if(mkdir(DirName,   0755)==-1)  
				{   
					printf("mkdir %s\n",DirName);   
					return   -1;   
				}  
			}  
			DirName[i]   =   '/';  
		}  
	}  
	return   0;  
} 

int usb_check()
{
	int re=0;
	char* base="/sys/bus/usb/devices";
	struct dirent *de;
	char busname[64], devname[64];
	int fd;
	int writable;
	int n;
	DIR *busdir, *devdir;
	char desc[1024];
	char busnum[64],devnum[64],devmajor[64],devminor[64];
	char buspath[128],devpath[128];
	busdir = opendir(base);
	if(busdir == 0) return 0;
	while(de = readdir(busdir)) {
		sprintf(busname, "%s/%s", base, de->d_name);
		devdir = opendir(busname);
		if(devdir == 0) continue;
		while((de = readdir(devdir)) ) {
			 sprintf(devname, "%s/%s", busname, de->d_name);
			  if(strstr(devname,"uevent")!=NULL)
			  	{
			  	 	writable = 1;
			  		if((fd = open(devname, O_RDWR)) < 0) {
						writable = 0;
						if((fd = open(devname, O_RDONLY)) < 0)
							continue;
					}
					memset(desc,0,1024);
					n = read(fd, desc, sizeof(desc));
					desc[n-1]='\n';
					desc[n]='\0';
					if(strstr(desc,"18d1/d00d")!=NULL&&strstr(desc,"DEVTYPE=usb_device")!=NULL)
					{
						if(charsplit(desc,busnum,64,"BUSNUM=")==-1||
						charsplit(desc,devnum,64,"DEVNUM=")==-1||
						charsplit(desc,devmajor,64,"MAJOR=")==-1||
						charsplit(desc,devminor,64,"MINOR=")==-1)
						{
							printf("[FASTBOOT] Split String Error\n");
							return 0;
						}
						memset(buspath,0,128);
						memset(devpath,0,128);
						sprintf(buspath,"/dev/bus/usb/%s",busnum);
						sprintf(devpath,"/dev/bus/usb/%s/%s",busnum,devnum);
                        
                       if (access(buspath, R_OK)) {
							if(CreateDir(buspath)!=0)
							{
								printf("[FASTBOOT] Create dir[%s] failed\n",buspath);
								return -1;
							}
                       }
                                            
						if (!access(buspath, R_OK) && access(devpath, R_OK)) {												
                            printf("mknod %s, MAJOR = %s, MINOR =%s\n", devpath, devmajor,devminor);
							if((0 != mknod(devpath,  S_IFCHR|0666, makedev(atoi(devmajor),atoi(devminor)))))
							{
								printf("mknod for %s failed, MAJOR = %s, MINOR =%s, errno = %d(%s)\n", devpath, devmajor,devminor, errno, strerror(errno));
								return -1;
							}
						}
					}					
			  	}
		}
	}
	return 0;
}
usb_handle *usb_open(ifc_match_func callback)
{
    return find_usb_device("/sys/bus/usb/devices", callback);
}
