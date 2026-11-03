import sys
import re
import yaml

def extract_link_pairs(direction_data):
    """Safely extracts [src, dst] pairs whether direction_data is a dict or a list."""
    pairs = []
    if isinstance(direction_data, dict):
        for _, link_list in direction_data.items():
            if isinstance(link_list, list):
                pairs.extend(link_list)
    elif isinstance(direction_data, list):
        pairs.extend(direction_data)
    return pairs

def parse_yaml_config(yaml_path):
    with open(yaml_path, 'r') as f:
        config = yaml.safe_load(f)

    topology = config.get('topology', '')
    router_configs = {}
    current_global_id = 0

    if topology == 'MULTI_MESH':
        mm = config.get('multi_mesh', {})
        
        def layer_key(name):
            match = re.search(r'\d+', name)
            return int(match.group()) if match else 0

        sorted_layers = sorted(mm.keys(), key=layer_key)

        for layer_name in sorted_layers:
            l_cfg = mm[layer_name]
            dim_x = l_cfg.get('dim_x', 4)
            dim_y = l_cfg.get('dim_y', 4)

            vlinks = l_cfg.get('vertical_links_placement', {}) or {}

            raw_up_links = extract_link_pairs(vlinks.get('up'))
            up_elevators = []
            for link in raw_up_links:
                if isinstance(link, (list, tuple)) and len(link) > 0:
                    src_lid = link[0]
                    if src_lid not in up_elevators:
                        up_elevators.append(src_lid)

            if not up_elevators:
                raw_down_links = extract_link_pairs(vlinks.get('down'))
                for link in raw_down_links:
                    if isinstance(link, (list, tuple)) and len(link) > 0:
                        src_lid = link[0]
                        if src_lid not in up_elevators:
                            up_elevators.append(src_lid)

            if not up_elevators:
                up_elevators = [0]

            for local_y in range(dim_y):
                for local_x in range(dim_x):
                    local_id = local_y * dim_x + local_x
                    gid = current_global_id + local_id

                    dists = []
                    for e_lid in up_elevators:
                        ex = e_lid % dim_x
                        ey = e_lid // dim_x
                        e_gid = current_global_id + e_lid

                        d = abs(local_x - ex) + abs(local_y - ey)
                        q = "NE" if (local_x <= ex and local_y <= ey) else \
                            "NW" if (local_x > ex and local_y <= ey) else \
                            "SE" if (local_x <= ex and local_y > ey) else "SW"

                        dists.append({"gid": e_gid, "dist": d, "quadrant": q})

                    dists.sort(key=lambda item: item["dist"])

                    subset_gids = [e["gid"] for e in dists[:3]]
                    while len(subset_gids) < 3:
                        subset_gids.append(subset_gids[0])

                    quads = {"NW": None, "NE": None, "SW": None, "SE": None}
                    for e in dists:
                        if quads[e["quadrant"]] is None:
                            quads[e["quadrant"]] = e["gid"]

                    ce_o = dists[0]["gid"]
                    c_nw = quads["NW"] if quads["NW"] is not None else ce_o
                    c_ne = quads["NE"] if quads["NE"] is not None else ce_o
                    c_sw = quads["SW"] if quads["SW"] is not None else ce_o
                    c_se = quads["SE"] if quads["SE"] is not None else ce_o

                    router_configs[gid] = {
                        "subset": subset_gids,
                        "ce_o": ce_o,
                        "ce_nw": c_nw,
                        "ce_ne": c_ne,
                        "ce_sw": c_sw,
                        "ce_se": c_se
                    }

            current_global_id += (dim_x * dim_y)

    return router_configs, current_global_id

def generate_header(yaml_path, output_path="src/ElevatorSubsets.h"):
    configs, total_routers = parse_yaml_config(yaml_path)

    entries = []
    for gid in range(total_routers):
        cfg = configs[gid]
        sub_str = ", ".join(map(str, cfg["subset"]))
        entries.append(
            f"    /* Router {gid:2d} */ RouterElevatorConfig{{ {{{sub_str}}}, {cfg['ce_o']}, {cfg['ce_nw']}, {cfg['ce_ne']}, {cfg['ce_sw']}, {cfg['ce_se']} }}"
        )

    formatted_entries = ",\n".join(entries)

    header_content = f"""#ifndef __ELEVATOR_SUBSETS_H__
#define __ELEVATOR_SUBSETS_H__

#include <array>
#include <cstddef>

struct RouterElevatorConfig {{
    std::array<int, 3> subset;
    int ce_overall;
    int ce_nw;
    int ce_ne;
    int ce_sw;
    int ce_se;
}};

constexpr std::size_t TOTAL_ROUTERS = {total_routers};

// Direct O(1) compile-time array lookup indexed by Global Router ID
constexpr std::array<RouterElevatorConfig, TOTAL_ROUTERS> ROUTER_ELEVATOR_MAP = {{{{
{formatted_entries}
}}}};

#endif // __ELEVATOR_SUBSETS_H__
"""

    with open(output_path, "w") as f:
        f.write(header_content)
    print(f"Parsed '{yaml_path}' successfully. Generated '{output_path}' for {total_routers} routers across all layers.")

if __name__ == "__main__":
    yaml_file = sys.argv[1] if len(sys.argv) > 1 else "config_examples/3DPARTIALLYCONNECTED.yaml"
    generate_header(yaml_file)