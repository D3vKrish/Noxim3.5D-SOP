/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the router
 */

#include "Router.h"
#include "NoC.h"

std::vector<Router*> Router::all_routers;

inline int toggleKthBit(int n, int k)
{
	return (n ^ (1 << (k-1)));
}

void Router::process()
{
    txProcess();
    rxProcess();
}

void Router::rxProcess()
{
    if (reset.read()) {
	TBufferFullStatus bfs;
        TFreeSlots fs;
	for (int i = 0; i < DIRECTIONS + 2; i++) {
	    ack_rx[i].write(0);
	    current_level_rx[i] = 0;
	    buffer_full_status_rx[i].write(bfs);
	}
	for (int i = 0; i < DIRECTIONS + 1; i++) {
	    free_slots[i].write(fs);
        }
	routed_flits = 0;
	local_drained = 0;
    } 
    else 
    {
#ifdef DEBUG_VC_UTILIZATION
        stats.updateVCCumulatedNumberOfFlits(*this);
#endif
        int router_id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ?
                         global_id : local_id;

	for (int i = 0; i < DIRECTIONS + 2; i++) {
	    if (req_rx[i].read() == 1 - current_level_rx[i])
	    { 
		Flit received_flit = flit_rx[i].read();
		int vc = received_flit.vc_id;

		if (!buffer[i][vc].IsFull())
		{
		    buffer[i][vc].Push(received_flit);
		    LOG << " Flit " << received_flit << " collected from Input[" << i << "][" << vc <<"]" << endl;
		    power.bufferRouterPush();
		    current_level_rx[i] = 1 - current_level_rx[i];

		    if (received_flit.src_id == router_id) {
			power.networkInterface();
                        stats.sentFlit(sc_time_stamp().to_double() /
                                       GlobalParams::clock_period_ps,
                                       received_flit);
#ifdef DEBUG_VC_UTILIZATION
                        stats.updateVCInjectedFlits(*this, received_flit);
#endif
                    }
		}
		else  
		{
		    LOG << " Flit " << received_flit << " buffer full Input[" << i << "][" << vc <<"]" << endl;
		    assert(i== DIRECTION_LOCAL);
		}
	    }
	    ack_rx[i].write(current_level_rx[i]);
	    TBufferFullStatus bfs;
	    for (int vc=0; vc < MAX_VIRTUAL_CHANNELS; vc++)
		bfs.mask[vc] = buffer[i][vc].IsFull();
	    buffer_full_status_rx[i].write(bfs);

            if (i < DIRECTIONS + 1) {
                TFreeSlots fs;
                for (int vc = 0; vc < MAX_VIRTUAL_CHANNELS; ++vc) {
                    fs.mask[vc] = buffer[i][vc].getCurrentFreeSlots();
                }
                free_slots[i].write(fs);
            }
            selectionStrategy->perCycleUpdate(this);
	}
    }
}

RouteData Router::getRouteData(const Flit & flit, int input_dir) const
{
    return RouteData {
        .current_id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ? global_id : local_id,
        .src_id = flit.src_id,
        .dst_id = flit.dst_id,
        .dir_in = input_dir,
        .vc_in  = flit.vc_id,
        .routing_towards_vlink = flit.routing_towards_vlink,
        .vlink_dest = flit.vlink_dest,
        .noc = &getNoC()
    };
}

