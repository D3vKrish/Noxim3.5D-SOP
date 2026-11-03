import subprocess
import re
import numpy as np
import csv
import sys
import argparse

# ==========================================
# CONFIGURATION DEFAULTS
# ==========================================
# Testing Modes:
#   'both'             : Shows BOTH the scenario table and the PIR values table (default)
#   'scenarios'        : Shows only the different traffic scenarios table (constant PIR)
#   'pir_sweep'        : Shows only the table across different PIR values
DEFAULT_MODE = "both"

# Constant Traffic Configuration (Table 1: Scenarios under fixed PIR)
DEFAULT_CONSTANT_PIR = 0.005
DEFAULT_SCENARIOS = ["random", "shuffle", "bitreversal", "local_chiplet 0.7"]

# Different PIR Values Configuration (Table 2: Sweep across injection rates)
DEFAULT_PIR_VALUES = [0.002, 0.004, 0.006, 0.008, 0.010, 0.012, 0.014]
DEFAULT_TRAFFIC_PATTERN = "random"

DEFAULT_SIM_CYCLES = 100000
DEFAULT_WARMUP_CYCLES = 10000

# Fixed random seed so Mode 1 (DB), Mode 2 (ERR), and Mode 0 (AdEle+) 
# experience the exact same packet injection stream.
RANDOM_SEED = 12345

CONFIG_FILE = "/home/krish/Noxim-3.5D-Updated/config_examples/3DPARTIALLYCONNECTED.yaml"
NOXIM_BIN = "/home/krish/Noxim-3.5D-Updated/bin/noxim"

# Threshold and smoothing factor from AdEle+ paper (Table 3: tr_DB = 0.06 - 0.08 for 4x4x4)
CALCULATED_TR_DB = 0.08
ALPHA_VAL = 0.2


