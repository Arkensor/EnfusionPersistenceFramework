[ComponentEditorProps(category: "Persistence", description: "Used to make an entity persistent.")]
/*sealed*/ class EPF_PersistenceComponentClass : ScriptComponentClass
{
	[Attribute(defvalue: EPF_ESaveType.INTERVAL_SHUTDOWN.ToString(), uiwidget: UIWidgets.ComboBox, desc: "Should the entity be saved automatically and if so only on shutdown or regulary.\nThe interval is configured in the persitence manager component on your game mode.", enums: ParamEnumArray.FromEnum(EPF_ESaveType))]
	EPF_ESaveType m_eSaveType;

	[Attribute(defvalue: "0", desc: "If enabled a copy of the last save-data is kept to compare against, so the databse is updated only if there are any differences to what is already persisted.\nHelps to reduce expensive database calls at the cost of additional base line memeory allocation.")]
	bool m_bUseChangeTracker;

	[Attribute(defvalue: "1", desc: "If enabled the entity will spawn back into the world automatically after session restart.\nAlways true for baked map objects.")]
	bool m_bSelfSpawn;

	[Attribute(defvalue: "1", desc: "If enabled the entity will deleted from persistence when deleted from the world automatically.")]
	bool m_bSelfDelete;

	[Attribute(defvalue: "0", desc: "Update the navmesh on being loaded back into the world. Only makes sense for prefabs that can affect the navmesh - e.g. houses.")]
	bool m_bUpdateNavmesh;

	[Attribute(defvalue: "1", desc: "Only storage root entities can be saved in the open world.\nIf disabled the entity will only be saved if inside another storage root (e.g. character, vehicle).")]
	bool m_bStorageRoot;

	[Attribute(desc: "Type of save-data to represent this entity.")]
	ref EPF_EntitySaveDataClass m_pSaveData;

	// Derived from shared initialization
	typename m_tSaveDataType;

	//------------------------------------------------------------------------------------------------
	static override array<typename> CannotCombine(IEntityComponentSource src)
	{
		return {EPF_PersistenceComponent}; //Prevent multiple persistence components from being added.
	}
}

