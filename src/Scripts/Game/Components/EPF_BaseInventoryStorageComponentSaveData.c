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
			EPF_BakedStorageChange slotChange = EPF_BakedStorageChange.Get(storageComponent, nSlot);
			EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);

			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(owner);
			bool baked = EPF_BitFlags.CheckFlags(persistence.GetFlags(), EPF_EPersistenceFlags.BAKED);

			bool bakedParent = true;
			IEntity parent = owner.GetParent();
			if (parent)
			{
				EPF_PersistenceComponent parentPersistence = EPF_Component<EPF_PersistenceComponent>.Find(parent);
				if (parentPersistence)
					bakedParent = EPF_BitFlags.CheckFlags(parentPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED);
			}

			if (!slotPersistence)
			{
				// Item on slot that has no persistece component needs to be explictly removed or else on load there would be nothing removing it
				// Also remove if slot is now empty but a slot change was recorded (can only be a removal then)
				if (baked && bakedParent && (slotEntity || slotChange))
				{
					EPF_PersistentInventoryStorageSlot persistentSlot();
					persistentSlot.m_iSlotIndex = nSlot;
					m_aSlots.Insert(persistentSlot);
				}

				// Skip emtpy slots for non baked, they will wipe any items not mentioned in save-data on load,
				// they always save all items to know the ids even if no changes on items
				continue;
			}

			EPF_EReadResult readResult;
			EPF_EntitySaveData saveData = slotPersistence.Save(readResult);
			if (!saveData)
				return EPF_EReadResult.ERROR;

			// We can safely ignore baked objects with default info on them, but anything else needs to be saved.
			if (attributes.m_bTrimDefaults &&
				readResult == EPF_EReadResult.DEFAULT &&
				EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
				baked &&
				bakedParent &&
				!slotChange)
			{
				// Double check parent slot that there is no baked storage change detected
				bool canSkip = true;
				InventoryStorageSlot parentSlot = storageComponent.GetParentSlot();
				while (parentSlot)
				{
					if (EPF_BakedStorageChange.Has(parentSlot.GetStorage(), parentSlot.GetID()))
					{
						canSkip = false;
						break;
					}

					BaseInventoryStorageComponent storage = parentSlot.GetStorage();
					if (!storage)
						break;

					parentSlot = storage.GetParentSlot();
				}

				if (canSkip)
					continue;
			}

			EPF_PersistentInventoryStorageSlot persistentSlot();
			persistentSlot.m_iSlotIndex = nSlot;
			persistentSlot.m_pEntity = saveData;
			m_aSlots.Insert(persistentSlot);
		}

		if (m_aSlots.IsEmpty() && !EPF_StorageChangeDetection.IsDirty(storageComponent))
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

		bool isNotBaked = !EPF_BitFlags.CheckFlags(EPF_Component<EPF_PersistenceComponent>.Find(owner).GetFlags(), EPF_EPersistenceFlags.BAKED);
		set<int> processedSlots;
		if (isNotBaked)
			processedSlots = new set<int>();

		foreach (EPF_PersistentInventoryStorageSlot slot : m_aSlots)
		{
			if (isNotBaked)
				processedSlots.Insert(slot.m_iSlotIndex);

			IEntity slotEntity = storageComponent.Get(slot.m_iSlotIndex);

			// Found matching entity, no need to spawn, just apply save-data
			if (slot.m_pEntity &&
				slotEntity &&
				EPF_Utils.GetPrefabName(slotEntity).StartsWith(slot.m_pEntity.m_rPrefab))
			{
				EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
				if (slotPersistence && !slotPersistence.Load(slot.m_pEntity, false))
					return EPF_EApplyResult.ERROR;

				continue;
			}

			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(owner);
			if (EPF_BitFlags.CheckFlags(persistence.GetFlags(), EPF_EPersistenceFlags.BAKED))
			{
				EPF_BakedStorageChange change();
				if (slotEntity)
				{
					EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
					if (!slotPersistence)
					{
						// Non persistence slot removals can only be overridden by dynamic entity placed into them
						change.m_sRemovedItemId = "PERMANENT_BAKED_REMOVAL";
					}
					else
					{
						// Remember slot removals
						change.m_sRemovedItemId = slotPersistence.GetPersistentId();
					}

				}
				else
				{
					// Remember slot replacement on baked entities
					change.m_bReplaced = true;
				}

				EPF_BakedStorageChange.Set(storageComponent, slot.m_iSlotIndex, change);
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
				EPF_WorldUtils.Teleport(slotEntity, storageComponent.GetOwner().GetOrigin(), storageComponent.GetOwner().GetYawPitchRoll()[0]);
		}

		// Delte any items not found in the storage data for non bakes that always save all slots
		if (isNotBaked)
		{
			for (int nSlot = 0, count = storageComponent.GetSlotsCount(); nSlot < count; nSlot++)
			{
				if (!processedSlots.Contains(nSlot))
				{
					IEntity slotEntity = storageComponent.Get(nSlot);
					if (slotEntity && slotEntity.FindComponent(EPF_PersistenceComponent))
						storageManager.TryDeleteItem(slotEntity);
				}
			}
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

		saveContext.WriteValue("_type", entityType);

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
		loadContext.ReadValue("_type", entityTypeString);

		// TODO: Remove backwards compatiblity in 0.9.9
		if (!entityTypeString && ContainerSerializationLoadContext.Cast(loadContext).GetContainer().IsInherited(JsonLoadContainer))
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
