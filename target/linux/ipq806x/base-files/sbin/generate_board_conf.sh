#!/bin/sh

CONFIG=/bin/config
board_model_id="$(/sbin/artmtd -r board_model_id | cut -f 2 -d ":")"

/sbin/artmtd -r region
firmware_region=`cat /tmp/firmware_region | awk '{print $1}'`

#When board_model_id on HW board data area is RBR50
if [ "$board_model_id" = "RBR50" ];then
	echo "RBR50" > /module_name
	echo "RBR50" > /hardware_version

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set wan_hostname="RBR50"
		/bin/config set netbiosname="RBR50"
		/bin/config set upnp_serverName="ReadyDLNA: RBR50"
	fi

	/bin/config set bridge_netbiosname="RBR50"
	/bin/config set ap_netbiosname="RBR50"
    	/bin/config set device_name="RBR50"
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
#When board_model_id on HW board data area is RBS50
else 
	echo "RBS50" > /module_name
	echo "RBS50" > /hardware_version
	echo 1 > /proc/sys/net/ipv4/is_satelite
	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set wan_hostname="RBS50"
		/bin/config set netbiosname="RBS50"
		/bin/config set upnp_serverName="ReadyDLNA: RBS50"
	fi

	/bin/config set bridge_netbiosname="RBS50"
	/bin/config set ap_netbiosname="RBS50"
    	/bin/config set device_name="RBS50"
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
	/bin/rm /etc/init.d/dnsmasq
	/bin/rm /usr/sbin/dnsmasq
	/bin/rm /sbin/ping-netgear
	/bin/rm /usr/sbin/net-wall
fi

