/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs.cpp PBS support routines */

#include "stdafx.h"
#include "viewport_func.h"
#include "vehicle_func.h"
#include "newgrf_station.h"
#include "station_func.h"
#include "pathfinder/follow_track.hpp"

/**
 * Get the reserved trackbits for any tile, regardless of type.
 * @param t the tile
 * @return the reserved trackbits. TRACK_BIT_NONE on nothing reserved or
 *     a tile without rail.
 */
TrackBits GetReservedTrackbits(TileIndex t)
{
	switch (GetTileType(t)) {
		case TT_RAILWAY:
			return GetRailReservationTrackBits(t);

		case TT_MISC:
			switch (GetTileSubtype(t)) {
				case TT_MISC_CROSSING: return GetCrossingReservationTrackBits(t);
				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(t) == TRANSPORT_RAIL) return GetTunnelReservationTrackBits(t);
					break;
				case TT_MISC_DEPOT:
					if (IsRailDepot(t)) return GetDepotReservationTrackBits(t);
					break;
				default:
					break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(t)) return GetStationReservationTrackBits(t);
			break;

		default:
			break;
	}
	return TRACK_BIT_NONE;
}

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param start starting tile of the platform
 * @param dir the direction in which to follow the platform
 * @param b the state the reservation should be set to
 */
void SetRailStationPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	assert(IsRailStationTile(start));
	assert(GetRailStationAxis(start) == DiagDirToAxis(dir));

	TileIndex     tile = start;
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	do {
		SetRailStationReservation(tile, b);
		MarkTileDirtyByTile(tile);
		tile = TILE_ADD(tile, diff);
	} while (IsCompatibleTrainStationTile(tile, start));
}

/**
 * Set the reservation for a complete station platform.
 * @pre !pos.in_wormhole() && IsRailStationTile(pos.tile)
 * @param pos starting tile and direction of the platform
 * @param b the state the reservation should be set to
 */
void SetRailStationPlatformReservation(const RailPathPos &pos, bool b)
{
	assert(!pos.in_wormhole());

	SetRailStationPlatformReservation(pos.tile, TrackdirToExitdir(pos.td), b);
}

/**
 * Try to reserve a specific track on a tile
 * @param tile the tile
 * @param t the track
 * @param trigger_stations whether to call station randomisation trigger
 * @return \c true if reservation was successful, i.e. the track was
 *     free and didn't cross any other reserved tracks.
 */
bool TryReserveRailTrack(TileIndex tile, Track t, bool trigger_stations)
{
	assert((GetTileRailwayStatus(tile) & TrackToTrackBits(t)) != 0);

	if (_settings_client.gui.show_track_reservation) {
		/* show the reserved rail if needed */
		MarkTileDirtyByTile(tile);
	}

	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			return TryReserveTrack(tile, t);

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING:
					if (HasCrossingReservation(tile)) break;
					SetCrossingReservation(tile, true);
					BarCrossing(tile);
					MarkTileDirtyByTile(tile); // crossing barred, make tile dirty
					return true;

				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(tile) == TRANSPORT_RAIL && !HasTunnelHeadReservation(tile)) {
						SetTunnelHeadReservation(tile, true);
						return true;
					}
					break;

				case TT_MISC_DEPOT:
					if (IsRailDepotTile(tile) && !HasDepotReservation(tile)) {
						SetDepotReservation(tile, true);
						MarkTileDirtyByTile(tile); // some GRFs change their appearance when tile is reserved
						return true;
					}
					break;

				default: break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(tile) && !HasStationReservation(tile)) {
				SetRailStationReservation(tile, true);
				if (trigger_stations && IsRailStation(tile)) TriggerStationRandomisation(NULL, tile, SRT_PATH_RESERVATION);
				MarkTileDirtyByTile(tile); // some GRFs need redraw after reserving track
				return true;
			}
			break;

		default:
			break;
	}
	return false;
}

/**
 * Lift the reservation of a specific track on a tile
 * @param tile the tile
 * @param t the track
 */