void Router::txProcess()
{
  if (reset.read()) 
    {
      for (int i = 0; i < DIRECTIONS + 2; i++) 
	{
	  req_tx[i].write(0);
	  current_level_tx[i] = 0;
	}
    } 
  else 
    { 
      // 1st phase: Reservation
      for (int j = 0; j < DIRECTIONS + 2; j++) 
	{
	  int i = (start_from_port + j) % (DIRECTIONS + 2);
          int vc_nb = getNoC().n_virtual_channels;
	  for (int k = 0; k < vc_nb; k++)
	  {
	      int vc = (start_from_vc[i]+k)%(vc_nb);

#ifdef ATOMIC_FLOW_CONTROL_CHECK
              if ( i != DIRECTION_LOCAL && i != DIRECTION_HUB &&
                   routingAlgorithm->requireAtomicFlowCtrl(reflexDirection(i), vc) )
                  buffer[i][vc].atomicFlowControlCheck();
#endif 

	      if (!buffer[i][vc].IsEmpty()) 
	      {
		  Flit flit = buffer[i][vc].Front();
		  power.bufferRouterFront();
		  
                  RouteData route_data(getRouteData(flit, i));
                  routingAlgorithm->customPreRoutingRes(*this, route_data, buffer[i][vc], flit);

                  if (flit.flit_type == FLIT_TYPE_HEAD)
                  {
		      RouteEntry route_entry = route(route_data);
                      int o    = route_entry.first;
                      int vc_o = route_entry.second;

                      bool skip_reservation =
                          routingAlgorithm->customPostRoutingRes(route_data, buffer[i][vc], flit, o);
                      if (skip_reservation)
                          continue;

		      if (o >= DIRECTION_HUB_RELAY)
			  {
		      	Flit f = buffer[i][vc].Pop();
		      	f.hub_relay_node = o - DIRECTION_HUB_RELAY;
		      	buffer[i][vc].Push(f);
		      	o = DIRECTION_HUB;
			  }

		      TReservation r;
                      r.input  = i;
                      r.vc_in  = vc;
                      r.vc_out = vc_o;

                      ReservationTable::res_t blocking_entry;
		      int rt_status = reservation_table.checkReservation(r, o, blocking_entry);

		      if (rt_status == RT_AVAILABLE)
		      {
			  reservation_table.reserve(r, o);
		      }
                      else if ( GlobalParams::enable_vc_reallocation &&
                            (rt_status == RT_AR_SAME_OUTPORT_OTHER_OUTVC ||
                             rt_status == RT_AR_OTHER_OUTPORT_SAME_OUTVC ||
                             rt_status == RT_AR_OTHER_OUTPORT_OTHER_OUTVC) )
                      {
                          TReservation existing_r;
                          existing_r.input  = r.input;
                          existing_r.vc_in  = r.vc_in;
                          existing_r.vc_out = get<2>(blocking_entry);
                          int existing_r_o  = get<0>(blocking_entry);

                          sc_assert(existing_r.vc_out > -1 && existing_r_o > -1);

                          bool target_buffer_empty =
                              free_slots_neighbor[o].read().mask[r.vc_out] == GlobalParams::buffer_depth;
                          bool target_buffer_full =
                              buffer_full_status_tx[o].read().mask[r.vc_out];
                          bool require_atomic_flow_ctrl =
                              routingAlgorithm->requireAtomicFlowCtrl(o, r.vc_out);

                          if ((require_atomic_flow_ctrl && target_buffer_empty) ||
                              (!require_atomic_flow_ctrl && !target_buffer_full))
                          {
                              reservation_table.release(existing_r, existing_r_o);
		              int retry_rt_status = reservation_table.checkReservation(r, o, blocking_entry);
                              if (retry_rt_status == RT_AVAILABLE)
                              {
                                  reservation_table.reserve(r, o);
                              }
                          }
                      }
                  }
              }
          }
          start_from_vc[i] = (start_from_vc[i]+1) % vc_nb;
      }

      start_from_port = (start_from_port + 1) % (DIRECTIONS + 2);

      // 2nd phase: Forwarding
      for (int i = 0; i < DIRECTIONS + 2; i++)
      {
          vector<ReservationTable::res_t> reservations = reservation_table.getReservations(i);

          if (reservations.size() != 0)
          {
              int rnd_idx = rand() % reservations.size();
              int o = get<RES_PORT_OUT>(reservations[rnd_idx]);
              int vc_in = get<RES_VC_IN>(reservations[rnd_idx]);
              int vc_out = get<RES_VC_OUT>(reservations[rnd_idx]);
              Flit flit;

              if (!buffer[i][vc_in].IsEmpty())
              {
                  flit = buffer[i][vc_in].Front();

                  int free_slots_available = free_slots_neighbor[o].read().mask[vc_out];
                  bool is_dest_buffer_full = buffer_full_status_tx[o].read().mask[vc_out];
                  bool is_dest_buffer_empty = (free_slots_available == GlobalParams::buffer_depth);
                  bool require_atomic_flow_ctrl = routingAlgorithm->requireAtomicFlowCtrl(o, vc_out);

                  if ((current_level_tx[o] == ack_tx[o].read()) &&
                      (!require_atomic_flow_ctrl || (o == DIRECTION_LOCAL) || is_dest_buffer_empty || (flit.flit_type != FLIT_TYPE_HEAD)) &&
                      (!is_dest_buffer_full))
                  {
                      RouteData rdata(getRouteData(flit, i));

                      if (vc_out != vc_in)
                          switchFlitVirtualChannel(buffer[i][vc_in], flit, vc_out);

                      bool erase_buffer = routingAlgorithm->customForwarding(rdata, buffer[i][vc_in], flit, o);

                      assert(!buffer[i][vc_in].IsEmpty());

                      flit_tx[o].write(flit);
                      current_level_tx[o] = 1 - current_level_tx[o];
                      req_tx[o].write(current_level_tx[o]);

                      if (erase_buffer)
                          buffer[i][vc_in].Pop();

                      // AdEle+ Equation (6) & (7):
                      // Record latency imposed by selecting elevator k when packet leaves the source router
                      int router_id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ? global_id : local_id;
                      if (flit.elevator_id != -1 && (i == DIRECTION_LOCAL || flit.src_id == router_id))
                      {
                          double current_cycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
                          if (flit.flit_type == FLIT_TYPE_HEAD)
                          {
                              recordHeaderDepart(flit.sequence_id, current_cycle);
                          }
                          else if (flit.flit_type == FLIT_TYPE_TAIL)
                          {
                              double h_time = popHeaderDepart(flit.sequence_id);
                              if (h_time >= 0)
                              {
                                  double lp = flit.sequence_no + 1;
                                  if (lp <= 0) lp = GlobalParams::min_packet_size;
                                  double Tk = (current_cycle - h_time - lp) / lp;
                                  if (Tk < 0.0) Tk = 0.0;
                                  updateElevatorCost(flit.elevator_id, Tk);
                              }
                          }
                      }

                      if (flit.flit_type == FLIT_TYPE_TAIL)
                      {
                          TReservation r;
                          r.input = i;
                          r.vc_in = vc_in;
                          r.vc_out = vc_out;
                          reservation_table.release(r, o);
                      }

                      if (o == DIRECTION_HUB)
                          power.r2hLink();
                      else
                          power.r2rLink();

                      power.bufferRouterPop();
                      power.crossBar();

                      if (o == DIRECTION_LOCAL)
                      {
                          power.networkInterface();
                          stats.receivedFlit(sc_time_stamp().to_double() / GlobalParams::clock_period_ps, flit);
#ifdef DEBUG_VC_UTILIZATION
                          stats.updateVCReceivedFlits(*this, flit);
#endif
                          if (GlobalParams::max_volume_to_be_drained)
                          {
                              if (drained_volume >= GlobalParams::max_volume_to_be_drained)
                                  sc_stop();
                              else
                              {
                                  drained_volume++;
                                  local_drained++;
                              }
                          }
                      }
                      else if (i != DIRECTION_LOCAL)
                          routed_flits++;
                  }
              }
          }
      }

      if ((int)(sc_time_stamp().to_double() / GlobalParams::clock_period_ps) % 2 == 0)
          reservation_table.updateIndex();
    }
}

