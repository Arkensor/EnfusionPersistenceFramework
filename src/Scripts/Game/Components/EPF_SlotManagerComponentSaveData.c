[EPF_ComponentSaveDataType(SlotManagerComponent), BaseContainerProps()]
class EPF_SlotManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_SlotManagerComponentSaveData : EPF_ComponentSaveData
{
	ref array<ref EPF_PersistentEntitySlot> m_aSlots;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		SlotManagerComponent slotManager = SlotManagerComponent.Cast(component);

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
				EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
				readResult == EPF_EReadResult.DEFAULT)
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
		if (saveData.m_pTransformation.ReadFrom(slotEntity, slotAttributes.m_pSaveData))
		{
			if (saveData.m_pTransformation.m_vOrigin != EPF_Const.VEC_INFINITY &&
				vector.Distance(saveData.m_pTransformation.m_vOrigin, prefabInfo.m_vOffset) > 0.001)
				readResult = EPF_EReadResult.OK;

			if (saveData.m_pTransformation.m_vAngles != EPF_Const.VEC_INFINITY)
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
		foreach (EPF_PersistentEntitySlot slot : m_aSlots)
		{
			EntitySlotInfo slot = slotManager.GetSlotByName(slot.m_sName);
			if (!slot)
				continue;

			EPF_EApplyResult slotResult = ApplySlot(outSlotInfos.Get(idx), slot.m_pEntity);
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

		// If there is an tramsform override saved we need to consume it before load operations
		EPF_PersistentTransformation persistentTransform;
		if (saveData)
		{
			persistentTransform = saveData.m_pTransformation;
			saveData.m_pTransformation = null;
		}

		// Found matching entity, no need to spawn, just apply save-data
		if (saveData &&
			slotEntity &&
			EPF_Utils.GetPrefabName(slotEntity) == saveData.m_rPrefab)
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

			if (persistentTransform.m_vOrigin != EPF_Const.VEC_INFINITY)
				transform[3] = persistentTransform.m_vOrigin;

			if (persistentTransform.m_vAngles != EPF_Const.VEC_INFINITY)
			{
				Math3D.AnglesToMatrix(persistentTransform.m_vAngles, transform);
			}
			else
			{
				Math3D.MatrixIdentity3(transform);
			}

			if (persistentTransform.m_fScale != float.INFINITY)
				SCR_Math3D.ScaleMatrix(transform, persistentTransform.m_fScale);

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
};

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

		saveContext.WriteValue("entityType", entityType);

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