# ==========================================
# RUN SIMULATION FUNCTION
# ==========================================
def run_simulation(config_file, pir, mode, traffic_pattern, sim_cycles, warmup_cycles, 
                   seed=RANDOM_SEED, tr_db=None, alpha=None):
    """
    Runs Noxim for a specific PIR and Mode, returning:
    latency, throughput, elev_cost, sent_flits, recv_flits, total_energy
    Mode 1 = Forced DB, Mode 2 = Forced ERR, Mode 0 = Adaptive AdEle+
    """
    traffic_args = traffic_pattern.split(" ")
    
    cmd = [
        NOXIM_BIN,
        "-config", config_file,
        "-power", "bin/power.yaml",
        "-seed", str(seed),
        "-pir", str(round(pir, 4)), "poisson"
    ]
    
    cmd.extend(["-traffic"] + traffic_args)
    
    cmd.extend([
        "-sim", str(sim_cycles),
        "-warmup", str(warmup_cycles),
        "-elevator_mode", str(mode)
    ])

    if tr_db is not None:
        cmd.extend(["-tr_db", str(round(tr_db, 4))])
    if alpha is not None:
        cmd.extend(["-alpha", str(round(alpha, 2))])
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print(f"[ERROR PIR={pir} Mode={mode} Pattern={traffic_pattern}] Noxim exited with code {result.returncode}")
        if result.stderr:
            print("STDERR:", result.stderr.strip())
        return float('inf'), 0.0, 0.0, 0, 0, 0.0
        
    output = result.stdout

    # Parse Latency
    latency_match = re.search(r"Global average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
    if not latency_match:
        latency_match = re.search(r"Average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
        
    # Parse Average IP Throughput (flits/cycle/IP)
    throughput_match = re.search(r"Average IP throughput \(flits/cycle/IP\):\s*([\d\.e\+\-]+)", output, re.IGNORECASE)
    
    # Parse Elevator Delay
    elevator_cost_match = re.search(r"Average elevator delay:\s*([\d\.e\+\-]+)", output)

    # Parse Flit counts
    sent_match = re.search(r"Total sent flits:\s*(\d+)", output)
    recv_match = re.search(r"Total received flits:\s*(\d+)", output)

    # Parse Energy
    energy_match = re.search(r"Total energy \(J\):\s*([\d\.e\+\-]+)", output)

    latency = float(latency_match.group(1)) if latency_match else float('inf')
    throughput = float(throughput_match.group(1)) if throughput_match else 0.0
    elev_cost = float(elevator_cost_match.group(1)) if elevator_cost_match else 0.0
    sent_flits = int(sent_match.group(1)) if sent_match else 0
    recv_flits = int(recv_match.group(1)) if recv_match else 0
    total_energy = float(energy_match.group(1)) if energy_match else 0.0

    return latency, throughput, elev_cost, sent_flits, recv_flits, total_energy


# ==========================================
# TABLE 1: CONSTANT TRAFFIC SCENARIO COMPARISON
# ==========================================
def evaluate_constant_traffic(config_file, scenarios, constant_pir, tr_db_val, alpha_val, 
                              sim_cycles, warmup_cycles):
    """
    Evaluates multiple scenarios under CONSTANT traffic injection rate (PIR).
    Ensures identical packet stream across Mode 1 (DB), Mode 2 (ERR), and Mode 0 (AdEle+).
    """
    print(f"\n" + "=" * 94)
    print(f"TABLE 1: SCENARIO COMPARISON UNDER CONSTANT TRAFFIC")
    print(f"Fixed PIR = {constant_pir:.4f} | Sim Cycles = {sim_cycles} | Random Seed = {RANDOM_SEED}")
    print(f"AdEle+ Parameters: TR_DB = {tr_db_val:.4f}, Alpha = {alpha_val:.2f}")
    print("=" * 94)
    print(f"{'Scenario':<18} | {'DB Lat':<8} | {'ERR Lat':<8} | {'Adapt Lat':<9} | {'DB Tput':<8} | {'ERR Tput':<8} | {'Adapt Tput':<10} | {'Sent Flits':<10}")
    print("-" * 94)

    csv_data = []

    for scenario in scenarios:
        lat_db, tput_db, elev_db, sent_db, recv_db, energy_db = run_simulation(
            config_file, constant_pir, mode=1, traffic_pattern=scenario, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED
        )
        lat_err, tput_err, elev_err, sent_err, recv_err, energy_err = run_simulation(
            config_file, constant_pir, mode=2, traffic_pattern=scenario, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED
        )
        lat_adapt, tput_adapt, elev_adapt, sent_adapt, recv_adapt, energy_adapt = run_simulation(
            config_file, constant_pir, mode=0, traffic_pattern=scenario, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED,
            tr_db=tr_db_val, alpha=alpha_val
        )

        csv_data.append([
            scenario, constant_pir,
            lat_db, lat_err, lat_adapt,
            tput_db, tput_err, tput_adapt,
            elev_db, elev_err, elev_adapt,
            sent_db, recv_db, recv_err, recv_adapt,
            energy_db, energy_err, energy_adapt
        ])

        print(f"{scenario:<18} | {lat_db:<8.2f} | {lat_err:<8.2f} | {lat_adapt:<9.2f} | {tput_db:<8.4f} | {tput_err:<8.4f} | {tput_adapt:<10.4f} | {sent_adapt:<10d}")

    print("-" * 94)

    csv_filename = "adele_comparison_constant_traffic.csv"
    with open(csv_filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'Scenario', 'PIR',
            'Latency_DB', 'Latency_ERR', 'Latency_Adaptive',
            'Throughput_DB', 'Throughput_ERR', 'Throughput_Adaptive',
            'ElevatorDelay_DB', 'ElevatorDelay_ERR', 'ElevatorDelay_Adaptive',
            'Sent_Flits', 'RecvFlits_DB', 'RecvFlits_ERR', 'RecvFlits_Adaptive',
            'Energy_DB', 'Energy_ERR', 'Energy_Adaptive'
        ])
        writer.writerows(csv_data)

    print(f"Table 1 saved to '{csv_filename}'.\n")
    return csv_data


# ==========================================
# TABLE 2: DIFFERENT PIR VALUES COMPARISON
# ==========================================
def evaluate_pir_sweep(config_file, pir_values, traffic_pattern, tr_db_val, alpha_val, 
                       sim_cycles, warmup_cycles):
    """
    Evaluates performance across different PIR values.
    Traffic injection is constant across Mode 1, Mode 2, and Mode 0 for each PIR point.
    """
    print(f"\n" + "=" * 94)
    print(f"TABLE 2: COMPARISON ACROSS DIFFERENT PIR VALUES")
    print(f"Traffic: {traffic_pattern.upper()} | Sim Cycles = {sim_cycles} | Random Seed = {RANDOM_SEED}")
    print(f"AdEle+ Parameters: TR_DB = {tr_db_val:.4f}, Alpha = {alpha_val:.2f}")
    print("=" * 94)
    print(f"{'PIR':<8} | {'DB Lat':<8} | {'ERR Lat':<8} | {'Adapt Lat':<9} | {'DB Tput':<8} | {'ERR Tput':<8} | {'Adapt Tput':<10} | {'Sent Flits':<10}")
    print("-" * 94)

    csv_data = []

    for pir in pir_values:
        pir = round(float(pir), 4)
        
        # All 3 modes receive the exact same seed and injection stream
        lat_db, tput_db, elev_db, sent_db, recv_db, energy_db = run_simulation(
            config_file, pir, mode=1, traffic_pattern=traffic_pattern, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED
        )
        lat_err, tput_err, elev_err, sent_err, recv_err, energy_err = run_simulation(
            config_file, pir, mode=2, traffic_pattern=traffic_pattern, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED
        )
        lat_adapt, tput_adapt, elev_adapt, sent_adapt, recv_adapt, energy_adapt = run_simulation(
            config_file, pir, mode=0, traffic_pattern=traffic_pattern, 
            sim_cycles=sim_cycles, warmup_cycles=warmup_cycles, seed=RANDOM_SEED,
            tr_db=tr_db_val, alpha=alpha_val
        )
        
        csv_data.append([
            pir, 
            lat_db, lat_err, lat_adapt, 
            tput_db, tput_err, tput_adapt,
            elev_db, elev_err, elev_adapt,
            sent_adapt, recv_db, recv_err, recv_adapt,
            energy_db, energy_err, energy_adapt
        ])
        
        print(f"{pir:<8.4f} | {lat_db:<8.2f} | {lat_err:<8.2f} | {lat_adapt:<9.2f} | {tput_db:<8.4f} | {tput_err:<8.4f} | {tput_adapt:<10.4f} | {sent_adapt:<10d}")

    print("-" * 94)
    
    csv_filename = f"adele_comparison_pir_{traffic_pattern.replace('/', '_').replace(' ', '_')}.csv"
    with open(csv_filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'PIR', 
            'Latency_DB', 'Latency_ERR', 'Latency_Adaptive', 
            'Throughput_DB', 'Throughput_ERR', 'Throughput_Adaptive',
            'ElevatorDelay_DB', 'ElevatorDelay_ERR', 'ElevatorDelay_Adaptive',
            'Sent_Flits', 'RecvFlits_DB', 'RecvFlits_ERR', 'RecvFlits_Adaptive',
            'Energy_DB', 'Energy_ERR', 'Energy_Adaptive'
        ])
        writer.writerows(csv_data)
    
    print(f"Table 2 saved to '{csv_filename}'.\n")
    return csv_data


