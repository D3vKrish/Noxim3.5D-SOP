/*
    (C) Copyright 2025 CEA LIST. All Rights Reserved.
    Contributor(s): Davy Million (davy.million@cea.fr)
*/

#include "Routing_ELEVATOR_FIRST.h"
#include "../MeshNoC.h"

RoutingAlgorithmsRegister Routing_ELEVATOR_FIRST::routingAlgorithmsRegister("ELEVATOR_FIRST", getInstance());

Routing_ELEVATOR_FIRST * Routing_ELEVATOR_FIRST::routing_ELEVATOR_FIRST = 0;

Routing_ELEVATOR_FIRST * Routing_ELEVATOR_FIRST::getInstance() {
	if ( routing_ELEVATOR_FIRST == 0 )
		routing_ELEVATOR_FIRST = new Routing_ELEVATOR_FIRST();

	return routing_ELEVATOR_FIRST;
}

int routeXY(const Coord & curr, const Coord & dest)
{
    // Removed because we can be moving in the same layer towards Vertical link, while not at dest layer
    //assert(curr.z == dest.z);

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

vector<RouteEntry> Routing_ELEVATOR_FIRST::route(Router * router,
                                                 const RouteData & routeData)
{
    const NoC& nc = router->getNoC();
    const MeshNoC& mesh_nc = static_cast<const MeshNoC&>(nc);

    // Get current router's coordinates within its chiplet
    // This is always safe — current_id always belongs to this chiplet
    Coord current = mesh_nc.getCoordFromGlobalId(routeData.current_id);

    // Determine vertical direction using NoC hierarchy, NOT z-coordinates
    // Reason: every chiplet has dim_z=1, so z is always 0 everywhere.
    // The old code used dest.z > curr.z which always evaluates to 0>0 = false,
    // meaning vdir was always DIRECTION_LOCAL — packets never went up or down.
    // Instead we check if dst_id falls inside this chiplet's range or not.
    int vdir = DIRECTION_LOCAL;
    if (!nc.getGlobalIdRange().isIncluded(routeData.dst_id)) {
        // Destination is in a different chiplet — determine if above or below
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        if (reg.find(routeData.dst_id) != reg.end())
            vdir = DIRECTION_UP;   // dst is in a sub-mesh above us
        else
            vdir = DIRECTION_DOWN; // dst is below us (toward interposer)
    }

    // Determine destination coordinates
    // We only need the actual dst coord when routing within the same chiplet.
    // When crossing chiplets, we route toward a VL node instead — so we
    // set destination to the VL node coord in those cases.
    Coord destination;
    if (vdir == DIRECTION_LOCAL) {
        // Same chiplet — safe to convert dst_id directly
        destination = mesh_nc.getCoordFromGlobalId(routeData.dst_id);
    }
    else if (routeData.routing_towards_vlink) {
        // Already heading toward a VL node — use vlink_dest as destination
        // vlink_dest is a global ID within this chiplet so conversion is safe
        destination = mesh_nc.getCoordFromGlobalId(routeData.vlink_dest);
    }
    else {
        // Not yet heading to VL — destination will be overridden below
        // when we call selectElevator(). Use dummy value for now.
        destination = current;
    }

    int direction = DIRECTION_LOCAL;
    int vc_out = routeData.vc_in;

    // --- Routing decision ---

    // Case 1: Already routing toward a VL node (local header inserted)
    // OR destination is in the same chiplet (no vertical movement needed)
    // In both cases: just do XY routing toward 'destination'
    if (routeData.routing_towards_vlink || vdir == DIRECTION_LOCAL) {
        direction = routeXY(current, destination);
    }

    // Case 2: Need to cross a chiplet boundary but no local header yet
    else {

        // Case 2a: Packet needs to go UP and this router HAS an upward link
        // This happens when:
        // (1) local header was popped on arrival at the VL node, OR
        // (2) elevator is at the source node itself (no horizontal travel needed)
        if (vdir == DIRECTION_UP && router->hasUpwardLink()) {
            //assert(routeData.vc_in == 0); 
            // We're correcting vc_out below
            direction = vdir;
        }

        // Case 2b: Packet needs to go DOWN and this router HAS a downward link
        // Same logic as 2a but for downward direction
        else if (vdir == DIRECTION_DOWN && router->hasDownwardLink()) {
            //assert(routeData.vc_in == 1); 
            // We're correcting vc_out below
            direction = vdir;
        }

        // Case 2c: Packet was just emitted OR just crossed a vertical link
        // In these cases the local routing header hasn't been inserted yet
        // (it gets inserted in customForwarding AFTER this routing decision).
        // So we preemptively route toward the elevator node now,
        // and customForwarding will insert the header on the way out.
        else if (vdir != DIRECTION_LOCAL &&
                 ((routeData.src_id == routeData.current_id) ||  // just emitted
                  (routeData.dir_in == DIRECTION_UP)           ||  // just came up
                  (routeData.dir_in == DIRECTION_DOWN)))           // just came down
        {
            // Find the nearest VL node in the required direction
            int elevator_id = selectElevator(vdir, routeData, *router);

            // Get coordinates of that VL node (it's in THIS chiplet — safe to convert)
            Coord elevator_dest = mesh_nc.getCoordFromGlobalId(elevator_id);

            // Route horizontally toward the elevator node
            direction = routeXY(current, elevator_dest);
        }

        // Should never reach here — all cases covered above
        else {
            LOG << "Should be unreachable" << std::endl;
            assert(false && "Should be unreachable");
        }
    }
    // BUG FIX #3: use the actual output direction, not vdir.
    // Using vdir caused packets routed locally (direction==DIRECTION_LOCAL)
    // to get the wrong VC when vdir happened to be UP or DOWN.
    if (direction == DIRECTION_UP)
        vc_out = 0;
    else if (direction == DIRECTION_DOWN)
        vc_out = 1;

    // BUG FIX #5: was std::cerr (unconditional), now uses LOG so it
    // respects verbose_mode and doesn't flood output in normal runs.
    LOG << "ROUTE: gid=" << routeData.current_id
              << " src=" << routeData.src_id
              << " dst=" << routeData.dst_id
              << " vdir=" << vdir
              << " dir_in=" << routeData.dir_in
              << " vc_in=" << routeData.vc_in
              << " vc_out=" << vc_out
              << " direction=" << direction
              << " towards_vlink=" << routeData.routing_towards_vlink
              << " vlink_dest=" << routeData.vlink_dest
              << std::endl;
    vector<RouteEntry> routing_decision;
    routing_decision.emplace_back(direction, vc_out);
    return routing_decision;
}

void Routing_ELEVATOR_FIRST::allocateVC(const Router& router, Packet& packet)
{
    const NoC& nc = router.getNoC();

    // If destination is local, no vertical traversal needed
    if (nc.getGlobalIdRange().isIncluded(packet.dst_id))
        return;

    // Check if destination is in a sub-mesh above (UP) or below (DOWN)
    const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
    if (reg.find(packet.dst_id) != reg.end()) {
        packet.vc_id = 0; // going UP
    } else {
        packet.vc_id = 1; // going DOWN
    }
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Coord & curr,
                                                 const Coord & dest) const
{
    // BUG FIX #4: in TOPOLOGY_MULTI_MESH every chiplet has dim_z=1 so
    // curr.z and dest.z are always 0 — this function always returned
    // DIRECTION_LOCAL regardless of actual topology. Guard against accidental
    // use; callers should use the Flit-based overload instead.
    assert(false &&
           "getVerticalDirection(Coord,Coord) must not be used in "
           "TOPOLOGY_MULTI_MESH: z is always 0. "
           "Use the Flit-based overload instead.");
    (void)curr; (void)dest;
    return DIRECTION_LOCAL;
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Flit & flit,
                                                 int current_id) const
{
    // BUG FIX #2: the old implementation inferred up/down by comparing
    // flit.dst_id numerically against current_id. That is WRONG in a
    // multi-chiplet mesh: global ID ordering does not reflect physical
    // topology. This overload is kept for ABI compatibility but should
    // not be called — use getVerticalDirection(flit, current_id, nc) instead.
    assert(false &&
           "getVerticalDirection(Flit, int) called without NoC context — "
           "use the three-argument overload via rdata.noc");
    (void)flit; (void)current_id;
    return DIRECTION_LOCAL;
}

int Routing_ELEVATOR_FIRST::getVerticalDirection(const Flit & flit,
                                                 int current_id,
                                                 const NoC & nc) const
{
    // Correct IdRange-based implementation — mirrors the logic in route().
    if (flit.routing_towards_vlink)
        return DIRECTION_LOCAL; // already handled upstream
    if (flit.dst_id == current_id)
        return DIRECTION_LOCAL;
    if (nc.getGlobalIdRange().isIncluded(flit.dst_id))
        return DIRECTION_LOCAL;
    const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
    if (reg.find(flit.dst_id) != reg.end())
        return DIRECTION_UP;
    return DIRECTION_DOWN;
}

// AFTER (fixed):
int Routing_ELEVATOR_FIRST::selectElevator(int vdir,
                                           const RouteData & rdata,
                                           const Router & router) const
{
    assert(vdir == DIRECTION_UP || vdir == DIRECTION_DOWN);

    bool use_db_mode = false;

    // Determine strategy based on active mode
    if (GlobalParams::elevator_mode == 1) {
        use_db_mode = true; // Forced Distance-Based
    } 
    else if (GlobalParams::elevator_mode == 2) {
        use_db_mode = false; // Forced Dynamic ERR
    } 
    else {
        // Mode 0 (Adaptive): Check if elevator congestion exceeds TR_DB threshold
        use_db_mode = true;
        const map<IdRange, int>& uplinks = router.upward_links();
        for (const auto& kv : uplinks) {
            if (router.getElevatorCost(kv.second) >= GlobalParams::TR_DB) {
                use_db_mode = false; // Switch to ERR if congested
                break;
            }
        }
    }

    // Execute Distance-Based (DB) routing
    if (use_db_mode) {
        if (vdir == DIRECTION_DOWN) {
            return router.downward_link();
        }
        const NoC& nc = router.getNoC();
        const IdRangeRegister& reg = nc.getHierarchicalSubMeshesIdRange();
        auto dest_id_range = reg.find(rdata.dst_id);
        if (dest_id_range == reg.end()) {
            const map<IdRange, int>& uplinks = router.upward_links();
            assert(!uplinks.empty());
            return uplinks.begin()->second;
        }
        const map<IdRange, int>& uplinks = router.upward_links();
        auto nearest_vlink = uplinks.find(dest_id_range->second);
        assert(nearest_vlink != uplinks.end() && "No upward VL found for destination range");
        return nearest_vlink->second;
    }

    // Execute Dynamic ERR routing
    int best_elevator = -1;
    double min_cost = 1e9;
    const map<IdRange, int>& uplinks = router.upward_links();
    for (const auto& kv : uplinks) {
        int elev_id = kv.second;
        double cost = router.getElevatorCost(elev_id);
        if (cost < min_cost) {
            min_cost = cost;
            best_elevator = elev_id;
        }
    }

    if (best_elevator != -1) return best_elevator;
    return router.downward_link();
}

// In the forwarding stage, we added the logic to insert a new header, in
// compliance with the original Elevator-First algorithm.
bool Routing_ELEVATOR_FIRST::customForwarding(const RouteData & rdata,
                                                    Buffer    & buffer,
                                                    Flit      & flit,
                                                    int         dir_out) const
{
    bool erase_buffer = true;

    // BUG FIX #2 (call site): use IdRange-based direction via rdata.noc
    // instead of the broken numeric dst_id comparison.
    assert(rdata.noc != nullptr && "RouteData::noc must be set in getRouteData()");
    int vdir = getVerticalDirection(flit, rdata.current_id, *rdata.noc);

    // When a packet crosses a vertical link, clear the VL routing metadata
    // from the flit so the new chiplet doesn't use stale vlink_dest values
    if (dir_out == DIRECTION_UP || dir_out == DIRECTION_DOWN) {
        flit.routing_towards_vlink = false;
        flit.vlink_dest = -1;
        buffer.UpdateFront(flit);
        return erase_buffer; // normal forwarding, no header insertion needed
    }

    // Incoming flit is a regular header, whose destination is on a different
    // layer
    if (flit.flit_type == FLIT_TYPE_HEAD && !flit.routing_towards_vlink &&
        vdir != DIRECTION_LOCAL)
    {
        Flit new_header(flit);
        new_header.dst_id = rdata.vlink_dest;

        // BUG FIX #1: if vlink_dest is -1 here it means customPreRoutingRes
        // did not set it, which produces a flit with dst_id=-1 and
        // routing_towards_vlink=true — the exact state that causes the
        // Router.cpp:391 assertion. Catch it early with a clear message.
        assert(new_header.dst_id != -1 &&
               "vlink_dest must be set by customPreRoutingRes before "
               "customForwarding inserts a local header");

        if (new_header.dst_id != rdata.current_id)
        {
            new_header.routing_towards_vlink = true;
            LOG << "Header flit "        << new_header
                << " inserted based on " << flit
                << " at Input[" << rdata.dir_in << "][" << rdata.vc_in <<"]"
                << endl;
            flit = new_header;
            Flit buffered_flit = buffer.Front();
            Flit old_buffered_flit(buffered_flit);
            buffered_flit.flit_type = FLIT_TYPE_BODY;
            LOG << "Convert previous header flit " << old_buffered_flit
                << " to body flit " << buffered_flit << endl;
            buffer.UpdateFront(buffered_flit);
            erase_buffer = false;
        }
    }
    return erase_buffer;
}

void Routing_ELEVATOR_FIRST::customPreRoutingRes(const Router    & router,
                                                       RouteData & rdata,
                                                       Buffer    & buffer,
                                                       Flit      & flit)
{
    auto& header_popped
        = headers_absorbed[rdata.current_id][rdata.dir_in][rdata.vc_in];
    
    if (header_popped) {
        assert(flit.flit_type == FLIT_TYPE_BODY);
        flit.flit_type = FLIT_TYPE_HEAD;
        buffer.UpdateFront(flit);
        LOG << "Convert back BODY to HEAD, Input["
            << rdata.dir_in << "][" << rdata.vc_in << "], flit: "
            << flit << endl;
        header_popped = false;
    }

    // Set vlink_dest in rdata ONLY (do NOT modify the flit here)
    // customForwarding will create the new local header using this value
    // The original flit must remain unchanged so its dst_id is preserved
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

        // Update rdata so route() can use it
        rdata.vlink_dest = vlink_id;

        // Also store vlink_dest in the flit so getRouteData() can retrieve it
        // during the forwarding stage. Do NOT set routing_towards_vlink=true
        // here — customForwarding needs to see it as false to insert the header.
        flit.vlink_dest = vlink_id;
        buffer.UpdateFront(flit);
    }
    if (!router.getNoC().getGlobalIdRange().isIncluded(flit.dst_id))
        rdata.routing_towards_vlink = true;
    //ENDFIX
}

