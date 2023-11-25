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
using System.Net.Http;
using System.Threading.Tasks;
using System.Text.Json;
using System.Windows.Controls;
using System.Drawing;
using System.Windows.Media;
using Brushes = System.Windows.Media.Brushes;
using System.Diagnostics;
using Wpf.Ui.Controls;
using Button = Wpf.Ui.Controls.Button;
using MessageBox = System.Windows.MessageBox;
using MessageBoxButton = System.Windows.MessageBoxButton;
using Color = System.Windows.Media.Color;
using ColorConverter = System.Windows.Media.ColorConverter;

namespace AurieInstaller.ViewModels.Pages
{
    public class RunnerData
    {
        public required string Name { get; set; }
        public required string Path { get; set; }
    }

    public class AppSettings
    {
        public List<RunnerData> AddedRunners { get; set; } = new List<RunnerData>();
        public string? CurrentSelectedRunner { get; set; }
    }

    public class SettingsManager
    {
        private const string SettingsFileName = "aurie-config.json";
        private static readonly string AppDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Aurie");
        private static readonly string SettingsFilePath = Path.Combine(AppDataPath, SettingsFileName);

        public static AppSettings LoadSettings()
        {
            if (File.Exists(SettingsFilePath))
            {
                string json = File.ReadAllText(SettingsFilePath);
                return JsonSerializer.Deserialize<AppSettings>(json);
            }
            else
            {
                return new AppSettings();
            }
        }

        public void SaveSettings(AppSettings settings)
        {
            string json = JsonSerializer.Serialize(settings, new JsonSerializerOptions
            {
                WriteIndented = true
            });
            Directory.CreateDirectory(AppDataPath);
            File.WriteAllText(SettingsFilePath, json);
        }
    }

    public partial class DashboardViewModel : ObservableObject
    {
        private static readonly string m_IFEOPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
        private ComboBox? runnerBox;
        private Button? installButton;
        private Button? playButton;
        private readonly SettingsManager settingsManager = new();
        private AppSettings settings = SettingsManager.LoadSettings();
        private bool canInstall = true;

        public bool CanInstall {
            get { return canInstall; }
            set
            {
                if (canInstall != value)
                {
                    canInstall = value;
                    OnPropertyChanged(nameof(CanInstall));
                }
            }
        }

        public string? GetRunnerPathFromName(string name)
        {
            foreach (RunnerData runner in settings.AddedRunners)
            {
                if (runner.Name == name) return runner.Path;
            }
            return null;
        }

        public void SetInstallButton(Button installButton)
        {
            this.installButton = installButton;
            if (installButton != null)
            {
                Console.WriteLine("installButton found!");
            }
        }
        public void SetRunnerBox(ComboBox runnerBox)
        {
            this.runnerBox = runnerBox;
            if (runnerBox != null)
            {
                Console.WriteLine("runnerBox found!");
                settings = SettingsManager.LoadSettings();
                runnerBox.Items.Clear();
                runnerBox.SelectedItem = null;
                runnerBox.Text = "Select a runner...";
                runnerBox.IsEnabled = false;
                installButton.Visibility = Visibility.Hidden;

                foreach (RunnerData runner in settings.AddedRunners)
                {
                    runnerBox.Items.Add(runner.Name);
                }

                if (runnerBox.Items.Count > 0)
                {
                    runnerBox.IsEnabled = true;
                    installButton.Visibility = Visibility.Visible;
                }

                if (settings.CurrentSelectedRunner != null)
                {
                    runnerBox.SelectedItem = settings.CurrentSelectedRunner;
                }
            }
        }