void UnreserveRailTrack(TileIndex tile, Track t)
{
	assert((GetTileRailwayStatus(tile) & TrackToTrackBits(t)) != 0);

	if (_settings_client.gui.show_track_reservation) {
		MarkTileDirtyByTile(tile);
	}

	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			UnreserveTrack(tile, t);
			break;

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING:
					SetCrossingReservation(tile, false);
					UpdateLevelCrossing(tile);
					break;

				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(tile) == TRANSPORT_RAIL) SetTunnelHeadReservation(tile, false);
					break;

				case TT_MISC_DEPOT:
					if (IsRailDepot(tile)) {
						SetDepotReservation(tile, false);
						MarkTileDirtyByTile(tile);
					}
					break;

				default:
					break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(tile)) {
				SetRailStationReservation(tile, false);
				MarkTileDirtyByTile(tile);
			}
			break;

		default:
			break;
	}
}


/** Follow a reservation starting from a specific tile to the end. */
static bool FollowReservation(Owner o, RailTypes rts, RailPathPos *pos, bool ignore_oneway = false)
{
	assert(HasReservedPos(*pos));

	/* Do not disallow 90 deg turns as the setting might have changed between reserving and now. */
	CFollowTrackRail ft(o, true, rts);
	ft.SetPos(*pos);
	RailPathPos cur = *pos;
	RailPathPos start;

	while (ft.FollowNext()) {
		/* No reservation --> path end found */
		if (ft.m_new.in_wormhole()) {
			if (!HasReservedPos(ft.m_new)) break;
		} else if (ft.m_flag != ft.TF_STATION) {
			TrackdirBits trackdirs = ft.m_new.trackdirs & TrackBitsToTrackdirBits(GetReservedTrackbits(ft.m_new.tile));
			if (trackdirs == TRACKDIR_BIT_NONE) break;

			/* Can't have more than one reserved trackdir */
			ft.m_new.set_trackdirs (trackdirs);
		} else if (!HasStationReservation(ft.m_new.tile)) {
			/* Check skipped station tiles as well, maybe our reservation ends inside the station. */
			TileIndexDiff diff = TileOffsByDiagDir(ft.m_exitdir);
			while (ft.m_tiles_skipped-- > 0) {
				ft.m_new.tile -= diff;
				if (HasStationReservation(ft.m_new.tile)) {
					cur = ft.m_new;
					break;
				}
			}
			break;
		}

		/* One-way signal against us. The reservation can't be ours as it is not
		 * a safe position from our direction and we can never pass the signal. */
		if (!ignore_oneway && ft.m_new.has_blocking_signal()) break;

		cur = ft.m_new;

		if (!start.is_valid_tile()) {
			/* Update the start tile after we followed the track the first
			 * time. This is necessary because the track follower can skip
			 * tiles (in stations for example) which means that we might
			 * never visit our original starting tile again. */
			start = cur;
		} else {
			/* Loop encountered? */
			if (cur == start) break;
		}
		/* Depot tile? Can't continue. */
		if (!cur.in_wormhole() && IsRailDepotTile(cur.tile)) break;
		/* Non-pbs signal? Reservation can't continue. */
		if (cur.has_signal_along() && !IsPbsSignal(cur.get_signal_type())) break;
	}

	*pos = cur;
	/* return whether there was any further reservation at all */
	return start.is_valid_tile();
}

/** Find a train on a specific tile track. */
static Train *FindTrainOnTrack (TileIndex tile, Track track)
{
	Train *best = NULL;
	VehicleTileIterator iter (tile);
	while (!iter.finished()) {
		Vehicle *v = iter.next();
		if (v->type != VEH_TRAIN || (v->vehstatus & VS_CRASHED)) continue;

		Train *t = Train::From(v);
		if (TrackdirToTrack(t->trackdir) != track) continue;

		t = t->First();

		/* ALWAYS return the lowest ID (anti-desync!) */
		if (best == NULL || t->index < best->index) best = t;
	}
	return best;
}

/** Find a train in a wormhole. */
static Train *FindTrainInWormhole (TileIndex tile)
{
	Train *best = NULL;
	VehicleTileIterator iter (tile);
	while (!iter.finished()) {
		Vehicle *v = iter.next();
		if (v->type != VEH_TRAIN || (v->vehstatus & VS_CRASHED)) continue;

		Train *t = Train::From(v);
		if (t->trackdir != TRACKDIR_WORMHOLE) continue;

		t = t->First();

		/* ALWAYS return the lowest ID (anti-desync!) */
		if (best == NULL || t->index < best->index) best = t;
	}
	return best;
}

