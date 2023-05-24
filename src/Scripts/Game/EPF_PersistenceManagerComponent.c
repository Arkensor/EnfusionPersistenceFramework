[ComponentEditorProps(category: "Persistence", description: "Must be attached to the gamemode entity to setup the persistence system.")]
class EPF_PersistenceManagerComponentClass : SCR_BaseGameModeComponentClass
{
	[Attribute(defvalue: "1", desc: "Enable or disable auto-save globally", category: "Auto-Save")]
	bool m_bEnableAutosave;

	[Attribute(defvalue: "600", desc: "Time between auto-save in seconds.", category: "Auto-Save")]
	float m_fAutosaveInterval;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.Slider, desc: "Maximum number of entities processed during a single update tick.", params: "1 128 1", category: "Auto-Save")]
	int m_iAutosaveIterations;

	[Attribute(defvalue: "0.33", uiwidget: UIWidgets.Slider, desc: "Adjust the tick rate of the persistence manager", params: "0.01 10 0.01", category: "Advanced", precision: 2)]
	float m_fUpdateRate;

	[Attribute(desc: "Default database connection. Can be overriden using \"-ConnectionString=...\" CLI argument", category: "Database")]
	ref EDF_DbConnectionInfoBase m_pConnectionInfo;

	//[Attribute(defvalue: "1", desc: "When using a buffered DB context changes are only flushed on auto-/shutdown-save or when called manually. Can increase constistency.", category: "Database")]
	//bool m_bBufferedDatabaseContext;

	//[Attribute(defvalue: "50", desc: "Max DB operations batch size per frame. Only relevant when buffered DB context is enabled.", category: "Database")]
	//int m_bBufferedDatabaseBatchsize;
};

class EPF_PersistenceManagerComponent : SCR_BaseGameModeComponent
{
	protected EPF_PersistenceManager m_pPersistenceManager;
	protected float m_fAccumulator;
	protected float m_fUpdateRateSetting;

	//------------------------------------------------------------------------------------------------
	override event void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (m_pPersistenceManager)
			m_pPersistenceManager.OnWorldPostProcess(world);
	}

	//------------------------------------------------------------------------------------------------
	override event void OnGameEnd()
	{
		super.OnGameEnd();
		if (m_pPersistenceManager)
			m_pPersistenceManager.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	override event void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);
		m_fAccumulator += timeSlice;
		if (m_fAccumulator >= m_fUpdateRateSetting)
		{
			m_pPersistenceManager.OnPostFrame(m_fAccumulator);
			m_fAccumulator = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	override event void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Persistence logic only runs on the server machine
		if (!EPF_PersistenceManager.IsPersistenceMaster())
			return;

		EPF_PersistenceManagerComponentClass settings = EPF_PersistenceManagerComponentClass.Cast(GetComponentData(owner));
		if (settings.m_fAutosaveInterval < settings.m_fUpdateRate)
		{
			Debug.Error(string.Format("Update rate '%1' must be smaller than auto-save interval '%2'.", settings.m_fUpdateRate, settings.m_fAutosaveInterval));
			return;
		}

		// Hive management
		EPF_PersistenceIdGenerator.SetHiveId(GetHiveId());

		// Db connection
		EDF_DbConnectionInfoBase connectionInfoOverride = GetConnectionInfo();
		if (connectionInfoOverride)
			settings.m_pConnectionInfo = connectionInfoOverride;

		// Auto-save
		m_fUpdateRateSetting = settings.m_fUpdateRate;
		m_pPersistenceManager = EPF_PersistenceManager.GetInstance();
		m_pPersistenceManager.OnPostInit(owner, settings);
		SetEventMask(owner, EntityEvent.POSTFRAME);
	}

	//------------------------------------------------------------------------------------------------
	//! Getting the hive id.
	//! Inherit or used modded if you want to get it from another info source.
	protected int GetHiveId()
	{
		string hiveId;
		if (!System.GetCLIParam("HiveId", hiveId))
			hiveId = "1";

		return hiveId.ToInt();
	}

	//------------------------------------------------------------------------------------------------
	//! Getting the database connection info.
	//! Inherit or used modded if you want to get it from another info source.
	protected EDF_DbConnectionInfoBase GetConnectionInfo()
	{
		string connectionString;
		if (!System.GetCLIParam("ConnectionString", connectionString))
			return null;

		int driverEndIdx = connectionString.IndexOf("://");
		if (driverEndIdx == -1)
		{
			Debug.Error(string.Format("Invalid -ConnectionString=... parameter value '%1'.", connectionString));
			return null;
		}

		string driverName = connectionString.Substring(0, driverEndIdx);
		string connectionInfoString = connectionString.Substring(driverEndIdx + 3, connectionString.Length() - (driverName.Length() + 3));

		typename driverType = EDF_DbDriverRegistry.Get(driverName);
		if (!driverType.IsInherited(EDF_DbDriver))
		{
			Debug.Error(string.Format("Incompatible database driver type '%1'.", driverType));
			return null;
		}

		typename connectionInfoType = EDF_DbConnectionInfoDriverType.GetConnectionInfoType(driverType);
		EDF_DbConnectionInfoBase connectionInfo = EDF_DbConnectionInfoBase.Cast(connectionInfoType.Spawn());
		if (!connectionInfo)
			return null;

		connectionInfo.Parse(connectionInfoString);

		return connectionInfo;
	}
};