# ==========================================
# MAIN ENTRY POINT
# ==========================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="AdEle+ Performance Comparison across DB (Mode 1), ERR (Mode 2), and Adaptive (Mode 0) modes."
    )
    parser.add_argument("--mode", choices=["both", "scenarios", "pir_sweep"], default=DEFAULT_MODE,
                        help="Display mode: 'both' shows both scenario and PIR tables (default), 'scenarios', or 'pir_sweep'")
    parser.add_argument("--pir", type=float, default=DEFAULT_CONSTANT_PIR,
                        help=f"Fixed PIR for scenario table (default: {DEFAULT_CONSTANT_PIR})")
    parser.add_argument("--pir_values", type=str, default="",
                        help="Comma-separated PIR values for Table 2 (e.g. '0.012,0.014,0.020,0.030')")
    parser.add_argument("--pir_start", type=float, default=0.01,
                        help="Start PIR for range sweep if --pir_values is omitted (default: 0.002)")
    parser.add_argument("--pir_end", type=float, default=0.05,
                        help="End PIR for range sweep if --pir_values is omitted (default: 0.014)")
    parser.add_argument("--pir_step", type=float, default=0.01,
                        help="Step size for range sweep (default: 0.002)")
    parser.add_argument("--traffic", type=str, default=DEFAULT_TRAFFIC_PATTERN,
                        help=f"Traffic pattern for PIR sweep (default: '{DEFAULT_TRAFFIC_PATTERN}')")
    parser.add_argument("--sim", type=int, default=DEFAULT_SIM_CYCLES,
                        help=f"Total simulation cycles per run (default: {DEFAULT_SIM_CYCLES})")
    parser.add_argument("--warmup", type=int, default=DEFAULT_WARMUP_CYCLES,
                        help=f"Warmup simulation cycles (default: {DEFAULT_WARMUP_CYCLES})")
    parser.add_argument("--tr_db", type=float, default=CALCULATED_TR_DB,
                        help=f"TR_DB threshold for AdEle+ (default: {CALCULATED_TR_DB})")
    parser.add_argument("--alpha", type=float, default=ALPHA_VAL,
                        help=f"Alpha EWMA smoothing weight (default: {ALPHA_VAL})")
    parser.add_argument("--config", type=str, default=CONFIG_FILE,
                        help="Path to YAML topology configuration")

    args = parser.parse_args()

    # Determine PIR values list
    if args.pir_values:
        pir_values = [float(x.strip()) for x in args.pir_values.split(",") if x.strip()]
    else:
        pir_values = list(np.arange(args.pir_start, args.pir_end + args.pir_step / 2.0, args.pir_step))

    # Execute requested tables
    # if args.mode in ["both", "scenarios"]:
    #     evaluate_constant_traffic(
    #         args.config, DEFAULT_SCENARIOS, args.pir, args.tr_db, args.alpha, 
    #         args.sim, args.warmup
    #     )

    if args.mode in ["both", "pir_sweep"]:
        evaluate_pir_sweep(
            args.config, pir_values, args.traffic, args.tr_db, args.alpha, 
            args.sim, args.warmup
        )
