class EPF_PersistentScriptedStateLoaderTests : TestSuite
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
		connectInfo.m_sDatabaseName = "PersistentScriptedStateLoaderTests";
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

class EPF_Test_ScriptedStateLoaderDummy : EPF_PersistentScriptedState
{
	int m_iIntValue;

	//------------------------------------------------------------------------------------------------
	static EPF_Test_ScriptedStateLoaderDummy Create(int intValue)
	{
		EPF_Test_ScriptedStateLoaderDummy instance();
		instance.m_iIntValue = intValue;
		return instance;
	}
};

[
	EPF_PersistentScriptedStateSettings(EPF_Test_ScriptedStateLoaderDummy, options: EPF_EPersistentScriptedStateOptions.SELF_DELETE),
	EDF_DbName("ScriptedStateLoaderDummy")
]
class EPF_Test_ScriptedStateLoaderDummySaveData : EPF_ScriptedStateSaveData
{
	int m_iIntValue;
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadSingleton_NotExisting_Created : TestBase
{
	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void ActAndAsset()
	{
		// Act
		EPF_Test_ScriptedStateLoaderDummy instance = EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadSingleton();

		// Assert
		SetResult(new EDF_TestResult(instance && instance.GetPersistentId()));
	}
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadSingleton_Existing_Returned : TestBase
{
	ref EPF_Test_ScriptedStateLoaderDummy m_pExisting;

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Setup)]
	void Arrange()
	{
		m_pExisting = EPF_Test_ScriptedStateLoaderDummy.Create(1001);
		m_pExisting.Save();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void ActAndAsset()
	{
		// Act
		EPF_Test_ScriptedStateLoaderDummy instance = EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadSingleton();

		// Assert
		SetResult(new EDF_TestResult(
			instance &&
			instance.GetPersistentId() == m_pExisting.GetPersistentId() &&
			instance.m_iIntValue == m_pExisting.m_iIntValue));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.TearDown)]
	void Cleanup()
	{
		m_pExisting = null;
	}
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadSingletonAsync_NotExisting_Created : TestBase
{
	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void Act()
	{
		EDF_DataCallbackSingle<EPF_Test_ScriptedStateLoaderDummy> callback(this, "Assert");
		EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadSingletonAsync(callback);
	}

	//------------------------------------------------------------------------------------------------
	void Assert(EPF_Test_ScriptedStateLoaderDummy instance, Managed context)
	{
		SetResult(new EDF_TestResult(instance && instance.GetPersistentId()));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadSingletonAsync_Existing_Returned : TestBase
{
	ref EPF_Test_ScriptedStateLoaderDummy m_pExisting;

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Setup)]
	void Arrange()
	{
		m_pExisting = EPF_Test_ScriptedStateLoaderDummy.Create(1002);
		m_pExisting.Save();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void Act()
	{
		EDF_DataCallbackSingle<EPF_Test_ScriptedStateLoaderDummy> callback(this, "Assert");
		EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadSingletonAsync(callback);
	}

	//------------------------------------------------------------------------------------------------
	void Assert(EPF_Test_ScriptedStateLoaderDummy instance, Managed context)
	{
		SetResult(new EDF_TestResult(
			instance &&
			instance.GetPersistentId() == m_pExisting.GetPersistentId() &&
			instance.m_iIntValue == m_pExisting.m_iIntValue));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.TearDown)]
	void Cleanup()
	{
		m_pExisting = null;
	}
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadAsync_Existing_Returned : TestBase
{
	ref EPF_Test_ScriptedStateLoaderDummy m_pExisting;

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Setup)]
	void Arrange()
	{
		m_pExisting = EPF_Test_ScriptedStateLoaderDummy.Create(1003);
		m_pExisting.Save();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void Act()
	{
		EDF_DataCallbackSingle<EPF_Test_ScriptedStateLoaderDummy> callback(this, "Assert");
		EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadAsync(m_pExisting.GetPersistentId(), callback);
	}

	//------------------------------------------------------------------------------------------------
	void Assert(EPF_Test_ScriptedStateLoaderDummy instance, Managed context)
	{
		SetResult(new EDF_TestResult(
			instance &&
			instance.GetPersistentId() == m_pExisting.GetPersistentId() &&
			instance.m_iIntValue == m_pExisting.m_iIntValue));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.TearDown)]
	void Cleanup()
	{
		m_pExisting = null;
	}
};

[Test("EPF_PersistentScriptedStateLoaderTests", 3)]
class EPF_Test_PersistentScriptedStateLoader_LoadAsync_MultipleExisting_AllReturned : TestBase
{
	ref EPF_Test_ScriptedStateLoaderDummy m_pExisting1;
	ref EPF_Test_ScriptedStateLoaderDummy m_pExisting2;
	ref array<string> m_aIds;

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Setup)]
	void Arrange()
	{
		m_pExisting1 = EPF_Test_ScriptedStateLoaderDummy.Create(1004);
		m_pExisting2 = EPF_Test_ScriptedStateLoaderDummy.Create(1005);
		m_pExisting1.Save();
		m_pExisting2.Save();
		m_aIds = {m_pExisting1.GetPersistentId(), m_pExisting2.GetPersistentId()};
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	void Act()
	{
		// Load all ids
		EDF_DataCallbackMultiple<EPF_Test_ScriptedStateLoaderDummy> callback(this, "Assert");
		EPF_PersistentScriptedStateLoader<EPF_Test_ScriptedStateLoaderDummy>.LoadAsync(null, callback);
	}

	//------------------------------------------------------------------------------------------------
	void Assert(array<EPF_Test_ScriptedStateLoaderDummy> instances, Managed context)
	{
		SetResult(new EDF_TestResult(
			instances &&
			instances.Count() == 2 &&
			m_aIds.Contains(instances.Get(0).GetPersistentId()) &&
			m_aIds.Contains(instances.Get(1).GetPersistentId())));
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.Main)]
	bool AwaitResult()
	{
		return GetResult();
	}

	//------------------------------------------------------------------------------------------------
	[Step(EStage.TearDown)]
	void Cleanup()
	{
		m_pExisting1 = null;
		m_pExisting2 = null;
	}
};
