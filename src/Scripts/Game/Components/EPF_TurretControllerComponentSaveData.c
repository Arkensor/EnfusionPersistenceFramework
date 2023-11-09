[EPF_ComponentSaveDataType(TurretControllerComponent), BaseContainerProps()]
class EPF_TurretControllerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_TurretControllerComponentSaveData : EPF_ComponentSaveData
{
	float m_fYaw;
	float m_fPitch;
	int m_iSelectedWeaponSlotIdx;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		TurretControllerComponent turretController = TurretControllerComponent.Cast(component);

		BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
		WeaponSlotComponent currentSlot = weaponManager.GetCurrentSlot();
		if (currentSlot)
			m_iSelectedWeaponSlotIdx = currentSlot.GetWeaponSlotIndex();

		TurretComponent turret = turretController.GetTurretComponent();
		if (!turret)
			return EPF_EReadResult.ERROR;

		vector angles = turret.GetAimingRotation();
		m_fYaw = angles[0];
		m_fPitch = angles[1];

		int defaultIdx;
		BaseContainer weaponManagerSource = weaponManager.GetComponentSource(owner);
		weaponManagerSource.Get("DefaultWeaponIndex", defaultIdx);
		if (defaultIdx == -1)
		{
			WeaponSlotComponent firstSlot = EPF_Component<WeaponSlotComponent>.Find(owner);
			if (firstSlot)
			{
				defaultIdx = firstSlot.GetWeaponSlotIndex();
			}
			else
			{
				defaultIdx = 0;
			}
		}

		if (vector.Distance(angles, turret.GetInitAiming()) <= 0.0001 && (m_iSelectedWeaponSlotIdx == defaultIdx))
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		TurretControllerComponent turretController = TurretControllerComponent.Cast(component);

		BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
		WeaponSlotComponent currentSlot = weaponManager.GetCurrentSlot();
		if (!currentSlot || currentSlot.GetWeaponSlotIndex() != m_iSelectedWeaponSlotIdx)
		{
			array<WeaponSlotComponent> outSlots();
			weaponManager.GetWeaponsSlots(outSlots);
			foreach (WeaponSlotComponent weaponSlot : outSlots)
			{
				if (weaponSlot.GetWeaponSlotIndex() == m_iSelectedWeaponSlotIdx)
				{
					weaponManager.SelectWeapon(weaponSlot);
					break;
				}
			}
		}

		// Need to push this 2 frames later because max angles are set on frame after turret spawned ...
		EPF_DeferredApplyResult.AddPending(this, "TurretControllerComponentSaveData::SetAimingAngles");
		GetGame().GetCallqueue().Call(SetAimingAngles, turretController, m_fYaw * Math.DEG2RAD, m_fPitch * Math.DEG2RAD, true);

		return EPF_EApplyResult.AWAIT_COMPLETION;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetAimingAngles(TurretControllerComponent turretController, float yaw, float pitch, bool wait)
	{
		if (turretController)
		{
			if (wait)
			{
				// Waiting for second frame ...
				GetGame().GetCallqueue().Call(SetAimingAngles, turretController, yaw, pitch, false);
				return;
			}

			turretController.SetAimingAngles(yaw, pitch);
		}

		EPF_DeferredApplyResult.SetFinished(this, "TurretControllerComponentSaveData::SetAimingAngles");
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_TurretControllerComponentSaveData otherData = EPF_TurretControllerComponentSaveData.Cast(other);
		return float.AlmostEqual(m_fYaw, otherData.m_fYaw) &&
			float.AlmostEqual(m_fPitch, otherData.m_fPitch) &&
			m_iSelectedWeaponSlotIdx == otherData.m_iSelectedWeaponSlotIdx;
	}
}
