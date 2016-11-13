//
// Created by keith on 2016-11-11.
//

#include "mydetect.h"
#include <android/log.h>
#include <stdio.h>
#include <string.h>

extern "C" {


// goes through the list of cameras and ports
// and tries to find a match.
// This replaces built in functionality of gphoto2
//	because libusb would crash when getting configuration.
int mydetect(CameraList*list, GPContext *context, JavaUsbDevice &javaDevice) {
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect");
    CameraAbilitiesList	*camAbilityList = NULL;
    GPPortInfoList		*portInfoList = NULL;
    int			ret, i;
    CameraList		*supportedCameraList = NULL;

    ret = gp_list_new (&supportedCameraList);
    if (ret < GP_OK) goto out;
    if (!portInfoList) {
        /* Load all the port drivers we have... */
        ret = gp_port_info_list_new (&portInfoList);
        if (ret < GP_OK) {
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_port_info_list_new");
            goto out;
        }
        ret = gp_port_info_list_load (portInfoList);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_port_info_list_load");
            goto out;
        }
        ret = gp_port_info_list_count (portInfoList);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_port_info_list_count");
            goto out;
        }
    }
    /* Load all the camera drivers we have... */
    ret = gp_abilities_list_new (&camAbilityList);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_abilities_list_new");
        goto out;
    }
    ret = gp_abilities_list_load (camAbilityList, context);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_abilities_list_load");
        goto out;
    }

    // BREAKS HERE
    /* ... and autodetect the currently attached cameras. */
    ret = abilities_list_detect(camAbilityList, portInfoList, supportedCameraList, context, javaDevice);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- abilities_list_detect -%d", ret);
        goto out;
    }
    ret = gp_list_count (supportedCameraList);
    if (ret < GP_OK) goto out;
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- gp_list_count (supportedCameraList) %d", ret);
    for (i=0;i<ret;i++) {
        const char *name, *value;

        gp_list_get_name (supportedCameraList, i, &name);
        gp_list_get_value (supportedCameraList, i, &value);
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect-add to list %s %s", name, value);
        if (!strcmp ("usb:",value)) {
            continue;
        }
        gp_list_append (list, name, value);
    }

out:
    if (portInfoList) gp_port_info_list_free (portInfoList);
    if (camAbilityList) gp_abilities_list_free (camAbilityList);
    gp_list_free (supportedCameraList);
    if (ret < GP_OK)
        return ret;
    return gp_list_count(list);
}


// replacing gp_abilities_list_detect
int abilities_list_detect(CameraAbilitiesList *list,
            GPPortInfoList *info_list, CameraList *l,
            GPContext *context, JavaUsbDevice &javaDevice) {

    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getDeviceName", "()Ljava/lang/String;");

    jobject xpathJString = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration);
    const char *xpath = javaDevice.env->GetStringUTFChars((jstring) xpathJString, 0);
    ///returns: /dev/bus/usb/001/002
    //  we want it in format: usb:{bus},{dev}
    //  http://unix.stackexchange.com/questions/108888/how-creation-of-new-usb-device-file-in-dev-bus-usb-001-directory-work
    //  so... 001 is the bus number, 002 changes each time?
    int busNum = 0;
    int devNum = 0;
    int origLength = strlen(xpath);
    int i;
    // we are assuming that its alwasy '/dev/bus/usb/'
    for (i = 13; i < origLength; i++) {
        if (xpath[i] == '/') {
            break;
        }
        busNum = busNum * 10 + ((int) xpath[i]) - 48;
    }
    i++;
    for (; i < origLength; i++) {
        if (xpath[i] == '/') {
            break;
        }
        devNum = devNum * 10 + ((int) xpath[i]) - 48;
    }
    char * outBusName = new char[256];
    sprintf(outBusName, "usb:%03d,%03d", busNum, devNum);

    GPPort *port;
    GPPortInfo info;
    gp_port_new (&port);
    gp_list_reset (l);

    int info_count = gp_port_info_list_count (info_list);

    for (int i = 0; i < info_count; i++) {
        gp_port_info_list_get_info (info_list, i, &info);
        gp_port_set_info (port, info);
        // TODO: examine port... so we choose the right one?
        char *name;
        char *path;
        gp_port_info_get_name(info, &name);
        gp_port_info_get_path(info, &path);
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- abilities_list_detect _ infocount_ index %d - name %s - path %s",
            i,
            name,
            path
        );
    }

    int ability;
    int res = abilities_list_detect_usb(list, &ability, port, javaDevice);
    if (res == GP_OK) {
        CameraAbilities abilities;
        gp_abilities_list_get_abilities(list, ability, &abilities);
        res = gp_list_append(l,
            abilities.model,
            outBusName);
        if (res != GP_OK) {
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_list_append failed %d", res);
        }
    }
    return GP_OK;

    // add in the rest;
}

