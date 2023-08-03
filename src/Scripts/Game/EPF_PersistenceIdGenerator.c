class EPF_PersistenceIdGenerator : EDF_DbEntityIdGenerator
{
	protected static int s_iHiveId;
	protected static ref set<string> s_aPrefabPosIdentifiers;

	//------------------------------------------------------------------------------------------------
	static void SetHiveId(int hiveId)
	{
		if (hiveId < 1 || hiveId > 4095)
		{
			Debug.Error("Argument 'hiveId' out of range. Allowed: 1-4095");
			return;
		}

		s_iHiveId = hiveId;
	}
	//------------------------------------------------------------------------------------------------
	static int GetHiveId()
	{
		return s_iHiveId;
	}

	//------------------------------------------------------------------------------------------------
	override static string Generate()
	{
		if (!s_pRandom)
		{
			s_pRandom = new RandomGenerator();
			s_pRandom.SetSeed(System.GetUnixTime());
		}

		//No need to look at past generated ids in db for conflict, because they have older timestamps
		string timeHex = EDF_HexHelper.Convert(System.GetUnixTime(), false, 8);

		string hiveHex = EDF_HexHelper.Convert(s_iHiveId, false, 4);

		//Sequence returns to 0 on overflow
		string seqHex = EDF_HexHelper.Convert(s_iSequence++, false, 8);
		if (s_iSequence == int.MAX) s_iSequence = 0;

		//Random component to ensure that even if all sequentials are generated in the same second it stays unique
		string randHex = EDF_HexHelper.Convert((int)(s_pRandom.RandFloatXY(0, int.MAX)), false, 8);
		string randHex2 = EDF_HexHelper.Convert((int)(s_pRandom.RandFloatXY(0, 65535)), false, 4);

		// TTTT TTTT-SEQ1-SEQ2-HHHH-RND1 RND1 RND2
		return string.Format("%1-%2-%3-%4-%5%6",
			timeHex,
			seqHex.Substring(0, 4),
			seqHex.Substring(4, 4),
			hiveHex,
			randHex,
			randHex2.Substring(0, 4));
	}

	//------------------------------------------------------------------------------------------------
	static string Generate(IEntity entity)
	{
		if (!s_pRandom)
		{
			s_pRandom = new RandomGenerator();
			s_pRandom.SetSeed(System.GetUnixTime());
		}

		string hiveHex = EDF_HexHelper.Convert(s_iHiveId, false, 4);

		string identifier = entity.GetName();
		if (!identifier)
		{
			if (!s_aPrefabPosIdentifiers)
				s_aPrefabPosIdentifiers = new set<string>();

			string type = EPF_Utils.GetPrefabName(entity);
			if (type)
			{
				type = EPF_Utils.GetPrefabGUID(type);
			}
			else
			{
				type = entity.ClassName();
			}

			IEntity parent = entity.GetParent();
			while (parent && !parent.GetName())
			{
				parent = parent.GetParent()
			}

			bool useParent = parent && parent.GetName();

			int duplicate;
			while (true)
			{
				if (useParent)
				{
					identifier = parent.GetName() + ":" + type + ":" + duplicate++;
				}
				else
				{
					identifier = type + ":" + entity.GetOrigin().ToString(false) + ":" + duplicate++;
				}

				if (s_aPrefabPosIdentifiers.Insert(identifier))
					break;
			}
		}

		int splitLength = Math.Max(1, identifier.Length() / 3);
		string split1 = EDF_HexHelper.Convert(Math.AbsInt(identifier.Substring(0, splitLength).Hash()), false, 8);
		string split2 = EDF_HexHelper.Convert(Math.AbsInt(identifier.Substring(splitLength, splitLength).Hash()), false, 8);
		int doubleSplit = splitLength * 2;
		string split3 = EDF_HexHelper.Convert(Math.AbsInt(identifier.Substring(doubleSplit, identifier.Length() - doubleSplit).Hash()), false, 8);

		// 00bb HHHH-SPL1-SPL1-SPL2-SPL2 SPL3 SPL3
		return string.Format("00bb%1-%2-%3-%4-%5%6",
			hiveHex,
			split1.Substring(0, 4),
			split1.Substring(4, 4),
			split2.Substring(0, 4),
			split2.Substring(4, 4),
			split3);
	}

	//------------------------------------------------------------------------------------------------
	override static void Reset()
	{
		EDF_DbEntityIdGenerator.Reset();
		s_aPrefabPosIdentifiers = null;
	}
};
