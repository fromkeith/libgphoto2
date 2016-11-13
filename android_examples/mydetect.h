//
// Created by keith on 2016-11-11.
//

#ifndef GPHOTO_MYDETECT_H
#define GPHOTO_MYDETECT_H

#include <jni.h>
#include <libusb-1.0/libusb.h>
#include <gphoto2/gphoto2.h>

extern "C" {

struct JavaUsbDevice {
    jobject     device;
    JNIEnv*     env;
};
typedef struct JavaUsbDevice JavaUsbDevice;

int mydetect(CameraList*list, GPContext *context, JavaUsbDevice &javaDevice);
int abilities_list_detect(CameraAbilitiesList *list,
            GPPortInfoList *info_list, CameraList *l,
            GPContext *context, JavaUsbDevice &javaDevice);
static int
abilities_list_detect_usb (CameraAbilitiesList *list,
			      int *ability, GPPort *port, JavaUsbDevice &javaDevice);
int findByClass(GPPort *port, int usbClass, int usbSubClass, int usbProtocol, JavaUsbDevice &javaDevice);
int matchDeviceByClass(int usbClass, int usbSubClass, int usbProtocol, int *configno, int *interfaceno, int *altsettingno, JavaUsbDevice &javaDevice);
int matchVendor(GPPort *port, int vendorId, int productId, JavaUsbDevice &javaDevice);
int findMaxPacketSize(JavaUsbDevice &javaDevice,  int configIndex, int interfaceIndex, int altsetting, int searchEndpointAddress);
int findEp(JavaUsbDevice &javaDevice, int configIndex, int interfaceIndex, int altsetting, int direction, int type);
int findFirstAltSetting(JavaUsbDevice &javaDevice, int *configIndex, int *interfaceIndex, int *altsetting);
int setPortInfo(GPPort *port, JavaUsbDevice &javaDevice, jobject usbManager);

}

#endif //GPHOTO_MYDETECT_H