// replacing gp_abilities_list_detect_usb
static int
abilities_list_detect_usb (CameraAbilitiesList *list,
			      int *ability, GPPort *port, JavaUsbDevice &javaDevice)
{
    int count = gp_abilities_list_count (list);

    for (int i = 0; i < count; i++) {
        javaDevice.env->PushLocalFrame(5);
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- abilities_list_detect_usb - index %d/%d", i, count - 1);

        CameraAbilities abilities;
        gp_abilities_list_get_abilities(list, i, &abilities);
        if (!(abilities.port & port->type)) {
            javaDevice.env->PopLocalFrame(NULL);
            continue;
        }
        int vendor = abilities.usb_vendor;
        int product = abilities.usb_product;

        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- abilities_list_detect_usb - index %d - vendor %d product %d", i, vendor, product);

        if (vendor) {
            int res = matchVendor(port, vendor, product, javaDevice);
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "matchVendor- %d", res);
            if (res == GP_OK) {
                *ability = i;
                javaDevice.env->PopLocalFrame(NULL);
                return GP_OK;
            }
        }
        int usbClass = abilities.usb_class;
        int usbSubClass = abilities.usb_subclass;
        int usbProtocol = abilities.usb_protocol;

        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- abilities_list_detect_usb - index %d - class %d subclass %d protocol %d", i, usbClass, usbSubClass, usbProtocol);
        if (usbClass) {
            // does it match class
            int res = findByClass(port, usbClass, usbSubClass, usbProtocol, javaDevice);
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "findByClass- %d", res);
            if (res == GP_OK) {
                *ability = i;
                javaDevice.env->PopLocalFrame(NULL);
                return GP_OK;
            }
        }
        javaDevice.env->PopLocalFrame(NULL);
    }
    return GP_ERROR_IO_USB_FIND;
}

// replaces: gp_libusb1_find_device_by_class_lib
int findByClass(GPPort *port, int usbClass, int usbSubClass, int usbProtocol, JavaUsbDevice &javaDevice) {
    int configIndex, interfaceIndex, altsetting;
    int ret = matchDeviceByClass(usbClass, usbSubClass, usbProtocol, &configIndex, &interfaceIndex, &altsetting, javaDevice);
    if (!ret) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- !findByClass");
        return GP_ERROR_IO_USB_FIND;
    }
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getId = javaDevice.env->GetMethodID(usbConfigurationClass, "getId", "()I");
    jmethodID getInterfaceClass = javaDevice.env->GetMethodID(usbInterfaceClass, "getInterfaceClass", "()I");
    jmethodID getInterfaceId = javaDevice.env->GetMethodID(usbInterfaceClass, "getId", "()I");
    jmethodID getAlternateSetting = javaDevice.env->GetMethodID(usbInterfaceClass, "getAlternateSetting", "()I");

    jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, configIndex);
    jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, interfaceIndex);
    jint interfaceClass = javaDevice.env->CallIntMethod(interface, getInterfaceClass);
    //if (interfaceClass == 'mass storage integer') {
    //    //TODO: log warning
    //}

    port->settings.usb.config = (int) javaDevice.env->CallIntMethod(config, getId);
    port->settings.usb.interface = (int) javaDevice.env->CallIntMethod(interface, getInterfaceId); //.bInterfaceNumber;
    port->settings.usb.altsetting = (int) javaDevice.env->CallIntMethod(interface, getAlternateSetting); //.bAlternateSetting;

    port->settings.usb.inep  = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.outep = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_OUT, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.intep = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_INTERRUPT);

    port->settings.usb.maxpacketsize = findMaxPacketSize (javaDevice, configIndex, interfaceIndex, altsetting, port->settings.usb.inep);

    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Detected defaults: config %d, interface %d, altsetting %d, "
                  "inep %02x, outep %02x, intep %02x, class %02x, subclass --",
                port->settings.usb.config,
                port->settings.usb.interface,
                port->settings.usb.altsetting,
                port->settings.usb.inep,
                port->settings.usb.outep,
                port->settings.usb.intep,
                interfaceClass
    );
    javaDevice.env->DeleteLocalRef(config);
    javaDevice.env->DeleteLocalRef(interface);
    return GP_OK;
}

