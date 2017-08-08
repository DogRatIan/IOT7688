This is the buildsystem for the OpenWrt Linux distribution,
  modified for MT7688 module.

## Setup of Ubuntu 14

    sudo apt-get install build-essential git
    sudo apt-get install flex unzip subversion
    sudo apt-get install zlib1g-dev
    sudo apt-get install libncurses5-dev libssl-dev

## After checkout
#### Run the following feeds commands to update the feeds package.

    ./scripts/feeds update -i
    ./scripts/feeds install node
    ./scripts/feeds install snmpd
    ./scripts/feeds install openssh-server-pam
    ./scripts/feeds install ntpdate
    ./scripts/feeds install evtest
    ./scripts/feeds install i2c-tools
    ./scripts/feeds install mtk-sdk-wifi


#### Restore .config file

    git checkout .config

#### Then run make menuconfig to update the .config files.

    make menuconfig

#### At last, start the build.

    make V=s