        public void SetPlayButton(Button playButton)
        {
            this.playButton = playButton;
            if (playButton != null)
            {
                Console.WriteLine("playButton found!");
                playButton.Visibility = Visibility.Hidden;
                if (runnerBox.Items.Count > 0)
                {
                    playButton.Visibility = Visibility.Visible;
                }
            }
        }

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
            OpenFileDialog runner_selection_dialog = new() {
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

        internal static Dictionary<string, string> GetDownloadUrls(string json, string[] fileNames)
        {
            Dictionary<string, string> downloadUrls = new();

            using (JsonDocument doc = JsonDocument.Parse(json))
            {
                JsonElement root = doc.RootElement;

                if (root.TryGetProperty("assets", out JsonElement assetsArray) && assetsArray.ValueKind == JsonValueKind.Array)
                {
                    foreach (JsonElement asset in assetsArray.EnumerateArray())
                    {
                        string assetName = asset.GetProperty("name").GetString();
                        string downloadUrl = asset.GetProperty("browser_download_url").GetString();

                        if (fileNames.Contains(assetName))
                        {
                            downloadUrls[assetName] = downloadUrl;
                        }
                    }
                }
            }

            return downloadUrls;
        }

        [RelayCommand]
        private void OnBrowseRunner()
        {
            OpenFileDialog runner_selection_dialog = PickGame("Select the game executable...", "PE Files|*.exe|All Files|*.*");
            // Make it not a nullable (thank you Issue!)
            bool filename_selected = runner_selection_dialog.ShowDialog() ?? false;
            if (!filename_selected)
                return;

            // Extract the required info from the dialog
            string runner_path = runner_selection_dialog.FileName;
            string runner_name = runner_selection_dialog.SafeFileName;

            if (!runnerBox.Items.Contains(runner_name))
            {
                runnerBox.Items.Add(runner_name);
                RunnerData newRunner = new() {
                    Name = runner_name,
                    Path = runner_path
                };
                settings.AddedRunners.Add(newRunner);
            }
            runnerBox.IsEnabled = true;
            installButton.Visibility = Visibility.Visible;
            playButton.Visibility = Visibility.Visible;
            runnerBox.SelectedItem = runner_name;
            settings.CurrentSelectedRunner = runner_name;

            settingsManager.SaveSettings(settings);
        }

        public void OnRunnerChange(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (runnerBox != null && runnerBox.SelectedItem != null)
            {
                string selectedRunnerName = (string)runnerBox.SelectedItem;
                settings.CurrentSelectedRunner = selectedRunnerName;
                settingsManager.SaveSettings(settings);
                Console.WriteLine(selectedRunnerName);

                using (RegistryKey? runner_key = GetRunnerKey(selectedRunnerName, true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    // Set up the UseFilter registry key
                    runner_key.SetValue("IsAurieInstallerKey", 1, RegistryValueKind.DWord);
                    runner_key.SetValue("UseFilter", 1, RegistryValueKind.DWord);

                    string runner_path = GetRunnerPathFromName(selectedRunnerName);
                    string registry_runner_path = runner_path.Replace('\\', '_');
                    // Check if the framework is already initialized
                    if (runner_key.GetSubKeyNames().Contains(registry_runner_path))
                    {
                        canInstall = false;
                        installButton.Content = "Uninstall Aurie";
                        installButton.Background = Brushes.DarkRed;
                        installButton.MouseOverBackground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#B20000"));
                        Console.WriteLine($"{selectedRunnerName} already has Aurie installed!");
                        return;
                    }
                    else
                    {
                        canInstall = true;
                        installButton.Content = "Install Aurie";
                        installButton.Background = Brushes.DarkGreen;
                        installButton.MouseOverBackground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#007800"));
                        Console.WriteLine($"{selectedRunnerName} doesn't have Aurie installed yet!");
                        return;
                    }
                }
            }
        }

        [RelayCommand]
        private async void OnInstallButton()
        {
            if (runnerBox.SelectedItem == null)
            {
                Console.WriteLine("No runner currently selected!");
                return;
            }
            string runner_name = (string)runnerBox.SelectedItem;
            string runner_path = GetRunnerPathFromName(runner_name);
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
                    if (canInstall == true)
                    {
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

                            string owner = "AurieFramework";
                            string[] repos = { "Aurie", "YYToolkit" };

                            Dictionary<string, string[]> fileNames = new()
                            {
                                { "Aurie", new string[] { "AurieLoader.exe", "AurieCore.dll" } },
                                { "YYToolkit", new string[] { "YYToolkit.dll" } }
                            };

                            Dictionary<string, string> localFilePaths = new()
                            {
                                { "AurieLoader.exe", Path.Combine(runner_directory, "mods", "AurieLoader.exe") },
                                { "AurieCore.dll", Path.Combine(runner_directory, "mods", "Native", "AurieCore.dll") },
                                { "YYToolkit.dll", Path.Combine(runner_directory, "mods", "Aurie", "YYToolkit.dll") }
                            };

                            using (HttpClient client = new())
                            {
                                foreach (string repo in repos)
                                {
                                    client.DefaultRequestHeaders.UserAgent.ParseAdd("Aurie");
                                    client.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github.v3+json");

                                    string apiUrl = $"https://api.github.com/repos/{owner}/{repo}/releases/latest";
                                    HttpResponseMessage response = await client.GetAsync(apiUrl);

                                    if (response.IsSuccessStatusCode)
                                    {
                                        string responseContent = await response.Content.ReadAsStringAsync();
                                        fileNames.TryGetValue(repo, out string[] fileNamesArray);
                                        Dictionary<string, string> downloadUrls = GetDownloadUrls(responseContent, fileNamesArray);

                                        if (downloadUrls.Count > 0)
                                        {
                                            foreach (string fileName in fileNamesArray)
                                            {
                                                if (downloadUrls.TryGetValue(fileName, out string downloadUrl))
                                                {
                                                    HttpResponseMessage fileResponse = await client.GetAsync(downloadUrl);

                                                    if (fileResponse.IsSuccessStatusCode)
                                                    {
                                                        byte[] fileBytes = await fileResponse.Content.ReadAsByteArrayAsync();
                                                        if (localFilePaths.TryGetValue(fileName, out string localFilePath))
                                                        {
                                                            Console.WriteLine($"Writing to '{localFilePath}'...");
                                                            System.IO.File.WriteAllBytes(localFilePath, fileBytes);
                                                            Console.WriteLine($"File '{fileName}' downloaded to '{localFilePath}' successfully.");
                                                        } else Console.WriteLine($"Local path not specified for file '{fileName}'.");
                                                    } else Console.WriteLine($"Failed to download file '{fileName}'. Status code: {fileResponse.StatusCode}");
                                                } else Console.WriteLine($"Failed to get download URL for file '{fileName}'.");
                                            }
                                        } else Console.WriteLine("No download URLs found for the specified files.");
                                    } else Console.WriteLine($"Failed to get latest release information. Status code: {response.StatusCode}");
                                }
                            }
                            MessageBox.Show(
                                "Aurie Framework was installed successfully.",
                                "Success!",
                                MessageBoxButton.OK,
                                MessageBoxImage.Information
                            );
                            canInstall = false;
                            installButton.Content = "Uninstall Aurie";
                            installButton.Background = Brushes.DarkRed;
                            installButton.MouseOverBackground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#B20000"));
                        }
                    }
                    else if (canInstall == false)
                    {
                        runner_key.DeleteSubKeyTree(registry_runner_path);

                        // If there's no more stuff registered for this runner key, remove the runner key completely
                        if (runner_key.GetSubKeyNames().Length == 0)
                        {
                            using RegistryKey? ifeo = GetIFEOKey(true);
                            ifeo?.DeleteSubKeyTree(runner_name);
                        }

                        MessageBox.Show(
                            "Aurie Framework was uninstalled successfully.",
                            "Success!",
                            MessageBoxButton.OK,
                            MessageBoxImage.Information
                        );
                        canInstall = true;
                        installButton.Content = "Install Aurie";
                        installButton.Background = Brushes.DarkGreen;
                        installButton.MouseOverBackground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#007800"));
                    }
                }
            }
            catch (Exception ex)
            {
                ThrowError("Failed to access the required registry keys!\n" + ex.Message);
            }
        }

        static void RunCommand(string directory, string command, string arguments)
        {
            ProcessStartInfo processStartInfo = new ProcessStartInfo
            {
                FileName = command,
                Arguments = arguments,
                WorkingDirectory = directory,
                UseShellExecute = true,
                CreateNoWindow = true
            };

            using (Process process = new Process())
            {
                process.StartInfo = processStartInfo;
                process.Start();
            }
        }

        [RelayCommand]
        private void OnPlayButton()
        {
            string runnerPath = GetRunnerPathFromName(settings.CurrentSelectedRunner);

            string winePrefix = Environment.GetEnvironmentVariable("WINEPREFIX");

            if (runnerPath != null)
            {
                if (winePrefix != null)
                {
                    Console.WriteLine("Running on Wine");
                    string directory = Path.GetDirectoryName(runnerPath);

                    string loader = $"mods\\AurieLoader.exe";
                    string arguments = $"{runnerPath} --debug";

                    RunCommand(directory, loader, arguments);
                }
                else
                {
                    Console.WriteLine("Not running on Wine");
                    Console.WriteLine($"Starting {settings.CurrentSelectedRunner}...");
                    Process.Start(runnerPath);
                }
            }
        }
    }
}