// replaces: gp_libusb1_match_device_by_class
int matchDeviceByClass(int usbClass, int usbSubClass, int usbProtocol, int *configno, int *interfaceno, int *altsettingno, JavaUsbDevice &javaDevice) {
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");

    jmethodID getDeviceClass = javaDevice.env->GetMethodID(usbDeviceClass, "getDeviceClass", "()I");
    jmethodID getDeviceSubclass = javaDevice.env->GetMethodID(usbDeviceClass, "getDeviceSubclass", "()I");
    jmethodID getDeviceProtocol = javaDevice.env->GetMethodID(usbDeviceClass, "getDeviceProtocol", "()I");
    jmethodID getConfigurationCount = javaDevice.env->GetMethodID(usbDeviceClass, "getConfigurationCount", "()I");
    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterfaceCount = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterfaceCount", "()I");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getInterfaceClass = javaDevice.env->GetMethodID(usbInterfaceClass, "getInterfaceClass", "()I");
    jmethodID getInterfaceSubclass = javaDevice.env->GetMethodID(usbInterfaceClass, "getInterfaceSubclass", "()I");
    jmethodID getInterfaceProtocol = javaDevice.env->GetMethodID(usbInterfaceClass, "getInterfaceProtocol", "()I");

    int deviceClass = (int) javaDevice.env->CallIntMethod(javaDevice.device, getDeviceClass);
    int deviceSubClass = (int) javaDevice.env->CallIntMethod(javaDevice.device, getDeviceSubclass);
    int deviceProtocol = (int) javaDevice.env->CallIntMethod(javaDevice.device, getDeviceProtocol);

    if (deviceClass == usbClass &&
	    (usbSubClass == -1 ||
	     deviceSubClass == usbSubClass) &&
	    (usbProtocol == -1 ||
	     deviceProtocol == usbProtocol)) {
		return 1;
    }
    int configCount = (int) javaDevice.env->CallIntMethod(javaDevice.device, getConfigurationCount);
    for (int i = 0; i < configCount; i++) {
        jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, i);
        jint interfaceCount = javaDevice.env->CallIntMethod(config, getInterfaceCount);
        for (int j = 0; j < (int) interfaceCount; j++) {
            jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, j);
            int interfaceClass = (int) javaDevice.env->CallIntMethod(interface, getInterfaceClass);
            int interfaceSubclass = (int) javaDevice.env->CallIntMethod(interface, getInterfaceSubclass);
            int interfaceProtocol = (int) javaDevice.env->CallIntMethod(interface, getInterfaceProtocol);
            javaDevice.env->DeleteLocalRef(interface);
            if (interfaceClass == usbClass &&
            				    (usbSubClass == -1 ||
            				     interfaceSubclass == usbSubClass) &&
            				    (usbProtocol == -1 ||
            				     interfaceProtocol == usbProtocol)) {
                *configno = i;
                *interfaceno = j;
                *altsettingno = 0;

                javaDevice.env->DeleteLocalRef(config);
                return 2;
            }
        }
        javaDevice.env->DeleteLocalRef(config);

    }
    return 0;
}