bool Routing_ELEVATOR_FIRST::customPostRoutingRes(const RouteData & rdata,
                                                        Buffer    & buffer,
                                                        Flit      & flit,
                                                        int         dir_out)
{
    // Extra local header HEAD flit arriving to its local destination
    if (flit.flit_type == FLIT_TYPE_HEAD &&
        flit.routing_towards_vlink && dir_out == DIRECTION_LOCAL)
    {
        auto& header_popped
            = headers_absorbed[rdata.current_id][rdata.dir_in][rdata.vc_in];
        // A local routing header cannot be preceded by another local routing
        // header. If header_popped is already true here the reservation table
        // was not cleaned up between two consecutive head flits on this port/vc.
        if (header_popped) {
            LOG << "ERROR: double-pop detected on Input["
                << rdata.dir_in << "][" << rdata.vc_in
                << "] at gid=" << rdata.current_id
                << " flit: " << flit << endl;
        }
        assert(header_popped == false);
        // Discards the flit without sending it
        buffer.Pop();
        LOG << "Pop header on Input[" << rdata.dir_in
            << "][" << rdata.vc_in << "], flit: " << flit << endl;
        // Report that the next flit on this port must be
        // interpreted as a header.
        header_popped = true;
        return true;
    }
    return false;
}

void Routing_ELEVATOR_FIRST::runtimeInit(const NoC& noc)
{
    // headers_absorbed is indexed by global ID, so we need total tiles
    // across ALL chiplets, not just the current one.
    // We compute max possible global ID by summing all chiplet sizes.
    int total_tiles = 0;
    if(GlobalParams::topology == TOPOLOGY_MULTI_MESH){
	    for (auto& entry : GlobalParams::mesh_component_config) {
		const MeshComponentConfig& cfg = entry.second;
                // BUG FIX #6: this algorithm assumes dim_z==1 per chiplet
                // (z-coordinates are always 0). Assert it at startup so a
                // misconfigured topology is caught before causing an
                // out-of-bounds access in headers_absorbed.
                assert(cfg.dim_z == 1 &&
                       "Routing_ELEVATOR_FIRST requires dim_z=1 per chiplet");
		total_tiles += cfg.dim_x * cfg.dim_y;
	    }
    }
    else
    	total_tiles = noc.getNumberOfTiles();

    headers_absorbed.resize(total_tiles);
    for (auto& headers_per_tile : headers_absorbed) {
        headers_per_tile.resize(DIRECTIONS+2);
        for (auto& headers_per_dir : headers_per_tile) {
            headers_per_dir.resize(noc.n_virtual_channels);
            for (auto& header_per_vc : headers_per_dir)
                header_per_vc = 0;
        }
    }
}