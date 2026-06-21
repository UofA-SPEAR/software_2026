#!/bin/bash
# eRob EtherCAT debug poller (motor 1 / slave position 0 only)
# Uses the IgH "ethercat" CLI to read CiA402 objects + AL state + domain WKC
# every interval, for a CAPPED number of samples (no infinite spam).
#
# Usage:
#   sudo ./erob_debug_poll.sh [master] [slave_pos] [interval_sec] [num_samples]
# Defaults:
#   master=0  slave_pos=0  interval=0.5s  num_samples=40   (=> ~20s of data)
#
# Paste the full output back to me.

MASTER="${1:-0}"
SLAVE="${2:-0}"
INTERVAL="${3:-0.5}"
SAMPLES="${4:-40}"

command -v ethercat >/dev/null 2>&1 || { echo "ERROR: 'ethercat' CLI not found in PATH"; exit 1; }

echo "# eRob debug poll: master=$MASTER slave_pos=$SLAVE interval=${INTERVAL}s samples=$SAMPLES"
echo "# Run this WHILE your ROS node is running and the bad behavior is happening."
echo "#"
echo "# --- Slave / domain snapshot (once, before polling) ---"
ethercat slaves -m"$MASTER" -v 2>&1 | sed -n "/^$SLAVE /,/^[0-9]/p" | head -20
echo "ethercat state:"
ethercat states -m"$MASTER" 2>&1
echo "ethercat domain (watch WC for working-counter errors):"
ethercat domains -m"$MASTER" 2>&1
echo "#"
echo "# --- Live poll (header below) ---"
printf "%-12s %-8s %-22s %-10s %-12s %-12s %-10s %-6s %-10s\n" \
  "time" "AL_st" "statusword(hex/flags)" "ctrlword" "err_code" "pos_act" "vel_act" "mode" "domain_wc"

read_sdo () {
  # args: index subindex type
  ethercat upload -m"$MASTER" -p"$SLAVE" -t"$3" "$1" "$2" 2>/dev/null | awk '{print $1}'
}

decode_status () {
  local sw_dec="$1"
  local flags=""
  (( sw_dec & 0x0001 )) && flags+="RTSO,"      # Ready to switch on
  (( sw_dec & 0x0002 )) && flags+="SO,"        # Switched on
  (( sw_dec & 0x0004 )) && flags+="OPEN,"      # Operation enabled
  (( sw_dec & 0x0008 )) && flags+="FAULT,"     # Fault
  (( sw_dec & 0x0010 )) && flags+="VEN,"       # Voltage enabled
  (( sw_dec & 0x0020 )) && flags+="QSTOP,"     # Quick stop
  (( sw_dec & 0x0040 )) && flags+="SOD,"       # Switch on disabled
  (( sw_dec & 0x0080 )) && flags+="WARN,"      # Warning
  (( sw_dec & 0x0800 )) && flags+="LIMIT,"     # Internal limit active
  echo "${flags%,}"
}

for ((i=1; i<=SAMPLES; i++)); do
  ts="$(date +%H:%M:%S.%3N)"

  al_state="$(ethercat states -m"$MASTER" 2>/dev/null | grep -i "^$SLAVE\b\|Slave $SLAVE" )"
  [ -z "$al_state" ] && al_state="$(ethercat states -m"$MASTER" 2>/dev/null | head -2 | tail -1)"

  sw_raw="$(read_sdo 0x6041 0 uint16)"
  cw_raw="$(read_sdo 0x6040 0 uint16)"
  err_raw="$(read_sdo 0x603F 0 uint16)"
  pos_raw="$(read_sdo 0x6064 0 int32)"
  vel_raw="$(read_sdo 0x606C 0 int32)"
  mode_raw="$(read_sdo 0x6061 0 int8)"
  wc_raw="$(ethercat domains -m"$MASTER" 2>/dev/null | grep -m1 -i "WC")"

  sw_dec=$((sw_raw))
  flags="$(decode_status "$sw_dec")"

  printf "%-12s %-8s 0x%04x[%-14s] 0x%-8x 0x%-10x %-12s %-10s %-6s %-10s\n" \
    "$ts" "$al_state" "$sw_dec" "$flags" "$((cw_raw))" "$((err_raw))" \
    "$pos_raw" "$vel_raw" "$mode_raw" "$wc_raw"

  sleep "$INTERVAL"
done

echo "# Done. ${SAMPLES} samples captured. Paste this whole block back."