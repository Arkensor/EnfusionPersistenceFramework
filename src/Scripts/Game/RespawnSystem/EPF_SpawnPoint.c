[EntityEditorProps(category: "Persistence/Respawn", description: "Character spawn point")]
class EPF_SpawnPointClass : SCR_PositionClass
{
}

class EPF_SpawnPoint : SCR_Position
{
	[Attribute("0", desc: "Find empty position for spawning within given radius. When none is found, entity position will be used.")]
	protected float m_fSpawnRadius;

	protected static ref array<EPF_SpawnPoint> s_aSpawnPoints = {};

	//------------------------------------------------------------------------------------------------
	[Obsolete("Use GetRandomSpawnPoint instead.")]
	static EPF_SpawnPoint GetDefaultSpawnPoint()
	{
		if (s_aSpawnPoints.IsEmpty()) 
			return null;
		
		return s_aSpawnPoints.Get(0);
	}

	//------------------------------------------------------------------------------------------------
	static EPF_SpawnPoint GetRandomSpawnPoint()
	{
		if (s_aSpawnPoints.IsEmpty()) 
			return null;
		
		return s_aSpawnPoints.GetRandomElement();
	}
	
	//------------------------------------------------------------------------------------------------
	static array<EPF_SpawnPoint> GetSpawnPoints()
	{
		return s_aSpawnPoints;
	}

	//------------------------------------------------------------------------------------------------
	#ifdef WORKBENCH
	override void SetColorAndText()
	{
		m_sText = "Spawnpoint";
		m_iColor = Color.CYAN;
	}
	#endif

	//------------------------------------------------------------------------------------------------
	void GetPosYPR(out vector position, out vector ypr)
	{
		position = GetOrigin();
		ypr = GetYawPitchRoll();
		SCR_WorldTools.FindEmptyTerrainPosition(position, position, m_fSpawnRadius);
	}

	//------------------------------------------------------------------------------------------------
	void EPF_SpawnPoint(IEntitySource src, IEntity parent)
	{
		SetFlags(EntityFlags.STATIC, true);

		if (GetGame().InPlayMode()) 
			s_aSpawnPoints.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	void ~EPF_SpawnPoint()
	{
		if (s_aSpawnPoints) 
			s_aSpawnPoints.RemoveItem(this);
	}
}
