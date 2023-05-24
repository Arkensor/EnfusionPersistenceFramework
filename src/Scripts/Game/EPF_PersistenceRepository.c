class EPF_PersistenceEntityHelper<Class TEntityType>
{
	//------------------------------------------------------------------------------------------------
	//! Get a database repository that is connected to the persistence database source
	//! \return registered or generic respository instance or null on failure
	static EDF_DbRepository<TEntityType> GetRepository()
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (!persistenceManager)
			return null;

		typename repositoryType = EDF_DbRepositoryRegistration.Get(TEntityType);
		if (!repositoryType)
		{
			string repositoryTypeStr = string.Format("EDF_DbRepository<%1>", TEntityType);
			Debug.Error(string.Format("Tried to get unknown entity repository type '%1'. Make sure you use it somewhere in your code e.g.: '%1 repository = ...;'", repositoryTypeStr));
		}

		return EDF_DbRepository<TEntityType>.Cast(EDF_DbRepositoryFactory.GetRepository(repositoryType, persistenceManager.GetDbContext()));
	}
};

class EPF_PersistenceRepositoryHelper<Class TRepositoryType>
{
	//------------------------------------------------------------------------------------------------
	//! Get a database repository that is connected to the persistence database source
	//! \return registered or generic respository instance or null on failure
	static TRepositoryType Get()
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		if (!persistenceManager)
			return null;

		return TRepositoryType.Cast(EDF_DbRepositoryFactory.GetRepository(TRepositoryType, persistenceManager.GetDbContext()));
	}
};
