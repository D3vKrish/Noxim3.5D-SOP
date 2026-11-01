import json

def generate_elevator_subsets(dim_x, dim_y, dim_z, elevator_coords, max_subset_size=3):
    """
    Generates candidate elevator subsets for each router based on proximity 
    and regional quadrants for AdEle/AdEle+ routing.
    
    :param dim_x: Grid size in X dimension
    :param dim_y: Grid size in Y dimension
    :param dim_z: Grid size in Z dimension (layers)
    :param elevator_coords: List of (x, y) tuples representing elevator positions
    :param max_subset_size: Maximum elevators per router subset (default: 3)
    :return: Dict mapping router_id to subset details
    """
    router_map = {}
    num_elevators = len(elevator_coords)
    
    for z in range(dim_z):
        for y in range(dim_y):
            for x in range(dim_x):
                router_id = z * (dim_x * dim_y) + y * dim_x + x
                
                # Calculate Manhattan distance from (x, y) to all elevators
                elevator_dists = []
                for e_idx, (ex, ey) in enumerate(elevator_coords):
                    dist = abs(x - ex) + abs(y - ey)
                    elevator_dists.append({
                        "id": e_idx,
                        "pos": (ex, ey),
                        "dist": dist,
                        "quadrant": "NE" if (x <= ex and y <= ey) else
                                    "NW" if (x > ex and y <= ey) else
                                    "SE" if (x <= ex and y > ey) else "SW"
                    })
                
                # Sort elevators by Manhattan distance
                elevator_dists.sort(key=lambda item: item["dist"])
                
                # 1. Primary subset: Top-K closest elevators
                top_k_subset = [e["id"] for e in elevator_dists[:max_subset_size]]
                
                # 2. Quadrant-based closest elevators (NW, NE, SW, SE)
                quadrants = {"NW": None, "NE": None, "SW": None, "SE": None}
                for e in elevator_dists:
                    q = e["quadrant"]
                    if quadrants[q] is None:
                        quadrants[q] = e["id"]
                
                # Fallback for empty quadrants: default to closest overall elevator
                ce_overall = elevator_dists[0]["id"]
                ce_nw = quadrants["NW"] if quadrants["NW"] is not None else ce_overall
                ce_ne = quadrants["NE"] if quadrants["NE"] is not None else ce_overall
                ce_sw = quadrants["SW"] if quadrants["SW"] is not None else ce_overall
                ce_se = quadrants["SE"] if quadrants["SE"] is not None else ce_overall
                
                router_map[router_id] = {
                    "coords": (x, y, z),
                    "subset": top_k_subset,
                    "ce_overall": ce_overall,
                    "ce_nw": ce_nw,
                    "ce_ne": ce_ne,
                    "ce_sw": ce_sw,
                    "ce_se": ce_se
                }
                
    return router_map

def export_cpp_header(router_map, filename="ElevatorSubsets.h"):
    """Generates C++ header file for Noxim 3.5D integration."""
    with open(filename, "w") as f:
        f.write("#ifndef __ELEVATOR_SUBSETS_H__\n")
        f.write("#define __ELEVATOR_SUBSETS_H__\n\n")
        f.write("#include <vector>\n")
        f.write("#include <unordered_map>\n\n")
        f.write("struct RouterElevatorConfig {\n")
        f.write("    std::vector<int> subset;\n")
        f.write("    int ce_overall;\n")
        f.write("    int ce_nw;\n")
        f.write("    int ce_ne;\n")
        f.write("    int ce_sw;\n")
        f.write("    int ce_se;\n")
        f.write("};\n\n")
        f.write("// Router ID -> Elevator Configuration mapping\n")
        f.write("inline const std::unordered_map<int, RouterElevatorConfig> ROUTER_ELEVATOR_MAP = {\n")
        
        entries = []
        for r_id, config in router_map.items():
            subset_str = "{" + ", ".join(map(str, config["subset"])) + "}"
            entry = f"    {{{r_id}, {{{subset_str}, {config['ce_overall']}, {config['ce_nw']}, {config['ce_ne']}, {config['ce_sw']}, {config['ce_se']}}}}}"
            entries.append(entry)
            
        f.write(",\n".join(entries))
        f.write("\n};\n\n")
        f.write("#endif // __ELEVATOR_SUBSETS_H__\n")

if __name__ == "__main__":
    # --- Configuration Parameters ---
    DIM_X, DIM_Y, DIM_Z = 8, 8, 4
    
    # Coordinates (x, y) of vertical elevators/TSVs
    ELEVATOR_POSITIONS = [
        (1, 1),  # Elevator ID 0
        (1, 6),  # Elevator ID 1
        (6, 1),  # Elevator ID 2
        (6, 6)   # Elevator ID 3
    ]
    SUBSET_SIZE = 3
    
    # Generate mapping
    results = generate_elevator_subsets(DIM_X, DIM_Y, DIM_Z, ELEVATOR_POSITIONS, max_subset_size=SUBSET_SIZE)
    
    # Export C++ Header for Noxim 3.5D
    export_cpp_header(results, "ElevatorSubsets.h")
    print("Successfully generated ElevatorSubsets.h for Noxim 3.5D!")