/** Find a train on a reserved path end */
static Train *FindTrainOnPathEnd(const RailPathPos &pos)
{
	if (pos.in_wormhole()) {
		Train *t = FindTrainInWormhole (pos.wormhole);
		if (t != NULL) return t;
		return FindTrainInWormhole (GetOtherTunnelBridgeEnd(pos.wormhole));
	} else {
		Train *t = FindTrainOnTrack (pos.tile, TrackdirToTrack(pos.td));
		if (t != NULL) return t;

		/* Special case for stations: check the whole platform for a vehicle. */
		if (IsRailStationTile(pos.tile)) {
			TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
			for (TileIndex tile = pos.tile + diff; IsCompatibleTrainStationTile(tile, pos.tile); tile += diff) {
				Train *t = FindTrainOnTrack (tile, TrackdirToTrack(pos.td));
				if (t != NULL) return t;
			}
		}

		return NULL;
	}
}

/**
 * Follow a train reservation to the last tile.
 *
 * @param v the vehicle
 * @param pos Pointer to receive the last tile of the reservation or the current train tile if no reservation present
 * @param check Whether to check if there is another train on the reservation
 * @return Whether the reservation is free (no other train on it) if check was true; undefined if check was false
 */
bool FollowTrainReservation (const Train *v, RailPathPos *pos, bool check)
{
	assert(v->type == VEH_TRAIN);

	/* Start track not reserved? This can happen if two trains
	 * are on the same tile, on trackdirs ending on the same side.
	 * The reservation on the next tile is not ours in this case.
	 * Also, if the reservation ends on the starting position, we
	 * will not look for a train on it, or else a train behind us
	 * on the same track can appear to block our way, because it
	 * would seem that our reservation ends in an occupied position. */
	RailPathPos res = v->GetPos();
	if (HasReservedPos (res)) {
		bool ext = FollowReservation(v->owner, GetRailTypeInfo(v->railtype)->compatible_railtypes, &res);
		assert(HasReservedPos(res));
		if (ext && check) {
			Train *t = FindTrainOnPathEnd(res);
			if (t != NULL && t->First()->index != v->index) check = false;
		}
	}

	*pos = res;
	return check;
}

/**
 * Find the train which has reserved a specific path.
 *
 * @param tile A tile on the path.
 * @param track A reserved track on the tile.
 * @param free Whether to free the reservation.
 * @return The vehicle holding the reservation if not heading into a depot, else NULL.
 */
Train *GetTrainForReservation (TileIndex tile, Track track, bool free)
{
	assert(HasReservedTrack(tile, track));
	Trackdir  trackdir = TrackToTrackdir(track);

	RailTypes rts = GetRailTypeInfo(GetRailType(tile, track))->compatible_railtypes;

	/* Follow the path from tile to both ends, one of the end tiles should
	 * have a train on it. We need FollowReservation to ignore one-way signals
	 * here, as one of the two search directions will be the "wrong" way. */
	for (int i = 0; i < 2; ++i, trackdir = ReverseTrackdir(trackdir)) {
		/* If the tile has a one-way block signal in the current trackdir, skip the
		 * search in this direction as the reservation can't come from this side.*/
		if (HasSignalOnTrackdir (tile, trackdir) &&
			!HasSignalOnTrackdir (tile, ReverseTrackdir(trackdir)) &&
			!IsPbsSignal (GetSignalType (tile, TrackdirToTrack (trackdir)))) continue;

		RailPathPos pos = RailPathPos(tile, trackdir);
		FollowReservation(GetTileOwner(tile), rts, &pos, true);
		Train *t = FindTrainOnPathEnd(pos);
		if (t != NULL) {
			assert (t->IsFrontEngine());
			if (IsRailDepotTile (t->tile)) {
				Trackdir depot_td = DiagDirToDiagTrackdir (GetGroundDepotDirection (t->tile));
				if (t->trackdir != depot_td) return NULL;
			}
			if (free) FreeTrainTrackReservation (t);
			return t;
		}
	}

	/* Stray reservation? */
	return NULL;
}


