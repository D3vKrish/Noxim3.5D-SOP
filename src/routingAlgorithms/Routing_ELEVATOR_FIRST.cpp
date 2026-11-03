/*
    (C) Copyright 2025 CEA LIST. All Rights Reserved.
    Contributor(s): Davy Million (davy.million@cea.fr)
*/

#include "Routing_ELEVATOR_FIRST.h"
#include "../MeshNoC.h"
#include "../ElevatorSubsets.h"

RoutingAlgorithmsRegister Routing_ELEVATOR_FIRST::routingAlgorithmsRegister("ELEVATOR_FIRST", getInstance());

Routing_ELEVATOR_FIRST * Routing_ELEVATOR_FIRST::routing_ELEVATOR_FIRST = 0;

Routing_ELEVATOR_FIRST * Routing_ELEVATOR_FIRST::getInstance() {
	if ( routing_ELEVATOR_FIRST == 0 )
		routing_ELEVATOR_FIRST = new Routing_ELEVATOR_FIRST();
	return routing_ELEVATOR_FIRST;
}

int routeXY(const Coord & curr, const Coord & dest)
{
    if (dest.x > curr.x)
        return DIRECTION_EAST;
    if (dest.x < curr.x)
        return DIRECTION_WEST;
    if (dest.y > curr.y)
        return DIRECTION_SOUTH;
    if (dest.y < curr.y)
        return DIRECTION_NORTH;
    return DIRECTION_LOCAL;
}

vector<RouteEntry> Routing_ELEVATOR_FIRST::route(Router * router, const RouteData & routeData)
{
    const NoC& nc = router->getNoC();
    const MeshNoC& mesh_nc = static_cast<const MeshNoC&>(nc);
    Coord current = mesh_nc.getCoordFromGlobalId(routeData.current_id);

    int vdir = DIRECTION_LOCAL;
    if (!nc.getGlobalIdRange().isIncluded(routeData.dst_id)) {
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        if (reg.find(routeData.dst_id) != reg.end())
            vdir = DIRECTION_UP;   
        else
            vdir = DIRECTION_DOWN; 
    }

    Coord destination;
    if (vdir == DIRECTION_LOCAL) {
        destination = mesh_nc.getCoordFromGlobalId(routeData.dst_id);
    }
    else if (routeData.routing_towards_vlink) {
        destination = mesh_nc.getCoordFromGlobalId(routeData.vlink_dest);
    }
    else {
        destination = current;
    }

    int direction = DIRECTION_LOCAL;
    int vc_out = routeData.vc_in;

    if (routeData.routing_towards_vlink || vdir == DIRECTION_LOCAL) {
        direction = routeXY(current, destination);
    }
    else {
        if (vdir == DIRECTION_UP && router->hasUpwardLink()) {
            direction = vdir;
        }
        else if (vdir == DIRECTION_DOWN && router->hasDownwardLink()) {
            direction = vdir;
        }
        else if (vdir != DIRECTION_LOCAL &&
                 ((routeData.src_id == routeData.current_id) ||  
                  (routeData.dir_in == DIRECTION_UP)           ||  
                  (routeData.dir_in == DIRECTION_DOWN)))           
        {
            int elevator_id = selectElevator(vdir, routeData, *router);
            Coord elevator_dest = mesh_nc.getCoordFromGlobalId(elevator_id);
            direction = routeXY(current, elevator_dest);
        }
        else {
            assert(false && "Should be unreachable");
        }
    }

    if (direction == DIRECTION_UP)
        vc_out = 0;
    else if (direction == DIRECTION_DOWN)
        vc_out = 1;

    vector<RouteEntry> routing_decision;
    routing_decision.emplace_back(direction, vc_out);
    return routing_decision;
}

void Routing_ELEVATOR_FIRST::allocateVC(const Router& router, Packet& packet)
{
    const NoC& nc = router.getNoC();
    if (nc.getGlobalIdRange().isIncluded(packet.dst_id))
        return;

    const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
    if (reg.find(packet.dst_id) != reg.end()) {
        packet.vc_id = 0; 
    } else {
        packet.vc_id = 1; 
    }
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Coord & curr, const Coord & dest) const
{
    assert(false && "getVerticalDirection(Coord,Coord) must not be used.");
    (void)curr; (void)dest;
    return DIRECTION_LOCAL;
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Flit & flit, int current_id) const
{
    assert(false && "getVerticalDirection(Flit, int) called without NoC context.");
    (void)flit; (void)current_id;
    return DIRECTION_LOCAL;
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Flit & flit, int current_id, const NoC & nc) const
{
    if (flit.routing_towards_vlink)
        return DIRECTION_LOCAL; 
    if (flit.dst_id == current_id)
        return DIRECTION_LOCAL;
    if (nc.getGlobalIdRange().isIncluded(flit.dst_id))
        return DIRECTION_LOCAL;
    const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
    if (reg.find(flit.dst_id) != reg.end())
        return DIRECTION_UP;
    return DIRECTION_DOWN;
}

