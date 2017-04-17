hyfi_network_sync() {
        lock -w /var/run/hyfi_network.lock
}

hyfi_network_restart() {
	trap __hyfi_trap_cb INT ABRT QUIT ALRM

	lock /var/run/hyfi_network.lock
	hyfi_echo "hyfi network" "process $0 ($$) requested network restart"
	/etc/init.d/network restart

	local radios=`uci show wireless | grep ".disabled=" | grep -v "@" | wc -l`
	local vaps=`uci show wireless | grep "].disabled=0" | wc -l`
	if [ $vaps -gt $radios ]; then
		# Workaround for Wi-Fi, needs a clean environment
		[ ! -f /sbin/wlan ] && {
			env -i /sbin/wifi
		}
	fi

	lock -u /var/run/hyfi_network.lock
	[ -f /sbin/wlan ] && {
		for i in $(cat /tmp/wifi_topology);
			do  mode=`echo  "$i" | awk -F ':' '{print $2}'` ;
				if [ "$mode" = "AP" ]; then
				radio=`echo  "$i" | awk -F ':' '{print $3}'`
				/sbin/wlan down $radio
				/sbin/wlan up $radio
				fi
			done

	}

	trap - INT ABRT QUIT ALRM
}

hyfi_network_update_dni_wifi(){
    /sbin/uci2dnicfg.sh
    /sbin/wifison.sh updateconf lbd
    /etc/init.d/hyd restart
}
