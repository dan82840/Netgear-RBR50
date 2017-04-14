#!/bin/sh
# Copyright (c) 2015 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

WIFIMON_DEBUG_OUTOUT=1

# Set this to a filename to log all commands executed.
# The output of relevant commands will be appeneded to the file.
WIFIMON_DEBUG_COMMAND_FILE=

WIFIMON_STATE_NOT_ASSOCIATED='NotAssociated'
WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS='AutoConfigInProgress'
WIFIMON_STATE_AUTOCONFIG_FINISH='AutoConfigFinish'
WIFIMON_STATE_MEASURING='Measuring'
WIFIMON_STATE_WPS_TIMEOUT='WPSTimeout'
WIFIMON_STATE_ASSOC_TIMEOUT='AssocTimeout'
WIFIMON_STATE_RE_MOVE_CLOSER='RE_MoveCloser'
WIFIMON_STATE_RE_MOVE_FARTHER='RE_MoveFarther'
WIFIMON_STATE_RE_LOCATION_SUITABLE='RE_LocationSuitable'
WIFIMON_STATE_CL_LINK_SUFFICIENT='CL_LinkSufficient'
WIFIMON_STATE_CL_LINK_INADEQUATE='CL_LinkInadequate'
WIFIMON_STATE_CL_ACTING_AS_RE='CL_ActingAsRE'
ONE_BACKHAUL_AUTOCONFIG_IN_PROGRESS='OneBackhaulWPSInProgress'
ONE_BACKHAUL_AUTOCONFIG_TIMEOUT='OneBackhaulWPSTimeout'


WIFIMON_PIPE_NAME='/var/run/repacd.pipe'

. /lib/functions.sh
. /lib/functions/whc-network.sh

# State information
local sta_iface_24g sta_iface_24g_config_name
local sta_iface_5g sta_iface_5g_config_name unknown_ifaces=0
local assoc_timeout_logged=0 wps_timeout_logged=0
local wps_in_progress=0 wps_start_time=''
local wps_stabilization=0 wps_assoc_count=0
local auto_mode_stabilization=0 auto_mode_assoc_count=0
local assoc_start_time='' last_assoc_state=0 backhaul_eval_time='' force_down_5g_timestamp=''
local ping_running=0 last_ping_gw_ip
local rssi_num=0 rssi_filename=
local force_down_5g=0 down_time_2g='' measurement_eval_count=0
local tmp_sta_iface_24g=
local backhaul24G_used=0
local cur_associate_state=0
local associate_state=0


# Config parameters
local device_type config_re_mode default_re_mode
local min_wps_assoc min_auto_mode_assoc
local rssi_samples rssi_far rssi_near rssi_min rssi_pref2G
local assoc_timeout wps_timeout config_downtime2G measuring_attempts=0
local config_short_eval_time5g=0 config_long_eval_time5g=0


