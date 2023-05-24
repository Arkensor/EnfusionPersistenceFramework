[EPF_ComponentSaveDataType(BaseInventoryStorageComponent), BaseContainerProps()]
class EPF_BaseInventoryStorageComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_BaseInventoryStorageComponentSaveData : EPF_ComponentSaveData
{
	int m_iPriority;
	EStoragePurpose m_ePurposeFlags;
	ref array<ref EPF_PersistentInventoryStorageSlot> m_aSlots;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseInventoryStorageComponent storageComponent = BaseInventoryStorageComponent.Cast(component);

		m_iPriority = storageComponent.GetPriority();
		m_ePurposeFlags = storageComponent.GetPurpose();
		m_aSlots = {};

		for (int nSlot = 0, slots = storageComponent.GetSlotsCount(); nSlot < slots; nSlot++)
		{
			IEntity slotEntity = storageComponent.Get(nSlot);
			ResourceName prefab = EPF_Utils.GetPrefabName(slotEntity);
			ResourceName defaultSlotPrefab = EPF_DefaultPrefabItemsInfo.GetDefaultPrefab(storageComponent, nSlot);

			EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
			if (!slotPersistence)
			{
				// Storage normally has a default prefab spawned onto this slot idx, but it is not persistent so it needs to be removed.
				if (defaultSlotPrefab)
				{
					EPF_PersistentInventoryStorageSlot persistentSlot();
					persistentSlot.m_iSlotIndex = nSlot;
					m_aSlots.Insert(persistentSlot);
				}

				continue;
			}

			EPF_EReadResult readResult;
			EPF_EntitySaveData saveData = slotPersistence.Save(readResult);
			if (!saveData)
				return EPF_EReadResult.ERROR;

			// We can safely ignore baked objects with default info on them, but anything else needs to be saved.
			if (attributes.m_bTrimDefaults &&
				prefab == defaultSlotPrefab &&
				EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
				readResult == EPF_EReadResult.DEFAULT)
			{
				// Ignore only if no parent or parent is also baked
				EPF_PersistenceComponent parentPersistence = EPF_Component<EPF_PersistenceComponent>.Find(owner.GetParent());
				if (!parentPersistence || EPF_BitFlags.CheckFlags(parentPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED))
					continue;
			}

			EPF_PersistentInventoryStorageSlot persistentSlot();
			persistentSlot.m_iSlotIndex = nSlot;
			persistentSlot.m_pEntity = saveData;
			m_aSlots.Insert(persistentSlot);
		}

		if (m_aSlots.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsFor(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseInventoryStorageComponent storageComponent = BaseInventoryStorageComponent.Cast(component);
		return (storageComponent.GetPriority() == m_iPriority) && (storageComponent.GetPurpose() == m_ePurposeFlags);
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseInventoryStorageComponent storageComponent = BaseInventoryStorageComponent.Cast(component);
		InventoryStorageManagerComponent storageManager = InventoryStorageManagerComponent.Cast(storageComponent.GetOwner().FindComponent(InventoryStorageManagerComponent));
		if (!storageManager) storageManager = EPF_GlobalInventoryStorageManagerComponent.GetInstance();

		foreach (EPF_PersistentInventoryStorageSlot slot : m_aSlots)
		{
			IEntity slotEntity = storageComponent.Get(slot.m_iSlotIndex);

			// Found matching entity, no need to spawn, just apply save-data
			if (slot.m_pEntity &&
				slotEntity &&
				EPF_Utils.GetPrefabName(slotEntity) == EPF_DefaultPrefabItemsInfo.GetDefaultPrefab(storageComponent, slot.m_iSlotIndex))
			{
				EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
				if (slotPersistence && !slotPersistence.Load(slot.m_pEntity, false))
					return EPF_EApplyResult.ERROR;

				continue;
			}

			// Slot did not match save-data, delete current entity on it
			storageManager.TryDeleteItem(slotEntity);

			if (!slot.m_pEntity)
				continue;

			// Spawn new entity and attach it
			slotEntity = slot.m_pEntity.Spawn(false);
			if (!slotEntity)
				return EPF_EApplyResult.ERROR;

			// Unable to add it to the storage parent, so put it on the ground at the parent origin
			if (!storageManager.TryInsertItemInStorage(slotEntity, storageComponent, slot.m_iSlotIndex))
				EPF_Utils.Teleport(slotEntity, storageComponent.GetOwner().GetOrigin(), storageComponent.GetOwner().GetYawPitchRoll()[0]);
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_BaseInventoryStorageComponentSaveData otherData = EPF_BaseInventoryStorageComponentSaveData.Cast(other);

		if (m_iPriority != otherData.m_iPriority ||
			m_ePurposeFlags != otherData.m_ePurposeFlags ||
			m_aSlots.Count() != otherData.m_aSlots.Count())
			return false;

		foreach (int idx, EPF_PersistentInventoryStorageSlot slot : m_aSlots)
		{
			// Try same index first as they are likely to be the correct ones.
			if (slot.Equals(otherData.m_aSlots.Get(idx)))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentInventoryStorageSlot otherSlot : otherData.m_aSlots)
			{
				if (compareIdx == idx)
					continue; // Already tried in idx direct compare

				if (slot.Equals(otherSlot))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}
};

class EPF_PersistentInventoryStorageSlot
{
	int m_iSlotIndex;
	ref EPF_EntitySaveData m_pEntity;

	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentInventoryStorageSlot other)
	{
		return m_iSlotIndex == other.m_iSlotIndex && m_pEntity.Equals(other.m_pEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid()) return false;

		saveContext.WriteValue("m_iSlotIndex", m_iSlotIndex);

		string entityType = "EMPTY";
		if (m_pEntity)
			entityType = EDF_DbName.Get(m_pEntity.Type());

		saveContext.WriteValue("entityType", entityType);

		if (entityType)
			saveContext.WriteValue("m_pEntity", m_pEntity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid()) return false;

		loadContext.ReadValue("m_iSlotIndex", m_iSlotIndex);

		string entityTypeString;
		loadContext.ReadValue("entityType", entityTypeString);

		if (entityTypeString == "EMPTY")
			return true;

		typename entityType = EDF_DbName.GetTypeByName(entityTypeString);
		if (!entityType)
			return false;

		m_pEntity = EPF_EntitySaveData.Cast(entityType.Spawn());
		loadContext.ReadValue("m_pEntity", m_pEntity);

		return true;
	}
};
