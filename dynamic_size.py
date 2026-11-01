import yaml # Requires: pip install pyyaml

def get_topology_size(yaml_file_path):
    with open(yaml_file_path, 'r') as file:
        config = yaml.safe_load(file)
        
    # Check if it's a Multi-Mesh topology
    if 'multi_mesh' in config:
        layers = config['multi_mesh']
        z_size = len(layers) # Number of layers is the Z dimension
        
        # Get X and Y dimensions from the first layer (assuming uniform layers)
        first_layer_name = list(layers.keys())[0]
        x_size = layers[first_layer_name].get('dim_x')
        y_size = layers[first_layer_name].get('dim_y')
        
        print(f"Dynamically Extracted Size: {x_size}x{y_size} across {z_size} layers.")
        return x_size, y_size, z_size
    else:
        print("Standard Mesh topology detected.")
        return None, None, None

if __name__ == "__main__":
    CONFIG_FILE = "/home/krish/Noxim-3.5D-Updated/config_examples/3DPARTIALLYCONNECTED.yaml"
    get_topology_size(CONFIG_FILE)