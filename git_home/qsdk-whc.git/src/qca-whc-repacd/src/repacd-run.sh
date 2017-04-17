#!/bin/sh
# Copyright (c) 2015 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

REPACD_DEBUG_OUTOUT=1

. /lib/functions/repacd-gwmon.sh
. /lib/functions/repacd-wifimon.sh
. /lib/functions/repacd-led.sh

GWMON_DEBUG_OUTOUT=$REPACD_DEBUG_OUTOUT

local cur_role managed_network
local link_check_delay
local re_mode_change=0
local daisy_chain

__repacd_info() {
    local stderr=''
    if [ "$REPACD_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd -p user.info "$1"
}

__repacd_restart() {
    local __mode="$1"
    __repacd_info "repacd: restart in $__mode mode"

    /etc/init.d/repacd restart_in_${__mode}_mode
    exit 0
}

__repacd_update_mode() {
    local new_mode=$1
    if [ "$new_mode" -eq $GWMON_MODE_CAP ]; then
        __repacd_info "Restarting in CAP mode"
        __repacd_restart 'cap'
    elif [ "$new_mode" -eq $GWMON_MODE_NON_CAP ]; then
        __repacd_info "Restarting in NonCAP mode"
        __repacd_restart 'noncap'
    fi
}

config_load repacd
config_get managed_network repacd 'ManagedNetwork' 'lan'
config_get cur_role repacd 'Role' 'NonCAP'
config_get link_check_delay repacd 'LinkCheckDelay' '2'
config_get dni_led_mode repacd 'DNI_Mode' '1'
config_get led_guardinterval repacd 'LED_GUARDINTERVAL' '120'
config_get daisy_chain WiFiLink 'DaisyChain' '0'
config_get traffic_separation_enabled repacd TrafficSeparationEnabled '0'
config_get traffic_separation_active repacd TrafficSeparationActive '0'
config_get backhaul_network repacd NetworkBackhaul 'backhaul'

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 <start_role> <config RE mode> <current RE mode> [autoconf]"
    exit 1
fi

local wps_guardinterval=$(($led_guardinterval/$link_check_delay))

local start_role=$1
local config_re_mode=$2
local current_re_mode=$3
local current_re_submode=$4
local re_mode_change=0
local adjustment=6/10

local led_solid_times=$((180/$link_check_delay*$adjustment))
local boot_time=$((150/$link_check_delay*$adjustment))
local wps_guardinterval_times=0

local solid_times=0
local blink_times=0
local WAR_times=0

local off_times=0
local wps_progress=0
local last_associated=0
local wps_fh_improgress=0
local IPLease_time=0
local IPLease_timeout=$((30/$link_check_delay*$adjustment))
local board_data=`/sbin/artmtd -r board_data | awk '{print $3}'`
local dns_hijack=`/bin/config get dns_hijack`
local ADD_ON_default=0

# Clean up the background ping and related logic when being terminated
# by the init system.
trap 'repacd_wifimon_fini; repacd_led_set_states Reset; exit 0' SIGTERM

__repacd_info "Starting: ConfiguredRole=$cur_role StartRole=$start_role"
__repacd_info "Starting: ConfigREMode=$config_re_mode CurrentREMode=$current_re_mode CurrentRESubMode=$current_re_submode"

local new_mode
__gwmon_init $cur_role $start_role $managed_network
new_mode=$?
__repacd_update_mode $new_mode

local cur_state new_state
local new_re_mode=$current_re_mode new_re_submode=$current_re_submode
local autoconf_restart

# If the start was actually a restart triggered by automatic configuration
# logic (eg. mode or role switching), note that here so it can influence the
# LED states.
if [ -n "$5" ]; then
    __repacd_info "Startup triggered by auto-config change"
    autoconf_restart=1
else
    autoconf_restart=0
fi

if [ "$traffic_separation_enabled" -gt 0 ] && \
   [ "$traffic_separation_active" -gt 0 ]; then
    repacd_wifimon_init $backhaul_network $current_re_mode $current_re_submode $autoconf_restart \
                        new_state new_re_mode new_re_submode
else
    repacd_wifimon_init $managed_network $current_re_mode $current_re_submode $autoconf_restart \
                        new_state new_re_mode new_re_submode
fi

# Since the Wi-Fi monitoring process does nothing when in CAP mode, force
# the state to one that indicates we are operating in CAP mode.
if [ "$cur_role" = 'CAP' ]; then
    new_state='InCAPMode'
fi

if [ -n "$new_state" ]; then
    __repacd_info "Setting initial LED states to $new_state"
    repacd_led_set_states $new_state
    cur_state=$new_state
else
    __repacd_info "Failed to resolve STA interface; will attempt periodically"
fi

if [ "$board_data" -eq "0002" -a "$dns_hijack" -eq "1" ]; then
    __repacd_info "ADD-ON default boot"
    ADD_ON_default=1
fi

# Loop forever (unless we are killed with SIGTERM which is handled above).
while true; do
    __gwmon_check
    new_mode=$?
    __repacd_update_mode $new_mode

    if [ -n "$cur_state" ]; then
        new_state=''
        repacd_wifimon_check $managed_network $current_re_mode $current_re_submode \
                             new_state new_re_mode new_re_submode

        # First test for range extender mode change, which could also include
        # a role change if the LED state is updated to indicate that.
        re_mode_change=0
        if [ "$config_re_mode" = 'auto' -a \
             ! "$current_re_mode" = "$new_re_mode" ]; then
            __repacd_info "New auto-derived RE mode=$new_re_mode"

            uci_set repacd repacd AssocDerivedREMode $new_re_mode
            uci_commit repacd

            re_mode_change=1
        fi
        # RE sub-mode change check.
        if [ ! "$current_re_submode" = "$new_re_submode" ]; then
            __repacd_info "New auto-derived RE sub-mode=$new_re_submode"

            uci_set repacd repacd AssocDerivedRESubMode $new_re_submode
            uci_commit repacd

            re_mode_change=1
        fi



        if [ -n "$new_state" -a ! "$new_state" = "$cur_state" ]; then

            if [ "$dni_led_mode" = "0" ]; then
                __repacd_info "Updating LED states to $new_state"
                repacd_led_set_states $new_state
            fi

            cur_state=$new_state

            solid_times=0
            if [ "$blink_times" -gt 0 -a "$cur_state" = "Measuring" ]; then
                blink_times=1
            else
                blink_times=0
            fi

            off_times=0
            WAR_times=0

            # Depending on the startup role, look for the special states
            # that indicate the new role should be different.
            if [ ! "$start_role" = 'RE' ]; then  # init and NonCAP roles
                if [ "$new_state" = $WIFIMON_STATE_CL_ACTING_AS_RE ]; then
                    __repacd_info "Restarting in RE role"
                    __repacd_restart 're'
                    re_mode_change=0  # role change includes mode change
                fi
            elif [ "$start_role" = 'RE' ]; then
                if [ "$new_state" = $WIFIMON_STATE_CL_LINK_INADEQUATE -o \
                     "$new_state" = $WIFIMON_STATE_CL_LINK_SUFFICIENT ]; then
                    __repacd_info "Restarting in Client role"
                    __repacd_restart 'noncap'
                    re_mode_change=0  # role change includes mode change
                fi
            fi
        fi


        if [ "$dni_led_mode" = "1" ]; then

            __repacd_info "$cur_state"


            case $cur_state in
                "RE_MoveFarther")
		    wps_guardinterval_times=0
                    ADD_ON_default=0

                    if [ $boot_time -gt 0 ]; then
                        boot_time=0
                    fi

                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        led_solid_times=$((180/$link_check_delay))
                        last_associated=1
                    fi

                    if [ "$led_solid_times" -gt 0 ]; then
                        led_solid_times=$(($led_solid_times - 1))

                       if [ "$led_solid_times" -eq 0 ]; then
                           dni_led_set_states "OFF"
                           WAR_times=100
                       fi

                    fi

                    if [ "$WAR_times" -gt 0 ]; then
                        WAR_times=$(($WAR_times - 1))
                        if [ "$WAR_times" -eq 0 ]; then 
                            dni_led_set_states "OFF"
                            WAR_times=200
                            echo "repacd - WAR led off" > /dev/console
                        fi
                    fi
                ;;

                "RE_LocationSuitable")
		    wps_guardinterval_times=0
                    ADD_ON_default=0

                    if [ $boot_time -gt 0 ]; then
                        boot_time=0
                    fi

                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        led_solid_times=$((180/$link_check_delay))
                        last_associated=1
                    fi

                    if [ "$led_solid_times" -gt 0 ]; then
                        led_solid_times=$(($led_solid_times - 1))

                        __repacd_info "LED SOLID TIMES--$led_solid_times"
                       if [ "$led_solid_times" -eq 0 ]; then
                           dni_led_set_states "OFF"
                           WAR_times=200
                       fi

                    fi

                    if [ "$WAR_times" -gt 0 ]; then
                        WAR_times=$(($WAR_times - 1))
                        if [ "$WAR_times" -eq 0 ]; then 
                            dni_led_set_states "OFF"
                            WAR_times=200
                            echo "repacd - WAR led off" > /dev/console
                        fi
                    fi
                ;;

                "RE_MoveCloser")
		    wps_guardinterval_times=0
                    ADD_ON_default=0

                    if [ $boot_time -gt 0 ]; then
                        boot_time=0
                    fi

                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        led_solid_times=$((180/$link_check_delay))
                        last_associated=1
                    fi

                    if [ "$led_solid_times" -gt 0 ]; then
                        led_solid_times=$(($led_solid_times - 1))

                       if [ "$led_solid_times" -eq 0 ]; then
                           dni_led_set_states "OFF"
                           WAR_times=200
                       fi

                    fi

                    if [ "$WAR_times" -gt 0 ]; then
                        WAR_times=$(($WAR_times - 1))
                        if [ "$WAR_times" -eq 0 ]; then 
                            dni_led_set_states "OFF"
                            WAR_times=200
                            echo "repacd - WAR led off" > /dev/console
                        fi
                    fi

                ;;

                "Measuring")
                    wps_progress=0
                ;;

                "AssocTimeout")
                ;;

                "NotAssociated")
                    if [ "$ADD_ON_default" -eq "1" ]; then
                        __repacd_info "ADD-ON default boot"
                        boot_time=0
                        if [ "$solid_times" -eq 0 -a repacd_wifimon_check_wifi_ready ]; then
                            __repacd_info "Set ADD-ON default boot"
                            dni_led_set_states "ADD-ON-default"
                            solid_times=1
                        fi

                    elif [ "$wps_guardinterval_times" -gt 0 ]; then
                        __repacd_info "WPS Guard Interval ::: $wps_guardinterval_times "
                    elif [ "$boot_time" -gt 0 ]; then
                        if [ "$blink_times" -eq 0 ]; then
                            dni_led_set_states "Booting"
                            blink_times=1
                        fi
                    elif [ "$solid_times" -eq 0 -a "$wps_progress" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=1
                    fi
                    last_associated=0
                ;;

                "AutoConfigInProgress")
                    wps_progress=1
                    ADD_ON_default=0
                    boot_time=0

                    if [ $last_associated -eq 1 ]; then
                        wps_fh_improgress=1
                        __repacd_info "FH WPS START"
                    fi

                ;;

                "AutoConfigFinish")
                    wps_progress=0
                    if [ $wps_fh_improgress -eq 1 ]; then
                        dni_led_set_states "OFF"
                        wps_fh_improgress=0
                        __repacd_info "FH WPS FINISH"
                    else
                        wps_guardinterval_times=$wps_guardinterval
                    fi
                ;;

                "AutoConfigFail")
                    wps_progress=0
                    __repacd_info "WPS FAIL"
                    if [ $wps_fh_improgress -eq 1 ]; then
                        dni_led_set_states "OFF"
                        wps_fh_improgress=0
                        __repacd_info "FH WPS FAIL"
                    fi

                ;;

                "OneBackhaulWPSInProgress")
                    __repacd_info "OneBackhaulWPSInProgress"
                ;;

                "OneBackhaulWPSTimeout")
                    __repacd_info "OneBackhaulWPSTimeout"
                ;;

                "IPLeaseFail")
                    __repacd_info "IPLeaseFail"
                    boot_time=0

                    if [ "$IPLease_time" -lt "$IPLease_timeout" ]; then
                        IPLease_time=$(($IPLease_time+1))
                        __repacd_info "IPLeaseFail ::: try $IPLease_time times"
                   
                        if [ "$IPLease_time" -eq "$IPLease_timeout" ]; then
                            __repacd_info "IPLease TIMEOUT"
                            dni_led_set_states "IPLeaseFail"
                            last_associated=0
                        fi
                    fi

                ;;


            esac

        fi

        # Handle the Best AP selection logic for STA interfaces
        if [ "$daisy_chain" -gt 0 ] && \
            [ "$new_state" = $WIFIMON_STATE_RE_MOVE_CLOSER -o \
            "$new_state" = $WIFIMON_STATE_RE_MOVE_FARTHER -o \
            "$new_state" = $WIFIMON_STATE_BSSID_ASSOC_TIMEOUT ]; then
            local restart_wifi=0

            if [ ! "$new_state" = $WIFIMON_STATE_BSSID_ASSOC_TIMEOUT ]; then
                __repacd_wifimon_config_best_ap restart_wifi
            else
                restart_wifi=1
            fi

            if [ "$restart_wifi" -eq 1 ]; then
                __repacd_info "WiFi BSSID config changed"
                uci_set repacd WiFiLink BSSIDUpdated '1'
                uci_commit repacd
                if [ ! "$start_role" = 'RE' ]; then  # init and NonCAP roles
                    __repacd_restart 'noncap'
                elif [ "$start_role" = 'RE' ]; then
                    __repacd_restart 're'
                fi
            fi
        fi

        # Handle any RE mode change not implicitly handled above.
        if [ "$re_mode_change" -gt 0 ]; then
            if [ ! "$start_role" = 'RE' ]; then  # init and NonCAP roles
                __repacd_restart 'noncap'
            elif [ "$start_role" = 'RE' ]; then
                __repacd_restart 're'
            fi
        fi
    else
        repacd_wifimon_init $managed_network $current_re_mode $current_re_submode $autoconf_restart \
                            new_state new_re_mode new_re_submode
        if [ -n "$new_state" ]; then
            __repacd_info "Setting initial LED states to $new_state"
            repacd_led_set_states $new_state
            cur_state=$new_state
        fi
    fi

    # Re-check the link conditions in a few seconds.

    if [ $boot_time -gt 0 ]; then
        boot_time=$(($boot_time - $link_check_delay))
    fi
    if [ $wps_guardinterval_times -gt 0 ]; then
        wps_guardinterval_times=$(($wps_guardinterval_times - 1))
    fi

    __repacd_info "Boot time count=$boot_time"

    sleep $link_check_delay
done
