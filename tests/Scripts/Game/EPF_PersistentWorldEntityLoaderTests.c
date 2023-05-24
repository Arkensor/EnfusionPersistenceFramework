class EPF_PersistentWorldEntityLoaderTests : TestSuite
{
	ref EDF_DbContext m_pPreviousContext;

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Setup)]
	void Setup()
	{
		// Change db context to in memory for this test suite
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		m_pPreviousContext = persistenceManager.GetDbContext();
		EDF_InMemoryDbConnectionInfo connectInfo();
		connectInfo.m_sDatabaseName = "PersistentWorldEntityLoaderTests";
		persistenceManager.SetDbContext(EDF_DbContext.Create(connectInfo));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.TearDown)]
	void TearDown()
	{
		EPF_PersistenceManager.GetInstance().SetDbContext(m_pPreviousContext);
		m_pPreviousContext = null;
	}
};

class PersistentWorldEntityLoaderBase : TestBase
{
	EPF_PersistenceComponent m_pExisting;

	[Step(EStage.Setup)]
	void Arrange()
	{
		m_pExisting = SpawnDummy();
	}

	EPF_PersistenceComponent SpawnDummy()
	{
		EPF_PersistenceComponent persistenceComponent;
		IEntity dummy = EPF_Utils.SpawnEntityPrefab("{C95E11C60810F432}Prefabs/Items/Core/Item_Base.et", "0 0 0");
		if (dummy)
		{
			persistenceComponent = EPF_PersistenceComponent.Cast(dummy.FindComponent(EPF_PersistenceComponent));
			if (persistenceComponent) persistenceComponent.Save();
		}
		return persistenceComponent;
	}

	[Step(EStage.TearDown)]
	void Cleanup()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(m_pExisting.GetOwner());
		m_pExisting = null;
	}
};

[Test("EPF_PersistentWorldEntityLoaderTests", 3)]
class EPF_Test_PersistentWorldEntityLoader_Load_Existing_Spawned : PersistentWorldEntityLoaderBase
{
	[Step(EStage.Main)]
	void ActAndAsset()
	{
		// Act
		IEntity worldEntity = EPF_PersistentWorldEntityLoader.Load(EPF_Utils.GetPrefabName(m_pExisting.GetOwner()), m_pExisting.GetPersistentId());

		// Assert
		SetResult(new EDF_TestResult(
			worldEntity &&
			m_pExisting.GetPersistentId() &&
			EPF_PersistenceComponent.GetPersistentId(worldEntity) == m_pExisting.GetPersistentId()));

		// Cleanup
		SCR_EntityHelper.DeleteEntityAndChildren(worldEntity);
	}
};

[Test("EPF_PersistentWorldEntityLoaderTests", 3)]
class EPF_Test_PersistentWorldEntityLoader_LoadAsync_Existing_Spawned : PersistentWorldEntityLoaderBase
{
	[Step(EStage.Main)]
	void Act()
	{
		EDF_DataCallbackSingle<IEntity> callback(this, "Assert");
		EPF_PersistentWorldEntityLoader.LoadAsync(EPF_ItemSaveData, m_pExisting.GetPersistentId(), callback);
	}

	void Assert(IEntity worldEntity, Managed context)
	{
		SetResult(new EDF_TestResult(
			worldEntity &&
			m_pExisting.GetPersistentId() &&
			EPF_PersistenceComponent.GetPersistentId(worldEntity) == m_pExisting.GetPersistentId()));

		// Cleanup
		SCR_EntityHelper.DeleteEntityAndChildren(worldEntity);
	}

	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}
};

[Test("EPF_PersistentWorldEntityLoaderTests", 3)]
class EPF_Test_PersistentWorldEntityLoader_Load_MultiExisting_AllSpawned : PersistentWorldEntityLoaderBase
{
	EPF_PersistenceComponent m_pExisting2;
	ref array<string> m_aIds;

	override void Arrange()
	{
		super.Arrange();
		m_pExisting2 = SpawnDummy();
		m_aIds = {m_pExisting.GetPersistentId(), m_pExisting2.GetPersistentId()};
	}

	[Step(EStage.Main)]
	void ActAndAsset()
	{
		// Act
		array<IEntity> worldEntities = EPF_PersistentWorldEntityLoader.Load(EPF_ItemSaveData, m_aIds);

		// Assert
		SetResult(new EDF_TestResult(
			worldEntities &&
			worldEntities.Count() == 2 &&
			m_aIds.Contains(EPF_PersistenceComponent.GetPersistentId(worldEntities.Get(0))) &&
			m_aIds.Contains(EPF_PersistenceComponent.GetPersistentId(worldEntities.Get(1)))));

		// Cleanup
		RemoveEntities(worldEntities);
	}

	void RemoveEntities(array<IEntity> worldEntities)
	{
		if (!worldEntities) return;
		foreach (IEntity worldEntity : worldEntities)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(worldEntity);
		}
	}

	override void Cleanup()
	{
		super.Cleanup();

		SCR_EntityHelper.DeleteEntityAndChildren(m_pExisting2.GetOwner());
		m_pExisting2 = null;
	}
};

[Test("EPF_PersistentWorldEntityLoaderTests", 3)]
class EPF_Test_PersistentWorldEntityLoader_LoadAsync_MultiExisting_AllSpawned : EPF_Test_PersistentWorldEntityLoader_Load_MultiExisting_AllSpawned
{
	[Step(EStage.Main)]
	void Act()
	{
		EDF_DataCallbackMultiple<IEntity> callback(this, "Assert");
		EPF_PersistentWorldEntityLoader.LoadAsync(EPF_ItemSaveData, m_aIds, callback);
	}

	void Assert(array<IEntity> worldEntities, Managed context)
	{
		SetResult(new EDF_TestResult(
			worldEntities &&
			worldEntities.Count() == 2 &&
			m_aIds.Contains(EPF_PersistenceComponent.GetPersistentId(worldEntities.Get(0))) &&
			m_aIds.Contains(EPF_PersistenceComponent.GetPersistentId(worldEntities.Get(1)))));

		// Cleanup
		RemoveEntities(worldEntities);
	}

	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}
};
