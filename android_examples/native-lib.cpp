//
// Created by keith on 2016-11-10.
//
#include <android/log.h>
#include <libusb-1.0/libusb.h>
#include <gphoto2/gphoto2.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "native-lib.h"
#include "mydetect.h"
extern "C" {

// taken from regular examples on tethering.
static void
camera_tether(Camera *camera, GPContext *context, const char *home) {
	int fd, retval;
	CameraFile *file;
	CameraEventType	evttype;
	CameraFilePath	*path;
	void	*evtdata;
	char output_file[256];


	__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Tethering...\n");

	while (1) {
		retval = gp_camera_wait_for_event (camera, 1000, &evttype, &evtdata, context);
		if (retval != GP_OK)
			break;
		switch (evttype) {
		case GP_EVENT_FILE_ADDED:
			path = (CameraFilePath*)evtdata;
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "File added on the camera: %s/%s\n", path->folder, path->name);

            sprintf(output_file, "%s/%s", home, path->name);

			fd = open(output_file, O_CREAT | O_WRONLY, 0644);
			retval = gp_file_new_from_fd(&file, fd);
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "  Downloading %s...\n", path->name);
			retval = gp_camera_file_get(camera, path->folder, path->name,
				     GP_FILE_TYPE_NORMAL, file, context);

			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "  Deleting %s on camera...\n", path->name);
			retval = gp_camera_file_delete(camera, path->folder, path->name, context);
			gp_file_free(file);

			break;
		case GP_EVENT_FOLDER_ADDED:
			path = (CameraFilePath*)evtdata;
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Folder added on camera: %s / %s\n", path->folder, path->name);
			break;
		case GP_EVENT_CAPTURE_COMPLETE:
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Capture Complete.\n");
			break;
		case GP_EVENT_TIMEOUT:
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Timeout.\n");
			break;
		case GP_EVENT_UNKNOWN:
			if (evtdata) {
				__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Unknown event: %s.\n", (char*)evtdata);
			} else {
				__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Unknown event.\n");
			}
			break;
		default:
			__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Type %d?\n", evttype);
			break;
		}
	}
}



static int
_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
	int ret;
	ret = gp_widget_get_child_by_name (widget, key, child);
	if (ret < GP_OK)
		ret = gp_widget_get_child_by_label (widget, key, child);
	return ret;
}

static int
get_config_value_string (Camera *camera, const char *key, char **str, GPContext *context) {
	CameraWidget		*widget = NULL, *child = NULL;
	CameraWidgetType	type;
	int			ret;
	char			*val;

	ret = gp_camera_get_config (camera, &widget, context);
	if (ret < GP_OK) {
		__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "camera_get_config failed: %d\n", ret);
		return ret;
	}
	ret = _lookup_widget (widget, key, &child);
	if (ret < GP_OK) {
		__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "lookup widget failed: %d\n", ret);
		goto out;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	ret = gp_widget_get_type (child, &type);
	if (ret < GP_OK) {
		__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "widget get type failed: %d\n", ret);
		goto out;
	}
	switch (type) {
        case GP_WIDGET_MENU:
        case GP_WIDGET_RADIO:
        case GP_WIDGET_TEXT:
		break;
	default:
		__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "widget has bad type %d\n", type);
		ret = GP_ERROR_BAD_PARAMETERS;
		goto out;
	}

	/* This is the actual query call. Note that we just
	 * a pointer reference to the string, not a copy... */
	ret = gp_widget_get_value (child, &val);
	if (ret < GP_OK) {
		__android_log_print(ANDROID_LOG_INFO, "native-lib-con", "could not query widget value: %d\n", ret);
		goto out;
	}
	/* Create a new copy for our caller. */
	*str = strdup (val);
out:
	gp_widget_free (widget);
	return ret;
}


jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "OnLoad");
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}

void hello() {
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Hello");
}

static void
ctx_error_func (GPContext *context, const char *str, void *data)
{
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "err %s", str);
}

static void log_gphoto (GPLogLevel level, const char *domain, const char *str, void *data) {
     __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "log %s %s", domain, str);
}


static void
ctx_status_func (GPContext *context, const char *str, void *data)
{
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "status %s", str);
}

GPContext* sample_create_context() {

	GPContext *context;

	/* This is the mandatory part */
	context = gp_context_new();

	/* All the parts below are optional! */
        gp_context_set_error_func (context, ctx_error_func, NULL);
        gp_context_set_status_func (context, ctx_status_func, NULL);
        gp_log_add_func(GPLogLevel::GP_LOG_DEBUG, log_gphoto, NULL);

	return context;
}



