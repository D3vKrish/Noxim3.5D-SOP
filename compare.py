import subprocess
import re
import numpy as np
import csv

# ==========================================
# CONFIGURATION
# ==========================================
PIR_START = 0.010
PIR_END = 0.150
PIR_STEP = 0.01
SIM_CYCLES = 100000
WARMUP_CYCLES = 10000

CONFIG_FILE = "/home/krish/Noxim-3.5D-Updated/config_examples/3DPARTIALLYCONNECTED.yaml"
NOXIM_BIN = "/home/krish/Noxim-3.5D-Updated/bin/noxim"

# Update this with the generic threshold you calculated in the previous step
CALCULATED_TR_DB = 1297.30
ALPHA_VAL = 0.2
TRAFFIC_PATTERN = "random" # Can be changed to 'local_chiplet' or 'table /path/to/t.txt'

# ==========================================
# RUN SIMULATION FUNCTION
# ==========================================
def run_simulation(config_file, pir, mode, traffic_pattern, tr_db=None, alpha=None):
    """
    Runs Noxim for a specific PIR and Mode, returning Latency, Throughput, and Elevator Cost.
    Mode 1 = Forced DB, Mode 2 = Forced ERR, Mode 0 = Adaptive AdEle+
    """
    # Handle custom table traffic paths if passed
    traffic_args = traffic_pattern.split(" ")
    
    cmd = [
        NOXIM_BIN,
        "-config", config_file,
        "-power", "bin/power.yaml",
        "-pir", str(round(pir, 4)), "poisson"
    ]
    
    cmd.extend(["-traffic"] + traffic_args)
    
    cmd.extend([
        "-sim", str(SIM_CYCLES),
        "-warmup", str(WARMUP_CYCLES),
        "-elevator_mode", str(mode)
    ])

    if tr_db is not None:
        cmd.extend(["-tr_db", str(round(tr_db, 2))])
    if alpha is not None:
        cmd.extend(["-alpha", str(round(alpha, 2))])
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print(f"[ERROR PIR={pir} Mode={mode}] Noxim exited with code {result.returncode}")
        return float('inf'), 0.0, 0.0
        
    output = result.stdout

    # Parse Latency
    latency_match = re.search(r"Global average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
    if not latency_match:
        latency_match = re.search(r"Average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
        
    # Parse Throughput
    throughput_match = re.search(r"throughput.*?:\s*([\d\.e\+\-]+)", output, re.IGNORECASE)
    
    # Parse Elevator Delay
    elevator_cost_match = re.search(r"Average elevator delay:\s*([\d\.e\+\-]+)", output)

    latency = float(latency_match.group(1)) if latency_match else float('inf')
    throughput = float(throughput_match.group(1)) if throughput_match else 0.0
    elev_cost = float(elevator_cost_match.group(1)) if elevator_cost_match else 0.0

    return latency, throughput, elev_cost

# ==========================================
# EVALUATION & EXPORT LOGIC
# ==========================================
def evaluate_and_compare(config_file, traffic_pattern, tr_db_val, alpha_val):
    pir_range = np.arange(PIR_START, PIR_END + PIR_STEP/2, PIR_STEP)
    
    print(f"Starting comparison evaluation for traffic: {traffic_pattern.upper()}")
    print(f"Using TR_DB = {tr_db_val} and Alpha = {alpha_val}\n")
    print(f"{'PIR':<8} | {'DB Lat':<10} | {'ERR Lat':<10} | {'Adapt Lat':<10} | {'DB Tput':<10} | {'ERR Tput':<10} | {'Adapt Tput':<10}")
    print("-" * 85)

    csv_data = []

    for pir in pir_range:
        pir = round(pir, 4)
        
        # Mode 1: Distance-Based (Baseline 1)
        lat_db, tput_db, _ = run_simulation(config_file, pir, mode=1, traffic_pattern=traffic_pattern)
        
        # Mode 2: Congestion-Aware (Baseline 2)
        lat_err, tput_err, _ = run_simulation(config_file, pir, mode=2, traffic_pattern=traffic_pattern)
        
        # Mode 0: AdEle+ Adaptive (Proposed)
        lat_adapt, tput_adapt, _ = run_simulation(
            config_file, pir, mode=0, traffic_pattern=traffic_pattern, 
            tr_db=tr_db_val, alpha=alpha_val
        )
        
        # Append row for CSV
        csv_data.append([
            pir, 
            lat_db, lat_err, lat_adapt, 
            tput_db, tput_err, tput_adapt
        ])
        
        # Console output
        print(f"{pir:<8.4f} | {lat_db:<10.2f} | {lat_err:<10.2f} | {lat_adapt:<10.2f} | {tput_db:<10.5f} | {tput_err:<10.5f} | {tput_adapt:<10.5f}")

    print("-" * 85)
    
    # Export to CSV
    csv_filename = f"adele_comparison_{traffic_pattern.replace('/', '_').replace(' ', '_')}.csv"
    with open(csv_filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'PIR', 
            'Latency_DB', 'Latency_ERR', 'Latency_Adaptive', 
            'Throughput_DB', 'Throughput_ERR', 'Throughput_Adaptive'
        ])
        writer.writerows(csv_data)
    
    print(f"\nEvaluation complete! Data saved to '{csv_filename}'.")
    print("You can now import this CSV into Excel, MATLAB, or Python Matplotlib to generate your comparison graphs.")

if __name__ == "__main__":
    evaluate_and_compare(CONFIG_FILE, TRAFFIC_PATTERN, CALCULATED_TR_DB, ALPHA_VAL)