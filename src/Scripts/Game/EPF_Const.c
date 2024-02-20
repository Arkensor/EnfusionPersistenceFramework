class EPF_Const
{
	static const vector VECTOR_INFINITY = "inf inf inf";

	//------------------------------------------------------------------------------------------------
	sealed static bool IsInf(vector value)
	{
		return value == VECTOR_INFINITY;
	}

	//------------------------------------------------------------------------------------------------
	sealed static bool IsInf(float value)
	{
		return value == float.INFINITY;
	}

};
