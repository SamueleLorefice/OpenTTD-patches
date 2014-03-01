/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_storage.cpp Functionality related to the temporary and persistent storage arrays for NewGRFs. */

#include "stdafx.h"
#include "newgrf_storage.h"
#include "core/pool_func.hpp"
#include "core/endian_func.hpp"
#include "debug.h"
#include <set>

template<> PersistentStorage::Pool PersistentStorage::PoolItem::pool ("PersistentStorage");
INSTANTIATE_POOL_METHODS(PersistentStorage)

/** The changed storage arrays */
static std::set<BasePersistentStorageArray*> *_changed_storage_arrays = new std::set<BasePersistentStorageArray*>;

/**
 * Remove references to use.
 */
BasePersistentStorageArray::~BasePersistentStorageArray()
{
	_changed_storage_arrays->erase(this);
}

/**
 * Add the changed storage array to the list of changed arrays.
 * This is done so we only have to revert/save the changed
 * arrays, which saves quite a few clears, etc. after callbacks.
 * @param storage the array that has changed
 */
void AddChangedPersistentStorage(BasePersistentStorageArray *storage)
{
	_changed_storage_arrays->insert(storage);
}

/**
 * Clear the changes made since the last #ClearStorageChanges.
 * This is done for *all* storages that have been registered to with
 * #AddChangedStorage since the previous #ClearStorageChanges.
 *
 * This can be done in two ways:
 *  - saving the changes permanently
 *  - reverting to the previous version
 * @param keep_changes do we save or revert the changes since the last #ClearChanges?
 */
void ClearPersistentStorageChanges(bool keep_changes)
{
	/* Loop over all changes arrays */
	for (std::set<BasePersistentStorageArray*>::iterator it = _changed_storage_arrays->begin(); it != _changed_storage_arrays->end(); it++) {
		if (!keep_changes) {
			DEBUG(desync, 1, "Discarding persistent storage changes: Feature %d, GrfID %08X, Tile %d", (*it)->feature, BSWAP32((*it)->grfid), (*it)->tile);
		}
		(*it)->ClearChanges(keep_changes);
	}

	/* And then clear that array */
	_changed_storage_arrays->clear();
}
