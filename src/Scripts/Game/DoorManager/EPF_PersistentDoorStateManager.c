class EPF_PersistentDoorStateManager : EPF_PersistentScriptedState
{
	ref set<ref Tuple2<IEntity, DoorComponent>> m_aTrackedDoors
		= new set<ref Tuple2<IEntity, DoorComponent>>();

	EPF_PersistentDoorStateFilter m_pFilter;

	//------------------------------------------------------------------------------------------------
	static void UpdateDoorState(IEntity doorEntity)
	{
		EPF_PersistentDoorStateManagerComponent managerComponent = EPF_PersistentDoorStateManagerComponent.GetInstance();
		if (!managerComponent)
			return;

		EPF_PersistentDoorStateManager manager = managerComponent.GetPersistentDoorManager();
		if (!manager)
			return;

		DoorComponent door = EPF_Component<DoorComponent>.Find(doorEntity);
		if (!door)
			return;

		if (manager.m_pFilter && !manager.m_pFilter.IsMatch(doorEntity))
			return;

		manager.m_aTrackedDoors.Insert(new Tuple2<IEntity, DoorComponent>(doorEntity, door));
	}
}