NoP_data Router::getCurrentNoPData()
{
    NoP_data NoP_data;
    for (int j = 0; j < DIRECTIONS; j++) {
        try {
            for (int vc = 0; vc < MAX_VIRTUAL_CHANNELS; vc++) {
		NoP_data.channel_status_neighbor[j][vc].free_slots = free_slots_neighbor[j].read().mask[vc];
                NoP_data.channel_status_neighbor[j][vc].available  = (reservation_table.isNotReserved(j, vc));
            }
	}
	catch (int e) {
	    if (e != NOT_VALID) assert(false);
	};
    }
    NoP_data.sender_id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ? global_id : local_id;
    return NoP_data;
}

void Router::perCycleUpdate()
{
    if (reset.read()) {
	for (int i = 0; i < DIRECTIONS + 1; i++) {
	    TFreeSlots fs;
	    for (int vc=0; vc < getNoC().n_virtual_channels; vc++)
		fs.mask[vc] = buffer[i][vc].GetMaxBufferSize();
	    free_slots[i].write(fs);
        }
    } else {
	power.leakageRouter();
	for (int i = 0; i < DIRECTIONS + 1; i++)
	{
	    for (int vc = 0; vc < getNoC().n_virtual_channels; vc++)
	    {
		power.leakageBufferRouter();
		power.leakageLinkRouter2Router();
	    }
	}
	power.leakageLinkRouter2Hub();
    }
}