int Routing_ELEVATOR_FIRST::selectElevator(int vdir, const RouteData & rdata, const Router & router) const
{
    assert(vdir == DIRECTION_UP || vdir == DIRECTION_DOWN);

    const NoC& nc = router.getNoC();
    const MeshNoC& mesh_nc = static_cast<const MeshNoC&>(nc);
    int router_gid = router.global_id;

    // Baseline fallback elevator
    int fallback_elev = -1;
    if (vdir == DIRECTION_DOWN) {
        fallback_elev = router.downward_link();
    } else {
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        auto dest_id_range = reg.find(rdata.dst_id);
        const map<IdRange, int>& uplinks = router.upward_links();
        if (dest_id_range == reg.end()) {
            fallback_elev = uplinks.empty() ? 0 : uplinks.begin()->second;
        } else {
            auto nearest_vlink = uplinks.find(dest_id_range->second);
            fallback_elev = (nearest_vlink != uplinks.end()) ? nearest_vlink->second : (uplinks.empty() ? 0 : uplinks.begin()->second);
        }
    }

    int db_elevator = fallback_elev;
    vector<int> candidates;

    // Use precomputed AdEle+ subset and quadrant map if available
    if (router_gid >= 0 && router_gid < (int)ROUTER_ELEVATOR_MAP.size()) {
        const auto& cfg = ROUTER_ELEVATOR_MAP[router_gid];

        int dim_x = GlobalParams::mesh_dim_x > 0 ? GlobalParams::mesh_dim_x : 4;
        int dim_y = GlobalParams::mesh_dim_y > 0 ? GlobalParams::mesh_dim_y : 4;
        int layer_size = dim_x * dim_y;

        int S_lid = rdata.src_id % layer_size;
        int Sx = S_lid % dim_x;
        int Sy = S_lid / dim_x;

        int D_lid = rdata.dst_id % layer_size;
        int Dx = D_lid % dim_x;
        int Dy = D_lid / dim_x;

        // 1. Algorithm 2: Distance-Based (DB) Elevator Selection
        int rce = cfg.ce_overall;
        if (Dx >= Sx && Dy >= Sy)
            rce = cfg.ce_ne;
        else if (Dx >= Sx && Dy <= Sy)
            rce = cfg.ce_se;
        else if (Dx <= Sx && Dy >= Sy)
            rce = cfg.ce_nw;
        else
            rce = cfg.ce_sw;

        int ce_lid = cfg.ce_overall % layer_size;
        int CEx = ce_lid % dim_x;
        int CEy = ce_lid / dim_x;

        int rce_lid = rce % layer_size;
        int RCEx = rce_lid % dim_x;
        int RCEy = rce_lid / dim_x;

        int dist_ce = std::abs(Sx - CEx) + std::abs(Sy - CEy)
                    + std::abs(CEx - Dx) + std::abs(CEy - Dy);
        int dist_rce = std::abs(Sx - RCEx) + std::abs(Sy - RCEy)
                     + std::abs(RCEx - Dx) + std::abs(RCEy - Dy);

        db_elevator = (dist_rce < dist_ce) ? rce : cfg.ce_overall;

        for (int e : cfg.subset) {
            if (e >= 0) candidates.push_back(e);
        }
    }

    if (candidates.empty()) {
        if (vdir == DIRECTION_DOWN && !router.getAllDownwardLinks().empty()) {
            candidates = router.getAllDownwardLinks();
        } else {
            const map<IdRange, vector<int>>& all_uplinks = router.getAllUpwardLinks();
            if (!all_uplinks.empty()) candidates = all_uplinks.begin()->second;
        }
    }
    if (candidates.empty()) candidates.push_back(db_elevator);

    size_t n_cand = candidates.size();

    // 2. Algorithm 1: Enhanced Round Robin (ERR) in AdEle+
    double total_cost = 0.0;
    for (int elev : candidates) {
        total_cost += router.getElevatorCost(elev);
    }

    vector<double> C_rel(n_cand, 1.0 / n_cand);
    if (total_cost > 0.0) {
        for (size_t k = 0; k < n_cand; ++k) {
            C_rel[k] = router.getElevatorCost(candidates[k]) / total_cost;
        }
    }

    double expected = 1.0 / n_cand;
    double xi = 0.05;
    vector<double> PS(n_cand, 0.0);
    for (size_t k = 0; k < n_cand; ++k) {
        if (C_rel[k] >= 2.0 * expected) {
            PS[k] = 1.0 - xi;
        } else if (C_rel[k] >= expected) {
            PS[k] = n_cand * (C_rel[k] - expected) * (1.0 - xi);
        } else {
            PS[k] = 0.0;
        }
    }

    int err_elevator = -1;
    for (size_t attempts = 0; attempts < n_cand * 2; ++attempts) {
        size_t idx = router.err_rr_index % n_cand;
        router.err_rr_index = (router.err_rr_index + 1) % n_cand;
        double r = ((double)rand()) / RAND_MAX;
        if (r >= PS[idx]) {
            err_elevator = candidates[idx];
            break;
        }
    }
    if (err_elevator == -1) {
        err_elevator = candidates[router.err_rr_index % n_cand];
    }

    // 3. Mode decision:
    // Mode 1: Distance-Based (DB)
    // Mode 2: Enhanced Round-Robin (ERR)
    // Mode 0: Adaptive AdEle+ (switches between DB and ERR)
    if (GlobalParams::elevator_mode == 1) {
        return db_elevator;
    } else if (GlobalParams::elevator_mode == 2) {
        return err_elevator;
    } else {
        // Mode 0: Adaptive AdEle+ (Section 3.5)
        bool congested = false;
        for (int elev : candidates) {
            if (router.getElevatorCost(elev) >= GlobalParams::TR_DB) {
                congested = true;
                break;
            }
        }
        return congested ? err_elevator : db_elevator;
    }
}

