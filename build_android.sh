NDK=/mnt/c/tools/linux/android-ndk-r13b
SYSROOT=/mnt/c/tools/linux/sysroot/sysroot

PATH=$PATH:/mnt/c/tools/linux/sysroot/bin

export CFLAGS="-I/mnt/c/tools/linux/libusb-bu/kbin/include -I/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/include -I$SYSROOT/usr/include"
export CPPFLAGS="--sysroot=$SYSROOT"
export LDFLAGS="-L/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/lib -I$SYSROOT/usr/lib"

#	--with-libusb-1.0=/mnt/c/tools/linux/libusb/kbin \

./configure --with-sysroot=$SYSROOT \
    --build=i686-linux-gnu \
    --host=arm-linux-androideabi \
	--with-libusb-1.0=/mnt/c/tools/linux/libusb-bu/kbin \
    --target=arm-linux-androideabi \
    --prefix=/mnt/c/tools/linux/libghoto2/kbin/ \
    --enable-libusb-1.0  
