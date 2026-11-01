import subprocess
import re
import numpy as np

# Sweep configuration
PIR_START = 0.002
PIR_END = 0.018
PIR_STEP = 0.002
SIM_CYCLES = 100000
WARMUP_CYCLES = 10000

CONFIG_FILE = "/home/krish/Noxim-3.5D-Updated/config_examples/3DPARTIALLYCONNECTED.yaml"
NOXIM_BIN = "/home/krish/Noxim-3.5D-Updated/bin/noxim"

def run_simulation(config_file, pir, mode, traffic_pattern, tr_db=None, alpha=None):
    cmd = [
        NOXIM_BIN,
        "-config", config_file,
        "-power", "bin/power.yaml",
        "-pir", str(round(pir, 4)), "poisson",
        "-traffic", traffic_pattern,
        "-sim", str(SIM_CYCLES),
        "-warmup", str(WARMUP_CYCLES),
        "-elevator_mode", str(mode),
        "-alpha", "0.2"
    ]

    if tr_db is not None:
        cmd.extend(["-tr_db", str(round(tr_db, 2))])
    if alpha is not None:
        cmd.extend(["-alpha", str(round(alpha, 2))])
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print(f"[ERROR PIR={pir} Mode={mode} Traffic={traffic_pattern}] Noxim exited with code {result.returncode}")
        return float('inf'), 0.0
        
    output = result.stdout

    latency_match = re.search(r"Global average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
    if not latency_match:
        latency_match = re.search(r"Average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
    elevator_cost_match = re.search(r"Average elevator delay:\s*([\d\.e\+\-]+)", output)

    latency = float(latency_match.group(1)) if latency_match else float('inf')
    elev_cost = float(elevator_cost_match.group(1)) if elevator_cost_match else 0.0

    return latency, elev_cost

def sweep_and_find_tr_db(config_file, traffic_pattern):
    pir_range = np.arange(PIR_START, PIR_END + PIR_STEP/2, PIR_STEP)
    print(f"{'PIR':<8} | {'DB Latency':<12} | {'ERR Latency':<12} | {'DB Elevator Delay':<18}")
    print("-" * 58)

    db_latencies = []
    err_latencies = []
    
    crossover_pir = None
    tr_db_value = None

    for pir in pir_range:
        pir = round(pir, 4)
        
        db_lat, db_cost = run_simulation(config_file, pir, mode=1, traffic_pattern=traffic_pattern)
        err_lat, _ = run_simulation(config_file, pir, mode=2, traffic_pattern=traffic_pattern)

        db_latencies.append(db_lat)
        err_latencies.append(err_lat)

        print(f"{pir:<8.4f} | {db_lat:<12.2f} | {err_lat:<12.2f} | {db_cost:<18.2f}")

        # Capture only the FIRST valid crossover point
        if tr_db_value is None and err_lat < db_lat and len(db_latencies) > 1:
            crossover_pir = pir
            tr_db_value = db_cost

    print("=" * 58)
    if tr_db_value is not None:
        print(f"Crossover detected for {traffic_pattern.upper()} at PIR = {crossover_pir}")
        print(f"Calculated TR_DB Threshold = {tr_db_value:.2f} cycles")
        
        print(f"Running Validation in Adaptive Mode (Mode 0)...")
        adaptive_lat, _ = run_simulation(config_file, crossover_pir, mode=0, traffic_pattern=traffic_pattern, tr_db=tr_db_value)
        print(f"Adaptive Latency with TR_DB {tr_db_value:.2f} = {adaptive_lat:.2f} cycles")
        return tr_db_value
    else:
        print(f"No crossover point detected for {traffic_pattern.upper()}.")
        return None

if __name__ == "__main__":
    # Omitted transpose1 and transpose2 due to 3D MULTI_MESH incompatibility 
    patterns = ["random", "bitreversal", "local_chiplet"]
    captured_thresholds = []

    print(f"Sweeping configuration file: {CONFIG_FILE}\n")
    
    for pattern in patterns:
        print(f"\n{'#'*60}")
        print(f"--- EVALUATING TRAFFIC PATTERN: {pattern.upper()} ---")
        print(f"{'#'*60}")
        
        threshold = sweep_and_find_tr_db(CONFIG_FILE, pattern)
        if threshold is not None:
            captured_thresholds.append(threshold)

    print(f"\n{'#'*60}")
    print("--- FINAL AGGREGATION ---")
    if captured_thresholds:
        generic_tr_db = sum(captured_thresholds) / len(captured_thresholds)
        print(f"Successfully calculated thresholds for {len(captured_thresholds)}/{len(patterns)} patterns.")
        print(f"Recommended Generic GlobalParams::TR_DB = {generic_tr_db:.2f} cycles")
    else:
        print("Failed to find any valid crossover points across the tested patterns.")