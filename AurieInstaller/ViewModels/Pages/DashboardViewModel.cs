// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using Microsoft.Extensions.Options;
using Microsoft.Win32;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Windows.Input;

namespace AurieInstaller.ViewModels.Pages
{
    public partial class DashboardViewModel : ObservableObject
    {
        private static readonly string m_IFEOPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";

        internal static void ThrowError(string Message)
        {
            MessageBox.Show(
                Message,
                "Fatal Error",
                MessageBoxButton.OK,
                MessageBoxImage.Error
            );
        }

        internal static OpenFileDialog PickGame(string Title, string Filter)
        {
            OpenFileDialog runner_selection_dialog = new()
            {
                Title = Title,
                Filter = Filter
            };

            return runner_selection_dialog;
        }

        internal static RegistryKey? GetIFEOKey(bool WriteAccess)
        {
            return Registry.LocalMachine.OpenSubKey(m_IFEOPath, WriteAccess);
        }

        internal static RegistryKey? GetRunnerKey(string RunnerName, bool WriteAccess)
        {
            using RegistryKey? ifeo = GetIFEOKey(WriteAccess);
            return ifeo?.CreateSubKey(RunnerName, true);
        }

        internal static List<RegistryKey> GetAurieInstallerKeys()
        {
            List<RegistryKey> keys = new();

            using (RegistryKey? ifeo = GetIFEOKey(false))
            {
                if (ifeo is null)
                {
                    ThrowError("Failed to get runner IFEO key!");
                    return keys;
                }

                foreach (string subkey in ifeo.GetSubKeyNames())
                {
                    RegistryKey? key = ifeo.OpenSubKey(subkey, false);
                    if (key is null)
                        continue;

                    if (!key.GetValueNames().Contains("IsAurieInstallerKey"))
                        continue;

                    keys.Add(key);
                }
            }

            return keys;
        }

        [RelayCommand]
        private void OnUninstallButton()
        {
            OpenFileDialog runner_selection_dialog = PickGame("Select the game executable...", "PE Files|*.exe|All Files|*.*");

            // Make it not a nullable (thank you Issue!)
            bool filename_selected = runner_selection_dialog.ShowDialog() ?? false;
            if (!filename_selected)
                return;

            // Extract the required info from the dialog
            string runner_path = runner_selection_dialog.FileName;
            string runner_name = runner_selection_dialog.SafeFileName;

            try
            {
                using (RegistryKey? runner_key = GetRunnerKey(runner_name, true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    string registry_runner_path = runner_path.Replace('\\', '_');

                    if (!runner_key.GetValueNames().Contains("IsAurieInstallerKey"))
                    {
                        MessageBox.Show(
                            "The Aurie Framework registry key is corrupt.\nReinstall the framework and try again.",
                            "Key is corrupt.",
                            MessageBoxButton.OK,
                            MessageBoxImage.Error
                        );

                        return;
                    }

                    if (!runner_key.GetSubKeyNames().Contains(registry_runner_path))
                    {
                        MessageBox.Show(
                            "The Aurie Framework not installed for this executable!",
                            "Framework not installed",
                            MessageBoxButton.OK,
                            MessageBoxImage.Error
                        );

                        return;
                    }

                    runner_key.DeleteSubKeyTree(registry_runner_path);

                    // If there's no more stuff registered for this runner key, remove the runner key completely
                    if (runner_key.GetSubKeyNames().Length == 0)
                    {
                        using RegistryKey? ifeo = GetIFEOKey(true);
                        ifeo?.DeleteSubKeyTree(runner_name);
                    }
                }

                MessageBox.Show(
                    "Aurie Framework was uninstalled successfully.",
                    "Success!",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information
                );
            }
            catch (Exception ex)
            {
                ThrowError("Failed to access the required registry keys!\n" + ex.Message);
            }
        }

        [RelayCommand]
        private void OnInstallButton()
        {
            // Initialize the dialog, show .exe files by default
            OpenFileDialog runner_selection_dialog = PickGame("Select the game executable...", "PE Files|*.exe|All Files|*.*");

            // Make it not a nullable (thank you Issue!)
            bool filename_selected = runner_selection_dialog.ShowDialog() ?? false;
            if (!filename_selected) 
                return;

            // Extract the required info from the dialog
            string runner_path = runner_selection_dialog.FileName;
            string runner_name = runner_selection_dialog.SafeFileName;

            try
            {
                using (RegistryKey? runner_key = GetRunnerKey(runner_name, true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    // Set up the UseFilter registry key
                    runner_key.SetValue("IsAurieInstallerKey", 1, RegistryValueKind.DWord);
                    runner_key.SetValue("UseFilter", 1, RegistryValueKind.DWord);

                    string registry_runner_path = runner_path.Replace('\\', '_');
                    // Check if the framework is already initialized
                    if (runner_key.GetSubKeyNames().Contains(registry_runner_path))
                    {
                        MessageBox.Show(
                            "The Aurie Framework is already installed for this executable!",
                            "Framework already installed",
                            MessageBoxButton.OK,
                            MessageBoxImage.Warning
                        );

                        return;
                    }

                    // Create the filter subkey with our name
                    using (RegistryKey? filter_subkey = runner_key.CreateSubKey(registry_runner_path, true))
                    {
                        if (filter_subkey is null)
                        {
                            ThrowError("Failed to create runner filter subkey!");
                            return;
                        }

                        // Craft the path to %GAMEFOLDER%\\mods
                        // TODO: Create an entry in %localappdata% that has the loader so that
                        // one loader can be reused for multiple games.
                        string runner_directory = Directory.GetParent(runner_path)?.FullName ?? "";
                        if (runner_directory.Equals(string.Empty))
                        {
                            ThrowError("Failed to get game folder!");
                            return;
                        }

                        // Create the mods directory
                        Directory.CreateDirectory(Path.Combine(runner_directory, "mods"));

                        // Create subdirectories
                        Directory.CreateDirectory(Path.Combine(runner_directory, "mods", "Aurie"));
                        Directory.CreateDirectory(Path.Combine(runner_directory, "mods", "Native"));

                        filter_subkey.SetValue("Debugger", Path.Combine(runner_directory, "mods", "AurieLoader.exe"));
                        filter_subkey.SetValue("FilterFullPath", runner_path);
                    }
                }

                MessageBox.Show(
                    "Aurie Framework was installed successfully.", 
                    "Success!",
                    MessageBoxButton.OK, 
                    MessageBoxImage.Information
                );
            }
            catch (Exception ex)
            {
                ThrowError("Failed to access the required registry keys!\n" + ex.Message);
            }
        }
    }
}