/*sealed*/ class EPF_PersistenceComponent : ScriptComponent
{
	private string m_sId;

	private int m_iLastSaved;

	[NonSerialized()]
	private EPF_EPersistenceFlags m_eFlags;

	[NonSerialized()]
	private ref ScriptInvoker<EPF_PersistenceComponent, EPF_EntitySaveData> m_pOnAfterSave;

	[NonSerialized()]
	private ref ScriptInvoker<EPF_PersistenceComponent, EPF_EntitySaveData> m_pOnAfterPersist;

	[NonSerialized()]
	private ref ScriptInvoker<EPF_PersistenceComponent, EPF_EntitySaveData> m_pOnBeforeLoad;

	[NonSerialized()]
	private ref ScriptInvoker<EPF_PersistenceComponent, EPF_EntitySaveData> m_pOnAfterLoad;

	[NonSerialized()]
	private static ref map<EPF_PersistenceComponent, ref EPF_EntitySaveData> m_mLastSaveData;

	//------------------------------------------------------------------------------------------------
	//! static helper see GetPersistentId()
	static string GetPersistentId(IEntity entity)
	{
		if (!entity) return string.Empty;
		EPF_PersistenceComponent persistence = EPF_PersistenceComponent.Cast(entity.FindComponent(EPF_PersistenceComponent));
		if (!persistence) return string.Empty;
		return persistence.GetPersistentId();
	}

	//------------------------------------------------------------------------------------------------
	//! Get the assigned persistent id of this entity.
	string GetPersistentId()
	{
		if (!m_sId) m_sId = EPF_PersistenceManager.GetInstance().Register(this);
		return m_sId;
	}

	//------------------------------------------------------------------------------------------------
	//! Set the assigned persistent id of this entity.
	//! USE WITH CAUTION! Only in rare situations you need to manually assign an id.
	void SetPersistentId(string id)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (m_sId && m_sId != id)
		{
			persistenceManager.Unregister(this);
			m_sId = string.Empty;
		}
		if (!m_sId) m_sId = persistenceManager.Register(this, id);
	}

	//------------------------------------------------------------------------------------------------
	//! Get the last time this entity was saved as UTC unix timestamp
	int GetLastSaved()
	{
		return m_iLastSaved;
	}

	//------------------------------------------------------------------------------------------------
	//! Get internal state flags of the persistence tracking
	EPF_EPersistenceFlags GetFlags()
	{
		return m_eFlags;
	}

	//------------------------------------------------------------------------------------------------
	//! Event invoker for when the save-data was read but was not yet persisted to the database.
	//! Args(EPF_PersistenceComponent, EPF_EntitySaveData)
	ScriptInvoker GetOnAfterSaveEvent()
	{
		if (!m_pOnAfterSave) m_pOnAfterSave = new ScriptInvoker();
		return m_pOnAfterSave;
	}

	//------------------------------------------------------------------------------------------------
	//! Event invoker for when the save-data was persisted to the database.
	//! Only called on world root entities (e.g. not on items stored inside other items, there it will only be called for the container).
	//! Args(EPF_PersistenceComponent, EPF_EntitySaveData)
	ScriptInvoker GetOnAfterPersistEvent()
	{
		if (!m_pOnAfterPersist) m_pOnAfterPersist = new ScriptInvoker();
		return m_pOnAfterPersist;
	}

	//------------------------------------------------------------------------------------------------
	//! Event invoker for when the save-data is about to be loaded/applied to the entity.
	//! Args(EPF_PersistenceComponent, EPF_EntitySaveData)
	ScriptInvoker GetOnBeforeLoadEvent()
	{
		if (!m_pOnBeforeLoad) m_pOnBeforeLoad = new ScriptInvoker();
		return m_pOnBeforeLoad;
	}

	//------------------------------------------------------------------------------------------------
	//! Event invoker for when the save-data was loaded/applied to the entity.
	//! Args(EPF_PersistenceComponent, EPF_EntitySaveData)
	ScriptInvoker GetOnAfterLoadEvent()
	{
		if (!m_pOnAfterLoad) m_pOnAfterLoad = new ScriptInvoker();
		return m_pOnAfterLoad;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause automated tracking for auto-/shutdown-save, root entity changes and removal.
	//! Used primarily to handle the conditional removal of an entity manually. E.g. pause before virtually storing a vehicle in a garage (during which the entity gets deleted).
	void PauseTracking()
	{
		EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.PAUSE_TRACKING);
	}

	//------------------------------------------------------------------------------------------------
	//! Undo PauseTracking().
	void ResumeTracking()
	{
		EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.PAUSE_TRACKING);
	}

	//------------------------------------------------------------------------------------------------
	//! Save the entity to the database
	//! \return the save-data instance that was submitted to the database
	EPF_EntitySaveData Save(out EPF_EReadResult readResult = EPF_EReadResult.ERROR)
	{
		GetPersistentId(); // Make sure the id has been assigned

		m_iLastSaved = System.GetUnixTime();

		IEntity owner = GetOwner();
		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		EPF_EntitySaveData saveData = EPF_EntitySaveData.Cast(settings.m_tSaveDataType.Spawn());
		
		if (saveData)
			readResult = saveData.ReadFrom(owner, settings.m_pSaveData);

		if (!readResult)
		{
			Debug.Error(string.Format("Failed to persist world entity '%1'@%2. Save-data could not be read.",
				EPF_Utils.GetPrefabName(owner),
				owner.GetOrigin()));
			return null;
		}

		if (m_pOnAfterSave)
			m_pOnAfterSave.Invoke(this, saveData);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		bool isPersistent = EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD);

		// Save root entities unless they are baked AND only have default values,
		// cause then we do not need the record to restore - as the ids will be
		// known through the name mapping table instead.
		bool wasPersisted;
		if (EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.ROOT) &&
			(!EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.BAKED) || readResult == EPF_EReadResult.OK))
		{
			// Check if the update is really needed
			EPF_EntitySaveData lastData;
			if (settings.m_bUseChangeTracker)
				lastData = m_mLastSaveData.Get(this);

			if (!isPersistent || !lastData || !lastData.Equals(saveData))
			{
				persistenceManager.AddOrUpdateAsync(saveData);
				EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD);
				wasPersisted = true;
			}
		}
		else if (isPersistent)
		{
			// Was previously saved as storage root but now is not anymore, so the toplevel db entry has to be deleted.
			// The save-data will be present inside the storage parent instead.
			// Remove immediately so that on e.g. a web driver there is no delete call coming for an id that was previously saved as child
			persistenceManager.RemoveAsync(settings.m_tSaveDataType, m_sId);
			EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD);
		}

		if (settings.m_bUseChangeTracker)
			m_mLastSaveData.Set(this, saveData);

		if (m_pOnAfterPersist && wasPersisted)
			m_pOnAfterPersist.Invoke(this, saveData);

		return saveData;
	}

	//------------------------------------------------------------------------------------------------
	//! Load existing save-data to apply to this entity
	//! \param saveData existing data to restore the entity state from
	//! \param isRoot true if the current entity is a world root (not a stored item inside a storage)
	bool Load(notnull EPF_EntitySaveData saveData, bool isRoot = true)
	{
		if (m_pOnBeforeLoad)
			m_pOnBeforeLoad.Invoke(this, saveData);

		if (isRoot)
		{
			// Restore information if this entity has its own root record in db to avoid unreferenced junk entries.
			EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD);
		}
		else
		{
			// Supress registration as root to reduce map interactions in manager when entity is added as child next call anyway.
			EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.ROOT);
		}

		// Restore transform info relevant for baked entity record trimming
		if (EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.BAKED) &&
			saveData.m_pTransformation &&
			!saveData.m_pTransformation.IsDefault())
		{
			FlagAsMoved();
		}

		SetPersistentId(saveData.GetId());

		IEntity owner = GetOwner();
		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		EPF_EApplyResult applyResult = saveData.ApplyTo(owner, settings.m_pSaveData);
		if (applyResult == EPF_EApplyResult.ERROR)
		{
			Debug.Error(string.Format("Failed to apply save-data '%1:%2' to entity.", saveData.Type().ToString(), saveData.GetId()));
			return false;
		}

		if (settings.m_bUseChangeTracker)
			m_mLastSaveData.Set(this, saveData);

		if (applyResult == EPF_EApplyResult.AWAIT_COMPLETION)
		{
			EPF_DeferredApplyResult.GetOnApplied(saveData).Insert(DeferredApplyCallback)
		}
		else
		{
			if (settings.m_bUpdateNavmesh)
				UpdateNavesh();

			if (m_pOnAfterLoad)
				m_pOnAfterLoad.Invoke(this, saveData);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Mark the persistence data of this entity for deletion. Does not delete the entity itself.
	void Delete()
	{
		// Only attempt to delete if there is a chance it was already saved as own entity in db
		if (m_sId && EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD))
		{
			EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.PERSISTENT_RECORD);
			EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
			EPF_PersistenceComponentClass settings = EPF_ComponentData<EPF_PersistenceComponentClass>.Get(GetOwner());
			persistenceManager.RemoveAsync(settings.m_tSaveDataType, m_sId);
		}

		m_sId = string.Empty;
		m_iLastSaved = 0;
	}

	//------------------------------------------------------------------------------------------------
	override protected event void OnPostInit(IEntity owner)
	{
		// Persistence logic only runs on the server
		if (!EPF_PersistenceManager.IsPersistenceMaster())
			return;
		
		// Avoid tracking anything in preload/preview world
		if (!ChimeraWorld.CastFrom(owner.GetWorld()) || (GetGame().GetWorld() != owner.GetWorld()))
			return;

		// Init and validate settings on shared class-class instance once
		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (!settings.m_tSaveDataType)
		{
			if (!settings.m_pSaveData || settings.m_pSaveData.Type() == EPF_EntitySaveDataClass)
			{
				Debug.Error(string.Format("Missing or invalid save-data type in persistence component on entity '%1'@%2. Entity will not be persisted!",
					EPF_Utils.GetPrefabName(owner),
					owner.GetOrigin()));
				return;
			}

			settings.m_tSaveDataType = EPF_Utils.TrimEnd(settings.m_pSaveData.ClassName(), 5).ToType();

			// Collect and validate component save data types
			array<typename> componentSaveDataTypes();
			componentSaveDataTypes.Reserve(settings.m_pSaveData.m_aComponents.Count());
			foreach (EPF_ComponentSaveDataClass componentSaveData : settings.m_pSaveData.m_aComponents)
			{
				if (!componentSaveData.m_bEnabled)
					continue;

				typename componentSaveDataType = componentSaveData.Type();

				if (!componentSaveDataType || componentSaveDataType == EPF_ComponentSaveDataClass)
				{
					Debug.Error(string.Format("Invalid save-data type '%1' in persistence component on entity '%2'@%3. Associated component data will not be persisted!",
						componentSaveDataType,
						EPF_Utils.GetPrefabName(owner),
						owner.GetOrigin()));
					continue;
				}

				componentSaveDataTypes.Insert(componentSaveDataType);
			}

			// Re-order save data class-classes in attribute instance by inheritance
			array<ref EPF_ComponentSaveDataClass> sortedComponents();
			sortedComponents.Reserve(settings.m_pSaveData.m_aComponents.Count());
			foreach (typename componentType : EPF_Utils.SortTypenameHierarchy(componentSaveDataTypes))
			{
				foreach (EPF_ComponentSaveDataClass componentSaveData : settings.m_pSaveData.m_aComponents)
				{
					if (componentSaveData.Type() == componentType) sortedComponents.Insert(componentSaveData);
				}
			}
			settings.m_pSaveData.m_aComponents = sortedComponents;

			if (settings.m_bUseChangeTracker && !m_mLastSaveData)
				m_mLastSaveData = new map<EPF_PersistenceComponent, ref EPF_EntitySaveData>();
		}

		if (settings.m_bStorageRoot)
			EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.ROOT);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (persistenceManager.GetState() < EPF_EPersistenceManagerState.SETUP)
			EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.BAKED);

		persistenceManager.EnqueueRegistration(this);

		bool isChar = ChimeraCharacter.Cast(owner);
		bool isTurret = EPF_Component<TurretControllerComponent>.Find(owner);
		if (isChar || isTurret)
		{
			// Weapon/Turret selected flag
			EventHandlerManagerComponent eventHandler = EPF_Component<EventHandlerManagerComponent>.Find(owner);
			if (eventHandler)
				eventHandler.RegisterScriptHandler("OnWeaponChanged", this, OnWeaponChanged, delayed: false);
		}
		if (isChar)
		{
			// Gadget selected flag
			SCR_CharacterControllerComponent characterController = EPF_Component<SCR_CharacterControllerComponent>.Find(owner);
			characterController.m_OnGadgetStateChangedInvoker.Insert(OnGadgetStateChanged);
		}

		// For vehicles we want to get notified when they encounter their first contact or start to be driven
		if (settings.m_pSaveData.m_bTrimDefaults && (owner.FindComponent(VehicleControllerComponent) || owner.FindComponent(VehicleControllerComponent_SA)))
		{
			SetEventMask(owner, EntityEvent.CONTACT);
			EventHandlerManagerComponent ev = EPF_Component<EventHandlerManagerComponent>.Find(owner);
			if (ev) ev.RegisterScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
		}

		// TODO remove after https://feedback.bistudio.com/T172461 has been fixed
		IEntity parent = owner.GetParent();
		if (parent)
			OnAddedToParent(owner, parent);
	}

	//------------------------------------------------------------------------------------------------
	override event protected void OnAddedToParent(IEntity child, IEntity parent)
	{
		// Todo: On 0.9.9 use this event to subscribe to entity slots and and inventory item invoker, remove postinit + eoninit workaround
		if (!EPF_PersistenceManager.IsPersistenceMaster())
			return;

		// TODO: Replace with subscribe to all parent slots after https://feedback.bistudio.com/T171945 is added.

		// Delay by a frame so we can know the actual slots they are in
		if (parent.FindComponent(SlotManagerComponent) || parent.FindComponent(BaseSlotComponent))
			GetGame().GetCallqueue().Call(StupidHackUntil099PleaseEndMySuffering, child, parent);

		InventoryItemComponent invItem = EPF_Component<InventoryItemComponent>.Find(child);
		if (invItem)
			invItem.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
	}

	//------------------------------------------------------------------------------------------------
	//! TODO: Refactor this stupid system of parent detection in 0.9.9
	protected void StupidHackUntil099PleaseEndMySuffering(IEntity child, IEntity parent)
	{
		if (!parent)
			return; // Maybe parent got deleted by the time this invokes on next frame

		array<Managed> outComponents();
		parent.FindComponents(SlotManagerComponent, outComponents);
		foreach (Managed componentRef : outComponents)
		{
			SlotManagerComponent slotManager = SlotManagerComponent.Cast(componentRef);
			array<EntitySlotInfo> outSlotInfos();
			slotManager.GetSlotInfos(outSlotInfos);
			foreach (EntitySlotInfo slotInfo : outSlotInfos)
			{
				if (child == slotInfo.GetAttachedEntity())
				{
					OnParentAdded(slotInfo);
					return;
				}
			}
		}
		parent.FindComponents(BaseSlotComponent, outComponents);
		foreach (Managed componentRef : outComponents)
		{
			BaseSlotComponent baseSlot = BaseSlotComponent.Cast(componentRef);
			if (child == baseSlot.GetAttachedEntity())
			{
				OnParentAdded(baseSlot.GetSlotInfo());
				return;
			}
		}
		/*
		// For weapons rely on EquipedWeaponStorageComponent
		parent.FindComponents(WeaponSlotComponent, outComponents);
		foreach (Managed componentRef : outComponents)
		{
			WeaponSlotComponent weaponSlot = WeaponSlotComponent.Cast(componentRef);
			if (child == weaponSlot.GetWeaponEntity())
			{
				OnParentAdded(null); // Don't have that info here.
				return;
			}
		}
		*/
	}

	//------------------------------------------------------------------------------------------------
	protected void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		if (oldSlot)
			OnParentRemoved(oldSlot);

		if (newSlot)
			OnParentAdded(newSlot);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnParentAdded(EntitySlotInfo newSlot)
	{
		// TODO: Remove this hack in 0.9.9, only needed because we manually trigger OnAddedToParent from OnPostInit
		if (!EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.ROOT))
			return;

		IEntity owner = GetOwner();
		IEntity parent = newSlot.GetOwner();

		EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.ROOT);
		FlagAsMoved();

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		InventoryStorageSlot newInvSlot = InventoryStorageSlot.Cast(newSlot);
		if (newInvSlot)
		{
			BaseInventoryStorageComponent storage = newInvSlot.GetStorage();
			if (storage)
			{
				if (EPF_Utils.IsInstanceAnyInherited(storage, {EquipedLoadoutStorageComponent, BaseEquipmentStorageComponent, BaseEquipedWeaponStorageComponent}))
					EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.WAS_EQUIPPED);

				EPF_PersistenceComponent parentPersistence = EPF_Component<EPF_PersistenceComponent>.Find(parent);
				if (persistenceManager.GetState() == EPF_EPersistenceManagerState.ACTIVE &&
					parentPersistence && EPF_BitFlags.CheckFlags(parentPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED))
				{
					EPF_BakedStorageChange.OnAdded(this, newInvSlot);
				}
			}
		}

		if (m_sId && !EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.PAUSE_TRACKING))
			persistenceManager.UpdateRootStatus(this, m_sId, EPF_ComponentData<EPF_PersistenceComponentClass>.Get(owner), false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnParentRemoved(EntitySlotInfo oldSlot)
	{
		// TODO: For 0.9.9 entity slot detach event subscribe should fire this removed event too.
		IEntity owner = GetOwner();

		if (!EPF_PersistenceManager.IsPersistenceMaster() ||
			EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.ROOT)) //Must have been non root, avoid event for remove from stuff like generic organizer layer entity
			return;

		EPF_PersistenceComponentClass settings = EPF_ComponentData<EPF_PersistenceComponentClass>.Get(owner);
		if (!settings.m_bStorageRoot)
			return;

		EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.ROOT);
		FlagAsMoved();

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		InventoryStorageSlot oldInvSlot = InventoryStorageSlot.Cast(oldSlot);
		if (oldInvSlot && persistenceManager.GetState() == EPF_EPersistenceManagerState.ACTIVE)
		{
			// Some inv slots have no storages sometimes e.g. clothing attachments, so ignore those.
			BaseInventoryStorageComponent storage = oldInvSlot.GetStorage();
			if (storage)
			{
				EPF_PersistenceComponent parentPersistence = EPF_Component<EPF_PersistenceComponent>.Find(oldInvSlot.GetOwner());
				if (parentPersistence && EPF_BitFlags.CheckFlags(parentPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED))
				{
					EPF_BakedStorageChange.OnRemoved(this, oldInvSlot);
				}
				else
				{
					EPF_StorageChangeDetection.SetDirty(storage);
				}
			}
		}

		if (m_sId && !EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.PAUSE_TRACKING))
			persistenceManager.UpdateRootStatus(this, m_sId, settings, true);

		// We only needed to know which slot we got attached to, so unsubscribe again
		InventoryItemComponent invItem = EPF_Component<InventoryItemComponent>.Find(owner);
		if (invItem)
			invItem.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);

		// TODO: Also unsubscribe any entity slots
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponChanged(BaseWeaponComponent weapon, BaseWeaponComponent prevWeapon)
	{
		if (!weapon)
			return;

		IEntity weaponEntity = weapon.GetOwner();
		WeaponSlotComponent weaponSlot = WeaponSlotComponent.Cast(weapon);
		if (weaponSlot)
			weaponEntity = weaponSlot.GetWeaponEntity();

		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(weaponEntity);
		if (persistence)
			persistence.FlagAsSelected();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetStateChanged(IEntity gadget, bool isInHand, bool isOnGround)
	{
		if (!gadget || !isInHand)
			return;

		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(gadget);
		if (persistence)
			persistence.FlagAsSelected();
	}

	//------------------------------------------------------------------------------------------------
	protected event void OnCompartmentEntered(IEntity vehicle, BaseCompartmentManagerComponent mgr, IEntity occupant, int managerId, int slotID)
	{
		// Somebody entered the vehicle to potentially drive it, start listening to physics movements
		EventHandlerManagerComponent eventHandler = EPF_Component<EventHandlerManagerComponent>.Find(vehicle);
		if (eventHandler)
			eventHandler.RemoveScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);

		SetEventMask(vehicle, EntityEvent.PHYSICSMOVE);
	}

	//------------------------------------------------------------------------------------------------
	override event protected void EOnContact(IEntity owner, IEntity other, Contact contact)
	{
		if (!GenericTerrainEntity.Cast(other))
			FlagAsMoved(); // Something moved the current vehicle via physics contact.
	}

	//------------------------------------------------------------------------------------------------
	override event protected void EOnPhysicsMove(IEntity owner)
	{
		// Check for if engine is one as there is tiny jitter movement during engine startup we want to ignore.
		VehicleControllerComponent_SA vehicleController_SA = EPF_Component<VehicleControllerComponent_SA>.Find(owner);
		if (vehicleController_SA)
		{
			if (vehicleController_SA.IsEngineOn())
				FlagAsMoved();
		}
		else
		{
			VehicleControllerComponent vehicleController = EPF_Component<VehicleControllerComponent>.Find(owner);
			if (vehicleController && vehicleController.IsEngineOn())
				FlagAsMoved();
		}
	}

	//------------------------------------------------------------------------------------------------
	override event protected void OnDelete(IEntity owner)
	{
		if (m_mLastSaveData)
			m_mLastSaveData.Remove(this);

		// Clean up storages
		EPF_StorageChangeDetection.Cleanup(owner);

		// Check that we are not in session dtor phase
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance(false);
		if (!persistenceManager || persistenceManager.GetState() == EPF_EPersistenceManagerState.SHUTDOWN)
			return;

		persistenceManager.Unregister(this);

		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (m_sId && !EPF_BitFlags.CheckFlags(m_eFlags, EPF_EPersistenceFlags.PAUSE_TRACKING))
		{
			persistenceManager.UpdateRootEntityCollection(this, m_sId, false);
			if (settings.m_bSelfDelete)
				Delete();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Get the instance of the save-data or component save-data class-class that was configured in the prefab/world editor.
	//! \param saveDataClassType typename of the entity or component save-data which should be returned
	//! \return instance of the save-data or null if not found
	Managed GetAttributeClass(typename saveDataClassType)
	{
		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(GetOwner()));
		if (!settings.m_pSaveData)
			return null;

		if (settings.m_pSaveData.IsInherited(saveDataClassType))
		{
			return settings.m_pSaveData;
		}
		else
		{
			// Find the first inheritance match. The typename array itereated is ordered by inheritance.
			foreach (EPF_ComponentSaveDataClass componentSaveDataClass : settings.m_pSaveData.m_aComponents)
			{
				if (componentSaveDataClass.IsInherited(saveDataClassType)) return componentSaveDataClass;
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Manually change the entity self-respawn behavior, ignoring the prefab configuration.
	//! \param selfSpawn True for self spawn, false for do not self spawn.
	void OverrideSelfSpawn(bool selfSpawn)
	{
		EPF_PersistenceManager.GetInstance().OverrideSelfSpawn(this, selfSpawn);
	}

	//------------------------------------------------------------------------------------------------
	void FlagAsSelected()
	{
		EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.WAS_SELECTED);
	}

	//------------------------------------------------------------------------------------------------
	void FlagAsBaked()
	{
		EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.BAKED);
	}

	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	void RevokeBakedFlag()
	{
		EPF_BitFlags.ClearFlags(m_eFlags, EPF_EPersistenceFlags.BAKED);
	}
	#endif

	//------------------------------------------------------------------------------------------------
	void FlagAsMoved()
	{
		StopMoveTracking();
		EPF_BitFlags.SetFlags(m_eFlags, EPF_EPersistenceFlags.WAS_MOVED);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopMoveTracking()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		ClearEventMask(owner, EntityEvent.CONTACT | EntityEvent.PHYSICSMOVE);
		EventHandlerManagerComponent eventHandler = EPF_Component<EventHandlerManagerComponent>.Find(owner);
		if (eventHandler)
			eventHandler.RemoveScriptHandler("OnCompartmentEntered", this, OnCompartmentEntered);
	}

	//------------------------------------------------------------------------------------------------
	protected void DeferredApplyCallback(EPF_EntitySaveData saveData)
	{
		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(GetOwner()));
		if (settings.m_bUpdateNavmesh)
			UpdateNavesh();

		if (m_pOnAfterLoad)
			m_pOnAfterLoad.Invoke(this, saveData);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateNavesh()
	{
		SCR_AIWorld aiworld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (aiworld)
			aiworld.RequestNavmeshRebuildEntity(GetOwner());
	}

	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override event void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);

		if (owner.GetName().StartsWith("EPF_ForceEmptyForTesting"))
		{
			owner.SetName("");
			return;
		}

		if (owner.GetName())
			return;

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor || worldEditor.IsPrefabEditMode())
			return;

		WorldEditorAPI worldEditorApi = GenericEntity.Cast(owner)._WB_GetEditorAPI();
		if (!worldEditorApi)
				return;

		worldEditorApi.RenameEntity(worldEditorApi.EntityToSource(owner), GenerateName(owner));
	}

	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems(IEntity owner)
	{
		array<ref WB_UIMenuItem> items();

		if (EPF_BaseSceneNameProxyEntity.GetProxyForBaseSceneEntity(owner))
		{
			items.Insert(new WB_UIMenuItem("Go to base scene name proxy", 0));
		}
		else if (!owner.GetName())
		{
			items.Insert(new WB_UIMenuItem("Spawn new base scene name proxy", 1));

			if (EPF_BaseSceneNameProxyEntity.s_pSelectedProxy)
				items.Insert(new WB_UIMenuItem(string.Format("Assign proxy '%1'", EPF_BaseSceneNameProxyEntity.s_pSelectedProxy.GetName()), 2));
		}

		return items;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_OnContextMenu(IEntity owner, int id)
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor || worldEditor.IsPrefabEditMode())
			return;

		WorldEditorAPI worldEditorApi = GenericEntity.Cast(owner)._WB_GetEditorAPI();
		if (!worldEditorApi)
				return;

		IEntitySource nameProxySource;

		if (id == 0)
		{
			auto nameProxy = EPF_BaseSceneNameProxyEntity.GetProxyForBaseSceneEntity(owner);
			nameProxySource = worldEditorApi.EntityToSource(nameProxy);
		}
		else if (id == 2)
		{
			worldEditorApi.BeginEntityAction("BaseSceneNameProxyEntity__Assign");
			nameProxySource = worldEditorApi.EntityToSource(EPF_BaseSceneNameProxyEntity.s_pSelectedProxy);
			EPF_BaseSceneNameProxyEntity.s_pSelectedProxy = null;
			worldEditorApi.SetVariableValue(nameProxySource, null, "coords", owner.GetOrigin().ToString(false));
			worldEditorApi.EndEntityAction();
		}
		else
		{
			worldEditorApi.BeginEntityAction("BaseSceneNameProxyEntity__Create");
			nameProxySource = worldEditorApi.CreateEntity(
				"EPF_BaseSceneNameProxyEntity",
				GenerateName(owner),
				worldEditorApi.GetCurrentEntityLayerId(),
				null,
				owner.GetOrigin(),
				owner.GetAngles());

			worldEditorApi.SetVariableValue(nameProxySource, null, "m_rTarget", EPF_Utils.GetPrefabName(owner));
			worldEditorApi.EndEntityAction();
		}

		worldEditorApi.SetEntitySelection(nameProxySource);
		worldEditorApi.UpdateSelectionGui();
	}

	//------------------------------------------------------------------------------------------------
	protected string GenerateName(IEntity owner)
	{
		string prefabNameOnly = FilePath.StripExtension(FilePath.StripPath(EPF_Utils.GetPrefabName(owner)));
		if (!prefabNameOnly)
			prefabNameOnly = owner.ClassName();

		return string.Format("%1_%2", prefabNameOnly, Workbench.GenerateGloballyUniqueID64());
	}
	#endif
}
