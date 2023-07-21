class EPF_BasicRespawnSystemComponentClass : SCR_RespawnSystemComponentClass
{
}

class EPF_BasicRespawnSystemComponent : SCR_RespawnSystemComponent
{
	[Attribute(defvalue: "{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et")]
	ResourceName m_rDefaultPrefab;

	protected PlayerManager m_pPlayerManager;

	protected ref map<int, IEntity> m_mLoadingCharacters = new map<int, IEntity>();

	//------------------------------------------------------------------------------------------------
	override void OnPlayerRegistered_S(int playerId)
	{
		//PrintFormat("EPF_BasicRespawnSystemComponent.OnPlayerRegistered_S(%1)", playerId);

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
	override void OnPlayerKilled_S(int playerId, IEntity player, IEntity killer)
	{
		//PrintFormat("EPF_BasicRespawnSystemComponent.OnPlayerKilled_S(%1, %2, %3)", playerId, player, killer);

		// Add the dead body root entity collection so it spawns back after restart for looting
		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(player);
		if (!persistence)
		{
			Print(string.Format("OnPlayerKilled(%1, %2, %3) -> Player killed that does not have persistence component?!? Something went terribly wrong!", playerId, player, killer), LogLevel.ERROR);
			return;
		}

		string newId = persistence.GetPersistentId();

		persistence.SetPersistentId(string.Empty); // Force generation of new id for dead body
		persistence.OverrideSelfSpawn(true);

		// Fresh character spawn (NOTE: We need to push this to next frame due to a bug where on the same death frame we can not hand over a new char).
		GetGame().GetCallqueue().Call(LoadCharacter, playerId, newId, null);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected_S(int playerId, KickCauseCode cause, int timeout)
	{
		//PrintFormat("EPF_BasicRespawnSystemComponent.OnPlayerDisconnected_S(%1, %2, %3)", playerId, typename.EnumToString(KickCauseCode, cause), timeout);

		IEntity character = m_pPlayerManager.GetPlayerController(playerId).GetControlledEntity();
		if (character)
		{
			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(character);
			persistence.PauseTracking();
			persistence.Save();
			// Game will cleanup the char for us because it was controlled by the player.
		}
		else
		{
			// Delete a still loading char that was not handed over to a player.
			IEntity transientCharacter = m_mLoadingCharacters.Get(playerId);
			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(transientCharacter);
			if (persistence)
				persistence.PauseTracking();

			SCR_EntityHelper.DeleteEntityAndChildren(transientCharacter);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDeleted_S(int playerId)
	{
		// Remove base implementation
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForUid(int playerId)
	{
		// Wait one frame after audit/sp join, then it is available.
		// TODO: Remove this method once https://feedback.bistudio.com/T165590 is fixed.
		GetGame().GetCallqueue().Call(OnUidAvailable, playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnUidAvailable(int playerId)
	{
		Tuple2<int, string> characterContext(playerId, EPF_Utils.GetPlayerUID(playerId));

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (persistenceManager.GetState() < EPF_EPersistenceManagerState.ACTIVE)
		{
			// Wait with character load until the persistence system is fully loaded
			EDF_ScriptInvokerCallback callback(this, "RequestCharacterLoad", characterContext);
			persistenceManager.GetOnActiveEvent().Insert(callback.Invoke);
			return;
		}

		RequestCharacterLoad(characterContext);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestCharacterLoad(Managed context)
	{
		// For this example just one character per player uid
		Tuple2<int, string> characterContext = Tuple2<int, string>.Cast(context);
		EDF_DbFindCallbackSingle<EPF_CharacterSaveData> characterDataCallback(this, "OnCharacterDataLoaded", characterContext);
		EPF_PersistenceEntityHelper<EPF_CharacterSaveData>.GetRepository().FindAsync(characterContext.param2, characterDataCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Handles the character data found for the players account
	protected void OnCharacterDataLoaded(EDF_EDbOperationStatusCode statusCode, EPF_CharacterSaveData characterData, Managed context)
	{
		Tuple2<int, string> characterContext = Tuple2<int, string>.Cast(context);
		LoadCharacter(characterContext.param1, characterContext.param2, characterData);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadCharacter(int playerId, string playerUid, EPF_CharacterSaveData saveData)
	{
		IEntity playerEntity;

		if (saveData)
		{
			//PrintFormat("Loading existing character '%1'...", playerUid);

			vector position = saveData.m_pTransformation.m_vOrigin;
			SCR_WorldTools.FindEmptyTerrainPosition(position, position, 2);
			saveData.m_pTransformation.m_vOrigin = position + "0 0.1 0"; // Anti lethal terrain clipping

			playerEntity = saveData.Spawn();

			EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(playerEntity);
			if (persistenceComponent)
			{
				m_mLoadingCharacters.Set(playerId, playerEntity);

				if (EPF_DeferredApplyResult.IsPending(saveData))
				{
					Tuple3<int, EPF_CharacterSaveData, EPF_PersistenceComponent> context(playerId, saveData, persistenceComponent);
					EDF_ScriptInvokerCallback callback(this, "OnCharacterLoadedCallback", context);
					persistenceComponent.GetOnAfterLoadEvent().Insert(callback.Invoke);

					// TODO: Remove hard loading time limit when we know all spawn block bugs are fixed.
					GetGame().GetCallqueue().CallLater(OnCharacterLoaded, 5000, false, playerId, saveData, persistenceComponent);
				}
				else
				{
					OnCharacterLoaded(playerId, saveData, persistenceComponent);
				}

				return;
			}
		}

		EPF_SpawnPoint spawnPoint = EPF_SpawnPoint.GetRandomSpawnPoint();
		if (!spawnPoint)
		{
			Print("Could not spawn character, no spawn point on the map.", LogLevel.ERROR);
			return;
		}

		vector position, ypr;
		spawnPoint.GetPosYPR(position, ypr);
		playerEntity = EPF_Utils.SpawnEntityPrefab(m_rDefaultPrefab, position + "0 0.1 0", ypr);
		m_mLoadingCharacters.Set(playerId, playerEntity);

		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(playerEntity);
		if (persistenceComponent)
		{
			persistenceComponent.SetPersistentId(playerUid);
			HandoverToPlayer(playerId, playerEntity);
		}
		else
		{
			Print(string.Format("Could not create new character, prefab '%1' is missing component '%2'.", m_rDefaultPrefab, EPF_PersistenceComponent), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(playerEntity);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCharacterLoadedCallback(Managed context)
	{
		auto typedContext = Tuple3<int, EPF_CharacterSaveData, EPF_PersistenceComponent>.Cast(context);
		OnCharacterLoaded(typedContext.param1, typedContext.param2, typedContext.param3);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCharacterLoaded(int playerId, EPF_EntitySaveData saveData, EPF_PersistenceComponent persistenceComponent)
	{
		if (!persistenceComponent || !saveData)
			return;

		// We only want to know this once
		persistenceComponent.GetOnAfterLoadEvent().Remove(OnCharacterLoaded);

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
	protected void HandoverToPlayer(int playerId, IEntity character)
	{
		//PrintFormat("HandoverToPlayer(%1, %2)", playerId, character);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_pPlayerManager.GetPlayerController(playerId));
		EDF_ScriptInvokerCallback callback(this, "OnHandoverComplete", new Tuple1<int>(playerId));
		playerController.m_OnControlledEntityChanged.Insert(callback.Invoke);

		playerController.SetPossessedEntity(character);
		playerController.SetControlledEntity(character);

		m_pGameMode.OnPlayerEntityChanged_S(playerId, null, character);

		SCR_RespawnComponent respawn = SCR_RespawnComponent.Cast(playerController.GetRespawnComponent());
		respawn.SGetOnSpawn().Invoke(); // TODO: Check if this is needed, the base game added it as a hack?!?
		respawn.NotifySpawn(character);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnHandoverComplete(Managed context)
	{
		Tuple1<int> typedContext = Tuple1<int>.Cast(context);
		//PrintFormat("OnHandoverComplete(%1)", typedContext.param1);
		m_mLoadingCharacters.Remove(typedContext.param1);
	}

	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		m_pGameMode = SCR_BaseGameMode.Cast(owner);
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_pPlayerManager = GetGame().GetPlayerManager();
		if (!m_pGameMode || !m_pRplComponent || !m_pPlayerManager)
			Debug.Error("SCR_RespawnSystemComponent setup is invalid!");
	}
}
