#!/usr/bin/env python3
"""
analyze_joint_states.py
Parses ros2 topic echo --csv output from /dynamic_joint_states
and analyzes for cutout events using statusword fault bit.

Usage:
    python3 analyze_joint_states.py joint_states.csv
"""

import sys
import numpy as np
import matplotlib.pyplot as plt

JOINTS = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'joint_5']

# Statusword bit definitions
SW_READY_TO_SWITCH_ON = 0
SW_SWITCHED_ON        = 1
SW_OPERATION_ENABLED  = 2
SW_FAULT              = 3
SW_VOLTAGE_ENABLED    = 4
SW_QUICK_STOP         = 5
SW_SWITCH_ON_DISABLED = 6
SW_WARNING            = 7

def decode_statusword(sw):
    sw = int(sw)
    return {
        'ready_to_switch_on': bool(sw & (1 << SW_READY_TO_SWITCH_ON)),
        'switched_on':        bool(sw & (1 << SW_SWITCHED_ON)),
        'operation_enabled':  bool(sw & (1 << SW_OPERATION_ENABLED)),
        'fault':              bool(sw & (1 << SW_FAULT)),
        'voltage_enabled':    bool(sw & (1 << SW_VOLTAGE_ENABLED)),
        'quick_stop':         bool(sw & (1 << SW_QUICK_STOP)),
        'switch_on_disabled': bool(sw & (1 << SW_SWITCH_ON_DISABLED)),
        'warning':            bool(sw & (1 << SW_WARNING)),
    }

# ── Parser ────────────────────────────────────────────────────────────────────

def detect_interfaces(parts, joint_start, n_joints):
    """Auto-detect interface names from the CSV header row."""
    data_start = joint_start + n_joints
    # Find interface names - they repeat for each joint
    # Interface names are strings (not numbers), values are numbers
    interfaces = []
    i = data_start
    while i < len(parts):
        try:
            float(parts[i])
            break  # hit values
        except ValueError:
            interfaces.append(parts[i])
            i += 1
    return interfaces

def parse_csv(filepath):
    records = []
    interfaces = None

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split(',')
            if len(parts) < 10:
                continue
            try:
                sec = int(parts[0])
                nanosec = int(parts[1])
                timestamp = sec + nanosec * 1e-9
            except ValueError:
                continue

            # Find joint names (after frame_id at index 2)
            joint_start = 3
            n_joints = len(JOINTS)
            data_start = joint_start + n_joints

            # Auto-detect interfaces on first record
            if interfaces is None:
                interfaces = detect_interfaces(parts, joint_start, n_joints)
                if not interfaces:
                    continue
                print(f"Detected interfaces: {interfaces}")

            n_ifaces = len(interfaces)
            block_size = n_ifaces * 2  # names + values per joint

            record = {'timestamp': timestamp}
            try:
                for j_idx, joint in enumerate(JOINTS):
                    block_start = data_start + j_idx * block_size
                    val_start = block_start + n_ifaces
                    for i, iface in enumerate(interfaces):
                        val_idx = val_start + i
                        if val_idx < len(parts):
                            try:
                                record[f'{joint}/{iface}'] = float(parts[val_idx])
                            except ValueError:
                                record[f'{joint}/{iface}'] = float('nan')
                        else:
                            record[f'{joint}/{iface}'] = float('nan')
                records.append(record)
            except Exception:
                continue

    return records, interfaces

# ── Fault detection ───────────────────────────────────────────────────────────

def detect_faults(records, interfaces):
    """Detect fault events using statusword bit 3."""
    faults = []
    has_statusword = 'status_word' in interfaces

    if not has_statusword:
        print("Warning: status_word not in data, falling back to velocity-based detection")
        return detect_faults_by_velocity(records)

    for joint in JOINTS:
        key = f'{joint}/status_word'
        prev_fault = False
        for i, r in enumerate(records):
            sw = r.get(key, 0)
            if np.isnan(sw):
                continue
            decoded = decode_statusword(sw)
            fault = decoded['fault']
            if fault and not prev_fault:
                faults.append({
                    'timestamp': r['timestamp'],
                    'joint': joint,
                    'index': i,
                    'error_code': r.get(f'{joint}/error_code', 0),
                    'bus_voltage': r.get(f'{joint}/bus_voltage', 0),
                    'current': r.get(f'{joint}/current', 0),
                })
            prev_fault = fault

    # Deduplicate events within 0.1s
    faults.sort(key=lambda x: x['timestamp'])
    deduped = []
    last_t = -999
    for f in faults:
        if f['timestamp'] - last_t > 0.1:
            deduped.append(f)
            last_t = f['timestamp']

    return deduped

