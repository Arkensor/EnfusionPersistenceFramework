class EPF_SavaDataUtils
{
	//------------------------------------------------------------------------------------------------
	static bool StructAutoCompare(notnull Managed a, notnull Managed b, bool floatingPrecision = 5)
	{
		return GetCleanedComparisonString(a, floatingPrecision) == GetCleanedComparisonString(b, floatingPrecision);
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetCleanedComparisonString(notnull Managed inst, bool floatingPrecision)
	{
		SCR_JsonSaveContext writer();
		JsonSaveContainer.Cast(writer.GetContainer()).SetMaxDecimalPlaces(floatingPrecision);
		if (!writer.WriteValue("", inst))
			return string.Empty;

		// Replace the UTC timestamp with XXXX so that the compare for the other data can be true
		string data = writer.ExportToString();
		int layoutStart = data.IndexOf("m_iDataLayoutVersion");
		int lastSavedStop = layoutStart + 49;
		return data.Substring(0, layoutStart + 39) + "XXXXXXXXXX" + data.Substring(lastSavedStop, data.Length() - lastSavedStop);
	}
};
