[EPF_ComponentSaveDataType(SlotManagerComponent), BaseContainerProps()]
class EPF_SlotManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
	[Attribute("1", desc: "Skip writing slot attached entities that are non banked if they have only default data and match the default slot prefab.\nDisable this option if you absoluetly need to track persistent ids of the slots. Can cause larger amounts of data for vehicles that have many nested slots.")]
	bool m_bSkipDefaultNonBaked;
}

[EDF_DbName.Automatic()]
class EPF_SlotManagerComponentSaveData : EPF_ComponentSaveData
{
	ref array<ref EPF_PersistentEntitySlot> m_aSlots;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		SlotManagerComponent slotManager = SlotManagerComponent.Cast(component);
		EPF_SlotManagerComponentSaveDataClass settings = EPF_SlotManagerComponentSaveDataClass.Cast(attributes);

		m_aSlots = {};
		array<ref EPF_EntitySlotPrefabInfo> slotinfos = EPF_EntitySlotPrefabInfo.GetSlotInfos(owner, slotManager);

		array<EntitySlotInfo> outSlotInfos();
		slotManager.GetSlotInfos(outSlotInfos);
		foreach (int idx, EntitySlotInfo entitySlot : outSlotInfos)
		{
			IEntity slotEntity = entitySlot.GetAttachedEntity();
			ResourceName prefab = EPF_Utils.GetPrefabName(slotEntity);

			EPF_EntitySlotPrefabInfo prefabInfo = slotinfos.Get(idx);
			bool isPrefabMatch = prefab == prefabInfo.GetEnabledSlotPrefab();

			EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
			if (!slotPersistence)
			{
				// Slot is different from prefab and the new slot has no persistence component so we clear it.
				if (!isPrefabMatch)
				{
					EPF_PersistentEntitySlot persistentSlot();
					persistentSlot.m_sName = prefabInfo.m_sName;
					m_aSlots.Insert(persistentSlot);
				}

				continue;
			}

			EPF_EReadResult readResult;
			EPF_EntitySaveData saveData = slotPersistence.Save(readResult);
			if (!saveData)
				return EPF_EReadResult.ERROR;

			// Read transform to see if slot uses OverrideTransformLS set.
			ReadTransform(slotEntity, saveData, prefabInfo, readResult);

			// We can safely ignore baked objects with default info on them, but anything else needs to be saved.
			if (attributes.m_bTrimDefaults &&
				isPrefabMatch &&
				readResult == EPF_EReadResult.DEFAULT &&
				(settings.m_bSkipDefaultNonBaked ||
				EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED)))
			{
				continue;
			}

