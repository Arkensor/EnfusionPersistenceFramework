[ComponentEditorProps(category: "Persistence/Testing", description: "Part of the re-spawn system on the gamemode.")]
class EPF_RespawnHandlerComponentClass : SCR_RespawnHandlerComponentClass
{
};

class EPF_RespawnHandlerComponent : SCR_RespawnHandlerComponent
{
	protected EPF_RespawnSytemComponent m_pRespawnSystem;
	protected PlayerManager m_pPlayerManager;

	//------------------------------------------------------------------------------------------------
	override void OnPlayerRegistered(int playerId)
	{
		// On dedicated servers we need to wait for OnPlayerAuditSuccess instead for the real UID to be available
		if (m_pGameMode.IsMaster() && (RplSession.Mode() != RplMode.Dedicated))
			OnUidAvailable(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerAuditSuccess(int playerId)
	{
		if (m_pGameMode.IsMaster())
			OnUidAvailable(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnUidAvailable(int playerId)
	{
		string playerUid = EPF_Utils.GetPlayerUID(playerId);
		if (!playerUid)
			return;

		Tuple2<int, string> characterContext(playerId, playerUid);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (persistenceManager.GetState() < EPF_EPersistenceManagerState.ACTIVE)
		{
			// Wait with character load until the persistence system is fully loaded
			EDF_ScriptInvokerCallback callback(this, "RequestCharacterLoad", characterContext);
			persistenceManager.GetOnActiveEvent().Insert(callback.Invoke);
			return;
		}

		RequestCharacterLoad(characterContext)
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

		if (characterData)
			PrintFormat("Loaded existing character '%1'.", characterContext.param2);

		// Prepare spawn data buffer with last known player data (null for fresh accounts) and queue player for spawn
		m_pRespawnSystem.PrepareCharacter(characterContext.param1, characterContext.param2, characterData);
		m_sEnqueuedPlayers.Insert(characterContext.param1);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		m_sEnqueuedPlayers.RemoveItem(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{
		if (!m_pGameMode.IsMaster())
			return;

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

		// Prepare and execute fresh character spawn
		m_pRespawnSystem.PrepareCharacter(playerId, newId, null);
		m_sEnqueuedPlayers.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (!m_pGameMode.IsMaster())
			return;

		m_sEnqueuedPlayers.RemoveItem(playerId);

		IEntity player = m_pPlayerManager.GetPlayerController(playerId).GetControlledEntity();
		if (player)
		{
			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(player);
			persistence.PauseTracking();
			persistence.Save();
			// Game will cleanup the char for us because it was controlled by the player.
		}
		else
		{
			// Delete a still loading char that was not handed over to a player.
			GenericEntity transientCharacter = m_pRespawnSystem.GetTransientCharacter(playerId);
			EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(transientCharacter);
			if (persistence)
				persistence.PauseTracking();

			SCR_EntityHelper.DeleteEntityAndChildren(transientCharacter);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_sEnqueuedPlayers.IsEmpty())
			return;

		set<int> iterCopy();
		iterCopy.Copy(m_sEnqueuedPlayers);
		foreach (int playerId : iterCopy)
		{
			if (!m_pRespawnSystem.IsReadyForSpawn(playerId))
				continue;

			PlayerController playerController = m_pPlayerManager.GetPlayerController(playerId);
			if (!playerController)
			{
				Print("Failed to get player controller for playerId: " + playerId, LogLevel.VERBOSE);
				continue;
			}

			playerController.RequestRespawn();
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_pRespawnSystem = EPF_Component<EPF_RespawnSytemComponent>.Find(owner);
		m_pPlayerManager = GetGame().GetPlayerManager();
	}
};
