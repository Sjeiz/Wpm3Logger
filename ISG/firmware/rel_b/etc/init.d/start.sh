#!/bin/sh
#
export LD_LIBRARY_PATH=$PWD/lib
ISG_DEBUG="-v5"


#setup
date 121000002019
mkdir -p $PWD/../log
ln -sf $PWD /var/volatile/firmware

#default firmware timezone UTC for simple device time
#NOT NEEDED: export TZ="UTC"

#setup product branding
FW_BRANDING="$(fw_printenv -n FW_branding)"
[ -z ${FW_BRANDING} ] && FW_BRANDING="UNKNOWN"
echo ${FW_BRANDING} > /var/cache/firmware-manufacturer
export FW_BRANDING

#check configuration and create if not exist
mkdir -p /firmware/config
check_config()
{
    [ -f /firmware/config/$1 ] || { cp -a $PWD/etc/default/$1 /firmware/config ; sync ; }
}
save_config()
{
    [ -f /firmware/config/$1 ] && cp -a /firmware/config/$1 /tmp
}
restore_config()
{
    [ -f /tmp/$1 ] && mv /tmp/$1 /firmware/config
}
[ -f /var/cache/setupdefaults ] && {
    save_config libisgclient.cfg
    save_config portal-user-data
    save_config portal-using
    rm -f /firmware/config/*
    cp -a $PWD/etc/default/* /firmware/config
    restore_config libisgclient.cfg
    restore_config portal-user-data
    restore_config portal-using
    /etc/fwsystem/sysreset
}
check_config global
check_config libisgclient.cfg
check_config modem
check_config network
check_config server.ca

#network structure to ramdisk
[ -f /var/cache/rootfs-mount-etc ] || {
    mkdir -p /var/volatile/etc
    cp -a /etc/network etc/network /etc/ppp bin/udhcpc.helper.bound bin/udhcpc.helper.leasefail bin/php.wrapper etc/php.ini /var/volatile/etc
    mount --bind /var/volatile/etc/network /etc/network
    mount --bind /var/volatile/etc/ppp /etc/ppp
    touch /var/cache/rootfs-mount-etc
}

#network start
etc/init.d/network start

#modem start
#etc/init.d/modem_start.sh

#default settings "NO_DEVICE"
cp -a etc/config/config.json.template /var/cache
cp -a etc/config/isg_server.cfg /var/cache

#start htpdate daemon
bin/htpdate -d -t -s servicewelt.stiebel-eltron.de

#hier comes the new code for SSH
#script to start sshd with private key access

remount rw /dev/mtdblock1 /

cp /firmware/sshd_config /etc/ssh/sshd_config
mkdir /home/root/.ssh
chmod -R 700 /home/root
cp /firmware/root/.ssh/authorized_keys /home/root/.ssh/authorized_keys
chmod 600 /home/root/.ssh/authorized_keys

#ssh-start
sh /etc/init.d/sshd start

remount ro /dev/mtdblock1 /

#start webserver with php.wrapper and php.ini
ln -sf $PWD/www /var/volatile/www
/etc/init.d/lighttpd start
#end ssh block

#set link for isg-manual
[ -h www/doc/bedienungsanleitungen ] || {
	ln -sf /firmware/manual www/doc/bedienungsanleitungen
}

if [ ! -f /firmware/config/driver_mbc.cfg ] ; then
    ### USE CAN DRIVER ###
    #identify can-bus connected devices - always call "device_identifier"
    bin/device_canbus -v7
    BITRATE="$(cat /var/cache/canbus-bitrate)"
    if [ "x$BITRATE" = "x50" ] ; then
        bin/device_identifier2 $ISG_DEBUG
    else
        bin/device_identifier $ISG_DEBUG
    fi
else
    ### USE MODBUS DRIVER ###
    #NOT here: identify modbus connected devices - discovery startet from WEB-UI
    #NOT here: bin/device_identifier_mbc

    #create server configuration
    bin/isg_server_config $ISG_DEBUG
fi

#start isg server
bin/isg_server $ISG_DEBUG

#start handler
bin/isg_fh $ISG_DEBUG
bin/isg_ph $ISG_DEBUG
bin/isg_button $ISG_DEBUG
bin/isg_dblh $ISG_DEBUG
bin/isg_dlh $ISG_DEBUG
bin/isg_ulh $ISG_DEBUG
[ -x bin/isg_see ] && bin/isg_see $ISG_DEBUG
[ -x bin/isg_sgreadyh ] && bin/isg_sgreadyh $ISG_DEBUG
[ -x bin/isg_mbh ] && bin/isg_mbh $ISG_DEBUG
[ -x etc/init.d/start-knx.sh ] && etc/init.d/start-knx.sh $ISG_DEBUG &
bin/isg_wdh $ISG_DEBUG
bin/isg_tester -s 65000 32768
#bin/isg_tester -s 65000 1
#ssh-start
if [ "x$BITRATE" = "x50" ] ; then
    bin/canupdater $ISG_DEBUG &
fi
exit 0
