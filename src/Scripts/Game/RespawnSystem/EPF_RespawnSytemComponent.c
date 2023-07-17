[ComponentEditorProps(category: "Persistence/Testing", description: "Part of the re-spawn system on the gamemode.")]
class EPF_RespawnSytemComponentClass : SCR_RespawnSystemComponentClass
{
};

class EPF_RespawnSytemComponent : SCR_RespawnSystemComponent
{
	[Attribute(defvalue: "{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et")]
	ResourceName m_rDefaultPrefab;

	protected ref map<GenericEntity, int> m_mLoadingCharacters = new map<GenericEntity, int>();
	protected ref map<int, GenericEntity> m_mPerparedCharacters = new map<int, GenericEntity>();

	//------------------------------------------------------------------------------------------------
	void PrepareCharacter(int playerId, string playerUid, EPF_CharacterSaveData saveData)
	{
		GenericEntity playerEntity;

		if (saveData)
		{
			// Spawn character from data
			EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

			saveData.m_pTransformation.m_bApplied = true;
			vector spawnAngles = Vector(saveData.m_pTransformation.m_vAngles[1], saveData.m_pTransformation.m_vAngles[0], saveData.m_pTransformation.m_vAngles[2]);
			playerEntity = DoSpawn(m_rDefaultPrefab, saveData.m_pTransformation.m_vOrigin, spawnAngles);

			EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(playerEntity);
			if (persistenceComponent)
			{
				// Remember which entity was for what player id
				m_mLoadingCharacters.Set(playerEntity, playerId);

				persistenceComponent.GetOnAfterLoadEvent().Insert(OnCharacterLoaded);

				// TODO: Remove hard loading time limit when we know all spawn block bugs are fixed.
				GetGame().GetCallqueue().CallLater(OnCharacterLoaded, 5000, false, persistenceComponent, saveData);

				if (persistenceComponent.Load(saveData))
					return;

				// On failure remove again
				persistenceComponent.GetOnAfterLoadEvent().Remove(OnCharacterLoaded);
				m_mLoadingCharacters.Remove(playerEntity);
			}

			SCR_EntityHelper.DeleteEntityAndChildren(playerEntity);
			playerEntity = null;
		}

		if (!playerEntity)
		{
			vector position;
			vector angles;

			EPF_SpawnPoint spawnPoint = EPF_SpawnPoint.GetDefaultSpawnPoint();
			if (!spawnPoint)
			{
				Print("Could not spawn character, no default spawn point configured.", LogLevel.ERROR);
				return;
			}

			spawnPoint.GetPosAngles(position, angles);

			playerEntity = DoSpawn(m_rDefaultPrefab, position, angles);

			EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(playerEntity);
			if (persistenceComponent)
			{
				persistenceComponent.SetPersistentId(playerUid);
			}
			else
			{
				Print(string.Format("Could not create new character, prefab '%1' is missing component '%2'.", m_rDefaultPrefab, EPF_PersistenceComponent), LogLevel.ERROR);
				SCR_EntityHelper.DeleteEntityAndChildren(playerEntity);
				return;
			}

			m_mPerparedCharacters.Set(playerId, playerEntity);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCharacterLoaded(EPF_PersistenceComponent persistenceComponent, EPF_EntitySaveData saveData)
	{
		if (!persistenceComponent || !saveData)
			return;

		GenericEntity playerEntity = GenericEntity.Cast(persistenceComponent.GetOwner());
		int playerId;
		if (!m_mLoadingCharacters.Find(playerEntity, playerId))
			return; // Already processed before the hard loading limit callqueue invoke

		m_mLoadingCharacters.Remove(playerEntity);

		// We only want to know this once
		persistenceComponent.GetOnAfterLoadEvent().Remove(OnCharacterLoaded);

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

				SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(GetGame().GetPlayerManager().GetPlayerRespawnComponent(playerId));
				respawnComponent.EPF_SetQuickBarItems(quickBarRplIds);
			}
		}

		m_mPerparedCharacters.Set(playerId, playerEntity);
		return; //Wait a few frame for character and weapon controller and gadgets etc to be setup
	}

	//------------------------------------------------------------------------------------------------
	bool IsReadyForSpawn(int playerId)
	{
		return m_mPerparedCharacters.Contains(playerId);
	}

	//------------------------------------------------------------------------------------------------
	GenericEntity GetTransientCharacter(int playerId)
	{
		GenericEntity transientCharacter = m_mPerparedCharacters.Get(playerId);
		if (!transientCharacter)
		{
			foreach (GenericEntity char, int id : m_mLoadingCharacters)
			{
				if (id == playerId)
					return char;
			}
		}
		return transientCharacter;
	}

	//------------------------------------------------------------------------------------------------
	protected override GenericEntity RequestSpawn(int playerId)
	{
		GenericEntity playerEntity = m_mPerparedCharacters.Get(playerId);
		if (playerEntity)
		{
			m_mPerparedCharacters.Remove(playerId);
			return playerEntity;
		}

		Debug.Error("Attempt to spawn a character that has not finished processing. IsReadyForSpawn was not checked?");
		return null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		// Hard override to not rely on faction or loadout manager
		m_pGameMode = SCR_BaseGameMode.Cast(owner);

		if (m_pGameMode)
		{
			// Use replication of the parent
			m_pRplComponent = RplComponent.Cast(m_pGameMode.FindComponent(RplComponent));
		}

		// Hard skip the rest of super implementation >:)
	}
};
