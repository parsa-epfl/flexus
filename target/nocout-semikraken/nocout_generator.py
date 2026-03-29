# %%

# This file generates the NoCout configuration file for a network with 3 types of nodes:
# 1. Compute nodes (cores)          (port 0)
# 2. LLC nodes                      (port 1)
# 3. MemoryController nodes         (port 2)
# Switches in the same row are connected with port 3 (right) and 4 (left)
# Switches in the middle column are flattened butterflies and are connected with port 5 (up) and 6 (down)
#    port 7 (up by 2) and 8 (down by 2) 
#    port 9 (up by 3) and 10 (down by 3)
#    port 11 (up by 4) and 12 (down by 4)
#    ... until port 2*y+3 (up by y-1) and 2*y+4 (down by y-1)

x=8
y=6
channel_lat = 1
fbfly_lat = 3

def create_boilerplate():
    return f"""
# Boilerplate stuff
ChannelLatency {channel_lat}
ChannelLatencyData 4
ChannelLatencyControl 1
LocalChannelLatencyDivider 4
SwitchInputBuffers 1
SwitchOutputBuffers 1
SwitchInternalBuffersPerVC 1

"""

def basic_definitions():
    return f"""
# Basic Switch/Node connections
NumNodes {x*y*3}
NumSwitches {x*y}
SwitchPorts   {y*2 + 3}
SwitchBandwidth 4
"""

def connect_cores():
    ret = "\n\n# Core connections\n"
    for yidx in range(y):
        for xidx in range(x):
            ret += f"Top Node {yidx*x + xidx} -> Switch {yidx*x + xidx}:0\n"
    return ret

def llc_and_mem_idx_to_switch_idx(idx):
    yidx = idx % y
    xidx = idx // y
    switchidx = yidx*x + (x-1)//2
    if xidx % 2 == 0:
        switchidx -= xidx//2
    else:
        switchidx += (xidx+1)//2
    return switchidx

def connect_llc():
    # Start connecting LLCs from middle to the right and left alternating
    ret = "\n\n# LLC connections\n"
    for llcidx in range(x*y):
        flexusidx = x*y + llcidx
        switchidx = llc_and_mem_idx_to_switch_idx(llcidx)
        ret += f"Top Node {flexusidx} -> Switch {switchidx}:1\n"
    return ret

def connect_mc():
    ret = "\n\n# Memory Controller connections\n"
    for memidx in range(x*y):
        flexusidx = 2*x*y + memidx
        switchidx = llc_and_mem_idx_to_switch_idx(memidx)
        ret += f"Top Node {flexusidx} -> Switch {switchidx}:2\n"
    return ret

def connect_switches_same_row():
    ret = "\n\n# Same row connections\n"
    for yidx in range(y):
        for xidx in range(x-1):
            switchidx = yidx*x + xidx
            ret += f"Top Switch {switchidx}:3:{channel_lat} -> Switch {switchidx+1}:4:{channel_lat}\n"
    return ret

def connect_switches_middle_column():
    ret = "\n\n# Middle column connections\n"
    xidx = (x-1)//2
    for connection_distance in range(1, y):
        for yidx in range(y - 1):
            switchidx = yidx*x + xidx
            if switchidx + connection_distance*x >= x*y:
                break
            ret += f"Top Switch {switchidx}:{2*connection_distance+3}:{fbfly_lat} -> Switch {switchidx + connection_distance*x}:{2*connection_distance+4}:{fbfly_lat}\n"
    return ret

def routing_algorithm():
    ret = "\n\n# Routing algorithm\n"
    for yidx in range(y):
        for xidx in range(x):
            ret += f"\n# Routing for switch {yidx*x + xidx}\n"
            # For each switch
            switchidx = yidx*x + xidx
            numNodes = x * y + 2 * y # xy Cores + y LLCs + y MCs
            for dest in range(3*x*y):
                # For each destination
                dest_flexusidx = dest
                type =""
                if dest < x*y:
                    destidx = dest
                    type = "Core"
                elif dest < 2*x*y:
                    destidx = llc_and_mem_idx_to_switch_idx(dest - x*y)
                    type = "LLC"
                elif dest < 3*x*y:
                    destidx = llc_and_mem_idx_to_switch_idx(dest - 2*x*y)
                    type = "MC"
                else:
                    # will never happen
                    destidx = 0
                    type = "Core"
                dest_row = destidx // x
                dest_col = destidx % x
                if destidx == switchidx:
                    if type == "Core":
                        # Destination is the core in this switch
                        ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 0:0 }}\n"
                    elif type == "LLC":
                        # Destination is the LLC in this switch
                        ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 1:0 }}\n"
                    else:
                        # Destination is the MC in this switch
                        ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 2:0 }}\n"
                elif dest_row == yidx and dest_col > xidx:
                    # Go right
                    ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 3:0 }}\n"
                elif dest_row == yidx and dest_col < xidx:
                    # Go left
                    ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 4:0 }}\n"
                elif xidx == (x-1)//2:
                    # If in the middle column go to correct row with flattened butterfly
                    distance = abs(dest_row - yidx)
                    port_to_use = 2*distance + 3 if dest_row > yidx else 2*distance + 4
                    ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ {port_to_use}:0 }}\n"
                elif xidx < (x-1)//2:
                    # Go right
                    ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 3:0 }}\n"
                else:
                    # Go left
                    ret += f"Route Switch {switchidx} -> {dest_flexusidx} {{ 4:0 }}\n"
    return ret

nocout = create_boilerplate()
nocout += basic_definitions()
nocout += connect_cores()
nocout += connect_llc()
nocout += connect_mc()
nocout += connect_switches_same_row()
nocout += connect_switches_middle_column()
nocout += routing_algorithm()

with open("nocout_config.txt", "w") as f:
    f.write(nocout)
# %%
