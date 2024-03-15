class EPF_Const
{
	static const vector VECTOR_UNSET = "-1337.42042 -1337.42042 -1337.42042";
	static const float FLOAT_UNSET = -1337.42042;

	//------------------------------------------------------------------------------------------------
	sealed static bool IsUnset(vector value)
	{
		return value == VECTOR_UNSET;
	}

	//------------------------------------------------------------------------------------------------
	sealed static bool IsUnset(float value)
	{
		return value == FLOAT_UNSET;
	}
}