			EPF_PersistentEntitySlot persistentSlot();
			persistentSlot.m_sName = prefabInfo.m_sName;
			persistentSlot.m_pEntity = saveData;
			m_aSlots.Insert(persistentSlot);
		}

		if (m_aSlots.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	static void ReadTransform(IEntity slotEntity, EPF_EntitySaveData saveData, EPF_EntitySlotPrefabInfo prefabInfo, out EPF_EReadResult readResult)
	{
		EPF_PersistenceComponentClass slotAttributes = EPF_ComponentData<EPF_PersistenceComponentClass>.Get(slotEntity);
		if (saveData.m_pTransformation.ReadFrom(slotEntity, slotAttributes.m_pSaveData, false))
		{
			if (!EPF_Const.IsUnset(saveData.m_pTransformation.m_vOrigin) &&
				vector.Distance(saveData.m_pTransformation.m_vOrigin, prefabInfo.m_vOffset) > 0.001)
			{
				readResult = EPF_EReadResult.OK;
			}

			if (!EPF_Const.IsUnset(saveData.m_pTransformation.m_vAngles))
			{
				vector localFixedAngles = slotEntity.GetLocalAngles();
				if (float.AlmostEqual(localFixedAngles[0], -180))
					localFixedAngles[0] = 180;

				if (float.AlmostEqual(localFixedAngles[1], -180))
					localFixedAngles[1] = 180;

				if (float.AlmostEqual(localFixedAngles[2], -180))
					localFixedAngles[2] = 180;

				if (vector.Distance(localFixedAngles, prefabInfo.m_vAngles) > 0.001)
					readResult = EPF_EReadResult.OK;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		SlotManagerComponent slotManager = SlotManagerComponent.Cast(component);
		array<EntitySlotInfo> outSlotInfos();
		slotManager.GetSlotInfos(outSlotInfos);

		EPF_EApplyResult result = EPF_EApplyResult.OK;
		foreach (EPF_PersistentEntitySlot persistentSlot : m_aSlots)
		{
			EntitySlotInfo slot = slotManager.GetSlotByName(persistentSlot.m_sName);
			if (!slot)
				continue;

			EPF_EApplyResult slotResult = ApplySlot(slot, persistentSlot.m_pEntity);
			if (slotResult == EPF_EApplyResult.ERROR)
				return EPF_EApplyResult.ERROR;

			if (slotResult == EPF_EApplyResult.AWAIT_COMPLETION)
				result = EPF_EApplyResult.AWAIT_COMPLETION;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	static EPF_EApplyResult ApplySlot(EntitySlotInfo entitySlot, EPF_EntitySaveData saveData)
	{
		IEntity slotEntity = entitySlot.GetAttachedEntity();

		// If there is an tramsform override we need to flag it before load as we handle it on our own
		EPF_PersistentTransformation persistentTransform = saveData.m_pTransformation;
		if (persistentTransform)
			persistentTransform.m_bApplied = true;

		// Found matching entity, no need to spawn, just apply save-data
		if (saveData &&
			slotEntity &&
			EPF_Utils.GetPrefabName(slotEntity).StartsWith(saveData.m_rPrefab))
		{
			EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
			if (slotPersistence && !slotPersistence.Load(saveData, false))
				return EPF_EApplyResult.ERROR;
		}
		else
		{
			// Slot did not match save-data, delete current entity on it
			SCR_EntityHelper.DeleteEntityAndChildren(slotEntity);

			if (!saveData)
				return EPF_EApplyResult.OK;

			// Spawn new entity and attach it
			slotEntity = saveData.Spawn(false);
			if (!slotEntity)
				return EPF_EApplyResult.ERROR;

			entitySlot.AttachEntity(slotEntity);
			if (entitySlot.GetAttachedEntity() != slotEntity)
				return EPF_EApplyResult.ERROR;
		}

		if (persistentTransform)
		{
			vector transform[4];

			if (!EPF_Const.IsUnset(persistentTransform.m_vOrigin))
				transform[3] = persistentTransform.m_vOrigin;

			if (!EPF_Const.IsUnset(persistentTransform.m_vAngles))
			{
				Math3D.AnglesToMatrix(persistentTransform.m_vAngles, transform);
			}
			else
			{
				Math3D.MatrixIdentity3(transform);
			}

			if (EPF_Const.IsUnset(persistentTransform.m_fScale))
				Math3D.MatrixScale(transform, persistentTransform.m_fScale);

			entitySlot.OverrideTransformLS(transform);
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_SlotManagerComponentSaveData otherData = EPF_SlotManagerComponentSaveData.Cast(other);

		if (m_aSlots.Count() != otherData.m_aSlots.Count())
			return false;

		foreach (int idx, EPF_PersistentEntitySlot slot : m_aSlots)
		{
			// Try same index first as they are likely to be the correct ones.
			if (slot.Equals(otherData.m_aSlots.Get(idx)))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentEntitySlot otherSlot : otherData.m_aSlots)
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
}

class EPF_PersistentEntitySlot
{
	string m_sName;
	ref EPF_EntitySaveData m_pEntity;

	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentEntitySlot other)
	{
		return m_sName == other.m_sName && m_pEntity.Equals(other.m_pEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		saveContext.WriteValue("m_sName", m_sName);

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
		if (!loadContext.IsValid())
			return false;

		loadContext.ReadValue("m_sName", m_sName);

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
}
