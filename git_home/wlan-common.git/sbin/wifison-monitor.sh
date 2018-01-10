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

hyctl flushatbl br0
hyctl flushdtbl br0
/etc/init.d/hyfi-bridging start
/etc/init.d/hyd restart
}

#to fix wifi interface down or disappeared problem.this just a workaround
wlan_updown_lock=/tmp/.wlan_updown_lockfile
wlan_updown_time=/tmp/.wlan_updown_time
wifi_restart_event_time=/tmp/.wifi_restart_event_time
wifi_restart_event(){
    if [ ! -f $wifi_restart_event_time ];then
        date +%s > $wifi_restart_event_time   
        echo "[$0]time:`date +%s` first time find wifi abnormal,wait and check it again.reason:$2" > /dev/console
        return 
    fi
    if [ `cat $wifi_restart_event_time` -lt $((`date +%s`-30))  ];then
        date +%s > $wifi_restart_event_time   
        echo "[$0]time:`date +%s` first time find wifi abnormal,wait and check it again.reason:$2" > /dev/console
        return 
    fi
        
    #if we use this function to restart wifi and it still has problem,we restart hostapd
    if [ -f $wlan_updown_time ]&&[ "`cat $wlan_updown_time`" -gt $((`date +%s`-180))  ];then
        echo "================[$0]wlan down/up can not fixed problem,request hostapd restart=============" >/dev/console
        echo "==========[$0]$1:wlan down/up call by $2" >/dev/console
        rm -f $wlan_updown_time
        rm -f $wifi_restart_event_time
        killall hostapd
        hostapd -g /var/run/hostapd/global -B -P /var/run/hostapd-global.pid
        ifname_hostapd=${ifname_fronthaul_ap}" "${ifname_backhaul_ap}" "${ifname_guestnetwork_ap}
        for ifname in $ifname_hostapd
        do
            wpa_cli -g /var/run/hostapd/global raw ADD bss_config=$ifname:/var/run/hostapd-$ifname.conf
        done
        wlan updateconf
        wlan down
        wlan up
        return 0
    fi
    echo "==========[$0]$1:wlan down/up call by $2" >/dev/console
    echo "===============ifconfig===================" >/dev/console
    ifconfig > /dev/console
    echo "===============iwconfig===================" >/dev/console
    iwconfig > /dev/console
    echo "===============ps=========================" >/dev/console
    ps >/dev/console
    date +%s >$wlan_updown_time
    rm -f $wifi_restart_event_time
    wlan updateconf
    wlan down
    wlan up
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

if [ "x$(/usr/sbin/ebtables -t filter -L |grep ath0)" = "x" -a "x$(cat /module_name)" = "xRBS50" ];then
	/usr/sbin/ebtables -t filter -A INPUT -p 0x893A -i ath0 -j DROP
	/usr/sbin/ebtables -t filter -A INPUT -p 0x893A -i ath1 -j DROP
fi

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

[ -f $wlan_lockfile ] && continue

if [ `cat /tmp/orbi_type` = "Base" ];then
	wl2g_backhaul=`config get wl2g_BACKHAUL_AP`
	wl5g_backhaul=`config get wl5g_BACKHAUL_AP`
else
	wl2g_backhaul=`config get wl2g_BACKHAUL_STA`
	wl5g_backhaul=`config get wl5g_BACKHAUL_STA`
fi

wl2g_backhaul_not_connected=`iwconfig $wl2g_backhaul 2>/dev/null | grep "Not-Associated"`
wl5g_backhaul_not_connected=`iwconfig $wl5g_backhaul 2>/dev/null | grep "Not-Associated"`


if [ "${wl2g_backhaul_not_connected}" -a "${wl5g_backhaul_not_connected}" ]; then
    echo "backhaul not connected"
else
    
    enable_24G=`config get endis_wl_radio`
    enable_5G=`config get endis_wla_radio`
    wifischedule_24G=`config get wladv_schedule_enable`
    wifischedule_5G=`config get wladv_schedule_enable_a`
    enable_24G_guestnetwork=`config get wlg1_endis_guestNet`
    enable_5G_guestnetwork=`config get wla1_endis_guestNet`
    ifname_fronthaul_ap_2g=`config get wl2g_NORMAL_AP`
    ifname_fronthaul_ap_5g=`config get wl5g_NORMAL_AP`
    ifname_guestnetwork_ap_2g=`config get wl2g_GUEST_AP`
    ifname_guestnetwork_ap_5g=`config get wl5g_GUEST_AP`
    fronthaul_5g_not_connected=
    fronthaul_2g_not_connected=
    guest_5g_not_connected=
    guest_2g_not_connected=
    if [ "x`config get lan_ipaddr`" != "x192.168.1.250" ];then
        if [ "$enable_5G" -eq '1' ]; then
            fronthaul_5g_not_connected=`iwconfig $ifname_fronthaul_ap_5g 2>/dev/null | grep "Not-Associated"`
            if [ -n "$fronthaul_5g_not_connected" ]; then
                echo "[$0]Bring fronthaul 5G interface up" > /dev/console
                ifconfig $ifname_fronthaul_ap_5g up
            fi
        fi
        if [ "$enable_24G" -eq '1' ]; then
            fronthaul_2g_not_connected=`iwconfig $ifname_fronthaul_ap_2g 2>/dev/null  | grep "Not-Associated"`
            if [ -n "$fronthaul_2g_not_connected" ]; then
                echo "[$0]Bring fronthaul 2.4G interface up" > /dev/console
                ifconfig $ifname_fronthaul_ap_2g up
            fi
        fi


 
        if [ "$enable_5G_guestnetwork" -eq '1' -a  "$enable_5G" -eq '1'  ]; then
            guest_5g_not_connected=`iwconfig $ifname_guestnetwork_ap_5g 2>/dev/null  | grep "Not-Associated"`
            if [ -n "$guest_5g_not_connected" ]; then
                echo "[$0]Bring guest 5G interface up" > /dev/console
                ifconfig $ifname_guestnetwork_ap_5g up
            fi
        fi
        if [ "$enable_24G_guestnetwork" -eq '1' -a  "$enable_24G" -eq '1'  ]; then
            guest_2g_not_connected=`iwconfig $ifname_guestnetwork_ap_2g 2>/dev/null  | grep "Not-Associated"`
            if [ -n "$guest_2g_not_connected" ]; then
                echo "[$0]Bring guest 2.4G interface up" > /dev/console
                ifconfig $ifname_guestnetwork_ap_2g up
            fi
        fi
    else 
        #ip=250  backhaul=connected  fronthaul=down  ,this situation we will restart wlan
        if [ ! -f $wlan_updown_lock ];then
            if [  "$enable_5G" -eq '1'  ]; then
                fronthaul_5g_not_connected=`iwconfig $ifname_fronthaul_ap_5g 2>/dev/null  | grep "Not-Associated"`
                if [ -n "$fronthaul_5g_not_connected" ]; then
                    wifi_restart_event $ifname_fronthaul_ap_5g "fronhaul not up ,it seems connected but can not get IP!!!!" 
                    sleep 10
                    continue
                fi
            fi
            if [ "$enable_24G" -eq '1'  ]; then
                fronthaul_2g_not_connected=`iwconfig $ifname_fronthaul_ap_2g 2>/dev/null  | grep "Not-Associated"`
                if [ -n "$fronthaul_2g_not_connected" ]; then
                    wifi_restart_event $ifname_fronthaul_ap_2g "fronhaul not up ,it seems connected but can not get IP!!!!" 
                    sleep 10
                    continue
                fi
            fi



            if [ "$enable_5G_guestnetwork" -eq '1' -a  "$enable_5G" -eq '1'  ]; then
                guest_5g_not_connected=`iwconfig $ifname_guestnetwork_ap_5g 2>/dev/null  | grep "Not-Associated"`
                if [ -n "$guest_5g_not_connected" ]; then
                    wifi_restart_event $ifname_guestnetwork_ap_5g "guest not up ,it seems connected but can not get IP!!!!" 
                    sleep 10
                    continue
                fi
            fi
            if [ "$enable_24G_guestnetwork" -eq '1' -a  "$enable_24G" -eq '1'  ]; then
                guest_2g_not_connected=`iwconfig $ifname_guestnetwork_ap_2g 2>/dev/null  | grep "Not-Associated"`
                if [ -n "$guest_2g_not_connected" ]; then
                    wifi_restart_event $ifname_guestnetwork_ap_2g "guest not up ,it seems connected but can not get IP!!!!" 
                    sleep 10
                    continue
                fi
            fi
       
        fi
    fi

fi

if [ "`cat /module_name`" = "RBS50" -a  "x`ps|grep "wpa_supplicant "|grep -v grep`" = "x" ];then
	wl2g_backhaul_sta=`config get wl2g_BACKHAUL_STA`
	wl5g_backhaul_sta=`config get wl5g_BACKHAUL_STA`
    echo "[$0]wpa_supplicant disappear ,restart it" >/dev/console
    killall wpa_cli
    rm -f /var/run/wpa_supplicant-global.pid
    wpa_supplicant -g /var/run/wpa_supplicantglobal -B -P /var/run/wpa_supplicant-global.pid 
    ifconfig $wl5g_backhaul_sta up
    ifconfig $wl2g_backhaul_sta up
    wpa_cli -g /var/run/wpa_supplicantglobal interface_add  $wl5g_backhaul_sta /var/run/wpa_supplicant-${wl5g_backhaul_sta}.conf athr /var/run/wpa_supplicant-$wl5g_backhaul_sta "" br0
    wpa_cli -g /var/run/wpa_supplicantglobal interface_add  $wl2g_backhaul_sta /var/run/wpa_supplicant-${wl2g_backhaul_sta}.conf athr /var/run/wpa_supplicant-$wl2g_backhaul_sta "" br0
fi


if [ ! -f "$wlan_updown_lock" ];then
    sleep 10
    [ -f $wlan_updown_lock ] && continue
    #get backhaul interface
    ifname_backhaul_ap=`config get wl5g_BACKHAUL_AP`" "`config get wl2g_BACKHAUL_AP`
    ifname_backhaul_sta=`config get wl5g_BACKHAUL_STA`" "`config get wl2g_BACKHAUL_STA`

    #get fronthaul interface
    enable_24G=`config get endis_wl_radio`
    enable_5G=`config get endis_wla_2nd_radio`
    wifischedule_24G=`config get wladv_schedule_enable`
    wifischedule_5G=`config get wladv_schedule_enable_a`
    ifname_fronthaul_ap=
    ifname_guestnetwork_ap=
    if [ "$enable_24G" -eq '1'  ]; then
        ifname_fronthaul_ap=$ifname_fronthaul_ap" "`config get wl2g_NORMAL_AP`
        ifname_guestnetwork_ap=$ifname_guestnetwork_ap" "`config get wl2g_GUEST_AP`
    fi
    if [ "$enable_5G" -eq '1'   ]; then
        ifname_fronthaul_ap=$ifname_fronthaul_ap" "`config get wl5g_NORMAL_AP`
        ifname_guestnetwork_ap=$ifname_guestnetwork_ap" "`config get wl5g_GUEST_AP`
    fi


    if [  "x`ps|grep "hostapd "|grep -v grep`" = "x"  ];then
        wifi_restart_event athx "hostapd disappear!!!!" 
    fi
    ifnamelist=$ifname_fronthaul_ap" "$ifname_backhaul_sta" "$ifname_backhaul_ap" "$ifname_guestnetwork_ap
    for ifname in $ifnamelist 
    do
        if [ "x`iwconfig $ifname 2>/dev/null |grep ESSID`" = "x" ];then
            wifi_restart_event $ifname "interface diappeared!!!!" 
            break
        fi
    done
    ifnamelist=$ifname_backhaul_sta" "$ifname_backhaul_ap
    for ifname in $ifnamelist 
    do
        if [ "x`ifconfig $ifname 2>/dev/null |grep UP`" = "x" ];then
            wifi_restart_event $ifname "backhaul interface not up!!!!" 
            break
        fi
    done
    ifnamelist=$ifname_fronthaul_ap" "$ifname_backhaul_ap
    for ifname in $ifnamelist 
    do
        if [  `ps|grep hostapd_cli|grep " $ifname "|grep -v grep|wc -l` -gt 1  ];then
            wifi_restart_event $ifname "hostapd_cli run twice!!!!" 
            break
        fi
    done

    


fi
done
