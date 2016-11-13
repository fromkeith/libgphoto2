NDK=/mnt/c/tools/linux/android-ndk-r13b
SYSROOT=/mnt/c/tools/linux/sysroot/sysroot

PATH=/mnt/c/tools/linux/sysroot/bin:$PATH

#export CXX=/mnt/c/tools/linux/sysroot/bin/arm-linux-androideabi-g++
#export CPP=/mnt/c/tools/linux/sysroot/bin/arm-linux-androideabi-cpp
#export CC=/mnt/c/tools/linux/sysroot/bin/arm-linux-androideabi-gcc
#export CFLAGS="-I/mnt/c/tools/linux/libusb/kbin/include/libusb-1.0 -I/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/include -I$SYSROOT/usr/include"
#export CPPFLAGS="--sysroot=$SYSROOT"
#export CFLAGS="-I/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/include"
#export LDFLAGS="-L/mnt/c/tools/linux/libusb/kbin/lib -L/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/lib"
#export LDFLAGS="-L/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/lib -I$SYSROOT/usr/lib"
#export LTDLINCL="-I/mnt/c/tools/linux/pocl-android-prebuilts/arm/libtool/include"
#export LIBS="-lusb-1.0 -lltdl"
#export LIBS="-lltdl"
#CPPFLAGS=
#CXXFLAGS=
#PKG_CONFIG_=
#PKG_CONFIG_PATH=
#CXXCPP=
#export build_alias=arm-linux-androideabi
#export build_os=linux-android
#export host_os=linux
#export host_alias=i686-linux-gnu


#PKG_CONFIG=$PKG_CONFIG_PATH:/mnt/c/tools/linux/libusb/kbin/lib/pkgconfig

make 
make install
