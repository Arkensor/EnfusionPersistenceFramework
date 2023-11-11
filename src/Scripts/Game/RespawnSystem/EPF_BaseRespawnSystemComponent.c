class EPF_BaseRespawnSystemComponentClass : SCR_RespawnSystemComponentClass
{
}

#ifdef WORKBENCH
enum EPF_EPlayFromCameraHandling
{
	IGNORE,
	USE_POSITION,
	OVERRIDE
}
#endif

class EPF_BaseRespawnSystemComponent : SCR_RespawnSystemComponent
{
	#ifdef WORKBENCH
	[Attribute(defvalue: EPF_EPlayFromCameraHandling.IGNORE.ToString(), uiwidget: UIWidgets.ComboBox, desc: "What should happen in the Workbench when play from camera is chosen?", enums: ParamEnumArray.FromEnum(EPF_EPlayFromCameraHandling))]
	EPF_EPlayFromCameraHandling m_ePlayFromCameraHandling;

	protected bool m_bUseFromCamera;
	protected vector m_vFromCameraPosition;
	protected vector m_vFromCameraYPR;
	#endif

	protected ref map<int, IEntity> m_mLoadingCharacters = new map<int, IEntity>();
	protected PlayerManager m_pPlayerManager;

	//------------------------------------------------------------------------------------------------
	override void OnPlayerRegistered_S(int playerId)
	{
		//PrintFormat("EPF_BaseRespawnSystemComponent.OnPlayerRegistered_S(%1)", playerId);

		if (RplSession.Mode() != RplMode.Dedicated)
		{
			WaitForUid(playerId);
		}
		else
		{
			EDF_ScriptInvokerCallback1<int> callback(this, "WaitForUid");
			m_pGameMode.GetOnPlayerAuditSuccess().Insert(callback.Invoke);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerKilled_S(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		//PrintFormat("EPF_BaseRespawnSystemComponent.OnPlayerKilled_S(%1, %2, %3)", playerId, playerEntity, killerEntity);

		// Add the dead body root entity collection so it spawns back after restart for looting
		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(playerEntity);
		if (!persistence)
		{
			Debug.Error(string.Format("OnPlayerKilled(%1, %2, %3) -> Player killed that does not have persistence component?!? Something went terribly wrong!", playerId, playerEntity, killerEntity));
			return;
		}

		string newId = persistence.GetPersistentId();

		persistence.SetPersistentId(string.Empty); // Force generation of new id for dead body
		persistence.OverrideSelfSpawn(true);

		// Fresh character spawn (NOTE: We need to push this to next frame due to a bug where on the same death frame we can not hand over a new char).
		GetGame().GetCallqueue().Call(CreateCharacter, playerId, newId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected_S(int playerId, KickCauseCode cause, int timeout)
	{
		//PrintFormat("EPF_BaseRespawnSystemComponent.OnPlayerDisconnected_S(%1, %2, %3)", playerId, typename.EnumToString(KickCauseCode, cause), timeout);

		IEntity character = m_pPlayerManager.GetPlayerController(playerId).GetControlledEntity();
		if (character)
		{
			SaveCharacter(playerId, character, false);
			// Game will cleanup the char for us because it was controlled by the player.
		}
		else
		{
			// Delete a still loading char that was not handed over to a player.
			IEntity transientCharacter = m_mLoadingCharacters.Get(playerId);
			if (transientCharacter)
			{
				SaveCharacter(playerId, transientCharacter, true);
				SCR_EntityHelper.DeleteEntityAndChildren(transientCharacter);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDeleted_S(int playerId)
	{
		// Skip base impementation because of hard wired respawn system aspects we do not make us of.
	}

	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void WaitForUid(int playerId)
	{
		// Wait one frame after audit/sp join, then it is available.
		// TODO: Remove this method once https://feedback.bistudio.com/T165590 is fixed.
		GetGame().GetCallqueue().Call(OnUidAvailable, playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnUidAvailable(int playerId)
	{
		Tuple2<int, string> uidContext(playerId, EPF_Utils.GetPlayerUID(playerId));

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (persistenceManager.GetState() < EPF_EPersistenceManagerState.ACTIVE)
		{
			// Wait with character load until the persistence system is fully loaded
			EDF_ScriptInvokerCallback callback(this, "HandlePlayerLoad", uidContext);
			persistenceManager.GetOnActiveEvent().Insert(callback.Invoke);
			return;
		}

		HandlePlayerLoad(uidContext);
	}

	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void HandlePlayerLoad(Managed context)
	{
		// For this example just one character per player uid
		Tuple2<int, string> characterContext = Tuple2<int, string>.Cast(context);

		#ifdef WORKBENCH
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (persistenceComponent && characterContext.param1 == 1) // Only for main workbench player id (1)
		{
			// Assign character persistence id to engine provided char
			persistenceComponent.RevokeBakedFlag();
			persistenceComponent.SetPersistentId(characterContext.param2);

			array<IEntity> children();
			SCR_EntityHelper.GetHierarchyEntityList(character, children);
			foreach (IEntity child : children)
			{
				EPF_PersistenceComponent childPersistence = EPF_Component<EPF_PersistenceComponent>.Find(child);
				if (!childPersistence)
					continue;

				childPersistence.RevokeBakedFlag();
				childPersistence.SetPersistentId(string.Empty); // Force new id
			}

			// Abort loading as we will juse use the engine provided controlled character.
			if (m_ePlayFromCameraHandling == EPF_EPlayFromCameraHandling.OVERRIDE)
				return;

			if (m_ePlayFromCameraHandling == EPF_EPlayFromCameraHandling.USE_POSITION)
			{
				m_bUseFromCamera = true;
				m_vFromCameraPosition = character.GetOrigin();
				m_vFromCameraYPR = character.GetYawPitchRoll();
			}

			//persistenceComponent.PauseTracking();
			SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		#endif

		EDF_DbFindCallbackSingle<EPF_CharacterSaveData> characterDataCallback(this, "OnCharacterDataLoaded", characterContext);
		EPF_PersistenceEntityHelper<EPF_CharacterSaveData>.GetRepository().FindAsync(characterContext.param2, characterDataCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Handles the character data found for the players account
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void OnCharacterDataLoaded(EDF_EDbOperationStatusCode statusCode, EPF_CharacterSaveData characterData, Managed context)
	{
		Tuple2<int, string> characterContext = Tuple2<int, string>.Cast(context);
		if (characterData)
		{
			LoadCharacter(characterContext.param1, characterContext.param2, characterData);
		}
		else
		{
			CreateCharacter(characterContext.param1, characterContext.param2);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadCharacter(int playerId, string characterPersistenceId, EPF_CharacterSaveData saveData)
	{
		//PrintFormat("Loading existing character '%1'...", characterPersistenceId);

		#ifdef WORKBENCH
		if (m_bUseFromCamera)
		{
			saveData.m_pTransformation.m_vOrigin = m_vFromCameraPosition;
			saveData.m_pTransformation.m_vAngles = m_vFromCameraYPR;
		}
		#endif

		vector position = saveData.m_pTransformation.m_vOrigin;
		SCR_WorldTools.FindEmptyTerrainPosition(position, position, 2);
		saveData.m_pTransformation.m_vOrigin = position + "0 0.1 0"; // Anti lethal terrain clipping

		IEntity character = saveData.Spawn();
		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (persistenceComponent)
		{
			m_mLoadingCharacters.Set(playerId, character);

			if (EPF_DeferredApplyResult.IsPending(saveData))
			{
				Tuple3<int, EPF_CharacterSaveData, EPF_PersistenceComponent> context(playerId, saveData, persistenceComponent);
				EDF_ScriptInvokerCallback callback(this, "OnCharacterLoadCompleteCallback", context);
				persistenceComponent.GetOnAfterLoadEvent().Insert(callback.Invoke);

				// TODO: Remove hard loading time limit when we know all spawn block bugs are fixed.
				GetGame().GetCallqueue().CallLater(OnCharacterLoadComplete, 5000, false, playerId, saveData, persistenceComponent);
			}
			else
			{
				OnCharacterLoadComplete(playerId, saveData, persistenceComponent);
			}

			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void OnCharacterLoadCompleteCallback(Managed context)
	{
		auto typedContext = Tuple3<int, EPF_CharacterSaveData, EPF_PersistenceComponent>.Cast(context);
		OnCharacterLoadComplete(typedContext.param1, typedContext.param2, typedContext.param3);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCharacterLoadComplete(int playerId, EPF_EntitySaveData saveData, EPF_PersistenceComponent persistenceComponent)
	{
		if (!persistenceComponent || !saveData)
			return;

		// We only want to know this once
		persistenceComponent.GetOnAfterLoadEvent().Remove(OnCharacterLoadComplete);

		IEntity playerEntity = persistenceComponent.GetOwner();
		if (m_pPlayerManager.GetPlayerControlledEntity(playerId) == playerEntity)
			return; // Player was force taken over after the time limit

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		SCR_CharacterInventoryStorageComponent inventoryStorage = EPF_Component<SCR_CharacterInventoryStorageComponent>.Find(playerEntity);
		if (inventoryStorage)
		{
			EPF_CharacterInventoryStorageComponentSaveData charInventorySaveData = EPF_ComponentSaveDataGetter<EPF_CharacterInventoryStorageComponentSaveData>.GetFirst(saveData);
			if (charInventorySaveData && charInventorySaveData.m_aQuickSlotEntities)
			{
				array<RplId> quickBarRplIds();
				// Init with invalid ids
				int nQuickslots = inventoryStorage.GetQuickSlotItems().Count();
				quickBarRplIds.Reserve(nQuickslots);
				for (int i = 0; i < nQuickslots; i++)
				{
					quickBarRplIds.Insert(RplId.Invalid());
				}

				foreach (EPF_PersistentQuickSlotItem quickSlot : charInventorySaveData.m_aQuickSlotEntities)
				{
					IEntity slotEntity = persistenceManager.FindEntityByPersistentId(quickSlot.m_sEntityId);
					if (slotEntity && quickSlot.m_iIndex < quickBarRplIds.Count())
					{
						quickBarRplIds.Set(quickSlot.m_iIndex, EPF_NetworkUtils.GetRplId(slotEntity));
					}
				}

				// Apply quick item slots serverside to avoid initial sync back from client with same data
				inventoryStorage.EPF_Rpc_UpdateQuickSlotItems(quickBarRplIds);

				// Send quickbar to client on next frame when ownership was handed over
				GetGame().GetCallqueue().Call(inventoryStorage.EPF_SetQuickBarItems, quickBarRplIds);
			}
		}

		HandoverToPlayer(playerId, playerEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateCharacter(int playerId, string characterPersistenceId)
	{
		ResourceName prefab = GetCreationPrefab(playerId, characterPersistenceId);

		vector position, yawPitchRoll;
		GetCreationPosition(playerId, characterPersistenceId, position, yawPitchRoll);

		#ifdef WORKBENCH
		if (m_bUseFromCamera)
		{
			position = m_vFromCameraPosition;
			yawPitchRoll = m_vFromCameraYPR;
		}
		#endif

		IEntity character = EPF_Utils.SpawnEntityPrefab(prefab, position + "0 0.1 0", yawPitchRoll);
		m_mLoadingCharacters.Set(playerId, character);

		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (persistenceComponent)
		{
			persistenceComponent.SetPersistentId(characterPersistenceId);
			OnCharacterCreated(playerId, characterPersistenceId, character);
			HandoverToPlayer(playerId, character);
		}
		else
		{
			Print(string.Format("Could not create new character, prefab '%1' is missing component '%2'.", prefab, EPF_PersistenceComponent), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Post process a newly created character before its handed over to the player.
	protected void OnCharacterCreated(int playerId, string characterPersistenceId, IEntity character);

	//------------------------------------------------------------------------------------------------
	//! Decide position and angles where a new character is spawned
	protected void GetCreationPosition(int playerId, string characterPersistenceId, out vector position, out vector yawPitchRoll);

	//------------------------------------------------------------------------------------------------
	//! Prefab for a newly created chracter
	protected ResourceName GetCreationPrefab(int playerId, string characterPersistenceId)
	{
		return "{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et";
	}

	//------------------------------------------------------------------------------------------------
	//! Hand over network ownership and control of a spawned or loaded character to the player
	protected void HandoverToPlayer(int playerId, IEntity character)
	{
		//PrintFormat("HandoverToPlayer(%1, %2)", playerId, character);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_pPlayerManager.GetPlayerController(playerId));
		EDF_ScriptInvokerCallback2<IEntity, IEntity> callback(this, "OnHandoverComplete", new Tuple1<int>(playerId));
		playerController.m_OnControlledEntityChanged.Insert(callback.Invoke);

		playerController.SetInitialMainEntity(character);

		m_pGameMode.OnPlayerEntityChanged_S(playerId, null, character);

		SCR_RespawnComponent respawn = SCR_RespawnComponent.Cast(playerController.GetRespawnComponent());
		respawn.NotifySpawn(character);
	}

	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void OnHandoverComplete(IEntity from, IEntity to, Managed context)
	{
		Tuple1<int> typedContext = Tuple1<int>.Cast(context);
		//PrintFormat("OnHandoverComplete(%1, %2, %3)", from, to, typedContext.param1);
		m_mLoadingCharacters.Remove(typedContext.param1);
	}

	//------------------------------------------------------------------------------------------------
	//! \param isTransient Marks character entities that were part of an aborted player join process.
	protected void SaveCharacter(int playerId, IEntity character, bool isTransient)
	{
		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (!persistence)
			return;

		persistence.PauseTracking();

		if (!isTransient)
			persistence.Save(); // Transient chars should not have changes, since no handover
	}

	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		m_pGameMode = SCR_BaseGameMode.Cast(owner);
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_pPlayerManager = GetGame().GetPlayerManager();

		if (!m_pGameMode || !m_pRplComponent || !m_pPlayerManager)
			Debug.Error("SCR_RespawnSystemComponent setup is invalid!");

		// Skip base impementation because of hard wired respawn system aspects we do not make us of.
	}
}
