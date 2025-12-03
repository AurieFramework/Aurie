namespace AurieInstaller
{
	internal static class FileUtils
	{
		/// <summary>
		/// Checks if a file is writable.
		/// </summary>
		/// <param name="FilePath">Absolute path to the file.</param>
		/// <returns>True if the file exists and is writable. Otherwise false.</returns>
		public static bool IsFileWritable(string FilePath)
		{
			try
			{
				using var stream = new FileStream(
					FilePath,
					FileMode.Open,
					FileAccess.Write,
					FileShare.None
				);
			}
			catch { return false; }

			return true;
		}

		/// <summary>
		/// Pops up a customized OpenFileDialog based off parameters.
		/// </summary>
		/// <param name="DialogName">
		/// The title of the dialog window.
		/// </param>
		/// <param name="FileTypes">
		/// The file types that are selectable. Refer to MSDN. First one is selected implicitely.
		/// </param>
		/// <param name="SelectedFilePath">
		/// The absolute path to the selected file (if returns true). Otherwise an empty string.
		/// </param>
		/// <returns>
		/// True if a file has been selected, false otherwise.
		/// </returns>
		public static bool SelectFile(string DialogName, string FileTypes, out string SelectedFilePath)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			ofd.Title = DialogName;
			ofd.ValidateNames = true;
			ofd.Filter = FileTypes;
			ofd.Multiselect = false;
			ofd.FilterIndex = 1;

			bool success = ofd.ShowDialog() == DialogResult.OK;
			SelectedFilePath = ofd.FileName;

			return success;
		}

		public static void CreateModDirectoryStructure(string BaseFolder)
		{
            Directory.CreateDirectory(Path.Combine(BaseFolder, "mods", "aurie"));
            Directory.CreateDirectory(Path.Combine(BaseFolder, "mods", "native"));
			Directory.CreateDirectory(Path.Combine(BaseFolder, "mods", "managed"));
            Directory.CreateDirectory(Path.Combine(BaseFolder, "mods", "bin"));
        }
    }
}
