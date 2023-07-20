modded class SCR_RespawnSystemComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		m_pGameMode = SCR_BaseGameMode.Cast(owner);
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_pGameMode || !m_pRplComponent || !m_SpawnLogic)
		{
			Print("SCR_RespawnSystemComponent setup is invalid!", LogLevel.ERROR);
			return;
		}

		if (!GetGame().InPlayMode())
			return;

		m_SpawnLogic.OnInit(this);
	}
}