bool Routing_ELEVATOR_FIRST::customForwarding(const RouteData & rdata, Buffer & buffer, Flit & flit, int dir_out) const
{
    bool erase_buffer = true;
    assert(rdata.noc != nullptr && "RouteData::noc must be set in getRouteData()");
    int vdir = getVerticalDirection(flit, rdata.current_id, *rdata.noc);

    if (dir_out == DIRECTION_UP || dir_out == DIRECTION_DOWN) {
        flit.routing_towards_vlink = false;
        flit.vlink_dest = -1;
        buffer.UpdateFront(flit);
        return erase_buffer; 
    }

    if (flit.flit_type == FLIT_TYPE_HEAD && !flit.routing_towards_vlink && vdir != DIRECTION_LOCAL)
    {
        Flit new_header(flit);
        new_header.dst_id = rdata.vlink_dest;

        assert(new_header.dst_id != -1 && "vlink_dest must be set before customForwarding inserts a local header");

        if (new_header.dst_id != rdata.current_id)
        {
            new_header.routing_towards_vlink = true;
            flit = new_header;
            Flit buffered_flit = buffer.Front();
            buffered_flit.flit_type = FLIT_TYPE_BODY;
            buffer.UpdateFront(buffered_flit);
            erase_buffer = false;
        }
    }
    return erase_buffer;
}

void Routing_ELEVATOR_FIRST::customPreRoutingRes(const Router & router, RouteData & rdata, Buffer & buffer, Flit & flit)
{
    auto& header_popped = headers_absorbed[rdata.current_id][rdata.dir_in][rdata.vc_in];
    
    if (header_popped) {
        assert(flit.flit_type == FLIT_TYPE_BODY);
        flit.flit_type = FLIT_TYPE_HEAD;
        buffer.UpdateFront(flit);
        header_popped = false;
    }

    if (rdata.vlink_dest == -1 &&
        flit.flit_type == FLIT_TYPE_HEAD &&
        !router.getNoC().getGlobalIdRange().isIncluded(flit.dst_id))
    {
        const NoC& nc = router.getNoC();
        int vdir = DIRECTION_LOCAL;
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        if (reg.find(flit.dst_id) != reg.end())
            vdir = DIRECTION_UP;
        else
            vdir = DIRECTION_DOWN;

        int vlink_id = selectElevator(vdir, rdata, router);

        rdata.vlink_dest = vlink_id;
        flit.vlink_dest = vlink_id;
        flit.elevator_id = vlink_id;
        buffer.UpdateFront(flit);
    }
    if (!router.getNoC().getGlobalIdRange().isIncluded(flit.dst_id))
        rdata.routing_towards_vlink = true;
}

bool Routing_ELEVATOR_FIRST::customPostRoutingRes(const RouteData & rdata, Buffer & buffer, Flit & flit, int dir_out)
{
    if (flit.flit_type == FLIT_TYPE_HEAD && flit.routing_towards_vlink && dir_out == DIRECTION_LOCAL)
    {
        auto& header_popped = headers_absorbed[rdata.current_id][rdata.dir_in][rdata.vc_in];
        assert(header_popped == false);
        buffer.Pop();
        header_popped = true;
        return true;
    }
    return false;
}

void Routing_ELEVATOR_FIRST::runtimeInit(const NoC& noc)
{
    int total_tiles = 0;
    if (GlobalParams::topology == TOPOLOGY_MULTI_MESH){
	    for (auto& entry : GlobalParams::mesh_component_config) {
		const MeshComponentConfig& cfg = entry.second;
                assert(cfg.dim_z == 1 && "Routing_ELEVATOR_FIRST requires dim_z=1 per chiplet");
		total_tiles += cfg.dim_x * cfg.dim_y;
	    }
    }
    else {
    	total_tiles = noc.getNumberOfTiles();
    }

    headers_absorbed.resize(total_tiles);
    for (auto& headers_per_tile : headers_absorbed) {
        headers_per_tile.resize(DIRECTIONS + 2);
        for (auto& headers_per_dir : headers_per_tile) {
            headers_per_dir.resize(noc.n_virtual_channels);
            for (auto& header_per_vc : headers_per_dir)
                header_per_vc = 0;
        }
    }
}