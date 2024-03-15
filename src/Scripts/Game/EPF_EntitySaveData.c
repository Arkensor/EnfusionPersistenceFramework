enum EPF_ETransformSaveFlags
{
	COORDS = 0x01,
	ANGLES = 0x02,
	SCALE = 0x04
}

[BaseContainerProps()]
class EPF_EntitySaveDataClass
{
	[Attribute("1", desc: "Trim default values from the save data to minimize storage and avoid default value entires in the database.\nE.g. there is no need to persist that a cars engine is off.")]
	bool m_bTrimDefaults;

	[Attribute("3", UIWidgets.Flags, desc: "Choose which aspects from the entity transformation are persisted.", enums: ParamEnumArray.FromEnum(EPF_ETransformSaveFlags))]
	EPF_ETransformSaveFlags m_eTranformSaveFlags;

	[Attribute("1", desc: "Only relevant if the world contains an active GarbageManager.")]
	bool m_bSaveRemainingLifetime;

	[Attribute(desc: "Components to persist.")]
	ref array<ref EPF_ComponentSaveDataClass> m_aComponents;
}

class EPF_EntitySaveData : EPF_MetaDataDbEntity
{
	ResourceName m_rPrefab;
	ref EPF_PersistentTransformation m_pTransformation;
	float m_fRemainingLifetime;
	ref array<ref EPF_PersistentComponentSaveData> m_aComponents;