def detect_faults_by_velocity(records):
    faults = []
    for joint in JOINTS:
        key = f'{joint}/velocity'
        for i in range(1, len(records)):
            prev_v = abs(records[i-1].get(key, 0))
            curr_v = abs(records[i].get(key, 0))
            if prev_v > 0.01 and curr_v < 0.001:
                faults.append({
                    'timestamp': records[i]['timestamp'],
                    'joint': joint,
                    'index': i,
                    'error_code': records[i].get(f'{joint}/error_code', 0),
                    'bus_voltage': records[i].get(f'{joint}/bus_voltage', 0),
                    'current': records[i].get(f'{joint}/current', 0),
                })
    faults.sort(key=lambda x: x['timestamp'])
    deduped = []
    last_t = -999
    for f in faults:
        if f['timestamp'] - last_t > 0.1:
            deduped.append(f)
            last_t = f['timestamp']
    return deduped

# ── Summary ───────────────────────────────────────────────────────────────────

def print_summary(records, faults, interfaces):
    times = [r['timestamp'] for r in records]
    duration = times[-1] - times[0] if len(times) > 1 else 0

    print(f"\n{'='*65}")
    print(f"JOINT STATE ANALYSIS SUMMARY")
    print(f"{'='*65}")
    print(f"Total records:  {len(records)}")
    print(f"Duration:       {duration:.1f}s ({duration/60:.1f} min)")
    print(f"Sample rate:    ~{len(records)/duration:.0f} Hz" if duration > 0 else "")
    print(f"Interfaces:     {', '.join(interfaces)}")
    print(f"\nFault events detected: {len(faults)}")

    if faults:
        print(f"\n{'─'*65}")
        print(f"{'#':<4} {'Time (s)':<14} {'Joint':<12} {'Error Code':<14} {'Voltage (mV)':<14} {'Current'}")
        print(f"{'─'*65}")
        for i, f in enumerate(faults):
            print(f"{i+1:<4} {f['timestamp']:<14.3f} {f['joint']:<12} "
                  f"{int(f['error_code']):<14} {f['bus_voltage']:<14.0f} {f['current']:.0f}")

    print(f"\n{'─'*65}")
    print("BUS VOLTAGE STATS (mV)")
    print(f"{'─'*65}")
    for joint in JOINTS:
        key = f'{joint}/bus_voltage'
        vals = [r[key] for r in records if not np.isnan(r.get(key, float('nan')))]
        if vals:
            print(f"  {joint}: min={min(vals):.0f}  max={max(vals):.0f}  "
                  f"mean={np.mean(vals):.0f}  std={np.std(vals):.1f}")

    if 'status_word' in interfaces:
        print(f"\n{'─'*65}")
        print("STATUSWORD STATE DISTRIBUTION (% of time)")
        print(f"{'─'*65}")
        for joint in JOINTS:
            key = f'{joint}/status_word'
            sws = [r[key] for r in records if not np.isnan(r.get(key, float('nan')))]
            if sws:
                op_en = sum(1 for sw in sws if decode_statusword(sw)['operation_enabled']) / len(sws) * 100
                fault  = sum(1 for sw in sws if decode_statusword(sw)['fault']) / len(sws) * 100
                warn   = sum(1 for sw in sws if decode_statusword(sw)['warning']) / len(sws) * 100
                print(f"  {joint}: op_enabled={op_en:.1f}%  fault={fault:.1f}%  warning={warn:.1f}%")

# ── Plots ─────────────────────────────────────────────────────────────────────

def plot_overview(records, faults, interfaces):
    times = np.array([r['timestamp'] for r in records])
    t0 = times[0]
    times = times - t0

    has_sw = 'status_word' in interfaces
    n_rows = 5 if has_sw else 4
    fig, axes = plt.subplots(n_rows, 1, figsize=(18, 14), sharex=True)
    fig.suptitle('Joint State Overview — Fault Events Marked in Red', fontsize=13)
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple']

    # Position
    ax = axes[0]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/position', float('nan')) for r in records]
        ax.plot(times, vals, label=joint, color=colors[j], linewidth=0.5)
    ax.set_ylabel('Position (rad)')
    ax.legend(loc='upper right', fontsize=7)
    ax.grid(True, alpha=0.3)

    # Velocity
    ax = axes[1]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/velocity', float('nan')) for r in records]
        ax.plot(times, vals, label=joint, color=colors[j], linewidth=0.5)
    ax.set_ylabel('Velocity (rad/s)')
    ax.grid(True, alpha=0.3)

    # Current
    ax = axes[2]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/current', float('nan')) for r in records]
        ax.plot(times, vals, label=joint, color=colors[j], linewidth=0.5)
    ax.set_ylabel('Current (‰ rated)')
    ax.grid(True, alpha=0.3)

    # Bus voltage
    ax = axes[3]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/bus_voltage', float('nan')) / 1000.0 for r in records]
        ax.plot(times, vals, label=joint, color=colors[j], linewidth=0.5)
    ax.set_ylabel('Bus Voltage (V)')
    ax.grid(True, alpha=0.3)

    # Statusword fault bit
    if has_sw:
        ax = axes[4]
        for j, joint in enumerate(JOINTS):
            vals = [1 if decode_statusword(r.get(f'{joint}/status_word', 0))['fault'] else 0
                    for r in records]
            ax.plot(times, [v + j*1.1 for v in vals], label=joint,
                    color=colors[j], linewidth=0.8, drawstyle='steps-post')
        ax.set_ylabel('Fault Bit\n(offset per joint)')
        ax.set_xlabel('Time (s)')
        ax.grid(True, alpha=0.3)

    # Mark faults
    for f in faults:
        ft = f['timestamp'] - t0
        for ax in axes:
            ax.axvline(ft, color='red', alpha=0.4, linewidth=1, linestyle='--')

    plt.tight_layout()
    plt.savefig('overview.png', dpi=150, bbox_inches='tight')
    print("Saved: overview.png")
    plt.close()

