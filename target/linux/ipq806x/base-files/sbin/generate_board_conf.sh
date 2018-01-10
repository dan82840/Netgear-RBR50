#!/bin/sh

CONFIG=/bin/config
board_model_id="$(cat /tmp/board_model_id)"

echo "$board_model_id" > /module_name
echo "$board_model_id" > /hardware_version
if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
	/bin/config set wan_hostname="$board_model_id"
	/bin/config set netbiosname="$board_model_id"
	/bin/config set upnp_serverName="ReadyDLNA: $board_model_id"
fi
/bin/config set bridge_netbiosname="$board_model_id"
/bin/config set ap_netbiosname="$board_model_id"
/bin/config set device_name="$board_model_id"

#When board_model_id on HW board data is Base
if [ "$(cat /tmp/orbi_type)" = "Base" ];then
	/bin/rm /sbin/udhcpd-ext
	/bin/rm /sbin/udhcpc-ext
	/bin/rm /usr/share/udhcpc/default.script-ext
	/bin/rm /usr/share/udhcpc/default.script.ap-ext
	/bin/rm /usr/sbin/net-scan-ext
	/bin/rm /usr/sbin/dev-scan-ext
	/bin/rm /usr/sbin/ntpclient-ext
	/bin/rm /etc/init.d/ntpclient-ext
	/bin/rm /etc/init.d/net-lan-ext
	/bin/rm /sbin/ap-led
	/bin/rm /usr/sbin/led-control.sh
#When board_model_id on HW board data area is Satellite
else 
	echo 1 > /proc/sys/net/ipv4/is_satelite
        if [ "x$($CONFIG get repacd_Daisy_Chain_Enable)" = "x0" ]; then
	    /bin/config set wlg_operation_mode=7
	    /bin/config set wla_2nd_operation_mode=5
        else
	    /bin/config set wlg_operation_mode=8
	    /bin/config set wla_2nd_operation_mode=6
        fi
	/bin/mv /sbin/udhcpd-ext /sbin/udhcpd
	/bin/mv /sbin/udhcpc-ext /sbin/udhcpc
	/bin/mv /usr/share/udhcpc/default.script-ext /usr/share/udhcpc/default.script
	/bin/mv /usr/share/udhcpc/default.script.ap-ext /usr/share/udhcpc/default.script.ap
	/bin/mv /usr/sbin/net-scan-ext /usr/sbin/net-scan
	/bin/mv /usr/sbin/dev-scan-ext /usr/sbin/dev-scan
	/bin/mv /usr/sbin/ntpclient-ext /usr/sbin/ntpclient
	/bin/mv /etc/init.d/ntpclient-ext /etc/init.d/ntpclient
	/bin/mv /etc/init.d/net-lan-ext /etc/init.d/net-lan
	/bin/rm /etc/init.d/net-wan
	/bin/rm /etc/init.d/soap_agent
	/bin/rm /usr/sbin/soap_agent
	/bin/rm /etc/init.d/dnsmasq
	/bin/rm /usr/sbin/dnsmasq
	/bin/rm /sbin/ping-netgear
	/bin/rm /usr/sbin/net-wall
	/bin/rm /etc/init.d/openvpn
fi

