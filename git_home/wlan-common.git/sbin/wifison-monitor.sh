#!/bin/sh
POLLING_SEC=10
ALIVE_CHECK_TIMES=10 
ALIVE_CHECK_SEC=1

hyd_enable=
wsplcd_enable=

hyd_alive=
wsplcd_alive=

log_size="100k"

wlan_lockfile=/tmp/.wlan_updown_lockfile
hyd_restart_log=/tmp/hyd-restart.log
wsplcd_restart_log=/tmp/wsplcd-restart.log

hyd_restart_cnt=0
wsplcd_restart_cnt=0


restart_hyd() {
alive_cnt=0

while [ $alive_cnt -lt $ALIVE_CHECK_TIMES ]; do 
    hyd_enable=`uci get hyd.config.Enable`
    hyd_alive=`ps | grep hyd | grep -v grep`

    if [ -n "$hyd_alive" ] || [ -f $wlan_lockfile ]; then
        return
    else
        alive_cnt=$(( $alive_cnt+1 ))
    fi
    sleep $ALIVE_CHECK_SEC
done

echo "======= hyd stoped - restart hyd" > /dev/console
hyd_restart_cnt=$(( $hyd_restart_cnt+1 ))
record=`tail -c $log_size $hyd_restart_log`
timestamp=`date`
wifi_status=`iwconfig`
echo "$record" > $hyd_restart_log
echo "" >> $hyd_restart_log
echo "[hyd restart $hyd_restart_cnt times]" >> $hyd_restart_log
echo "=====  $timestamp" >> $hyd_restart_log
echo "$wifi_status" >> $hyd_restart_log
/etc/init.d/hyd restart
}


restart_wsplcd() {
alive_cnt=0

while [ $alive_cnt -lt $ALIVE_CHECK_TIMES ]; do 
    wsplcd_enable=`uci get wsplcd.config.HyFiSecurity`
    wsplcd_alive=`ps | grep wsplcd | grep -v grep`

    if [ -n "$wsplcd_alive" ] || [ -f $wlan_lockfile ] ; then
        return
    else
        alive_cnt=$(( $alive_cnt+1 ))
    fi
    sleep $ALIVE_CHECK_SEC
done

echo "======= wsplcd stoped - restart wsplcd" > /dev/console
wsplcd_restart_cnt=$(( $wsplcd_restart_cnt+1 ))
record=`tail -c $log_size $wsplcd_restart_log`
timestamp=`date`
wifi_status=`iwconfig`
echo "$record" > $wsplcd_restart_log
echo "" >> $wsplcd_restart_log
echo "[wsplcd restart $wsplcd_restart_cnt times]" >> $wsplcd_restart_log
echo "=====  $timestamp" >> $wsplcd_restart_log
echo "$wifi_status" >> $wsplcd_restart_log
/etc/init.d/wsplcd restart
}

factory_mode=`/bin/config get factory_mode`

if [ "$factory_mode" -eq 1 ]; then
    exit
fi

if [ ! -f $hyd_restart_log ] ; then
    touch $hyd_restart_log
fi

if [ ! -f $wsplcd_restart_log ] ; then
    touch $wsplcd_restart_log
fi

while [ 1 ] ; do

hyd_enable=`uci get hyd.config.Enable`
wsplcd_enable=`uci get wsplcd.config.HyFiSecurity`

hyd_alive=`ps | grep hyd | grep -v grep`
wsplcd_alive=`ps | grep wsplcd | grep -v grep`

if [ "$hyd_enable" -eq 1 ] && [ -z "$hyd_alive" ] && [ ! -f $wlan_lockfile ] ; then
    restart_hyd
fi

if [ "$wsplcd_enable" -eq 1 ] && [ -z "$wsplcd_alive" ] && [ ! -f $wlan_lockfile ]; then
    restart_wsplcd
fi

sleep $POLLING_SEC

done
