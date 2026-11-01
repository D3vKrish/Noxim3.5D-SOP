import subprocess
import re
import numpy as np

# Sweep configuration
PIR_START = 0.002
PIR_END = 0.018
PIR_STEP = 0.002
SIM_CYCLES = 100000
WARMUP_CYCLES = 10000
# Fixed trailing spaces in CONFIG_FILE string
CONFIG_FILE = "/home/krish/Noxim-3.5D-Updated/config_examples/3DPARTIALLYCONNECTED.yaml"
NOXIM_BIN = "/home/krish/Noxim-3.5D-Updated/bin/noxim"

def run_simulation(config_file, pir, mode, tr_db = None, alpha = None):
    """
    Runs Noxim for a specific Packet Injection Rate (PIR) and Mode.
    Mode 1 = Forced DB, Mode 2 = Forced ERR
    """
    cmd = [
        NOXIM_BIN,
        "-config", config_file,
        "-power", "bin/power.yaml",
        "-pir", str(round(pir, 4)), "poisson",
        "-traffic", "table", "/home/krish/Noxim-3.5D-Updated/bin/t.txt",
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
        print(f"[ERROR PIR={pir} Mode={mode}] Noxim exited with code {result.returncode}")
        print("STDOUT:", result.stdout.strip())
        print("STDERR:", result.stderr.strip())
        return float('inf'), 0.0
    output = result.stdout

    # Print error output if Noxim fails to execute
    if not output and result.stderr:
        print(f"[Noxim Error PIR={pir} Mode={mode}]: {result.stderr.strip()}")

    # Parse Global Average Delay — supports scientific notation (e.g., 4.48e+02)
    latency_match = re.search(r"Global average delay \(cycles\):\s*([\d\.e\+\-]+)", output)
    if not latency_match:
        latency_match = re.search(r"Average delay \(cycles\):\s*([\d\.e\+\-]+)", output)

    # Parse Average Elevator Delay
    elevator_cost_match = re.search(r"Average elevator delay:\s*([\d\.e\+\-]+)", output)

    latency = float(latency_match.group(1)) if latency_match else float('inf')
    elev_cost = float(elevator_cost_match.group(1)) if elevator_cost_match else 0.0

    return latency, elev_cost

def sweep_and_find_tr_db(config_file):
    pir_range = np.arange(PIR_START, PIR_END + PIR_STEP/2, PIR_STEP)

    print(f"Sweeping configuration file: {config_file}")
    print(f"{'PIR':<8} | {'DB Latency':<12} | {'ERR Latency':<12} | {'DB Elevator Delay':<18}")
    print("-" * 58)

    db_latencies = []
    err_latencies = []
    db_elev_costs = []

    crossover_pir = None
    tr_db_value = None

    for pir in pir_range:
        pir = round(pir, 4)
        
        db_lat, db_cost = run_simulation(config_file, pir, mode=1)
        err_lat, _ = run_simulation(config_file, pir, mode=2)

        db_latencies.append(db_lat)
        err_latencies.append(err_lat)
        db_elev_costs.append(db_cost)

        print(f"{pir:<8.4f} | {db_lat:<12.2f} | {err_lat:<12.2f} | {db_cost:<18.2f}")

        # Detect crossover: First point where ERR latency becomes lower than DB latency
        if tr_db_value is None and err_lat < db_lat and len(db_latencies) > 1:
            crossover_pir = pir
            tr_db_value = db_cost

    print("=" * 58)
    if tr_db_value is not None:
        print(f"Crossover detected at PIR = {crossover_pir}")
        print(f"Calculated TR_DB Threshold = {tr_db_value:.2f} cycles")

        # Running a validation run
        print(f"\nRunning Validation in Adaptive Mode (Mode 0) at Crossover PIR...")
        adaptive_lat, _ = run_simulation(config_file, crossover_pir, mode=0, tr_db=tr_db_value)
        print(f"Adaptive Latency with TR_DB {tr_db_value:.2f} = {adaptive_lat:.2f} cycles")
        print(f"\nUpdate 'Router::TR_DB = {tr_db_value:.2f};' in Router.h")
    else:
        print("No crossover point detected in this PIR range. Try increasing PIR_END.")

if __name__ == "__main__":
    sweep_and_find_tr_db(CONFIG_FILE)