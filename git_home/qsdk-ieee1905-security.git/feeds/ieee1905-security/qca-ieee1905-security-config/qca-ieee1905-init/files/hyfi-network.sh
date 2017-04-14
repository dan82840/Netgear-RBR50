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
		/sbin/wlan down
		/sbin/wlan up
	}

	trap - INT ABRT QUIT ALRM
}

hyfi_network_update_dni_wifi(){
    /sbin/uci2dnicfg.sh
}

hyfi_network_dni_set_module_reload(){
    local reload=$1
    [ -z "$reload" ] && reload=1
    uci set wireless.qcawifi=qcawifi
    uci set wireless.qcawifi.module_reload=$reload
    uci commit wireless
}
