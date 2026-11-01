# Noxim 3.5D
Please find the original Noxim README.md content below.

Welcome to Noxim 3.5D, a modification of the Noxim simulator (based on https://github.com/davidepatti/noxim/commit/39fa87c6b729a63e0ade946d5b250f4615fd725d).
It is a Network-On-Chip (NoC) simulator for a multi-chiplet architecture, where the chiplets are interconnected vertically.
3.5D means that the topology corresponds to a "tree of chiplet", where a chiplet can have multiple chiplets stacked vertically onto it, but it can be only stacked onto one.
Each chiplet has an internal 2D or 3D mesh NoC. 
This simulator can be also used to simulate more classical topologies, such as 2.5D and 3D topologies (3.5D being a super-set of both 2.5D/3D topologies).

Main authors: Davy Million (davy.million@cea.fr), César Fuguet (cesar.fuguet@univ-grenoble-alpes.fr)

This simulator has been used for a paper (currently in review) titled "Depth-First A Deterministic and Scalable NoC Routing Protocol for 3.5D Packaged Architectures".
The instructions given in the section "Results reproduction" can be used to reproduce the results we obtained in this paper.

## New features
* Support of 3D-Mesh topology ;
* Support of 3.5D (Multi-Mesh) topology and implementation of a generic 3.5D hierarchical routing (Depth-First), that can be repurposed to test different VC allocations ;
* Virtual Channel (VC) switching in routers: ability for a routing algorithm to change the VC of a packet during routing ;
* VC Reallocation: ability for a router to deallocate a reserved VC and reallocate another one (depending on the routing algorithm) when a packet header cannot be forwarded ;
* Support for atomic flow control, where a given buffer can contain at most one packet (see Duato's Theory) ;
* Add new routing algorithms such as Elevator-First (with an extra routing header), DeFT, VDA, deterministic Depth-First and (experimental) adaptive Depth-First ;
* Noxim Explorer: add a new Python3 backend (prior was matlab only) to generate performance results ;
* Add new scripts to generate an optimized Vertical Link (VL) selection based on the method proposed by DeFT (exhaustive search) and our MILP implementation.

## Cloning this repository
```
git clone <this_repository>
cd noxim
```

## Building the simulator
To build the simulator, you need a compiler that supports `c++14` and an installation of both the SystemC and the YAML C++ libraries.
To ease this step, the script `bin/install.sh` can be used to clone, build and install those dependencies:
```
bin/install.sh
```
This installation must be done only once. Once the dependencies are installed, make sure that the environment variables `SYSTEMC_HOME` and `YAML_HOME` are properly set by sourcing `bin/env.sh`:
```
source bin/env.sh
```
This must be done each time a new shell is used.
Finally, the simulator can be built with:
```
make -C bin clean
export JOBS=8
make -C bin -j$JOBS
```
where `JOBS` is the number of parallel jobs when compiling.

To ease development, you can compile the simulator in debug mode:
```
make -C bin -j$JOBS DEBUG=1
```
The debug mode gives information about the VC utilization for each internal NoC of the multi-mesh.
You can also use gdb on it.

## Launching noxim
The multi-mesh configuration files contain the topology of the network (starting from the `multi_mesh` node) and the parameters of the simulation/routers.
These files (.yaml) are in the `config_examples` folder (files with 'MultiMesh' in the name).
We left the original config files (`default_configMesh.yaml`...) in the folder but we provide no garantee that they are still working.

Once built, the simulator can be launched by picking any multi-mesh configuration file and placing it after the `-config` option as shown below:
```
bin/noxim -config config_examples/2D5MultiMeshBaselineI1VC.yaml -power bin/power.yaml
```
The power model (bin/power.yaml) is also necessary.
Please note that the power model of the Depth-First router is the same as the one of an X-Y router, which is not realistic.
Hence the energy estimation reported by the simulator should not be considered as a meaningful data.

## 3.5D topology description (YAML)
Compared to the base version of Noxim, the simulator supports a new `multi_mesh` node in the YAML configuration files.
This node describes the network topology of the architecture.
It contains several entries (`Interposer`, `chiplet_A`, `chiplet_B`...) that each represent an interposer/chiplet composing the system:
```
multi_mesh:
  Interposer:
    [...]
  chiplet_A:
    [...]
  chiplet_B:
    [...]
  [...]
```
The chiplets must be defined in the order of a bottom-up traversal of the topology (bottom=interposer).
The network characteristics of an entry is defined as follows:
```
  chiplet_A
    dim_x: 3
    dim_y: 2
    dim_z: 1
    n_virtual_channels: 2
    custom_pe: [0, 3]
```
* `dim_x` is the size of the X dimension of the 3D-Mesh ;
* `dim_y` is the size of the Y dimension of the 3D-Mesh ;
* `dim_z` is the size of the Z dimension of the 3D-Mesh, `dim_z=1` models a 2D-Mesh ;
* `n_virtual_channels` corresponds to the number of virtual channels in the chiplet network ;
* `custom_pe` is an optional entry listing the nodes acting as Processing Elements (PEs), hence emitting and sending packets in the network.
If not defined, it is assumed that all the nodes are valid PEs.
The IDs used in the list corresponds to the local ID of the nodes in the chiplet.
These local IDs are allocated based on the X/Y/Z coordinnates using the formula: `z*(dim_x*dim_y) + y*dim_x + x`
```
     2x3x1 (X/Y/Z)
   ---    ---    ---
  | 3 |--| 4 |--| 5 |
   ---    ---    ---
    |      |      |
Y  ---    ---    ---
^ | 0 |--| 1 |--| 2 |
|  ---    ---    ---
 --> X
```
The vertical links between the chiplets are defined using the `vertical_links_placement` node.
This node must be defined for each chiplet that is vertically connected to other chiplets.
This node has two sub-nodes: `up` and `down`.
```
    vertical_links_placement:
      up:
        chiplet_C:
          # Upward VL of src 0 in A and dest 1 in C
          - [0, 1]
          # Upward VL of src 1 in A and dest 2 in C
          - [1, 2]
        chiplet_D:
          # Upward VL of src 3 in A and dest 1 in D
          - [3, 1]
          # Upward VL of src 4 in A and dest 2 in D
          - [4, 2]
      down:
        [...]
```
In the `up` case, the list of chiplets that are directly stacked on the current chiplet being defined must be then provided.
For each chiplet mentioned, an entry composed of a list of 2 (local) IDs must be inserted for each unidirectional upward VL going from the chiplet being defined (A in our example) to the directly stacked chiplet being mentioned (C and D).

In the `down` case, there is no need to mention the destination as it has been already expressed by the 'up' node defined in the chiplet below.
This is due to the fact that in a 3.5D system, a chiplet cannot be stacked on multiple chiplets.
```
    vertical_links_placement:
      up:
        [...]
      down:
        # Downward VL of src 0 in A to its 'parent'
        - [0, 1]
```
The interposer (first node defined in the multimesh) does not have a `down` sub-node as it is not stacked on anything.

To route a packet to another chiplet, a router must select a VL that target this chiplet.
This selection is done statically as a pre-processing step of the topology, hence there is no dynamic reconfiguration depending on the load of the NoC.
The default method used is the Nearest Vertical Link Selection which selects the VL that is the closest to the router emitting the packet.
This method is simple but it is not always optimal.
To improve performance, an optional node called `vertical_links_selection` can be inserted.
This node has the same structure as the `vertical_links_placement` node with `up`, `down` and the list of chiplets in the `up` case, although now the list of IDs to provide represents the mapping between an emitting router and another carrying a VL that goes to the targeted chiplet.
```
    vertical_links_selection:
      up:
        chiplet_C:
          # Router 0 of A targets the VL at node 0 to go to chiplet_C
          - [ 0,  0]
          [...]
          # Router 7 of A targets the VL at node 1 to go to chiplet_C
          - [ 7,  1]
```

Examples of these definitions can be found in the `config_examples` folder (`2D5MultiMeshBaselineI1VC.yaml`, `2D5MultiMeshBaseline.yaml`, `3D5MultiMeshBaselineWithoutVLSelection.yaml` and `3D5MultiMeshBaseline.yaml`).

## Generate an optimized vertical link selection
When developing a new topology, a user can optimize the selection of the VLs applied to each router.
The script `generate_selected_vertical_link.py` in `other/vlink_selection/` parses a given multi-mesh YAML file (which does not have any `vertical_links_selection` YAML node) and re-generates the same file with a `vertical_links_selection` node for each mesh.
The following commands show how to perform this process:
```
cd other/vlink_selection
python3 generate_selected_vertical_link.py ../../config_examples/3D5MultiMeshBaselineWithoutVLSelection.yaml
```
This command generates the same configuration file in `other/vlink_selection`, containing now the `vertical_links_selection` nodes.
Under the hood, this script calls the `milp.py` python module that apply the MILP-based VL selection to each 2D/3D-mesh of the topology.

## Results reproduction
The following instructions allows a user to launch the set of experiments that we performed in our paper:
* Compile `noxim` in standard mode (not DEBUG) to speed-up simulation time.
To do so, use the instructions provided in the section "Building the simulator" ;
* Compile noxim-explorer by doing:
```
make -C other clean
make -C other
```
* Launch all Depth-First related experiments:
```
cd other
./depth_first_launch_all.sh
```
Please note that some experiments requires python3 with scipy, for milp optimization, and ruamel.yaml, to insert the optimized router selection automatically in the yaml file.
Expect ~2 days to end all the simulations.
* Generate the graph plots:
```
./depth_first_plots.sh
```
This will generate all the graphs in `other`.

## Known issues
* Power estimation of a Depth-First router is not accurate ;
* Elevator-First must be rewritten to work with the 3.5D format ;
* 2.5D/3D/3.5D topology Checking is not working!

## Possible interesting extensions
This fork of Noxim was designed for research purposes (during a PhD) and as such, it fully met our needs. However, we acknowledge that the code quality is not perfect. For example, it lacks proper testing and some features (regarding the non-mesh topology of the supported NoC) were removed. Hence, it is not ready to even tentatively merge it with the upstream.
In summary, some strong following work is needed to propose a cleaner version. In particular, the following contributions might be of interest:
* Extend the multi-mesh topology to a multi-NoC topology where each local NoC can be an arbitrary topology (mesh, ring, tree...). We removed the code from NoC.cpp that is generating the non-mesh topologies. This would require to bring it back and adapt it to the new class hierarchy ;
* Probably, the class hierarchy could be rewritten to ensure a better modularity ;
* Extending the current unit testing infrastructure would be great.

Noxim - the NoC Simulator
=========================

Welcome to Noxim, the Network-on-Chip Simulator developed at the University of Catania (Italy).
The Noxim simulator is developed using SystemC, a system description language based on C++, and
it can be downloaded under GPL license terms.

**If you use Noxim in your research, we would appreciate the following citation in any publications to which it has contributed:**

V. Catania, A. Mineo, S. Monteleone, M. Palesi and D. Patti, "Improving the energy efficiency of wireless Network on Chip architectures through online selective buffers and receivers shutdown," 2016 13th IEEE Annual Consumer Communications & Networking Conference (CCNC), Las Vegas, NV, 2016, pp. 668-673, doi: 10.1109/CCNC.2016.7444860.
[Scopus reference](https://www.scopus.com/record/display.uri?eid=2-s2.0-84966659566&origin=resultslist&sort=plf-f&src=s&sid=b531296d946a78b05f463c35c681a44c&sot=autdocs&sdt=autdocs&sl=18&s=AU-ID%2835610853000%29&relpos=14&citeCnt=6&searchTerm=)

V. Catania, A. Mineo, S. Monteleone, M. Palesi and D. Patti, "Energy efficient transceiver in wireless Network on Chip architectures," 2016 Design, Automation & Test in Europe Conference & Exhibition (DATE), Dresden, 2016, pp. 1321-1326.
[Scopus reference](https://www.scopus.com/record/display.uri?eid=2-s2.0-84973661681&origin=resultslist&sort=plf-f&src=s&sid=4bd3ffce04cc0093a84655249383aefa&sot=autdocs&sdt=autdocs&sl=18&s=AU-ID%2835610853000%29&relpos=11&citeCnt=11&searchTerm=)

Registration
------------
To receive information about new Noxim features, updates and events, please register here:
[Registration Form](https://docs.google.com/forms/d/e/1FAIpQLSfJnYQZwxC4gr4jUc-nuwuGp0MDBA-0N_TVf8hqV1DIa325Dg/viewform?c=0&w=1)

What's New ? 
------------
**[March 2018]**

  * Support for virtual channels for improved traffic management
  * Support for multiple radio-frequency channels for each Radio-Hub
  * New yaml examples (please update yours, since format is slightly different)
  
**[April 2017]** [Noxim tutorial slides](doc/noxim_tutorial.pdf) from lecture given at "Advanced Computer Architectures" (ELEC3219 - University of Southampton)

**[June 2015]** Massively improved version of Noxim. Major changes include:

  * Wireless transmission support
  * Hub connections for eterogeneous topologies
  * YAML configuration file for all features/parameters
  * Totally rewritten power model to support fine-grained estimation
  * Modular plugin-like addition of Routing/Selection strategies
  * Optional accurate logs for deep debugging (see DEBUG in Makefile)

Installation & Quick Start
-----------

If you are working on Ubuntu, you can install noxim and all the dependencies with the following command:
(**BE sure of copying the entire line, i.e., ending with "ubuntu.sh**)

    bash <(wget -qO- --no-check-certificate https://raw.githubusercontent.com/davidepatti/noxim/master/other/setup/ubuntu.sh)

Similarly for macOS:

    /bin/zsh -c "$(curl -fsSL https://raw.githubusercontent.com/davidepatti/noxim/master/other/setup/macos.zsh)"

Or, to get just the latest master sources, you can run:

    git clone https://github.com/davidepatti/noxim.git

Noxim has a command line interface for defining several parameters of a NoC. In particular the
user can customize the network size, buffer size, packet size distribution, routing algorithm,
selection strategy, packet injection rate, traffic time distribution, traffic pattern, hot-spot
traffic distribution.

The simulator allows NoC evaluation in terms of throughput, delay and power consumption. This
information is delivered to the user both in terms of average and per-communication results.

In detail, the user is allowed to collect different evaluation metrics including the total number
of received packets/flits, global average throughput, max/min global delay, total energy consumption,
per-communications delay/throughput/energy etc.

The Noxim simulator is shipped along with Noxim Explorer, a tool useful during the design space
exploration phase. Infact, Noxim Explorer executes many simulations using Noxim in order to explore
the design space, and modifying the conﬁguration parameters for each simulation. Noxim Explorer will
create new configuration parameters for you, or complete the exploration according to the information
read from a script (known as exploration script or space file).