vector<int> Router::nextDeltaHops(RouteData rd) {
	if (GlobalParams::topology == TOPOLOGY_MESH) {
		assert(false);
	}
	int src = rd.src_id;
	int dst = rd.dst_id;
	int current_node = src;
	vector<RouteEntry> direction;
	vector<int> next_hops;
	int stg = log2(GlobalParams::n_delta_tiles);
	int c;
	if (GlobalParams::topology == TOPOLOGY_OMEGA) {
	    if(current_node < (GlobalParams::n_delta_tiles/2)) c = current_node;
	    else c = (current_node - (GlobalParams::n_delta_tiles/2));		
	}
	else if ((GlobalParams::topology == TOPOLOGY_BUTTERFLY)||(GlobalParams::topology == TOPOLOGY_BASELINE)) {
	    c = (current_node >> 1);
	}
	Coord temp_coord;
	temp_coord.x = 0;
	temp_coord.y = c;
	int N = coord2Id(temp_coord);
	next_hops.push_back(N);
	current_node = N;
	
	int current_stage = 0;
	while (current_stage < stg - 1) {
		Coord new_coord;
		int y = id2Coord(current_node).y;
		rd.current_id = current_node;
		direction = routingAlgorithm->route(this, rd);
		int bit_to_check = stg - current_stage - 1;
		int bit_checked = (y & (1 << (bit_to_check - 1))) > 0 ? 1 : 0;
		new_coord.x = current_stage + 1;
		if (bit_checked ^ direction[0].first)
			new_coord.y = toggleKthBit(y, bit_to_check);
		else
			new_coord.y = y;
		current_node = coord2Id(new_coord);
		next_hops.push_back(current_node);
		current_stage = id2Coord(current_node).x;
	}
	next_hops.push_back(dst);
	return next_hops;
}

vector<RouteEntry> Router::routingFunction(const RouteData & route_data)
{
	if (GlobalParams::use_winoc) {
		if (hasRadioHub(local_id)) {
			if (hasRadioHub(route_data.dst_id) && !sameRadioHub(local_id, route_data.dst_id)) {
                map<int, int>::iterator it1 = GlobalParams::hub_for_tile.find(route_data.dst_id);
                map<int, int>::iterator it2 = GlobalParams::hub_for_tile.find(route_data.current_id);
                if (connectedHubs(it1->second, it2->second)) {
                    vector<RouteEntry> dirv;
                    dirv.push_back({DIRECTION_HUB, route_data.vc_in});
                    return dirv;
                }
			}
            if (GlobalParams::winoc_dst_hops > 0) {
                vector<int> nexthops = nextDeltaHops(route_data);
                for (int i = 1; i <= GlobalParams::winoc_dst_hops; i++) {
                	int dest_position = nexthops.size() - 1;
                	int candidate_hop = nexthops[dest_position - i];
					if (hasRadioHub(candidate_hop) && !sameRadioHub(local_id, candidate_hop)) {
						vector<RouteEntry> dirv;
						dirv.push_back({DIRECTION_HUB_RELAY + candidate_hop, route_data.vc_in});
						return dirv;
					}
				}
            }
		}
	}
	return routingAlgorithm->route(this, route_data);
}

RouteEntry Router::route(const RouteData & route_data)
{
    int target_id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ? global_id : local_id;
    if (route_data.dst_id == target_id)
	return {DIRECTION_LOCAL, route_data.vc_in};

    power.routing();
    vector<RouteEntry> candidate_channels = routingFunction(route_data);
    power.selection();
    return selectionFunction(candidate_channels, route_data);
}

void Router::NoP_report() const
{
    NoP_data NoP_tmp;
    for (int i = 0; i < DIRECTIONS; i++) {
	NoP_tmp = NoP_data_in[i].read();
	if (NoP_tmp.sender_id != NOT_VALID) cout << NoP_tmp;
    }
}

