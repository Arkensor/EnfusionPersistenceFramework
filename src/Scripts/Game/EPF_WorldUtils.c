class EPF_WorldUtils
{
	protected static ResourceName s_pQueryPrefab;
	protected static IEntity s_pQueryResult;

	//------------------------------------------------------------------------------------------------
	static IEntity FindNearestPrefab(ResourceName prefab, vector origin, float radius)
	{
		s_pQueryPrefab = prefab;
		s_pQueryResult = null;
		GetGame().GetWorld().QueryEntitiesBySphere(origin, radius, FindNearestPrefab_FirstEntity, FindNearestPrefab_FilterEntities);
		IEntity result = s_pQueryResult;
		s_pQueryResult = null;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindNearestPrefab_FirstEntity(IEntity ent)
	{
		s_pQueryResult = ent;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindNearestPrefab_FilterEntities(IEntity ent)
	{
		return EPF_Utils.GetPrefabName(ent).Contains(s_pQueryPrefab);
	}

	//------------------------------------------------------------------------------------------------
	//! Teleport an entity (and align it to terrain if no angles are specified)
	//! \param entity Entity instance to be teleported
	//! \param position Position where to teleport to
	//! \param angles (yaw, pitch, rolle in degrees) to apply after teleportation
	static void Teleport(notnull IEntity entity, vector position, float yaw = "nan".ToFloat())
	{
		vector transform[4];

		if (!EPF_Const.IsNan(yaw))
		{
			Math3D.AnglesToMatrix(Vector(yaw, 0, 0), transform);
		}
		else
		{
			entity.GetWorldTransform(transform);
		}

		transform[3] = position;
		SCR_TerrainHelper.OrientToTerrain(transform);

		ForceTransformEx(entity, transform);
	}

	//------------------------------------------------------------------------------------------------
	//! Force entity into a new transformation matrix
	//! \param entity
	//! \param origin
	//! \param angles (yaw, pitch, roll in degrees)
	//! \param scale
	static void ForceTransform(notnull IEntity entity, vector origin = EPF_Const.VECTOR_NAN, vector angles = EPF_Const.VECTOR_NAN, float scale = "nan".ToFloat())
	{
		bool needsChange;
		vector transform[4];
		entity.GetWorldTransform(transform);

		if (!EPF_Const.IsNan(origin))
		{
			transform[3] = origin;
			needsChange = true;
		}

		if (!EPF_Const.IsNan(angles))
		{
			Math3D.AnglesToMatrix(angles, transform);
			needsChange = true;
		}

		// TODO: Repace with EPF_Const.FLOAT_NAN after https://feedback.bistudio.com/T172797 is fixed. In EPF_Utils.Teleport() too.
		if (!EPF_Const.IsNan(scale))
		{
			Math3D.MatrixScale(transform, scale);
			needsChange = true;
		}

		if (needsChange)
			ForceTransformEx(entity, transform);
	}

	//------------------------------------------------------------------------------------------------
	//! Set the transformation matrix and update the entity
	static void ForceTransformEx(notnull IEntity entity, vector transform[4])
	{
		vector previousOrigin = entity.GetOrigin();

		BaseGameEntity baseGameEntity = BaseGameEntity.Cast(entity);
		if (baseGameEntity && !BaseVehicle.Cast(baseGameEntity))
		{
			baseGameEntity.Teleport(transform);
		}
		else
		{
			entity.SetWorldTransform(transform);
		}

		Physics physics = entity.GetPhysics();
		if (physics)
		{
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
		}

		RplComponent replication = EPF_Component<RplComponent>.Find(entity);
		if (replication)
			replication.ForceNodeMovement(previousOrigin);

		if (!ChimeraCharacter.Cast(entity))
			entity.Update();
	}
}