// replaces: gp_libusb1_find_device_lib
int matchVendor(GPPort *port, int vendorId, int productId, JavaUsbDevice &javaDevice) {
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jmethodID getVendorId = javaDevice.env->GetMethodID(usbDeviceClass, "getVendorId", "()I");
    jint deviceVendorId = javaDevice.env->CallIntMethod(javaDevice.device, getVendorId);
    if (vendorId != (int) deviceVendorId) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - !vendor");
        return GP_ERROR_IO_USB_FIND;
    }
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - matches vendor");
    jmethodID getProductId = javaDevice.env->GetMethodID(usbDeviceClass, "getProductId", "()I");
    jint deviceProductId = javaDevice.env->CallIntMethod(javaDevice.device, getProductId);
    if (vendorId != (int) deviceProductId) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - !product");
        return GP_ERROR_IO_USB_FIND;
    }
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - matches product");

    int configIndex, interfaceIndex, altsetting;
    if (findFirstAltSetting(javaDevice, &configIndex, &interfaceIndex, &altsetting) != 0) {
        return GP_ERROR_IO_USB_FIND;
    }

    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - matches this much....");

    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getId = javaDevice.env->GetMethodID(usbConfigurationClass, "getId", "()I");
    jmethodID getInterfaceClass = javaDevice.env->GetMethodID(usbInterfaceClass, "getInterfaceClass", "()I");
    jmethodID getInterfaceId = javaDevice.env->GetMethodID(usbInterfaceClass, "getId", "()I");
    jmethodID getAlternateSetting = javaDevice.env->GetMethodID(usbInterfaceClass, "getAlternateSetting", "()I");

    jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, configIndex);
    jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, interfaceIndex);
    jint interfaceClass = javaDevice.env->CallIntMethod(interface, getInterfaceClass);
    //if (interfaceClass == 'mass storage integer') {
    //    //TODO: log warning
    //}
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- matchVendor - matches this much more...");
    port->settings.usb.config = (int) javaDevice.env->CallIntMethod(config, getId);
    port->settings.usb.interface = (int) javaDevice.env->CallIntMethod(interface, getInterfaceId); //.bInterfaceNumber;
    port->settings.usb.altsetting = (int) javaDevice.env->CallIntMethod(interface, getAlternateSetting); //.bAlternateSetting;

    port->settings.usb.inep  = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.outep = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_OUT, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.intep = findEp(javaDevice, configIndex, interfaceIndex, altsetting, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_INTERRUPT);

    port->settings.usb.maxpacketsize = findMaxPacketSize (javaDevice, configIndex, interfaceIndex, altsetting, port->settings.usb.inep);

    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Detected defaults: config %d, interface %d, altsetting %d, "
    			  "inep %02x, outep %02x, intep %02x, class %02x, subclass --",
    			port->settings.usb.config,
    			port->settings.usb.interface,
    			port->settings.usb.altsetting,
    			port->settings.usb.inep,
    			port->settings.usb.outep,
    			port->settings.usb.intep,
    			interfaceClass
    );

    javaDevice.env->DeleteLocalRef(config);
    javaDevice.env->DeleteLocalRef(interface);
    return GP_OK;
}