int Router::NoPScore(const NoP_data & nop_data, const vector<RouteEntry> & nop_channels) const
{
    int score = 0;
    for (unsigned int i = 0; i < nop_channels.size(); i++) {
	int available;
        int dir = nop_channels[i].first;
        int vc  = nop_channels[i].second;
	if (nop_data.channel_status_neighbor[dir][vc].available) available = 1;
	else available = 0;
	int free_slots = nop_data.channel_status_neighbor[dir][vc].free_slots;
	score += available * free_slots;
    }
    return score;
}

RouteEntry Router::selectionFunction(const vector<RouteEntry> & directions, const RouteData & route_data)
{
    if (directions.size() == 1) return directions[0];
    return selectionStrategy->apply(this, directions, route_data);
}

void Router::configure(const double _warm_up_time, const unsigned int _max_buffer_size, GlobalRoutingTable & grt)
{
    int _id = (GlobalParams::topology == TOPOLOGY_MULTI_MESH) ? global_id : local_id;
    stats.configure(getNoC(), _id, _warm_up_time);
    start_from_port = DIRECTION_LOCAL;

    if (grt.isValid()) routing_table.configure(grt, _id);
    reservation_table.setSize(DIRECTIONS + 2);

    for (int i = 0; i < DIRECTIONS + 2; i++) {
	for (int vc = 0; vc < getNoC().n_virtual_channels; vc++) {
	    buffer[i][vc].SetMaxBufferSize(_max_buffer_size);
	    buffer[i][vc].setLabel(string(name()) + "->buffer[" + i_to_string(i) + "]");
	}
	start_from_vc[i] = 0;
    }

    if (GlobalParams::topology == TOPOLOGY_MESH) {
	int layer = _id / (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y);
	int row   = (_id - (layer * GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y)) / GlobalParams::mesh_dim_x;
	int col   = (_id - (layer * GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y)) % GlobalParams::mesh_dim_x;

	for (int vc = 0; vc < getNoC().n_virtual_channels; vc++) {
	    if (row == 0) buffer[DIRECTION_NORTH][vc].Disable();
	    if (row == GlobalParams::mesh_dim_y - 1) buffer[DIRECTION_SOUTH][vc].Disable();
	    if (col == 0) buffer[DIRECTION_WEST][vc].Disable();
	    if (col == GlobalParams::mesh_dim_x - 1) buffer[DIRECTION_EAST][vc].Disable();
            if (layer == 0) buffer[DIRECTION_DOWN][vc].Disable();
            if (layer == GlobalParams::mesh_dim_z - 1) buffer[DIRECTION_UP][vc].Disable();
	}
    }
}

unsigned long Router::getRoutedFlits() { return routed_flits; }

int Router::reflexDirection(int direction) const
{
    if (direction == DIRECTION_NORTH) return DIRECTION_SOUTH;
    if (direction == DIRECTION_EAST) return DIRECTION_WEST;
    if (direction == DIRECTION_WEST) return DIRECTION_EAST;
    if (direction == DIRECTION_SOUTH) return DIRECTION_NORTH;
    if (direction == DIRECTION_UP) return DIRECTION_DOWN;
    if (direction == DIRECTION_DOWN) return DIRECTION_UP;
    assert(false);
    return NOT_VALID;
}

int Router::getNeighborId(int _id, int direction) const
{
    assert(GlobalParams::topology == TOPOLOGY_MESH);
    Coord my_coord = id2Coord(_id); 
    switch (direction) {
    case DIRECTION_NORTH: if (my_coord.y == 0) return NOT_VALID; my_coord.y--; break;
    case DIRECTION_SOUTH: if (my_coord.y == GlobalParams::mesh_dim_y - 1) return NOT_VALID; my_coord.y++; break;
    case DIRECTION_EAST: if (my_coord.x == GlobalParams::mesh_dim_x - 1) return NOT_VALID; my_coord.x++; break;
    case DIRECTION_WEST: if (my_coord.x == 0) return NOT_VALID; my_coord.x--; break;
    case DIRECTION_UP: if (my_coord.z == GlobalParams::mesh_dim_z - 1) return NOT_VALID; my_coord.z++; break;
    case DIRECTION_DOWN: if (my_coord.z == 0) return NOT_VALID; my_coord.z--; break;
    default: assert(false);
    }
    return coord2Id(my_coord);
}

