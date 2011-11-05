/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pathfinder_type.h General types related to pathfinders. */

#ifndef PATHFINDER_TYPE_H
#define PATHFINDER_TYPE_H

#include "../tile_type.h"
#include "../track_type.h"
#include "../track_func.h"

/** Length (penalty) of one tile with NPF */
static const int NPF_TILE_LENGTH = 100;

/**
 * This penalty is the equivalent of "infinite", which means that paths that
 * get this penalty will be chosen, but only if there is no other route
 * without it. Be careful with not applying this penalty to often, or the
 * total path cost might overflow..
 */
static const int NPF_INFINITE_PENALTY = 1000 * NPF_TILE_LENGTH;


/** Length (penalty) of one tile with YAPF */
static const int YAPF_TILE_LENGTH = 100;

/** Length (penalty) of a corner with YAPF */
static const int YAPF_TILE_CORNER_LENGTH = 71;

/**
 * This penalty is the equivalent of "infinite", which means that paths that
 * get this penalty will be chosen, but only if there is no other route
 * without it. Be careful with not applying this penalty to often, or the
 * total path cost might overflow..
 */
static const int YAPF_INFINITE_PENALTY = 1000 * YAPF_TILE_LENGTH;

/**
 * Helper container to find a depot
 */
struct FindDepotData {
	TileIndex tile;   ///< The tile of the depot
	uint best_length; ///< The distance towards the depot in penalty, or UINT_MAX if not found
	bool reverse;     ///< True if reversing is necessary for the train to get to this depot

	/**
	 * Create an instance of this structure.
	 * @param tile        the tile of the depot
	 * @param best_length the distance towards the depot, or UINT_MAX if not found
	 * @param reverse     whether we need to reverse first.
	 */
	FindDepotData(TileIndex tile = INVALID_TILE, uint best_length = UINT_MAX, bool reverse = false) :
		tile(tile), best_length(best_length), reverse(reverse)
	{
	}
};

/**
 * Pathfinder current position
 */
struct PFPos {
	TileIndex tile;
	Trackdir td;

	/**
	 * Create an empty PFPos
	 */
	PFPos() : tile(INVALID_TILE), td(INVALID_TRACKDIR) { }

	/**
	 * Create a PFPos for a given tile and trackdir
	 */
	PFPos(TileIndex t, Trackdir d) : tile(t), td(d) { }

	/**
	 * Set to a given tile and trackdir
	 */
	void Set(TileIndex t, Trackdir d) {
		tile = t;
		td = d;
	}

	/**
	 * Compare with another PFPos
	 */
	bool operator == (const PFPos &other) const {
		return (tile == other.tile) && (td == other.td);
	}

	/**
	 * Compare with another PFPos
	 */
	bool operator != (const PFPos &other) const {
		return (tile != other.tile) || (td != other.td);
	}
};

/**
 * Pathfinder new position; td will be INVALID_POSITION unless trackdirs has exactly one trackdir set
 */
struct PFNewPos : PFPos {
	TrackdirBits trackdirs;

	inline void SetTrackdir()
	{
		td = (KillFirstBit(trackdirs) == TRACKDIR_BIT_NONE) ? FindFirstTrackdir(trackdirs) : INVALID_TRACKDIR;
	}

	inline bool IsTrackdirSet() const { return td != INVALID_TRACKDIR; }
};

#endif /* PATHFINDER_TYPE_H */