def plot_fault_detail(records, fault, idx, t0, interfaces):
    ft = fault['timestamp']
    window = [r for r in records if ft - 1.5 <= r['timestamp'] <= ft + 0.5]
    if len(window) < 5:
        return

    times = np.array([r['timestamp'] - t0 for r in window])
    fmark = ft - t0
    has_sw = 'status_word' in interfaces

    n_rows = 5 if has_sw else 4
    fig, axes = plt.subplots(n_rows, 1, figsize=(12, 11), sharex=True)
    fig.suptitle(f'Fault #{idx+1} — {fault["joint"]} at t={fmark:.3f}s  '
                 f'error_code=0x{int(fault["error_code"]):04X}  '
                 f'voltage={fault["bus_voltage"]:.0f}mV', fontsize=11)
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple']

    ax = axes[0]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/velocity', float('nan')) for r in window]
        ax.plot(times, vals, label=joint, color=colors[j])
    ax.set_ylabel('Velocity (rad/s)')
    ax.axvline(fmark, color='red', linewidth=2, label='fault')
    ax.legend(fontsize=7)
    ax.grid(True, alpha=0.3)

    ax = axes[1]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/current', float('nan')) for r in window]
        ax.plot(times, vals, label=joint, color=colors[j])
    ax.set_ylabel('Current (‰ rated)')
    ax.axvline(fmark, color='red', linewidth=2)
    ax.grid(True, alpha=0.3)

    ax = axes[2]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/torque_demand', float('nan')) for r in window]
        ax.plot(times, vals, label=joint, color=colors[j])
    ax.set_ylabel('Torque Demand')
    ax.axvline(fmark, color='red', linewidth=2)
    ax.grid(True, alpha=0.3)

    ax = axes[3]
    for j, joint in enumerate(JOINTS):
        vals = [r.get(f'{joint}/bus_voltage', float('nan')) / 1000.0 for r in window]
        ax.plot(times, vals, label=joint, color=colors[j])
    ax.set_ylabel('Bus Voltage (V)')
    ax.axvline(fmark, color='red', linewidth=2)
    ax.grid(True, alpha=0.3)

    if has_sw:
        ax = axes[4]
        for j, joint in enumerate(JOINTS):
            sw_vals = [r.get(f'{joint}/status_word', 0) for r in window]
            op_en = [1 if decode_statusword(sw)['operation_enabled'] else 0 for sw in sw_vals]
            fault_bit = [1 if decode_statusword(sw)['fault'] else 0 for sw in sw_vals]
            ax.plot(times, [v + j*2.2 for v in op_en],
                    color=colors[j], linewidth=1, drawstyle='steps-post', label=f'{joint} op_en')
            ax.plot(times, [v*0.8 + j*2.2 for v in fault_bit],
                    color=colors[j], linewidth=1, linestyle='--', drawstyle='steps-post')
        ax.set_ylabel('Status\n(solid=op_en, dash=fault)')
        ax.set_xlabel('Time (s)')
        ax.axvline(fmark, color='red', linewidth=2)
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    fname = f'fault_{idx+1}_{fault["joint"]}.png'
    plt.savefig(fname, dpi=150, bbox_inches='tight')
    print(f"Saved: {fname}")
    plt.close()

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_joint_states.py joint_states.csv")
        sys.exit(1)

    filepath = sys.argv[1]
    print(f"Parsing {filepath}...")
    records, interfaces = parse_csv(filepath)
    print(f"Parsed {len(records)} records.")

    if not records:
        print("No records parsed.")
        sys.exit(1)

    faults = detect_faults(records, interfaces)
    print_summary(records, faults, interfaces)

    t0 = records[0]['timestamp']
    print("\nGenerating plots...")
    plot_overview(records, faults, interfaces)
    for i, fault in enumerate(faults[:15]):
        plot_fault_detail(records, fault, i, t0, interfaces)

    print(f"\nDone. {1 + min(len(faults), 15)} plots generated.")

if __name__ == '__main__':
    main()