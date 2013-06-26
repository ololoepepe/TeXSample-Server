#include "storagelocker.h"
#include "storage.h"

/*============================================================================
================================ StorageLocker ===============================
============================================================================*/

/*============================== Public constructors =======================*/

StorageLocker::StorageLocker()
{
    Storage::lock();
}

StorageLocker::~StorageLocker()
{
    Storage::unlock();
}
