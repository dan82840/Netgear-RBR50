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

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 <start_role> <config RE mode> <current RE mode> [autoconf]"
    exit 1
fi

local start_role=$1
local config_re_mode=$2
local current_re_mode=$3

local led_solid_times=$((10/$link_check_delay))

local solid_times=0
local blink_times=0
local off_times=0
local wps_progress=0
local last_associated=0

__repacd_info "led_solid_times:$led_solid_times"


# Clean up the background ping and related logic when being terminated
# by the init system.
trap 'repacd_wifimon_fini; repacd_led_set_states Reset; exit 0' SIGTERM

__repacd_info "Starting: ConfiguredRole=$cur_role StartRole=$start_role"
__repacd_info "Starting: ConfigREMode=$config_re_mode CurrentREMode=$current_re_mode"

local new_mode
__gwmon_init $cur_role $start_role $managed_network
new_mode=$?
__repacd_update_mode $new_mode

local cur_state new_state new_re_mode=$current_re_mode
local autoconf_restart

# If the start was actually a restart triggered by automatic configuration
# logic (eg. mode or role switching), note that here so it can influence the
# LED states.
if [ -n "$4" ]; then
    __repacd_info "Startup triggered by auto-config change"
    autoconf_restart=1
else
    autoconf_restart=0
fi

repacd_wifimon_init $managed_network $current_re_mode $autoconf_restart \
                    new_state new_re_mode

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

#Wait for WIFI ready
while repacd_wifimon_check_wifi_ready; do
    sleep 2
done

# Loop forever (unless we are killed with SIGTERM which is handled above).
while true; do
    __gwmon_check
    new_mode=$?
    __repacd_update_mode $new_mode

    if [ -n "$cur_state" ]; then
        new_state=''
        repacd_wifimon_check $managed_network $current_re_mode new_state new_re_mode

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
                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        last_associated=1
                    fi
                ;;

                "RE_LocationSuitable")
                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        last_associated=1
                    fi
                ;;

                "RE_MoveCloser")
                    if [ $solid_times -eq 0 -a "$last_associated" -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                        last_associated=1
                    fi
                ;;

                "Measuring")
                    wps_progress=0
                    if [ "$blink_times" -eq 0 -a "$last_associated" -eq 0 ]; then
                        #Wait WPS Finish LED finish
                        #sleep 5
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        blink_times=$(($blink_times+1))
                    fi
                ;;

                "AssocTimeout")
                    if [ $solid_times -eq 0 ]; then
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        solid_times=$(($solid_times+1))
                    fi
                ;;

                "NotAssociated")
                    if [ "$blink_times" -eq 0 -a "$wps_progress" -eq 0 ]; then
                        sleep 5
                        __repacd_info "LED STATE--$cur_state"
                        dni_led_set_states $cur_state
                        blink_times=1
                    fi
                    last_associated=0
                ;;

                "AutoConfigInProgress")
                    wps_progress=1
                ;;

                "AutoConfigFinish")
                    wps_progress=0
                ;;


                "OneBackhaulWPSInProgress")
                    __repacd_info "OneBackhaulWPSInProgress"
                ;;

                "OneBackhaulWPSTimeout")
                    __repacd_info "OneBackhaulWPSTimeout"
                ;;
            esac

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
        repacd_wifimon_init $managed_network $current_re_mode $autoconf_restart \
                            new_state new_re_mode
        if [ -n "$new_state" ]; then
            __repacd_info "Setting initial LED states to $new_state"
            repacd_led_set_states $new_state
            cur_state=$new_state
        fi
    fi

    # Re-check the link conditions in a few seconds.
    sleep $link_check_delay
done