// main start-entry point!
// ensure you have already gotten permission from android
// to open the usb before proceeding.
JNIEXPORT void JNICALL
        Java_gphoto_fromkeith_gphoto_GPhotoService_connectToDevice(
            JNIEnv *env,
            jobject obj,
            jstring libraryPath,
            jstring homePath,
            jobject device,
            jobject usbManager
        ) {
    const char *nativeString = env->GetStringUTFChars(libraryPath, 0);
    const char *nativeHomeString = env->GetStringUTFChars(homePath, 0);
    setenv("IOLIBS", nativeString, 1);
    setenv("CAMLIBS", nativeString, 1);
    setenv("HOME", nativeHomeString, 1);
    env->ReleaseStringUTFChars(libraryPath, nativeString);

    int retval;


    GPContext *context = sample_create_context();

    CameraList	*list;
    retval = gp_list_new (&list);
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_list_new - retval %d", retval);
    if (retval != GP_OK) {
        return;
    }
    JavaUsbDevice javaDevice;
    javaDevice.env = env;
    javaDevice.device = device;
	// we use JNI to detect which camera is actually usable.
    int cameraCount = mydetect(list, context, javaDevice);
    if (cameraCount <= 0) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "cameracount %d", cameraCount);
        return;
    }

    Camera	*camera;
    retval = gp_camera_new(&camera);
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_camera_new - retval %d", retval);
    if (retval != GP_OK) {
        return;
    }

    // lets start opening up the first camera
    const char	*model, *port;
    gp_list_get_name(list, 0, &model);
    gp_list_get_value (list, 0, &port);
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Camera %s is on port %s", model, port);

    CameraAbilitiesList	*abilities = NULL;
    /* Load all the camera drivers we have... */
    int ret = gp_abilities_list_new (&abilities);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_abilities_list_new %d", ret);
        return;
    }
    ret = gp_abilities_list_load (abilities, context);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_abilities_list_load %d", ret);
        return;
    }

    /* First lookup the model / driver */
    int m = gp_abilities_list_lookup_model (abilities, model);
    if (m < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_abilities_list_lookup_model %d", ret);
        return;
    }
    CameraAbilities	a;
    ret = gp_abilities_list_get_abilities (abilities, m, &a);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_abilities_list_get_abilities %d", ret);
        return;
    }
    ret = gp_camera_set_abilities (camera, a);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_camera_set_abilities %d", ret);
        return;
    }
    GPPortInfoList		*portinfolist = NULL;
    /* Load all the port drivers we have... */
    ret = gp_port_info_list_new (&portinfolist);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_port_info_list_new %d", ret);
        return;
    }
    ret = gp_port_info_list_load (portinfolist);
    if (ret < 0) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_port_info_list_load %d", ret);
        return;
    }
    ret = gp_port_info_list_count (portinfolist);
    if (ret < 0) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_port_info_list_count %d", ret);
        return;
    }

    /* Then associate the camera with the specified port */
    int p = gp_port_info_list_lookup_path (portinfolist, port);
    switch (p) {
    case GP_ERROR_UNKNOWN_PORT:
            __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "The port you specified "
                    "('%s') can not be found. Please "
                    "specify one of the ports found by "
                    "'gphoto2 --list-ports' and make "
                    "sure the spelling is correct "
                    "(i.e. with prefix 'serial:' or 'usb:').",
                            port);
            break;
    default:
            break;
    }
    if (p < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_port_info_list_lookup_path %d", p);
        return;
    }
    GPPortInfo pi;
    ret = gp_port_info_list_get_info (portinfolist, p, &pi);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_port_info_list_get_info %d", ret);
        return;
    }
    ret = gp_camera_set_port_info (camera, pi);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "gp_camera_set_port_info %d", ret);
        return;
    }

	// where we do our magically Android USB Linking
    setPortInfo(camera->port, javaDevice, usbManager);

    CameraText	text;
    char 		*owner;
    ret = gp_camera_get_summary (camera, &text, context);
    if (ret < GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con",  "Failed to get summary. %d", ret);
        return;
    }
    const char *name, *value;
    gp_list_get_name  (list, 0, &name);
    gp_list_get_value (list, 0, &value);
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "%-30s %-16s", name, value);
    __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Summary: %s", text.text);

    ret = get_config_value_string (camera, "owner", &owner, context);
    if (ret >= GP_OK) {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Owner: %s", owner);
        free (owner);
    } else {
        __android_log_print(ANDROID_LOG_INFO, "native-lib-con", "Owner: No owner found.");
    }

    camera_tether(camera, context, nativeHomeString, env, photoCallback);

    gp_camera_exit(camera, context);
}

}
