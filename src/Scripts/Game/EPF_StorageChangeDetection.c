class EPF_StorageChangeDetection
{
	protected static ref set<BaseInventoryStorageComponent> s_aDirtyStorages = new set<BaseInventoryStorageComponent>();

	//------------------------------------------------------------------------------------------------
	static void SetDirty(notnull BaseInventoryStorageComponent storage)
	{
		s_aDirtyStorages.Insert(storage);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDirty(notnull BaseInventoryStorageComponent storage)
	{
		return s_aDirtyStorages.Contains(storage);
	}

	//------------------------------------------------------------------------------------------------
	static void Cleanup(notnull IEntity owner)
	{
		array<Managed> storages();
		owner.FindComponents(BaseInventoryStorageComponent, storages);
		foreach (Managed storageRef : storages)
		{
			s_aDirtyStorages.RemoveItem(BaseInventoryStorageComponent.Cast(storageRef));
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Reset()
	{
		s_aDirtyStorages = new set<BaseInventoryStorageComponent>();
		EPF_BakedStorageChange.Reset();
	}
};

class EPF_BakedStorageChange
{
	protected static ref map<string, ref EPF_BakedStorageChange> s_mStorageChanges = new map<string, ref EPF_BakedStorageChange>();

	string m_sRemovedItemId;
	bool m_bReplaced;

	//------------------------------------------------------------------------------------------------
	static void OnAdded(EPF_PersistenceComponent childPersistence, InventoryStorageSlot parentSlot)
	{
		int slotId = parentSlot.GetID();
		BaseInventoryStorageComponent storage = parentSlot.GetStorage();

		string storageKey = string.Format("%1:%2", storage, slotId);
		EPF_BakedStorageChange change = s_mStorageChanges.Get(storageKey);
		if (!change)
		{
			change = new EPF_BakedStorageChange();
		}
		else if (change.m_sRemovedItemId == childPersistence.GetPersistentId())
		{
			// Previously removed item from baked storage is returned into the same slot.
			change = null;
		}

		if (!change)
		{
			//PrintFormat("Baked %1 was returned to default because removed item was added back.", storageKey);
			s_mStorageChanges.Remove(storageKey);
			return;
		}

		//PrintFormat("Baked %1 was changed with new item in slot.", storageKey);
		change.m_bReplaced = true;
		s_mStorageChanges.Set(storageKey, change);
	}

	//------------------------------------------------------------------------------------------------
	static void OnRemoved(EPF_PersistenceComponent childPersistence, InventoryStorageSlot parentSlot)
	{
		string storageKey = string.Format("%1:%2", parentSlot.GetStorage(), parentSlot.GetID());

		EPF_BakedStorageChange change = s_mStorageChanges.Get(storageKey);
		if (!change && EPF_BitFlags.CheckFlags(childPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED))
		{
			change = new EPF_BakedStorageChange();
			change.m_sRemovedItemId = childPersistence.GetPersistentId();
			//PrintFormat("Baked %1 was changed because baked item was removed.", storageKey);
			s_mStorageChanges.Set(storageKey, change);
		}
		else if (change)
		{
			if (!change.m_sRemovedItemId)
			{
				//PrintFormat("Baked %1 was returned to default because added dynamic item was removed again.", storageKey);
				s_mStorageChanges.Remove(storageKey);
				return;
			}

			//PrintFormat("Baked %1 is still removed, but no longer replaced.", storageKey);
			change.m_bReplaced = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool Has(BaseInventoryStorageComponent storage, int slotId)
	{
		string storageKey = string.Format("%1:%2", storage, slotId);
		return s_mStorageChanges.Contains(storageKey);
	}

	//------------------------------------------------------------------------------------------------
	static EPF_BakedStorageChange Get(BaseInventoryStorageComponent storage, int slotId)
	{
		string storageKey = string.Format("%1:%2", storage, slotId);
		return s_mStorageChanges.Get(storageKey);
	}

	//------------------------------------------------------------------------------------------------
	static void Set(BaseInventoryStorageComponent storage, int slotId, EPF_BakedStorageChange change)
	{
		string storageKey = string.Format("%1:%2", storage, slotId);
		s_mStorageChanges.Set(storageKey, change);
	}

	//------------------------------------------------------------------------------------------------
	static void Reset()
	{
		// Create a new map for next game start
		s_mStorageChanges = new map<string, ref EPF_BakedStorageChange>();
	}
};