# Emit a message at debug level.
# input: $1 - the message to log
__repacd_wifimon_debug() {
    local stderr=''
    if [ "$WIFIMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.wifimon -p user.debug "$1"
}

# Log the output of a command to a file (when enabled).
# This is a nop unless WIFIMON_DEBUG_COMMAND_FILE is set.
# input: $1 - command output to log
__repacd_wifimon_dump_cmd() {
    if [ -n "$WIFIMON_DEBUG_COMMAND_FILE" ]; then
        touch $WIFIMON_DEBUG_COMMAND_FILE
        date >> $WIFIMON_DEBUG_COMMAND_FILE
        echo "$1" >> $WIFIMON_DEBUG_COMMAND_FILE
        echo >> $WIFIMON_DEBUG_COMMAND_FILE
    fi
}

# Emit a message at info level.
__repacd_wifimon_info() {
    local stderr=''
    if [ "$WIFIMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.wifimon -p user.info "$1"
}

# Obtain a timestamp from the system.
#
# These timestamps will be monontonically increasing and be unaffected by
# any time skew (eg. via NTP or manual date commands).
#
# output: $1 - the timestamp as an integer (with any fractional time truncated)
__repacd_wifimon_get_timestamp() {
    timestamp=`cat /proc/uptime | cut -d' ' -f1 | cut -d. -f 1`
    eval "$1=$timestamp"
}

# Terminate any background ping that may be running.
# If no background pings are running, this will be a nop.
__repacd_stop_ping() {
    if [ "$ping_running" -gt 0 ]; then
        local child_id=$(echo $!)
        kill "$child_id"
        ping_running=0
        __repacd_wifimon_debug "Stopped ping to GW IP $last_ping_gw_ip"
    fi

    if [ -n "$rssi_filename" ]; then
        # Clean up the temporary file
        rm -f $rssi_filename
        rssi_filename=
    fi
}

# Start a background ping to the gateway address (if it can be resolved).
# This helps ensure the RSSI values are updated (as firmware will not report
# updates if only beacons are being received on the STA interface).
# input: $1 - network: the name of the network being managed
# return: 0 if the ping was started or is already running; otherwise 1
__repacd_start_ping() {
    gw_ip=`route -n | grep ^0.0.0.0 | grep br$1 | awk '{print $2}'`
    if [ -n "$gw_ip" ]; then
        if [ ! "$gw_ip" = "$last_ping_gw_ip" ]; then
            # First need to kill the existing one due to the IP change.
            __repacd_stop_ping
            # This will leave ping_running set to 0.
        fi

        if [ "$ping_running" -eq 0 ]; then
            __repacd_wifimon_debug "Pinging GW IP $gw_ip"

            # Unfortunately the busybox ping command does not support an
            # interval. Thus, we can only ping once per second so there will
            # only be a handful of measurements over the course of our RSSI
            # sampling.
            ping $gw_ip > /dev/null &
            ping_running=1
            last_ping_gw_ip=$gw_ip
        fi

        # Ping is running now or was started.
        return 0
    fi

    __repacd_wifimon_info "Failed to resolve GW when starting ping; will re-attempt"
    return 1
}

# Determine if the gateway is reachable.
#
# Ideally this would be limited to only the 5 GHz STA interface, but there
# is no good way to do this (since packets would need to be received on the
# bridge interface).
#
# return: 0 if the gateway is reachable; otherwise 1
__repacd_is_gw_reachable() {
    if [ -n "$last_ping_gw_ip" ]; then
        if ping -c 1 ${last_ping_gw_ip} > /dev/null; then
            return 0
        fi
    fi

    # Gateway is unknown or is not reachable
    return 1
}

# Determine if the STA interface named is current associated.
#
# Note that for the purposes of this function, an empty interface name is
# considered associated. This is done because in some configurations, only
# one interface is enabled (the 5 GHz one).
#
# input: $1 - sta_iface: the name of the interface (eg. ath01)
# return: 0 if associated or if the interface name is empty; otherwise 1
__repacd_wifimon_is_assoc() {
    local sta_iface=$1
    if [ -n "$sta_iface" ];
    then
        if [ "$sta_iface" = "$sta_iface_5g" ] &&
               [ "$force_down_5g" -gt 0 ];then
            return 0
        fi

        local assoc_str=$(iwconfig $sta_iface)
        __repacd_wifimon_dump_cmd "State of $sta_iface: $assoc_str"

        if $(echo "$assoc_str" | grep 'Access Point: ' | grep -v 'Not-Associated' > /dev/null); then
            return 0
        else
            return 1
        fi
    else
        # An unknown STA interface is considered associated.
        return 0
    fi
}

# Determine if the device is already operating in the desired range extender
# mode when using automatic mode switching.
#
# input: $1 - cur_re_mode: the current operating range extender mode
# output: $2 - new_re_mode: the new range extender mode
# return: 0 if already operating in the desired mode; otherwise 1
__repacd_wifimon_resolve_mode() {
    local old_re_mode=$(eval echo -n \$"$2")
    local re_mode_changed=0

    # Finally, if the serving AP is operating in one of the special
    # modes, write back the association derived RE mode. Otherwise,
    # write back the default mode.
    if [ "$config_re_mode" = 'auto' ]; then
        # Since when operating in SON mode we rely on wsplcd to force
        # the association to the CAP, do not check whether the CAP is
        # the serving AP when determining whether to switch modes.
        if __repacd_wifimon_is_serving_ap_son; then
            if [ ! "$old_re_mode" = 'son' ]; then
                __repacd_wifimon_info "Serving AP has SON enabled"
                eval "$2=son"
                re_mode_changed=1
            fi
        elif __repacd_wifimon_is_serving_ap_wds; then
            if [ ! "$old_re_mode" = 'wds' ]; then
                __repacd_wifimon_info "Serving AP has WDS enabled"
                eval "$2=wds"
                re_mode_changed=1
            fi
        else
            if [ ! "$old_re_mode" = "$default_re_mode" ]; then
                __repacd_wifimon_info "Serving AP does not advertise WDS"
                eval "$2=$default_re_mode"
                re_mode_changed=1
            fi
        fi

        if [ "$re_mode_changed" -gt 0 ]; then
            return 1
        else
            return 0
        fi
    else
        # By definition, when operating in non-auto mode, we are always in
        # the desired mode.
        return 0
    fi
}

# Determine if the peer BSSID has been written back to UCI.
# This assumes the wireless config file has already been loaded.
# input: $1 - iface_section: the name of the section in UCI
# return: 0 if the BSSID has been set or there is no section; otherwise 1
__repacd_wifimon_is_peer_bssid_set() {
    local iface_section=$1

    if [ -z "$iface_section" ]; then
        # Nothing to clone.
        return 0
    elif [ -n "$iface_section" ]; then
        config_get peer_bssid $iface_section 'bssid' ''
        if [ -n "$peer_bssid" ]; then
            return 0
        fi
    fi

    return 1
}

# Determine if the deep cloning process has completed (if enabled).
# return: 0 if the process is complete or is not enabled; otherwise 1
__repacd_wifimon_is_deep_cloning_complete() {
    # First check if wsplcd and deep cloning are even enabled. If not,
    # then consider it complete.
    local wsplcd_enabled
    local deep_cloning_enabled
    local config_sta_enabled
    local peer_bssid

    config_load wsplcd
    config_get wsplcd_enabled 'config' 'HyFiSecurity' '0'
    config_get deep_cloning_enabled 'config' 'DeepClone' '0'
    config_get config_sta_enabled 'config' 'ConfigSta' '0'

    if [ "$wsplcd_enabled" -eq 0 -o "$deep_cloning_enabled" -eq 0 -o \
         "$config_sta_enabled" -eq 0 ]; then
        return 0
    fi

    # For each of the STA interfaces, see if the peer BSSID has been set.
    config_load wireless
    __repacd_wifimon_is_peer_bssid_set $sta_iface_24g_config_name || return 1
    __repacd_wifimon_is_peer_bssid_set $sta_iface_5g_config_name || return 1

    return 0
}

# Determine if the STA association is stable enough to be able to start
# the next step of the process.
# return: 0 if the association is stable; non-zero if it is not yet deemed
#         stable
__repacd_wifimon_is_assoc_stable() {
    if [ "$wps_stabilization" -gt 0 ]; then
        if [ "$wps_assoc_count" -ge "$min_wps_assoc" ]; then
            return 0
        else
            return 1
        fi
    elif [ "$auto_mode_stabilization" -gt 0 ]; then
        if [ "$auto_mode_assoc_count" -ge "$min_auto_mode_assoc" ]; then
            return 0
        else
            return 1
        fi
    else
        # No stabilization in progress
        return 0
    fi
}

# Determine if the STA is associated and update the state accordingly.
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the currently configured range extender mode
# input: $3 - whether this is a check during init for a restart triggered
#             by mode switching
# output: $4 - state: the variable to update with the new state name (if there
#                     was a change)
# output: $5 - re_mode: the desired range extender mode
# return: 0 if associated; otherwise 1
__repacd_wifimon_check_associated() {
    local network=$1
    local cur_re_mode=$2
    local autoconf_start=$3

    __repacd_wifimon_debug "CHECK associatd"

    if __repacd_wifimon_is_assoc $sta_iface_24g  &&
        __repacd_wifimon_is_assoc $sta_iface_5g ; then
        associate_state=1
        __repacd_wifimon_debug "All backhaul connected"

    elif !(__repacd_wifimon_is_assoc $sta_iface_24g) &&
         (__repacd_wifimon_is_assoc $sta_iface_5g); then
        associate_state=2
        __repacd_wifimon_debug "Only 5g backhaul connected"

    elif (__repacd_wifimon_is_assoc $sta_iface_24g) &&
         !(__repacd_wifimon_is_assoc $sta_iface_5g); then
        associate_state=3
        __repacd_wifimon_debug "Only 2.4g backhaul connected"

    elif !(__repacd_wifimon_is_assoc $sta_iface_24g) &&
         !(__repacd_wifimon_is_assoc $sta_iface_5g); then
        associate_state=0
        __repacd_wifimon_debug "All backhaul disconnected"
    fi

    if [ "$associate_state" -gt 0 -a  "$wps_in_progress" -eq 0 ]; then

        __repacd_wifimon_debug "[repacd]Network associated"

        # Only update the LED state if we transitioned from not associated
        # to associated.
        #if [ "$last_assoc_state" -eq 0 ]; then
        if [ "$last_assoc_state" -eq 0 ] || [ "$cur_associate_state" != "$associate_state" ]; then
            cur_associate_state=$associate_state
            if [ "$wps_in_progress" -gt 0 ]; then
                # If WPS was triggered, it could take some time for the
                # interfaces to settle into their final state. Thus, update
                # the start time for the measurement to the point at which
                # the WPS button was pressed.
                assoc_start_time=$wps_start_time

                # Clear this as we only want to extend the association time
                # for this one instance. All subsequent ones should be based
                # on the time we detect a disassociation (unless WPS is
                # triggered again).
                wps_start_time=''

                # Clear this flag so that we now use the association timer
                # instead of the WPS one.
                wps_in_progress=0

                wps_stabilization=1
            elif [ "$config_re_mode" = 'auto' ] && [ "$auto_mode_stabilization" -eq 0 ]; then
                # When making mode switching decisions, we also want to allow
                # for enough time for the interfaces to stabilize.
                __repacd_wifimon_get_timestamp assoc_start_time

                auto_mode_stabilization=1
                auto_mode_assoc_count=0
                assoc_timeout_logged=0

                eval "$4=$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS"
            fi

            if [ "$wps_stabilization" -gt 0 ]; then
                wps_assoc_count=$((wps_assoc_count + 1))
                __repacd_wifimon_debug "Assoc post WPS (#$wps_assoc_count)"
            elif [ "$auto_mode_stabilization" -gt 0 ]; then
                if __repacd_wifimon_is_deep_cloning_complete; then
                    auto_mode_assoc_count=$((auto_mode_assoc_count + 1))
                    __repacd_wifimon_debug "Auto mode stabilization (#$auto_mode_assoc_count)"
                else
                    __repacd_wifimon_debug "Waiting for deep cloning"
                fi
            fi

            if __repacd_wifimon_is_assoc_stable; then
                auto_mode_stabilization=0

                # Check the mode to see if we are already in the desired mode.
                # If not, we will want to trigger the mode switch first as
                # otherwise the RSSI measurements may not be updated (due to
                # pings not going through).
                if __repacd_wifimon_resolve_mode $cur_re_mode $5; then
                    eval "$4=$WIFIMON_STATE_MEASURING"
                    assoc_start_time=''
                    last_assoc_state=1
                    wps_stabilization=0

                    # Restart the measurements. We do not remember any past
                    # ones as they might not reflect the current state (eg.
                    # if the root AP was moved).
                    rssi_num=0
                else
                    # RE mode switch is required. Do not start measuring link.
                    return 0
                fi
            else
                # Pretend like we are not associated since we need it to be
                # stable.
                return 1
            fi
        fi

        # Association is considered stable. Measure the link RSSI.
        if [ "$rssi_num" -le "$rssi_samples" ] && \
           __repacd_start_ping $network; then
            __repacd_wifimon_measure_link $network $4
        fi
        return 0
    # All cases below are for not associated.
    elif [ "$autoconf_start" -gt 0 ]; then
        # When making mode switching decisions, we also want to allow
        # for enough time for the interfaces to stabilize.
        __repacd_wifimon_get_timestamp assoc_start_time

        auto_mode_stabilization=1
        auto_mode_assoc_count=0
        assoc_timeout_logged=0

        eval "$4=$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS"
    elif [ "$wps_in_progress" -eq 0 -a "$wps_stabilization" -eq 0 -a \
           "$auto_mode_stabilization" -eq 0 ]; then
        # Record the first time we detected ourselves as not being associated.
        # This will drive a timer in the check function that will change the
        # state if we stay disassociated for too long.
        if [ -z "$assoc_start_time" ]; then
            __repacd_wifimon_get_timestamp assoc_start_time
        fi

        last_assoc_state=0
        __repacd_stop_ping

            __repacd_wifimon_debug "sta_iface_24g:$sta_iface_24g  :::  force_down_5g:$force_down_5g "
#        if !(__repacd_wifimon_is_assoc $sta_iface_24g) &&
#           !(__repacd_wifimon_is_assoc $sta_iface_5g); then
            eval "$4=$WIFIMON_STATE_NOT_ASSOCIATED"
#            __repacd_wifimon_debug "BACKHAUL all disconnect"
#        elif [ -z $sta_iface_24g ] && !(__repacd_wifimon_is_assoc $sta_iface_5g); then
#            __repacd_wifimon_debug "BACKHAUL all disconnect"
#            sta_iface_24g=$tmp_sta_iface_24g
#            eval "$4=$WIFIMON_STATE_NOT_ASSOCIATED"
#        elif [ "$force_down_5g" -gt 0 ] && !(__repacd_wifimon_is_assoc $sta_iface_24g); then
#            __repacd_wifimon_debug "BACKHAUL all disconnect"
#            eval "$4=$WIFIMON_STATE_NOT_ASSOCIATED"
#
#        fi

    elif [ "$wps_in_progress" -gt 0 -o "$auto_mode_stabilization" -gt 0 ]; then

        if __repacd_wifimon_is_assoc $sta_iface_24g ||
        __repacd_wifimon_is_assoc $sta_iface_5g; then
            eval "$4=$ONE_BACKHAUL_AUTOCONFIG_IN_PROGRESS"
        fi

        if [ "$wps_timeout_logged" -eq 0 -a "$assoc_timeout_logged" -eq 0 ]; then
            # Suppress logs after we've timed out
            __repacd_wifimon_debug "Auto config in progress - not assoc"

        elif [ "$wps_timeout_logged" -gt 0 ]; then
            if __repacd_wifimon_is_assoc $sta_iface_24g ||
                __repacd_wifimon_is_assoc $sta_iface_5g; then
                eval "$4=$ONE_BACKHAUL_AUTOCONFIG_TIMEOUT"
            fi
        fi

        wps_assoc_count=0
        auto_mode_assoc_count=0

    fi

    # Not associated and WPS is in progress. No LED update.
    return 1
}

# Check whether the given WHC feature is advertised by the AP on which the
# given STA interface is connected.
# input: $1 - sta_iface: the interface to check for the AP capabilities
# input: $2 - ioctl_name: the name of the private ioctl to use to check the
#                         feature
# return: 0 if the feature is on or the interface name is invalid; otherwise 1
__repacd_wifimon_is_whc_feature_on_iface() {
    local sta_iface=$1
    local ioctl_name=$2
    if [ -z "$sta_iface" ]; then
        return 0
    fi

    local command_result
    command_result=$(iwpriv $sta_iface $ioctl_name)
    __repacd_wifimon_dump_cmd "$ioctl_name on $sta_iface: $command_result"

    if [ -n "$command_result" ]; then
        local feature_enabled
        feature_enabled=$(echo "$command_result" | cut -d: -f2)
        if [ "$feature_enabled" -gt 0 ]; then
            return 0
        fi
    else
        __repacd_wifimon_debug "iwpriv failed on $sta_iface for $ioctl_name"
    fi

    # Feature must not be enabled or we cannot resolve it.
    return 1
}

# Determine if the serving AP has WDS enabled or not on all valid interfaces.
# return: 0 if it does have WDS enabled; otherwise 1
__repacd_wifimon_is_serving_ap_wds() {
    if __repacd_wifimon_is_whc_feature_on_iface "$sta_iface_24g" 'get_whc_wds' &&
       __repacd_wifimon_is_whc_feature_on_iface "$sta_iface_5g" 'get_whc_wds'; then
        return 0
    fi

    return 1
}

# Determine if the serving AP has SON mode enabled or not.
# return: 0 if it does have SON enabled; otherwise 1
__repacd_wifimon_is_serving_ap_son() {
    if __repacd_wifimon_is_whc_feature_on_iface "$sta_iface_24g" 'get_whc_son' &&
       __repacd_wifimon_is_whc_feature_on_iface "$sta_iface_5g" 'get_whc_son'; then
        return 0
    fi

    return 1
}

# Determine the number of hops a given STA interface is from the
# root AP.
# input: $1 - sta_iface: the name of the STA interface (eg. ath01)
# output: $2 - dist: the distance (in terms of hops) from the root AP
__repacd_get_root_ap_dist() {
    local sta_iface=$1

    local command_result
    command_result=$(iwpriv $sta_iface get_whc_dist)
    __repacd_wifimon_dump_cmd "root dist for $sta_iface: $command_result"

    if [ -n "$command_result" ]; then
        local root_dist
        root_dist=$(echo "$command_result" | cut -d: -f2)
        eval "$2=$root_dist"
    fi
}

# Determine the maximum number of hops the STA interfaces are from the
# root AP.
# output: $1 - max_dist: the maximum distance (in terms of hops) from the
#                       root AP
__repacd_get_max_root_ap_dist() {
    local root_dist_24g=0 root_dist_5g=0
    if [ -n "$sta_iface_24g" ]; then
        __repacd_get_root_ap_dist $sta_iface_24g root_dist_24g
    fi

    if [ -n "$sta_iface_5g" ]; then
        __repacd_get_root_ap_dist $sta_iface_5g root_dist_5g
    fi

    if [ "$root_dist_24g" -gt "$root_dist_5g" ]; then
        eval "$1=$root_dist_24g"
    else
        eval "$1=$root_dist_5g"
    fi
}

# Determine if the serving AP is currently the CAP or not.
# return: 0 if associated to the CAP; otherwise 1
__repacd_wifimon_is_cap_serving() {
    # If the root AP is not operating in WDS mode, it may not even be a WHC
    # AP. To be conservative, we do not enable the AP functionality.
    local wds_enabled root_dist

    if __repacd_wifimon_is_serving_ap_wds; then
        __repacd_get_max_root_ap_dist root_dist
        if [ "$root_dist" -eq 1 ]; then
            # The STA device saying it is 1 hop away means that the serving
            # AP indicated it was 0 hops away from the CAP. This means that
            # the serving AP is the CAP.
            return 0
        else
            __repacd_wifimon_debug "CAP is not serving"
        fi
    else
        __repacd_wifimon_debug "Serving AP is not WHC enabled"
    fi

    # Either we are more than 1 hop from the root or the distance is
    # unknown. In either case, indicate that the CAP is not serving.
    return 1
}

# Bring up sta vap interface.
# input: $1 - sta interface: the name of the interface for
#
__repacd_wifimon_bring_iface_up() {

    local sta_iface=$1
    if [ -n "$sta_iface" ];then
        disabled_5g_network=`wpa_cli -i $sta_iface_5g -p /var/run/wpa_supplicant-$sta_iface_5g list_network | grep "\[DISABLED\]$" | awk '{print $1}'`
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface_5g enable_network $disabled_5g_network
        __repacd_wifimon_info "Interface $sta_iface Brought up"
        force_down_5g=0
    fi
}

# Bring down sta vap interface.
# input: $1 - sta interface: the name of the interface for bringing down.
#

__repacd_wifimon_bring_iface_down() {

    local sta_iface=$1
    if [ -n "$sta_iface" ];then

        enabled_5g_network=`wpa_cli -i $sta_iface_5g -p /var/run/wpa_supplicant-$sta_iface_5g list_network | grep "\[CURRENT\]$" | awk '{print $1}'`
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface disable_network $enabled_5g_network
        __repacd_wifimon_info "Interface $sta_iface Brought down "
        force_down_5g=1
        if [ -n "$force_down_5g_timestamp" ] ;then
            backhaul_eval_time=$config_long_eval_time5g
        else
            backhaul_eval_time=$config_short_eval_time5g
        fi
        __repacd_wifimon_get_timestamp force_down_5g_timestamp
    fi
}

# Measure the RSSI to the serving AP and update the state accordingly.
# input: $1 - network: the name of the network being monitored
# output: $2 - state: the variable to update with the new state name (if there
#                     was a change)
__repacd_wifimon_measure_link() {
    local rssi
    local rssi24G_used=0

    if ! __repacd_is_gw_reachable; then
        if [ -n "$last_ping_gw_ip" ]; then
            __repacd_wifimon_debug "GW ${last_ping_gw_ip} not reachable"
        else
            __repacd_wifimon_debug "GW unknown"
        fi
        return
    fi

    if [ "$rssi_num" -eq 0 ]; then
        measuring_cnt=$((measuring_cnt + 1))
    fi

    if [ "$measuring_cnt" -gt "$measuring_attempts" ]; then
        if [ -n "$sta_iface_24g" ] &&
               __repacd_wifimon_is_assoc $sta_iface_24g ; then
            __repacd_wifimon_bring_iface_down $sta_iface_5g
            measuring_cnt=0
            eval "$2=$WIFIMON_STATE_RE_MOVE_CLOSER"
            return
        fi
    fi

    # Only the 5 GHz link is measured. This is especially done since we
    # generally cannot control which interface is used to reach upstream.
    # Generally 5 GHz will be used (per the rules to set the broadcast bit
    # and choose a default path), so we may not have any valid RSSI data on
    # 2.4 GHz.
    if __repacd_wifimon_is_assoc $sta_iface_5g; then
        __repacd_wifimon_debug "measure 5G backhaul"
        rssi24G_used=0
        rssi_far=$rssi_far5g
        rssi=`iwconfig $sta_iface_5g | grep 'Signal level' | awk -F'=' '{print $3}' | awk '{print $1}'`
    elif __repacd_wifimon_is_assoc $sta_iface_24g; then
        __repacd_wifimon_debug "measure 2.4G backhaul"
        rssi24G_used=1
        rssi_far=$rssi_far24g
        rssi=`iwconfig $sta_iface_24g | grep 'Signal level' | awk -F'=' '{print $3}' | awk '{print $1}'`
    fi


    if [ "$rssi_num" -eq 0 ]; then
        backhaul24G_used=$rssi24G_used
    fi

    if [ $backhaul24G_used -ne $rssi24G_used ]; then
        backhaul24G_used=$rssi24G_used
        rssi_num=0
    fi

    # We explicitly ignore clearly bogus values. -95 dBm has been seen in
    # some instances where the STA is not associated by the time the RSSI
    # check is done. The check against 0 tries to guard against scenarios
    # where the firmware has yet to report an RSSI value (although this may
    # never happen if the RSSI gets primed through the association messaging).
    if [ "$rssi" -gt -95 -a "$rssi" -lt 0 ]; then
        if [ "$rssi_num" -lt "$rssi_samples" ]; then
            __repacd_wifimon_debug "RSSI sample #$rssi_num = $rssi dBm"

            # Ignore the very first sample since it is taken at the same time
            # the ping is started (and thus the RSSI might not have been
            # updated).
            if [ "$rssi_num" -eq 0 ]; then
                rssi_filename=`mktemp /tmp/repacd-rssi.XXXXXX`
            else
                # Not the first sample
                echo $rssi >> $rssi_filename
            fi
            rssi_num=$((rssi_num + 1))
        elif [ "$rssi_num" -eq "$rssi_samples" ]; then
            __repacd_wifimon_debug "RSSI sample #$rssi_num = $rssi dBm"

            # We will take one more sample and then draw the conclusion.
            # No further measurements will be taken (although this may be
            # changed in the future).
            echo $rssi >> $rssi_filename

            # This is a crude way to compute the median when the number of
            # samples is odd. It is not strictly correct for an even number
            # of samples since it does not compute the average of the two
            # samples in the middle and rather just takes the lower one, but
            # this should be sufficient for our purposes. The average is not
            # performed due to the values being on the logarithmic scale and
            # because shell scripts do not directly support floating point
            # arithmetic.
            local rssi_median_index=$((($rssi_num + 1) / 2))
            local rssi_median=$(cat $rssi_filename | sort -n |
                                head -n $rssi_median_index | tail -n 1)
            __repacd_wifimon_debug "Median RSSI = $rssi_median dBm"
            measuring_cnt=0
            if [ "$device_type" = 'RE' ]; then
                if [ "$rssi_median" -lt "$rssi_far" ]; then
                    eval "$2=$WIFIMON_STATE_RE_MOVE_CLOSER"
                else
                    eval "$2=$WIFIMON_STATE_RE_LOCATION_SUITABLE"
                fi

                if [ "$rssi_median" -le "$rssi_pref2G" ] &&
                           [ "$rssi24G_used" -eq 0 ] ;then
                    if [ -n "$sta_iface_24g" ] &&
                           __repacd_wifimon_is_assoc $sta_iface_24g ; then
                        __repacd_wifimon_bring_iface_down $sta_iface_5g
                    fi
                fi
            else  # must be standalone client
                if [ "$rssi_median" -lt "$rssi_min" ]; then
                    eval "$2=$WIFIMON_STATE_CL_LINK_INADEQUATE"
                elif [ "$rssi_median" -gt "$rssi_near" ]; then
                    eval "$2=$WIFIMON_STATE_CL_LINK_SUFFICIENT"
                elif __repacd_wifimon_is_cap_serving; then
                    eval "$2=$WIFIMON_STATE_CL_ACTING_AS_RE"
                else
                    eval "$2=$WIFIMON_STATE_CL_LINK_SUFFICIENT"
                fi
            fi

            rssi_num=$((rssi_num + 1))  # to prevent future samples

            # We have our measurement, so the ping is no longer needed.
            __repacd_stop_ping

            # In case we disassociate after this, we will want to start the
            # association timer again, so clear our state of the last time we
            # started it so that it can be started afresh upon disassociation.
            assoc_start_time=''
        fi
    fi
}

# Determine if a provided amount of time has elapsed.
# input: $1 - start_time: the timestamp (in seconds)
# input: $2 - duration: the amount of time to check against (in seconds)
# return: 0 on timeout; non-zero if no timeout
__repacd_wifimon_is_timeout() {
    local start_time=$1
    local duration=$2

    # Check if the amount of elapsed time exceeds the timeout duration.
    local cur_time
    __repacd_wifimon_get_timestamp cur_time
    local elapsed_time=$(($cur_time - $start_time))
    if [ "$elapsed_time" -gt $duration ]; then
        return 0
    fi

    return 1
}

# Check whether the given interface is the STA interface on the desired
# network and the desired band.
#
# For now, only the 5 GHz band is monitored.
#
# input: $1 - config: the name of the interface config section
# input: $2 - network: the name of the network to which the STA interface
#                      must belong to be matched
# output: $3 - iface: the resolved STA interface name on 2.4 GHz (if found)
# output: $4 - iface_config_name: the resolved name of the config section
#                                 for the STA interface on 2.4 GHz (if found)
# output: $5 - iface: the resolved STA interface name on 5 GHz (if found)
# output: $6 - iface_config_name: the resolved name of the config section
#                                 for the STA interface on 5 GHz (if found)
# output: $7 - unknown_ifaces: whether any Wi-Fi interfaces are as yet
#                              unknown (in terms of their interface name)
__repacd_wifimon_is_sta_iface() {
    local config="$1"
    local network_to_match="$2"
    local iface disabled mode device hwmode

    config_get network "$config" network
    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get device "$config" device
    config_get hwmode "$device" hwmode

    if [ "$hwmode" != "11ad" ]; then
        if [ "$network" = $network_to_match -a -n "$iface" -a "$mode" = "sta" \
             -a "$disabled" -eq 0 ]; then
            if whc_is_5g_vap $config; then
                eval "$5=$iface"
                eval "$6=$config"
            else
                eval "$3=$iface"
                eval "$4=$config"
            fi
        elif [ -z "$iface" -a "$disabled" -eq 0 ]; then
            # If an interface is showing as enabled but no name is known for it,
            # mark it as such. Without doing this, we can resolve the interface
            # names improperly.
            eval "$7=1"
        fi
    fi
}

# Initialize the sta_iface_5g variable with the STA interface that is enabled
# on the specified network (if any).
# input: $1 - network: the name of the network being managed
__repacd_wifimon_get_sta_iface() {
    unknown_ifaces=0

    config_load wireless
    config_foreach __repacd_wifimon_is_sta_iface wifi-iface $1 \
        sta_iface_24g sta_iface_24g_config_name \
        sta_iface_5g sta_iface_5g_config_name unknown_ifaces

    if [ "$unknown_ifaces" -gt 0 ]; then
        # Clear out everything because we cannot be certain we have the
        # right names (eg. interfaces may not all be up yet).
        sta_iface_24g=
        sta_iface_24g_config_name=
        sta_iface_5g=
        sta_iface_5g_config_name=
    fi
}
# Back haul  monitoring logic : in case 5G was brought down
# forcefully due to RSSI constraints and 2.4 G also went down
# and stayed of for more then 2GBackhaulSwitchDownTime sec
# bring back 5G.
# input: $1 -None
# output:None

__repacd_wifimon_evaluate_backhaul_link() {

    if [ "$force_down_5g" -gt 0 ] ; then
        if  ! __repacd_wifimon_is_assoc $sta_iface_24g;then
            if [ -z "$down_time_2g" ]; then
                __repacd_wifimon_get_timestamp down_time_2g
            fi

            if __repacd_wifimon_is_timeout $down_time_2g $config_downtime2G; then
                __repacd_wifimon_bring_iface_up $sta_iface_5g
                __repacd_wifimon_debug "Bringing up 5G as 2G went down"
                down_time_2g=''
            fi
        else
            down_time_2g=''
        fi

        if  __repacd_wifimon_is_timeout $force_down_5g_timestamp $backhaul_eval_time; then
            __repacd_wifimon_bring_iface_up $sta_iface_5g
            __repacd_wifimon_get_timestamp force_down_5g_timestamp
        fi
    fi
}
# Initialize the Wi-Fi monitoring logic with the name of the network being
# monitored.
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the current operating range extender mode
# input: $3 - autoconfig: whether it was an auto-config restart
# output: $4 - state: the name of the initial state
# output: $5 - new_re_mode: the resolved range extender mode
repacd_wifimon_init() {
    # Resolve the STA interfaces.
    # Here we assume that if we have the 5 GHz interface, that is sufficient,
    # as not all modes will have a 2.4 GHz interface.
    __repacd_wifimon_get_sta_iface $1
    if [ -n "$sta_iface_5g" ]; then
        if [ -n "$sta_iface_24g" ]; then
            __repacd_wifimon_debug "Resolved 2.4 GHz STA interface to $sta_iface_24g"
            __repacd_wifimon_debug "2.4 GHz STA interface section $sta_iface_24g_config_name"
        fi

        __repacd_wifimon_debug "Resolved 5 GHz STA interface to $sta_iface_5g"
        __repacd_wifimon_debug "5 GHz STA interface section $sta_iface_5g_config_name"

        # First resolve the config parameters.
        config_load repacd
        config_get device_type 'repacd' 'DeviceType' 'RE'
        config_get config_re_mode 'repacd' 'ConfigREMode' 'auto'
        config_get default_re_mode 'repacd' 'DefaultREMode' 'qwrap'

        config_get min_auto_mode_assoc 'WiFiLink' 'MinAssocCheckAutoMode' '5'
        config_get min_wps_assoc 'WiFiLink' 'MinAssocCheckPostWPS' '5'
        config_get wps_timeout 'WiFiLink' 'WPSTimeout' '120'
        config_get assoc_timeout 'WiFiLink' 'AssociationTimeout' '300'
        config_get rssi_samples 'WiFiLink' 'RSSINumMeasurements' '5'
        config_get rssi_far5g 'WiFiLink' 'RSSIThresholdFar5g' '-75'
        config_get rssi_far24g 'WiFiLink' 'RSSIThresholdFar24g' '-75'
        config_get rssi_near 'WiFiLink' 'RSSIThresholdNear' '-60'
        config_get rssi_min 'WiFiLink' 'RSSIThresholdMin' '-75'
        config_get rssi_pref2G 'WiFiLink' 'RSSIThresholdPrefer2GBackhaul' '-100'
        config_get config_downtime2G 'WiFiLink' '2GBackhaulSwitchDownTime' '10'
        config_get measuring_attempts 'WiFiLink' 'MaxMeasuringStateAttempts' '3'

        config_get config_short_eval_time5g 'WiFiLink' '5GBackhaulEvalTimeShort' '1800'
        config_get config_long_eval_time5g  'WiFiLink' '5GBackhaulEvalTimeLong' '7200'

        # Create ourselves a named pipe so we can be informed of WPS push
        # button events.
        if [ -e $WIFIMON_PIPE_NAME ]; then
            rm -f $WIFIMON_PIPE_NAME
        fi

        mkfifo $WIFIMON_PIPE_NAME

        # If already associated, go to the InProgress state.
        __repacd_wifimon_check_associated $1 $2 $3 $4 $5
    fi
    # Otherwise, must be operating in CAP mode.
}

# Check the status of the Wi-Fi link (WPS, association, and RSSI).
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the currently configured range extender mode
# output: $3 - state: the name of the new state (only set upon a change)
# output: $4 - re_mode: the desired range extender mode (updated only once
#                       the link to the AP is considered stable)
repacd_wifimon_check() {
    if [ -n "$sta_iface_5g" ]; then

        # Check if the WPS button was pressed.
        local wps_pbc
        local pipe_empty=0
        local wps_event

        for pipe_empty in 0
        do
        read -t 1 wps_pbc <>$WIFIMON_PIPE_NAME
        pipe_empty=$?
        wps_event=$wps_pbc

        __repacd_wifimon_debug "WPS CHECK LOOP:$pipe_empty wps:$wps_event"


        done

        if [ "$wps_event" = "wps_pbc" ]; then
            assoc_timeout_logged=0
            wps_timeout_logged=0

            __repacd_wifimon_debug "WPS START"
            eval "$3=$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS"
            wps_in_progress=1
            __repacd_wifimon_get_timestamp wps_start_time
            return 0

        elif [ "$wps_event" = "wps_finish" ]; then
            wps_in_progress=0
            __repacd_wifimon_debug "WPS FINISH"
            eval "$3=$WIFIMON_STATE_AUTOCONFIG_FINISH"
            return 0
        fi


        if  __repacd_wifimon_check_associated $1 $2 0 $3 $4 ; then
            assoc_timeout_logged=0
            wps_timeout_logged=0
        else  # not associated

            if [ "$wps_in_progress" -gt 0 ]; then
                if __repacd_wifimon_is_timeout $wps_start_time $wps_timeout; then
                    if [ "$wps_timeout_logged" -eq 0 ]; then
                        __repacd_wifimon_debug "WPS timeout"
                        wps_timeout_logged=1
                    fi

                    eval "$3=$WIFIMON_STATE_WPS_TIMEOUT"
                    #wps_in_progress=0
                fi
            else
                if __repacd_wifimon_is_timeout $assoc_start_time $assoc_timeout; then
                    # If we have two STA interfaces and only 5 GHz is
                    # associated, see if mode switching is necessary for it
                    # alone. Note that here we are temporarily resetting the
                    # other interface name to ensure mode switching only
                    # considers the one that is associated.
                    #
                    # If we eventually start supporting multiple STA interfaces
                    # even in the fallback modes, we may need to make this
                    # smarter and consider both possible STA interfaces that
                    # may be associated.
                    if [ -n "$sta_iface_24g" ] && [ -n "$sta_iface_5g" ] && \
                       __repacd_wifimon_is_assoc $sta_iface_5g; then
                        tmp_sta_iface_24g=$sta_iface_24g
                        sta_iface_24g=
                        if ! __repacd_wifimon_resolve_mode $2 $4; then
                            # Not currently in the right mode. Restore the
                            # interface name and return to allow for a restart.
                            sta_iface_24g=$tmp_sta_iface_24g
                            return
                        fi
                    fi

                    if [ "$assoc_timeout_logged" -eq 0 ]; then
                        __repacd_wifimon_debug "Association timeout"
                        assoc_timeout_logged=1
                    fi

                    eval "$3=$WIFIMON_STATE_ASSOC_TIMEOUT"
                    assoc_start_time=''
                fi
            fi
        fi
        __repacd_wifimon_evaluate_backhaul_link
    fi
}

# Terminate the Wi-Fi link monitoring, cleaning up any state in preparation
# for shutdown.
repacd_wifimon_fini() {
    __repacd_stop_ping
}

repacd_wifimon_check_wifi_ready() {
    vapinfo=$(iwconfig "$sta_iface_5g" | grep ESSID ) > /dev/null
    if [ -z "$vapinfo" ]; then
        return 0
    else
        return 1
    fi
}
