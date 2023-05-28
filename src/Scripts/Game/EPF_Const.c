class EPF_Const
{
	const vector VECTOR_INFINITY = "inf inf inf";

	const vector VECTOR_NAN = "nan nan nan";
	const float FLOAT_NAN = "nan".ToFloat();

	//------------------------------------------------------------------------------------------------
	static bool IsNan(vector value)
	{
		return value.ToString(false).StartsWith("nan");
	}

	//------------------------------------------------------------------------------------------------
	static bool IsNan(float value)
	{
		return value.ToString() == "nan";
	}
};