// replaces libusb_get_max_packet_size
int findMaxPacketSize(JavaUsbDevice &javaDevice,  int configIndex, int interfaceIndex, int altsetting, int searchEndpointAddress) {
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jclass usbEndpointClass = javaDevice.env->FindClass("android/hardware/usb/UsbEndpoint");

    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getEndpointCount = javaDevice.env->GetMethodID(usbInterfaceClass, "getEndpointCount", "()I");
    jmethodID getEndpoint = javaDevice.env->GetMethodID(usbInterfaceClass, "getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;");
    jmethodID getAddress = javaDevice.env->GetMethodID(usbEndpointClass, "getAddress", "()I");
    jmethodID getMaxPacketSize = javaDevice.env->GetMethodID(usbEndpointClass, "getMaxPacketSize", "()I");

    jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, configIndex);
    jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, interfaceIndex);

    int endpointCount = (int) javaDevice.env->CallIntMethod(interface, getEndpointCount);
    for (int i = 0; i < endpointCount; i++) {
        jobject endpoint = javaDevice.env->CallObjectMethod(interface, getEndpoint, i);
        int endpointAddress = (int) javaDevice.env->CallIntMethod(endpoint, getAddress);
        if (endpointAddress == searchEndpointAddress) {
            int ret = (int) javaDevice.env->CallIntMethod(endpoint, getMaxPacketSize);
            javaDevice.env->DeleteLocalRef(config);
            javaDevice.env->DeleteLocalRef(interface);
            javaDevice.env->DeleteLocalRef(endpoint);
            return ret;
        }
        javaDevice.env->DeleteLocalRef(endpoint);
    }

    javaDevice.env->DeleteLocalRef(config);
    javaDevice.env->DeleteLocalRef(interface);
    return -1;
}

// replaces gp_libusb1_find_ep
int findEp(JavaUsbDevice &javaDevice, int configIndex, int interfaceIndex, int altsetting, int direction, int type) {
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jclass usbEndpointClass = javaDevice.env->FindClass("android/hardware/usb/UsbEndpoint");

    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getEndpointCount = javaDevice.env->GetMethodID(usbInterfaceClass, "getEndpointCount", "()I");
    jmethodID getEndpoint = javaDevice.env->GetMethodID(usbInterfaceClass, "getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;");
    jmethodID getAddress = javaDevice.env->GetMethodID(usbEndpointClass, "getAddress", "()I");
    jmethodID getAttributes = javaDevice.env->GetMethodID(usbEndpointClass, "getAttributes", "()I");

    jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, configIndex);
    jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, interfaceIndex);

    int endpointCount = (int) javaDevice.env->CallIntMethod(interface, getEndpointCount);
    for (int i = 0; i < endpointCount; i++) {
        jobject endpoint = javaDevice.env->CallObjectMethod(interface, getEndpoint, i);
        int endpointAddress = (int) javaDevice.env->CallIntMethod(endpoint, getAddress);
        int attributes = (int) javaDevice.env->CallIntMethod(endpoint, getAttributes);

        javaDevice.env->DeleteLocalRef(endpoint);
        if (((endpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == direction) && ((attributes & LIBUSB_TRANSFER_TYPE_MASK) == type)) {
            javaDevice.env->DeleteLocalRef(config);
            javaDevice.env->DeleteLocalRef(interface);
            return endpointAddress;
        }
    }
    javaDevice.env->DeleteLocalRef(config);
    javaDevice.env->DeleteLocalRef(interface);
    return -1;
}

// replaces gp_libusb1_find_first_altsetting
int findFirstAltSetting(JavaUsbDevice &javaDevice, int *configIndex, int *interfaceIndex, int *altsetting) {
    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jmethodID getConfigurationCount = javaDevice.env->GetMethodID(usbDeviceClass, "getConfigurationCount", "()I");
    jint configCount = javaDevice.env->CallIntMethod(javaDevice.device, getConfigurationCount);

    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterfaceCount = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterfaceCount", "()I");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getEndpointCount = javaDevice.env->GetMethodID(usbInterfaceClass, "getEndpointCount", "()I");
    for (int i = 0; i < (int) configCount; i++) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- findFirstAltSetting - config %d", i);
        jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, i);

        jint interfaceCount = javaDevice.env->CallIntMethod(config, getInterfaceCount);
        for (int j = 0; j < (int) interfaceCount; j++) {
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "mydetect- findFirstAltSetting - config %d - interface %d", i, j);
            jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, j);
            jint endpointCount = javaDevice.env->CallIntMethod(interface, getEndpointCount);

            javaDevice.env->DeleteLocalRef(interface);
            if (endpointCount) {
                *configIndex = i;
                *interfaceIndex = j;
                *altsetting = 0;

                javaDevice.env->DeleteLocalRef(config);
                return 0;
            }
            // // TODO: https://developer.android.com/reference/android/hardware/usb/UsbInterface.html#getAlternateSetting()
        }
        javaDevice.env->DeleteLocalRef(config);
    }
    return -1;
}


