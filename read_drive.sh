#!/bin/bash
# read_drive.sh
# Reads all drive values via SDO for all 5 slaves
# Usage: ./read_drive.sh [hz]
# Example: ./read_drive.sh 10  (read at 10Hz, default 1Hz)

HZ=${1:-1}
SLEEP=$(echo "scale=6; 1/$HZ" | bc)

SLAVES=(0 1 2 3 4)
JOINTS=(joint_1 joint_2 joint_3 joint_4 joint_5)

print_slave() {
    local p=$1
    local joint=$2

    pos=$(sudo ethercat upload -p $p --type int32 0x6064 0x00 2>/dev/null | awk '{print $2}')
    vel=$(sudo ethercat upload -p $p --type int32 0x606C 0x00 2>/dev/null | awk '{print $2}')
    effort=$(sudo ethercat upload -p $p --type int16 0x6077 0x00 2>/dev/null | awk '{print $2}')
    dig_in=$(sudo ethercat upload -p $p --type uint32 0x60FD 0x00 2>/dev/null | awk '{print $2}')
    voltage=$(sudo ethercat upload -p $p --type uint32 0x6079 0x00 2>/dev/null | awk '{print $2}')
    current=$(sudo ethercat upload -p $p --type int16 0x6078 0x00 2>/dev/null | awk '{print $2}')
    vel_dem=$(sudo ethercat upload -p $p --type int32 0x606B 0x00 2>/dev/null | awk '{print $2}')
    pos_dem=$(sudo ethercat upload -p $p --type int32 0x60FC 0x00 2>/dev/null | awk '{print $2}')
    torq_dem=$(sudo ethercat upload -p $p --type int16 0x6074 0x00 2>/dev/null | awk '{print $2}')
    status=$(sudo ethercat upload -p $p --type uint16 0x6041 0x00 2>/dev/null | awk '{print $1}')
    mode=$(sudo ethercat upload -p $p --type uint8 0x6061 0x00 2>/dev/null | awk '{print $2}')
    error=$(sudo ethercat upload -p $p --type uint16 0x603F 0x00 2>/dev/null | awk '{print $1}')
    touch=$(sudo ethercat upload -p $p --type int16 0x60B9 0x00 2>/dev/null | awk '{print $2}')

    # Convert position to radians
    pos_rad=$(echo "scale=6; $pos * 0.000011991" | bc 2>/dev/null)
    vel_rad=$(echo "scale=6; $vel * 0.000011991" | bc 2>/dev/null)
    voltage_v=$(echo "scale=3; $voltage / 1000" | bc 2>/dev/null)

    echo "  [$joint slave=$p]"
    echo "    position:       $pos counts ($pos_rad rad)"
    echo "    velocity:       $vel counts/s ($vel_rad rad/s)"
    echo "    effort:         $effort"
    echo "    current:        $current ‰ rated"
    echo "    bus_voltage:    $voltage mV ($voltage_v V)"
    echo "    vel_demand:     $vel_dem"
    echo "    pos_demand:     $pos_dem"
    echo "    torq_demand:    $torq_dem"
    echo "    digital_inputs: $dig_in"
    echo "    status_word:    $status"
    echo "    mode:           $mode"
    echo "    error_code:     $error"
    echo "    touch_probe:    $touch"
}

echo "Reading drive values at ${HZ}Hz (Ctrl+C to stop)"
echo "=================================================="

while true; do
    echo ""
    echo "$(date '+%H:%M:%S.%3N')"
    echo "──────────────────────────────────────────────────"
    for i in "${!SLAVES[@]}"; do
        print_slave ${SLAVES[$i]} ${JOINTS[$i]}
    done
    sleep $SLEEP
done