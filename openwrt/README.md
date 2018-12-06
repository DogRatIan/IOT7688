This is the buildsystem for the OpenWrt Linux distribution,
  modified for IOT7688 module.

    Serial console is UART2, 115200 8N1.
    Login user: root
    Login password: root

## Setup of Ubuntu 16.04

    sudo apt-get install build-essential git
    sudo apt-get install flex unzip subversion
    sudo apt-get install zlib1g-dev
    sudo apt-get install libncurses5-dev libssl-dev

## After checkout
#### Run the following feeds commands to update the feeds package.

    ./scripts/feeds update -i
    ./scripts/feeds install node openssh-server-pam ntpdate mtk-sdk-wifi
    ./scripts/feeds install snmpd
    ./scripts/feeds install evtest i2c-tools

#### Restore .config file

    git checkout .config

#### Start the build.

    make V=s

When done, a image file called "lks7688.img" will generated at "bin/ramips".