// Initializes and opens the USB port.
int setPortInfo(GPPort *port, JavaUsbDevice &javaDevice, jobject usbManager) {

    jclass usbDeviceClass = javaDevice.env->FindClass("android/hardware/usb/UsbDevice");
    jclass usbConfigurationClass = javaDevice.env->FindClass("android/hardware/usb/UsbConfiguration");
    jclass usbInterfaceClass = javaDevice.env->FindClass("android/hardware/usb/UsbInterface");
    jclass usbManagerClass = javaDevice.env->FindClass("android/hardware/usb/UsbManager");
    jclass usbDeviceConnectionClass = javaDevice.env->FindClass("android/hardware/usb/UsbDeviceConnection");

    jmethodID getConfiguration = javaDevice.env->GetMethodID(usbDeviceClass, "getConfiguration", "(I)Landroid/hardware/usb/UsbConfiguration;");
    jmethodID getInterface = javaDevice.env->GetMethodID(usbConfigurationClass, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
    jmethodID getId = javaDevice.env->GetMethodID(usbConfigurationClass, "getId", "()I");
    jmethodID getInterfaceId = javaDevice.env->GetMethodID(usbInterfaceClass, "getId", "()I");
    jmethodID getAlternateSetting = javaDevice.env->GetMethodID(usbInterfaceClass, "getAlternateSetting", "()I");

	// not sure how this scales... as we jsut using the 1 interface and config...
	// may not work in the long run...
    jobject config = javaDevice.env->CallObjectMethod(javaDevice.device, getConfiguration, 0);
    jobject interface = javaDevice.env->CallObjectMethod(config, getInterface, 0);

    port->settings.usb.config = (int) javaDevice.env->CallIntMethod(config, getId);
    port->settings.usb.interface = (int) javaDevice.env->CallIntMethod(interface, getInterfaceId); //.bInterfaceNumber;
    port->settings.usb.altsetting = (int) javaDevice.env->CallIntMethod(interface, getAlternateSetting); //.bAlternateSetting;

    port->settings.usb.inep  = findEp(javaDevice, 0, 0, 0, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.outep = findEp(javaDevice, 0, 0, 0, LIBUSB_ENDPOINT_OUT, LIBUSB_TRANSFER_TYPE_BULK);
    port->settings.usb.intep = findEp(javaDevice, 0, 0, 0, LIBUSB_ENDPOINT_IN, LIBUSB_TRANSFER_TYPE_INTERRUPT);

    port->settings.usb.maxpacketsize = findMaxPacketSize (javaDevice, 0, 0, 0, port->settings.usb.inep);

	// get our Android File descriptor to the usb port!
    jmethodID openDevice = javaDevice.env->GetMethodID(usbManagerClass, "openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;");
    jmethodID getFileDescriptor = javaDevice.env->GetMethodID(usbDeviceConnectionClass, "getFileDescriptor", "()I");

    jobject usbDeviceConnection = javaDevice.env->CallObjectMethod(usbManager, openDevice, javaDevice.device);
    int usbFd = javaDevice.env->CallIntMethod(usbDeviceConnection, getFileDescriptor);
    port->type = GP_PORT_USB;
	// init the port and its settings with the ones we found,
	// as libusb will not be able to do it.
    gp_android_init_port(port, usbFd);
    gp_port_set_settings(port, port->settings);
    gp_port_open(port);

    return GP_OK;
}

}