/**
 * Analyse a waiting position, to check if it is safe and/or if it is free.
 *
 * @param v the vehicle to test for
 * @param pos The position
 * @param forbid_90deg Don't allow trains to make 90 degree turns
 * @param cb Checking behaviour
 * @return Depending on cb:
 *  * PBS_CHECK_FULL: Do a full check. Return PBS_UNSAFE, PBS_BUSY, PBS_FREE
 *    depending on the waiting position state.
 *  * PBS_CHECK_SAFE: Only check if the position is safe. Return PBS_UNSAFE
 *    iff it is not.
 *  * PBS_CHECK_FREE: Assume that the position is safe, and check if it is
 *    free. Return PBS_FREE iff it is. The behaviour is undefined if the
 *    position is actually not safe.
 *  * PBS_CHECK_SAFE_FREE: Check if the position is both safe and free.
 *    Return PBS_FREE iff it is.
 */
PBSPositionState CheckWaitingPosition(const Train *v, const RailPathPos &pos, bool forbid_90deg, PBSCheckingBehaviour cb)
{
	PBSPositionState state;
	if (pos.in_wormhole()) {
		if ((cb != PBS_CHECK_SAFE) && HasReservedPos(pos)) {
			/* Track reserved? Can never be a free waiting position. */
			if (cb != PBS_CHECK_FULL) return PBS_BUSY;
			state = PBS_BUSY;
		} else {
			/* Track not reserved or we do not care (PBS_CHECK_SAFE). */
			state = PBS_FREE;
		}
	} else {
		/* Depots are always safe, and free iff unreserved. */
		if (IsRailDepotTile(pos.tile) && pos.td == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(pos.tile)))) return HasDepotReservation(pos.tile) ? PBS_BUSY : PBS_FREE;

		if (pos.has_signal_along() && !IsPbsSignal(pos.get_signal_type())) {
			/* For non-pbs signals, stop on the signal tile. */
			if (cb == PBS_CHECK_SAFE) return PBS_FREE;
			return HasReservedTrack(pos.tile, TrackdirToTrack(pos.td)) ? PBS_BUSY : PBS_FREE;
		}

		if ((cb != PBS_CHECK_SAFE) && TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
			/* Track reserved? Can never be a free waiting position. */
			if (cb != PBS_CHECK_FULL) return PBS_BUSY;
			state = PBS_BUSY;
		} else {
			/* Track not reserved or we do not care (PBS_CHECK_SAFE). */
			state = PBS_FREE;
		}
	}

	/* Check next tile. */
	CFollowTrackRail ft(v, !forbid_90deg, v->railtype);

	/* End of track? Safe position. */
	if (!ft.Follow(pos)) return state;

	assert(!ft.m_new.is_empty());
	assert((state == PBS_FREE) || (cb == PBS_CHECK_FULL));

	if (cb != PBS_CHECK_FREE) {
		if (!ft.m_new.is_single()) return PBS_UNSAFE;

		if (ft.m_new.has_signal_along()) {
			/* PBS signal on next trackdir? Safe position. */
			if (!IsPbsSignal(ft.m_new.get_signal_type())) return PBS_UNSAFE;
		} else if (ft.m_new.has_signal_against()) {
			/* One-way PBS signal against us? Safe position. */
			if (ft.m_new.get_signal_type() != SIGTYPE_PBS_ONEWAY) return PBS_UNSAFE;
		} else {
			/* No signal at all? Unsafe position. */
			return PBS_UNSAFE;
		}

		if (cb == PBS_CHECK_SAFE) return PBS_FREE;
		if (state != PBS_FREE) return PBS_BUSY;
	} else if (!IsStationTile(pos.tile)) {
		/* With PBS_CHECK_FREE, all these should be true. */
		assert(ft.m_new.is_single());
		assert(ft.m_new.has_signals());
		assert(IsPbsSignal(ft.m_new.get_signal_type()));
	}

	assert(state == PBS_FREE);

	return HasReservedPos(ft.m_new) ? PBS_BUSY : PBS_FREE;
}
