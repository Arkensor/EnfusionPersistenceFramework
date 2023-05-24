modded class SCR_RespawnComponent
{
	protected ref array<RplId> EPF_m_aQuickBarRplIds;

	//------------------------------------------------------------------------------------------------
	void EPF_SetQuickBarItems(array<RplId> quickBarRplIds)
	{
		Rpc(Rpc_SetQuickBarItems, quickBarRplIds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void Rpc_SetQuickBarItems(array<RplId> quickBarRplIds)
	{
		EPF_m_aQuickBarRplIds = quickBarRplIds;
	}

	//------------------------------------------------------------------------------------------------
	override protected void RpcAsk_NotifyOnPlayerSpawned()
	{
		super.RpcAsk_NotifyOnPlayerSpawned();

		// TODO: Refactor after 0.9.9's respawn system changes and make sure the controlled entity is known as early as it should.
		SCR_PlayerController playerController = SCR_PlayerController.Cast(m_PlayerController);
		if (playerController)
			playerController.m_OnControlledEntityChanged.Insert(EPF_OnControlledEntityChanged);
	}

	//------------------------------------------------------------------------------------------------
	protected void EPF_OnControlledEntityChanged(IEntity from, IEntity to)
	{
		// Postpone to next frame because otherwide not all RplId's are known already.
		GetGame().GetCallqueue().Call(EPF_ApplyQuickbar);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void EPF_ApplyQuickbar()
	{
		SCR_CharacterInventoryStorageComponent inventoryStorage = EPF_Component<SCR_CharacterInventoryStorageComponent>.Find(m_PlayerController.GetControlledEntity());
		if (inventoryStorage)
		{
			inventoryStorage.EPF_Rpc_UpdateQuickSlotItems(EPF_m_aQuickBarRplIds);
			EPF_m_aQuickBarRplIds = null;
		}
	}
};
