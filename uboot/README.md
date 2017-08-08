# uboot
This is the UBoot bootloader source for IOT7688.

# Compile
Please install the cross toolchain required to build the source

`sudo tar xjf buildroot-gcc342.tar.bz2 -C /opt/`

Start building the source

`make`

Notes: Uboot firmware is uboot.bin (or lks7688.ldr). NOT uboot.img


For Ubuntu 16.04 64bit system, you need to add the i386 lib to run the toolchain.

`sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386`
