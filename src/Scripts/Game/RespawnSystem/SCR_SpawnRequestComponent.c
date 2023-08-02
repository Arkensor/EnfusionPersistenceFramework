modded class SCR_SpawnRequestComponent
{
	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		// Mute warnings on current minimalistic EPF respawn system "hotfix"
		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (respawnSystem && respawnSystem.IsInherited(EPF_BaseRespawnSystemComponent))
			return;

		super.OnPostInit(owner);
	}
}