	//------------------------------------------------------------------------------------------------
	//! Spawn the world entity based on this save-data instance
	//! \param isRoot true if the current entity is a world root (not a stored item inside a storage)
	//! \return world entity or null if it could not be correctly spawned/loaded
	IEntity Spawn(bool isRoot = true)
	{
		return EPF_PersistenceManager.GetInstance().SpawnWorldEntity(this, isRoot);
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the save-data from the world entity
	//! \param entity to read the save-data from
	//! \param attributes the class-class shared configuration attributes assigned in the world editor
	//! \return EPF_EReadResult.OK if save-data could be read, ERROR if something failed, NODATA for all default values
	EPF_EReadResult ReadFrom(IEntity entity, EPF_EntitySaveDataClass attributes)
	{
		EPF_EReadResult statusCode = EPF_EReadResult.DEFAULT;
		if (!attributes.m_bTrimDefaults)
			statusCode = EPF_EReadResult.OK;

		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(entity);
		ReadMetaData(persistenceComponent);

		// Prefab
		m_rPrefab = EPF_Utils.GetPrefabName(entity);

		// Transform
		m_pTransformation = new EPF_PersistentTransformation();
		// We save it on root entities and always on characters (in case the parent vehicle is not loaded back in)
		// We can skip transform for baked entities that were not moved.
		EPF_EPersistenceFlags flags = persistenceComponent.GetFlags();
		if (EPF_BitFlags.CheckFlags(flags, EPF_EPersistenceFlags.ROOT) &&
			(!EPF_BitFlags.CheckFlags(flags, EPF_EPersistenceFlags.BAKED) || EPF_BitFlags.CheckFlags(flags, EPF_EPersistenceFlags.WAS_MOVED)))
		{
			if (m_pTransformation.ReadFrom(entity, attributes, true))
				statusCode = EPF_EReadResult.OK;
		}

		// Lifetime
		if (EPF_BitFlags.CheckFlags(flags, EPF_EPersistenceFlags.ROOT) &&
			attributes.m_bSaveRemainingLifetime)
		{
			auto garbage = SCR_GarbageSystem.GetByEntityWorld(entity);
			if (garbage)
				m_fRemainingLifetime = garbage.GetRemainingLifetime(entity);

			if (m_fRemainingLifetime == -1)
			{
				m_fRemainingLifetime = 0;
			}
			else if (m_fRemainingLifetime > 0)
			{
				statusCode = EPF_EReadResult.OK;
			}
		}

		// Components
		m_aComponents = {};

		array<Managed> processedComponents();

		// Go through hierarchy sorted component types
		foreach (EPF_ComponentSaveDataClass componentSaveDataClass : attributes.m_aComponents)
		{
			typename saveDataType = EPF_Utils.TrimEnd(componentSaveDataClass.ClassName(), 5).ToType();
			if (!saveDataType)
				return EPF_EReadResult.ERROR;

			array<Managed> outComponents();
			entity.FindComponents(EPF_ComponentSaveDataType.Get(componentSaveDataClass.Type()), outComponents);
			foreach (Managed componentRef : outComponents)
			{
				// Ingore base class find machtes if a parent class was already processed
				if (processedComponents.Contains(componentRef))
					continue;

				processedComponents.Insert(componentRef);

				EPF_ComponentSaveData componentSaveData = EPF_ComponentSaveData.Cast(saveDataType.Spawn());
				if (!componentSaveData)
					return EPF_EReadResult.ERROR;

				componentSaveDataClass.m_bTrimDefaults = attributes.m_bTrimDefaults;
				EPF_EReadResult componentRead = componentSaveData.ReadFrom(entity, GenericComponent.Cast(componentRef), componentSaveDataClass);
				if (componentRead == EPF_EReadResult.ERROR)
					return componentRead;

				if (componentRead == EPF_EReadResult.DEFAULT && attributes.m_bTrimDefaults)
					continue;

				EPF_PersistentComponentSaveData persistentComponent();
				persistentComponent.m_pData = componentSaveData;
				m_aComponents.Insert(persistentComponent);
			}
		}

		if (!m_aComponents.IsEmpty())
			statusCode = EPF_EReadResult.OK;

		return statusCode;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies the save-data a world entity
	//! \param entity to apply the data to
	//! \param attributes the class-class shared configuration attributes assigned in the world editor
	//! \return true if save-data could be applied, false if something failed.
	EPF_EApplyResult ApplyTo(IEntity entity, EPF_EntitySaveDataClass attributes)
	{
		EPF_EApplyResult result = EPF_EApplyResult.OK;

		// Transform
		if (m_pTransformation && !m_pTransformation.m_bApplied)
			EPF_WorldUtils.ForceTransform(entity, m_pTransformation.m_vOrigin, m_pTransformation.m_vAngles, m_pTransformation.m_fScale);

		// Lifetime
		if (attributes.m_bSaveRemainingLifetime)
		{
			auto garbage = SCR_GarbageSystem.GetByEntityWorld(entity);
			if (garbage && m_fRemainingLifetime > 0)
				garbage.Insert(entity, m_fRemainingLifetime);
		}

		// Components
		set<Managed> processedComponents();
		set<typename> processedSaveDataTypes();
		foreach (EPF_ComponentSaveDataClass componentSaveDataClass : attributes.m_aComponents)
		{
			EPF_EApplyResult componentResult = ApplyComponent(componentSaveDataClass, entity, processedSaveDataTypes, processedComponents, attributes);

			if (componentResult == EPF_EApplyResult.ERROR)
				return EPF_EApplyResult.ERROR;

			if (componentResult == EPF_EApplyResult.AWAIT_COMPLETION)
				result = EPF_EApplyResult.AWAIT_COMPLETION;
		}

		// Update any non character entity. On character this can cause fall through ground.
		if (!ChimeraCharacter.Cast(entity))
			entity.Update();

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Compare entity save-data instances to see if there is any noteable difference
	//! \param other entity save-data to compare against
	//! \return true if save-data is considered to describe the same data. False on differences.
	bool Equals(notnull EPF_EntitySaveData other)
	{
		// Same prefab?
		if (m_rPrefab != other.m_rPrefab)
			return false;

		// Same transformation?
		if (m_pTransformation.m_vOrigin != other.m_pTransformation.m_vOrigin ||
			m_pTransformation.m_vAngles != other.m_pTransformation.m_vAngles ||
			m_pTransformation.m_fScale != other.m_pTransformation.m_fScale)
		{
			return false;
		}

		// Same lifetime?
		if (m_fRemainingLifetime != other.m_fRemainingLifetime)
			return false;

		// See if we can match all component save-data instances
		if (m_aComponents.Count() != other.m_aComponents.Count())
			return false;

		foreach (int idx, EPF_PersistentComponentSaveData componentSaveData : m_aComponents)
		{
			// Try same index first as they are likely to be the correct ones.
			if (componentSaveData.m_pData.Equals(other.m_aComponents.Get(idx).m_pData))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentComponentSaveData otherComponent : other.m_aComponents)
			{
				if (compareIdx == idx)
					continue; // Already tried in idx direct compare

				if (componentSaveData.m_pData.Equals(otherComponent.m_pData))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false; //Unable to find any matching component save-data
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected EPF_EApplyResult ApplyComponent(
		EPF_ComponentSaveDataClass componentSaveDataClass,
		IEntity entity,
		set<typename> processedSaveDataTypes,
		set<Managed> processedComponents,
		EPF_EntitySaveDataClass attributes)
	{
		EPF_EApplyResult result = EPF_EApplyResult.OK;

		typename componentSaveDataType = EPF_Utils.TrimEnd(componentSaveDataClass.ClassName(), 5).ToType();

		// Skip already processed save-data
		if (processedSaveDataTypes.Contains(componentSaveDataType))
			return result;

		processedSaveDataTypes.Insert(componentSaveDataType);

		// Make sure required save-data is already applied
		array<typename> requiredSaveDataClasses = componentSaveDataClass.Requires();
		if (requiredSaveDataClasses)
		{
			foreach (typename requiredSaveDataClass : requiredSaveDataClasses)
			{
				if (!requiredSaveDataClass.ToString().EndsWith("Class"))
				{
					Debug.Error(string.Format("Save-data class '%1' lists invalid (non xyzClass) requirement type '%2'. Fix or remove it.", componentSaveDataClass.Type().ToString(), requiredSaveDataClass));
					return EPF_EApplyResult.ERROR;
				}

				foreach (EPF_ComponentSaveDataClass possibleComponentClass : attributes.m_aComponents)
				{
					if (possibleComponentClass == componentSaveDataClass ||
						!possibleComponentClass.IsInherited(requiredSaveDataClass)) continue;

					EPF_EApplyResult componentResult = ApplyComponent(possibleComponentClass, entity, processedSaveDataTypes, processedComponents, attributes);

					if (componentResult == EPF_EApplyResult.ERROR)
						return EPF_EApplyResult.ERROR;

					if (componentResult == EPF_EApplyResult.AWAIT_COMPLETION)
						result = EPF_EApplyResult.AWAIT_COMPLETION;
				}
			}
		}

		// Apply save-data to matching components
		array<Managed> outComponents();
		entity.FindComponents(EPF_ComponentSaveDataType.Get(componentSaveDataClass.Type()), outComponents);
		foreach (EPF_PersistentComponentSaveData persistentComponentSaveData : m_aComponents)
		{
			EPF_ComponentSaveData componentSaveData = persistentComponentSaveData.m_pData;

			if (componentSaveData.Type() != componentSaveDataType)
				continue;

			bool applied = false;
			foreach (Managed componentRef : outComponents)
			{
				if (!processedComponents.Contains(componentRef) && componentSaveData.IsFor(entity, GenericComponent.Cast(componentRef), componentSaveDataClass))
				{
					EPF_EApplyResult componentResult = componentSaveData.ApplyTo(entity, GenericComponent.Cast(componentRef), componentSaveDataClass);

					if (componentResult == EPF_EApplyResult.ERROR)
						return EPF_EApplyResult.ERROR;

					if (componentResult == EPF_EApplyResult.AWAIT_COMPLETION &&
						EPF_DeferredApplyResult.SetEntitySaveData(componentSaveData, this))
					{
						result = EPF_EApplyResult.AWAIT_COMPLETION;
					}

					processedComponents.Insert(componentRef);
					applied = true;
					break;
				}
			}

			if (!applied)
			{
				Print(string.Format("No matching component for '%1' found on entity '%2'@%3", componentSaveData.Type().ToString(), EPF_Utils.GetPrefabName(entity), entity.GetOrigin()), LogLevel.VERBOSE);
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		bool isJson = ContainerSerializationSaveContext.Cast(saveContext).GetContainer().IsInherited(BaseJsonSerializationSaveContainer);

		SerializeMetaData(saveContext);

		// Prefab - go through string for debugging
		string prefabString = m_rPrefab;
		#ifndef PERSISTENCE_DEBUG
		if (prefabString.StartsWith("{")) //keep this solution even though as of 1.0.0.95 it would be saved as just GUID anyway
			prefabString = EPF_Utils.GetPrefabGUID(m_rPrefab);
		#endif
		saveContext.WriteValue("m_rPrefab", prefabString);

		// Transform
		saveContext.WriteValue("m_pTransformation", m_pTransformation);

		// Lifetime
		if (m_fRemainingLifetime > 0 || !isJson)
			saveContext.WriteValue("m_fRemainingLifetime", m_fRemainingLifetime);

		// Components
		if (!m_aComponents.IsEmpty() || !isJson)
			saveContext.WriteValue("m_aComponents", m_aComponents);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid())
			return false;

		DeserializeMetaData(loadContext);

		// Prefab
		string prefabString;
		loadContext.ReadValue("m_rPrefab", prefabString);
		if (prefabString && prefabString.Get(0) != "{")
			prefabString = string.Format("{%1}", prefabString);

		m_rPrefab = prefabString;

		// Transform
		loadContext.ReadValue("m_pTransformation", m_pTransformation);

		// Lifetime
		loadContext.ReadValue("m_fRemainingLifetime", m_fRemainingLifetime);

		// Components
		loadContext.ReadValue("m_aComponents", m_aComponents);
		if (!m_aComponents)
			m_aComponents = {}; // Info might be omitted in json but code expects an instance.

		return true;
	}
}

class EPF_PersistentTransformation
{
	vector m_vOrigin;
	vector m_vAngles;
	float m_fScale;

	// Mark as already applied to entity, e.g. already spawned at target coords via spawn params
	bool m_bApplied;

	//------------------------------------------------------------------------------------------------
	void Reset()
	{
		m_vOrigin = EPF_Const.VECTOR_UNSET;
		m_vAngles = EPF_Const.VECTOR_UNSET;
		m_fScale = EPF_Const.FLOAT_UNSET;
	}

	//------------------------------------------------------------------------------------------------
	bool IsDefault()
	{
		return EPF_Const.IsUnset(m_vOrigin) && EPF_Const.IsUnset(m_vAngles) && EPF_Const.IsUnset(m_fScale);
	}

	//------------------------------------------------------------------------------------------------
	bool ReadFrom(IEntity entity, EPF_EntitySaveDataClass attributes, bool isRoot)
	{
		bool anyData;

		vector angles, transform[4];
		float scale = entity.GetScale();

		// For chars (in vehicles) we want to keep the world transform
		// for if the parent vehicle is deleted they can still spawn
		if (!ChimeraCharacter.Cast(entity) && !isRoot)
		{
			entity.GetLocalTransform(transform);
			angles = entity.GetLocalYawPitchRoll();
		}
		else
		{
			entity.GetWorldTransform(transform);
			angles = entity.GetYawPitchRoll();
		}

		vector origin = transform[3];

		if ((attributes.m_eTranformSaveFlags & EPF_ETransformSaveFlags.COORDS) &&
			(!attributes.m_bTrimDefaults || (origin != vector.Zero)))
		{
			m_vOrigin = origin;
			anyData = true;
		}

		if ((attributes.m_eTranformSaveFlags & EPF_ETransformSaveFlags.ANGLES) &&
			(!attributes.m_bTrimDefaults || (angles != vector.Zero)))
		{
			m_vAngles = angles;
			anyData = true;
		}

		if ((attributes.m_eTranformSaveFlags & EPF_ETransformSaveFlags.SCALE) &&
			(!attributes.m_bTrimDefaults || (scale != 1.0)))
		{
			m_fScale = scale;
			anyData = true;
		}

		return anyData;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid()) return false;

		// For binary stream the info which of the 3 possible props will be written after needs to be known.
		// JSON just has the keys or not, so there it is not a problem.
		EPF_ETransformSaveFlags flags;
		if (!EPF_Const.IsUnset(m_vOrigin))
			flags |= EPF_ETransformSaveFlags.COORDS;

		if (!EPF_Const.IsUnset(m_vAngles))
			flags |= EPF_ETransformSaveFlags.ANGLES;

		if (!EPF_Const.IsUnset(m_fScale))
			flags |= EPF_ETransformSaveFlags.SCALE;

		if (ContainerSerializationSaveContext.Cast(saveContext).GetContainer().IsInherited(BinSerializationSaveContainer))
			saveContext.WriteValue("transformSaveFlags", flags);

		if (flags & EPF_ETransformSaveFlags.COORDS)
			saveContext.WriteValue("m_vOrigin", m_vOrigin);

		if (flags & EPF_ETransformSaveFlags.ANGLES)
			saveContext.WriteValue("m_vAngles", m_vAngles);

		if (flags & EPF_ETransformSaveFlags.SCALE)
			saveContext.WriteValue("m_fScale", m_fScale);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid()) return false;

		EPF_ETransformSaveFlags flags = EPF_ETransformSaveFlags.COORDS | EPF_ETransformSaveFlags.ANGLES | EPF_ETransformSaveFlags.SCALE;
		if (ContainerSerializationLoadContext.Cast(loadContext).GetContainer().IsInherited(BinSerializationLoadContainer))
			loadContext.ReadValue("transformSaveFlags", flags);

		if (flags & EPF_ETransformSaveFlags.COORDS)
			loadContext.ReadValue("m_vOrigin", m_vOrigin);

		if (flags & EPF_ETransformSaveFlags.ANGLES)
			loadContext.ReadValue("m_vAngles", m_vAngles);

		if (flags & EPF_ETransformSaveFlags.SCALE)
			loadContext.ReadValue("m_fScale", m_fScale);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void EPF_PersistentTransformation()
	{
		Reset();
	}
}

class EPF_PersistentComponentSaveData
{
	ref EPF_ComponentSaveData m_pData;

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		saveContext.WriteValue("_type", EDF_DbName.Get(m_pData.Type()));
		saveContext.WriteValue("m_pData", m_pData);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid())
			return false;

		string dataTypeString;
		loadContext.ReadValue("_type", dataTypeString);

		typename dataType = EDF_DbName.GetTypeByName(dataTypeString);
		if (!dataType)
			return false;

		m_pData = EPF_ComponentSaveData.Cast(dataType.Spawn());
		loadContext.ReadValue("m_pData", m_pData);

		return true;
	}
}

class EPF_ComponentSaveDataGetter<Class T>
{
	//------------------------------------------------------------------------------------------------
	static T GetFirst(EPF_EntitySaveData saveData)
	{
		if (saveData)
		{
			foreach (EPF_PersistentComponentSaveData persistentComponent : saveData.m_aComponents)
			{
				if (persistentComponent.m_pData.Type().IsInherited(T))
					return T.Cast(persistentComponent.m_pData);
			}
		}

		return null;
	}
}
