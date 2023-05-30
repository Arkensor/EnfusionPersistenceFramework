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
	//! NaN safe almost equal. nan == not nan will be false
	static bool IsNanEqual(vector v1, vector v2, float epsilon = 0.0001)
	{
		// nan <-> !nan
		bool isNanV1 = IsNan(v1);
		bool isNanV2 = IsNan(v2);
		if (isNanV1 && isNanV2)
			return true;

		if (isNanV1 != isNanV2)
			return false;

		return vector.Distance(v1, v2) < epsilon;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsNan(float value)
	{
		return value.ToString() == "nan";
	}

	//------------------------------------------------------------------------------------------------
	//! NaN safe almost equal. nan == not nan will be false
	static bool IsNanEqual(float f1, float f2, float epsilon = 0.0001)
	{
		// nan <-> !nan
		bool isNanF1 = IsNan(f1);
		bool isNanF2 = IsNan(f2);
		if (isNanF1 && isNanF2)
			return true;

		if (isNanF1 != isNanF2)
			return false;

		return float.AlmostEqual(f1, f2, epsilon);
	}
};
