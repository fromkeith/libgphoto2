
This Fork
---------

This fork gets libgphoto2 building for android.



### Differences:
- Added an android_init function to libusb1.
- Added gp_android_init_port to gphoto2 so we can pass through the file descriptor of the usb port.
- Port library's are linked against math ('m'). Otherwise we crash at runtime...
- Changed the libusb open call to use a modified version of libusb
	- See: https://github.com/martinmarinov/rtl_tcp_andro-
- Locally the make file here was modify to change the library name to libusb-1.0.so
- See android_examples/
	- for some really rough JNI bridge code

### Extra File
- build_android.sh
	- Configures the build for armeabi
	- Be sure to change the paths to your libusb location, and ndk locations.
- make_android.sh
	- makes and installs the library
	- Be sure to change the paths as necessary.

### Extra work required
- The .so files that get output are versioned... android doesn't like this.
	- This will help you fix the shared libraries so they can be loaded by android
```		
	rpl -R -e libgphoto2.so.6 "libgphoto2.so\x00\x00" kbin/lib/libgphoto2.so
	rpl -R -e libgphoto2_port.so.12 "libgphoto2_port.so\x00\x00\x00" kbin/lib/libgphoto2.so
	rpl -R -e libgphoto2_port.so.12 "libgphoto2_port.so\x00\x00\x00" kbin/lib/libgphoto2_port.re.so
``

- The camera and port libraries that get output aren't prefixed with 'lib'... android doesn't like this.
	- They also reference the versioned shared libraries...
```
	rpl -R -e libgphoto2.so.6 "libgphoto2.so\x00\x00" kbin/lib/libgphoto2/2.5.10/libptp2.so
	rpl -R -e libgphoto2_port.so.12 "libgphoto2_port.so\x00\x00\x00" kbin/lib/libgphoto2/2.5.10/libptp2.so
	rpl -R -e libgphoto2_port.so.12 "libgphoto2_port.so\x00\x00\x00" kbin/lib/libgphoto2_port/0.12.0/libusb1.so
```

### Output
- Put the shared libraries into your android project - jni- libs...
	- libgphoto2.so
	- libgphoto2_port.so
	- libptp2.so
	- libusb-1.0.so
	- libusb1.so
- In your JNI native code you need to set the following environment variables
	- IOLIBS
	- CAMLIBS
	- Both of this environemnt variables will need to point to your apk's arm libs folder.

### Other notes
- Yes, there are probably more elagant ways to get this library working... but hey..
- Libusb doesn't seem to properly get the descriptor and configuration of usbs on android.
	- This can instead be done using JNI to call android.
	- That will allow you to see which cams are supported.
- GPL of course...
- libtool from here
	- https://github.com/krrishnarraj/pocl-android-prebuilts
- See android_examples/