bool Router::inCongestion()
{
    for (int i = 0; i < DIRECTIONS; i++) {
        for (int vc = 0; vc < MAX_VIRTUAL_CHANNELS; vc++) {
	    if (free_slots_neighbor[i].read().mask[vc] == NOT_VALID) continue;
            int flits = GlobalParams::buffer_depth - free_slots_neighbor[i].read().mask[vc];
            if (flits > (int) (GlobalParams::buffer_depth * GlobalParams::dyad_threshold)) return true;
        }
    }
    return false;
}

void Router::ShowBuffersStats(std::ostream & out)
{
  for (int i = 0; i < DIRECTIONS + 2; i++)
      for (int vc = 0; vc < getNoC().n_virtual_channels; vc++)
	    buffer[i][vc].ShowStats(out);
}

bool Router::connectedHubs(int src_hub, int dst_hub) {
    vector<int> &first = GlobalParams::hub_configuration[src_hub].txChannels;
    vector<int> &second = GlobalParams::hub_configuration[dst_hub].rxChannels;
    vector<int> intersection;
    for (unsigned int i = 0; i < first.size(); i++) {
        for (unsigned int j = 0; j < second.size(); j++) {
            if (first[i] == second[j]) intersection.push_back(first[i]);
        }
    }
    return intersection.size() > 0;
}

int Router::computeMinimumManhattanDistance(const vector<int>& vlinks) const
{
    std::map<int, unsigned> vlink_id_dist;
    Coord src_coord = parent_noc.getCoordFromGlobalId(global_id);
    for (auto& vlink_id : vlinks) {
        Coord vlink_coord = parent_noc.getCoordFromGlobalId(vlink_id);
        unsigned dist = std::abs(src_coord.x - vlink_coord.x) + std::abs(src_coord.y - vlink_coord.y);
        vlink_id_dist.insert({vlink_id, dist});
    }
    auto it_min = min_element(vlink_id_dist.begin(), vlink_id_dist.end(), 
                              [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
    sc_assert(it_min != vlink_id_dist.end());
    return it_min->first;
}

void Router::configureUpwardLinks(IdRangeRegister& upward_link_iic,
                                  map<IdRange, vector<int>>& id_ranges_to_vlink_ids,
                                  map<IdRange, vector<pair<int, int>>>& hidrange_link_selection)
{
    all_upward_links = id_ranges_to_vlink_ids;
    for (auto& entry_pair : id_ranges_to_vlink_ids)
    {
        int sel_vlink_id;
        int custom_vlink_id = -1;
        const IdRange& curr_target_hid = entry_pair.first;
        int router_id = global_id;
        
        auto it = hidrange_link_selection.find(curr_target_hid);
        if (it != hidrange_link_selection.end()) {
            const vector<pair<int, int>>& t = it->second;
            auto it_pair = find_if(t.begin(), t.end(), [&](const pair<int, int>& src_to_link) {
                return src_to_link.first == router_id;
            });
            custom_vlink_id = (it_pair != t.end()) ? it_pair->second : -1;
        }

        bool custom_mapping = (custom_vlink_id != -1);
        sel_vlink_id = custom_mapping ? custom_vlink_id : computeMinimumManhattanDistance(entry_pair.second);
        selected_upward_links[entry_pair.first] = sel_vlink_id;
    }
}

void Router::configureDownwardLink(const vector<int>& vlink_pos,
                                   const vector<pair<int, int>>& link_selected)
{
    all_downward_links = vlink_pos;
    int router_id = global_id;
    auto it = find_if(link_selected.begin(), link_selected.end(), [&](const pair<int, int>& src_to_link) {
        return src_to_link.first == router_id;
    });
    bool custom_mapping = (it != link_selected.end());
    selected_downward_link = custom_mapping ? it->second : computeMinimumManhattanDistance(vlink_pos);
}

void Router::switchFlitVirtualChannel(Buffer & buffer, Flit & flit, int vc_out)
{
    flit.vc_id = vc_out;
    buffer.UpdateFront(flit);
}

bool Router::incomingDownUpTransition(int dest_gid) const
{
    if (noc_down)
    {
        const NoC& nc = *noc_down;
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        if (nc.getGlobalIdRange().isIncluded(dest_gid)) return true;
        if (reg.size() > 0) {
            auto dest_id_range = reg.find(dest_gid);
            if (dest_id_range != reg.end()) return true;
        }
    }
    return false;
}