modded class SCR_CharacterInventoryStorageComponent
{
	protected int m_iEPFDebounceTime = -1;

	//------------------------------------------------------------------------------------------------
	override void StoreItemToQuickSlot(notnull IEntity pItem, int iSlotIndex = -1, bool isForced = false)
	{
		super.StoreItemToQuickSlot(pItem, iSlotIndex, isForced);

		// Debounce sync to configured interval after the last change.
		RplComponent replication = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (replication && replication.IsOwnerProxy())
		{
			if (m_iEPFDebounceTime == -1)
			{
				EPF_PersistenceComponent persistence = EPF_PersistenceComponent.Cast(GetOwner().FindComponent(EPF_PersistenceComponent));
				if (persistence)
				{
					auto charInvSaveDataClass = EPF_CharacterInventoryStorageComponentSaveDataClass.Cast(persistence.GetAttributeClass(EPF_CharacterInventoryStorageComponentSaveDataClass));
					if (charInvSaveDataClass) m_iEPFDebounceTime = charInvSaveDataClass.m_iMaxQuickbarSaveTime * 1000; // Convert to ms from seconds
				}
			}

			if (m_iEPFDebounceTime != -1)
			{
				ScriptCallQueue callQueue = GetGame().GetCallqueue();
				if (callQueue.GetRemainingTime(EPF_SyncQuickSlots) == -1)
					callQueue.CallLater(EPF_SyncQuickSlots, m_iEPFDebounceTime);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void EPF_SyncQuickSlots()
	{
		array<RplId> rplIds();
		rplIds.Reserve(m_aQuickSlots.Count());

		foreach (IEntity quickSlotItem : m_aQuickSlots)
		{
			RplId rplId = RplId.Invalid();

			if (quickSlotItem)
			{
				RplComponent replication = RplComponent.Cast(quickSlotItem.FindComponent(RplComponent));
				if (replication) rplId = replication.Id();
			}

			rplIds.Insert(rplId);
		}

		Rpc(EPF_Rpc_UpdateQuickSlotItems, rplIds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void EPF_Rpc_UpdateQuickSlotItems(array<RplId> rplIds)
	{
		if (!rplIds || (m_aQuickSlots.Count() != rplIds.Count())) return;

		// Dequeue any pending update if we just received data from server
		RplComponent replication = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (replication && replication.IsOwner() && (m_iEPFDebounceTime != -1))
		{
			GetGame().GetCallqueue().Remove(EPF_SyncQuickSlots);
		}

		int slotsCount = DEFAULT_QUICK_SLOTS.Count();
		if (m_aQuickSlotsHistory.Count() < slotsCount)
		{
			m_aQuickSlotsHistory.Resize(slotsCount);
		}

		foreach (int idx, RplId rplId : rplIds)
		{
			IEntity slotEntity = EPF_NetworkUtils.FindEntityByRplId(rplId);
			m_aQuickSlots.Set(idx, slotEntity);
			if (slotEntity) m_aQuickSlotsHistory.Set(idx, GetItemType(slotEntity));
		}
	}
};
