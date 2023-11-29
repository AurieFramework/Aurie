// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using Microsoft.Extensions.Options;
using Microsoft.Win32;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Runtime.InteropServices;
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
using Color = System.Windows.Media.Color;
using ColorConverter = System.Windows.Media.ColorConverter;
using TextBlock = Wpf.Ui.Controls.TextBlock;
using Wpf.Ui.Appearance;
using System.Threading;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using static AurieInstaller.ViewModels.Pages.DashboardViewModel;
using System.Windows.Shapes;
using Path = System.IO.Path;
using System.ComponentModel;

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
        private ProgressBar? downloadProgressBar;
        private TextBlock? fileNameText;
        private Canvas? modListCanvas;
        private Border? modListMask;
        private ListView? modListView;
        private Button? addModsButton;
        private Button? removeModsButton;
        private readonly SettingsManager settingsManager = new();
        private AppSettings settings = SettingsManager.LoadSettings();
        private bool canInstall = true;
        private bool mod_list_view_initialized = false;

        /*public const uint DONT_RESOLVE_DLL_REFERENCES = 0x00000001;

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FreeLibrary(IntPtr hModule);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct ModInfo
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string mod_name;
            public int version_major;
            public int version_minor;
            public int version_patch;
            public string mod_repo;
            public string mod_icon;
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AurieGetModInfoDelegate(ref ModInfo info);*/

        private static ISnackbarService? _snackbarService;

        public ISnackbarService SnackbarService
        {
            get => _snackbarService;
            set
            {
                _snackbarService = value;
                OnPropertyChanged(nameof(SnackbarService));
            }
        }

        private static IContentDialogService? _contentDialogService;

        public IContentDialogService ContentDialogService
        {
            get => _contentDialogService;
            set
            {
                _contentDialogService = value;
                OnPropertyChanged(nameof(ContentDialogService));
            }
        }

        private ObservableCollection<ModItem> mods;

        public ObservableCollection<ModItem> Mods
        {
            get { return mods; }
            set
            {
                if (mods != value)
                {
                    mods = value;
                    OnPropertyChanged(nameof(Mods));
                }
            }
        }

        public class ModItem : ObservableObject
        {
            private string? mod_name;
            public string? ModPath { get; set; }
            public string? ModName
            {
                get { return mod_name; }
                set
                {
                    if (mod_name != value)
                    {
                        mod_name = value;
                        OnPropertyChanged(nameof(ModName));
                    }
                }
            }
            public string? ModIcon { get; set; }
            public string? ModRepo { get; set; }
            public bool IsEnabled { get; set; }
            public bool HasUpdate { get; set; }
            public bool IsNative { get; set; }
        }

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

        public class UIElements
        {
            public Button? InstallButton { get; set; }
            public ComboBox? RunnerBox { get; set; }
            public ProgressBar? DownloadProgressBar { get; set; }
            public Button? PlayButton { get; set; }
            public TextBlock? FileNameText { get; set; }
            public Canvas? ModListCanvas { get; set; }
            public Border? ModListMask { get; set; }
            public ListView? ModListView { get; set; }
            public Button? AddModsButton { get; set; }
            public Button? RemoveModsButton {  get; set; }
        }

        public void SetUIElements(UIElements uIElements)
        {
            installButton = uIElements.InstallButton;
            runnerBox = uIElements.RunnerBox;
            downloadProgressBar = uIElements.DownloadProgressBar;
            playButton = uIElements.PlayButton;
            fileNameText = uIElements.FileNameText;
            modListCanvas = uIElements.ModListCanvas;
            modListMask = uIElements.ModListMask;
            modListView = uIElements.ModListView;
            addModsButton = uIElements.AddModsButton;
            removeModsButton = uIElements.RemoveModsButton;

            if (installButton != null)
            {
                Console.WriteLine("installButton found!");
            }

            if (addModsButton != null)
            {
                Console.WriteLine("addModsButton found!");
                addModsButton.Visibility = Visibility.Hidden;
            }

            if (removeModsButton != null)
            {
                Console.WriteLine("removeModsButton found!");
                removeModsButton.Visibility = Visibility.Hidden;
            }

            if (runnerBox != null)
            {
                Console.WriteLine("runnerBox found!");
                settings = SettingsManager.LoadSettings();
                runnerBox.Items.Clear();
                runnerBox.SelectedItem = null;
                runnerBox.Text = "Select a game...";
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

            if (downloadProgressBar != null)
            {
                Console.WriteLine("progressBar found!");
                downloadProgressBar.Visibility = Visibility.Hidden;
            }

            if (playButton != null)
            {
                Console.WriteLine("playButton found!");
                playButton.Visibility = Visibility.Hidden;
                if (runnerBox.Items.Count > 0)
                {
                    playButton.Visibility = Visibility.Visible;
                }
            }

            if (fileNameText != null)
            {
                Console.WriteLine("fileNameText found!");
                fileNameText.Visibility = Visibility.Hidden;
            }

            if (modListMask != null)
            {
                Console.WriteLine("modListMask found!");
                modListMask.Visibility = Visibility.Hidden;
                ThemeType theme = Theme.GetAppTheme();
                if (theme == ThemeType.Dark)
                {
                    modListMask.Background = (SolidColorBrush)new BrushConverter().ConvertFromString("#202020");
                }
                else if (theme == ThemeType.Light)
                {
                    modListMask.Background = (SolidColorBrush)new BrushConverter().ConvertFromString("#fafafa");
                }
            }

            if (modListView != null && mod_list_view_initialized == false)
            {
                Console.WriteLine("modListView found!");
                mod_list_view_initialized = true;
                modListView.Visibility = Visibility.Hidden;
                SetModList();
            }
        }

        public static SnackbarPresenter GetSnackbarPresenter()
        {
            return _snackbarService.GetSnackbarPresenter();
        }

        public ContentPresenter GetContentPresenter()
        {
            return _contentDialogService.GetContentPresenter();
        }

        private void SetModList()
        {
            Mods ??= new ObservableCollection<ModItem>();
            Mods.Clear();
            List<ModItem> tempMods = new();
            tempMods.Clear();
            if (runnerBox == null || runnerBox.SelectedItem == null) return;
            
            try
            {
                string runner_path = GetRunnerPathFromName(runnerBox.SelectedItem.ToString());
                string runner_directory = Directory.GetParent(runner_path)?.FullName ?? "";
                string[] aurieFiles = Directory.GetFiles(Path.Combine(runner_directory, "mods", "Aurie"), "*.dll")
                    .Union(Directory.GetFiles(Path.Combine(runner_directory, "mods", "Aurie"), "*.dll.disabled"))
                    .ToArray();
                string[] nativeFiles = Directory.GetFiles(Path.Combine(runner_directory, "mods", "Native"), "*.dll")
                    .Union(Directory.GetFiles(Path.Combine(runner_directory, "mods", "Native"), "*.dll.disabled"))
                    .ToArray();

                foreach (string file in aurieFiles)
                {
                    string mod_name = Path.GetFileName(file);
                    /*if (mod_name == "discord-rpc.dll")
                    {
                        Console.WriteLine($"'{file}'");
                        IntPtr module_handle = LoadLibraryEx(file, IntPtr.Zero, DONT_RESOLVE_DLL_REFERENCES);
                        if (module_handle != IntPtr.Zero)
                        {
                            IntPtr function_pointer = GetProcAddress(module_handle, "aurie_get_mod_info");
                            if (function_pointer != IntPtr.Zero)
                            {
                                AurieGetModInfoDelegate aurie_get_mod_info = Marshal.GetDelegateForFunctionPointer<AurieGetModInfoDelegate>(function_pointer);

                                ModInfo mod_info = new();
                                aurie_get_mod_info(ref mod_info);

                                Console.WriteLine($"Mod Name: {mod_info.mod_name}");
                                Console.WriteLine($"Version: {mod_info.version_major}.{mod_info.version_minor}.{mod_info.version_patch}");
                                Console.WriteLine($"Mod Repo: {mod_info.mod_repo}");
                            }
                            else
                            {
                                Console.WriteLine($"'aurie_get_mod_info' not found for {mod_name}!");
                            }
                            FreeLibrary(module_handle);
                        }
                        else
                        {
                            int errorCode = Marshal.GetLastWin32Error();
                            Console.WriteLine($"Cannot load library for {mod_name}! Error code: {errorCode}");
                        }
                    }*/
                    bool mod_enabled = !mod_name.Contains(".dll.disabled");
                    ModItem mod = new() {
                        ModPath = file,
                        ModName = mod_name,
                        IsEnabled = mod_enabled,
                        HasUpdate = false,
                        IsNative = false
                    };
                    tempMods.Add(mod);
                }

                foreach (string file in nativeFiles)
                {
                    string mod_name = Path.GetFileName(file);
                    bool mod_enabled = !mod_name.Contains(".dll.disabled");
                    ModItem mod = new() {
                        ModPath = file,
                        ModName = mod_name,
                        IsEnabled = mod_enabled,
                        HasUpdate = false,
                        IsNative = true
                    };
                    tempMods.Add(mod);
                }

                int yytkIndex = tempMods.FindIndex(mod => mod.ModName == "YYToolkit.dll");
                ModItem yytkMod = tempMods[yytkIndex];
                tempMods.RemoveAt(yytkIndex);
                tempMods.Insert(0, yytkMod);

                int aurieIndex = tempMods.FindIndex(mod => mod.ModName == "AurieCore.dll");
                ModItem aurieMod = tempMods[aurieIndex];
                tempMods.RemoveAt(aurieIndex);
                tempMods.Insert(0, aurieMod);

                foreach (ModItem mod in tempMods)
                {
                    Mods.Add(mod);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }

            addModsButton.Visibility = Visibility.Visible;
            removeModsButton.Visibility = Visibility.Visible;
            modListView.ItemsSource = Mods;
            if (Mods.Count > 0)
            {
                modListCanvas.Visibility = Visibility.Visible;
                modListView.Visibility = Visibility.Visible;
                modListMask.Visibility = Visibility.Visible;
                removeModsButton.IsEnabled = true;

                if (Mods.Count > 6)
                {
                    modListCanvas.Width = 300;
                    modListView.Width = 300;
                }
                else
                {
                    modListCanvas.Width = 286;
                    modListView.Width = 286;
                }
            }
            else
            {
                modListCanvas.Width = 286;
                modListView.Width = 286;
                removeModsButton.IsEnabled = false;
            }
        }

        internal static void ThrowError(string Message)
        {
            Snackbar snackbar = new(GetSnackbarPresenter()) {
                MinHeight = 0,
                Content = Message,
                Timeout = System.TimeSpan.FromSeconds(4),
                Appearance = ControlAppearance.Danger,
                VerticalContentAlignment = VerticalAlignment.Center
            };
            snackbar.Show();
            System.Media.SystemSounds.Exclamation.Play();
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
            SetModList();
        }

        public void OnRunnerChange(object sender, SelectionChangedEventArgs e)
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
                    string runner_directory = Path.GetDirectoryName(runner_path);
                    string registry_runner_path = runner_path.Replace('\\', '_');
                    // Check if the framework is already initialized
                    // Registry check is optional when running Wine
                    if ((runner_key.GetSubKeyNames().Contains(registry_runner_path) || IsRunningOnWine()) &&
                        CheckDirectoryStructure(runner_directory))
                    {
                        canInstall = false;
                        installButton.Content = "Uninstall Aurie";
                        installButton.BorderBrush = Brushes.Red;
                        Console.WriteLine($"{selectedRunnerName} already has Aurie installed!");
                    }
                    else
                    {
                        canInstall = true;
                        installButton.Content = "Install Aurie";
                        installButton.BorderBrush = Brushes.Green;
                        Console.WriteLine($"{selectedRunnerName} doesn't have Aurie installed yet!");
                    }
                    SetModList();
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
                                fileNameText.Text = "";
                                int totalFiles = localFilePaths.Count;
                                int downloadedFiles = 0;

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
                                                        long totalFileSize = fileResponse.Content.Headers.ContentLength ?? 0;
                                                        long bytesDownloaded = 0;
                                                        byte[] buffer = new byte[4096];
                                                        Console.WriteLine($"Writing to '{localFilePaths[fileName]}'...");
                                                        fileNameText.Text = $"Downloading {fileName}...";
                                                        downloadProgressBar.Visibility = Visibility.Visible;
                                                        fileNameText.Visibility = Visibility.Visible;
                                                        using (Stream stream = await fileResponse.Content.ReadAsStreamAsync())
                                                        {
                                                            using (FileStream fileStream = new FileStream(localFilePaths[fileName], FileMode.Create, FileAccess.Write))
                                                            {
                                                                int bytesRead;
                                                                while ((bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length)) > 0)
                                                                {
                                                                    await fileStream.WriteAsync(buffer, 0, bytesRead);

                                                                    bytesDownloaded += bytesRead;

                                                                    int fileProgress = (int)((double)bytesDownloaded / totalFileSize * 100);
                                                                    downloadProgressBar.Value = (int)(100.0 / totalFiles) * downloadedFiles + (int)((double)fileProgress / totalFiles);
                                                                }
                                                            }
                                                        }
                                                        downloadedFiles++;
                                                        Console.WriteLine($"File '{fileName}' downloaded to '{localFilePaths[fileName]}' successfully.");
                                                    } else Console.WriteLine($"Failed to download file '{fileName}'. Status code: {fileResponse.StatusCode}");
                                                } else Console.WriteLine($"Failed to get download URL for file '{fileName}'.");
                                            }
                                        } else Console.WriteLine("No download URLs found for the specified files.");
                                    } else Console.WriteLine($"Failed to get latest release information. Status code: {response.StatusCode}");
                                }
                            }

                            downloadProgressBar.Value = 100;
                            downloadProgressBar.Visibility = Visibility.Hidden;
                            fileNameText.Visibility = Visibility.Hidden;

                            await GetSnackbarPresenter().HideCurrent();
                            Snackbar snackbar = new(GetSnackbarPresenter()) {
                                MinHeight = 0,
                                Content = "Aurie Framework was installed successfully!",
                                Timeout = System.TimeSpan.FromSeconds(4),
                                Appearance = ControlAppearance.Success,
                                VerticalContentAlignment = VerticalAlignment.Center
                            };
                            snackbar.Show();
                            System.Media.SystemSounds.Hand.Play();
                            canInstall = false;
                            installButton.Content = "Uninstall Aurie";
                            installButton.BorderBrush = Brushes.Red;
                            SetModList();
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
                        await GetSnackbarPresenter().HideCurrent();
                        Snackbar snackbar = new(GetSnackbarPresenter()) {
                            MinHeight = 0,
                            Content = "Aurie Framework was uninstalled successfully!",
                            Timeout = TimeSpan.FromSeconds(4),
                            Appearance = ControlAppearance.Caution,
                            VerticalContentAlignment = VerticalAlignment.Center
                        };
                        System.Media.SystemSounds.Exclamation.Play();
                        snackbar.Show();
                        canInstall = true;
                        installButton.Content = "Install Aurie";
                        installButton.BorderBrush = Brushes.Green;
                        SetModList();
                    }
                }
            }
            catch (Exception ex)
            {
                ThrowError("Failed to access the required registry keys!\n" + ex.Message);
            }
        }

        internal static void RunCommand(string directory, string command, string arguments)
        {
            ProcessStartInfo process_start_info = new ProcessStartInfo
            {
                FileName = command.ToString(),
                Arguments = arguments.ToString(),
                WorkingDirectory = directory.ToString(),
                UseShellExecute = true,
                CreateNoWindow = true
            };

            using (Process process = new Process())
            {
                process.StartInfo = process_start_info;
                process.Start();
            }
        }

        [DllImport("ntdll.dll", SetLastError = true)]
        private static extern IntPtr wine_get_version();

        internal static bool IsRunningOnWine()
        {
            try
            {
                return wine_get_version() != IntPtr.Zero;
            }
            catch
            {
                return false;
            }
        }

        internal static bool CheckDirectoryStructure(string root_path)
        {
            string mods_path = Path.Combine(root_path, "mods");
            if (!Directory.Exists(mods_path))
                return false;

            string aurie_path = Path.Combine(mods_path, "Aurie");
            if (!Directory.Exists(aurie_path))
                return false;

            string native_path = Path.Combine(mods_path, "Native");
            if (!Directory.Exists(native_path))
                return false;

            string aurie_core_path = Path.Combine(native_path, "AurieCore.dll");
            if (!File.Exists(aurie_core_path))
                return false;

            string aurie_loader_path = Path.Combine(mods_path, "AurieLoader.exe");
            if (!File.Exists(aurie_loader_path))
                return false;

            return true;
        }

        [RelayCommand]
        private void OnPlayButton()
        {
            string runnerPath = GetRunnerPathFromName(settings.CurrentSelectedRunner);

            if (runnerPath == null)
                return;

            string directory = Path.GetDirectoryName(runnerPath);
            string loader = $"mods\\AurieLoader.exe";
            string arguments = $"{Path.GetFileName(runnerPath)}";

            try
            {
                using (RegistryKey? runner_key = GetRunnerKey(Path.GetFileName(runnerPath), true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    Console.WriteLine($"Starting {settings.CurrentSelectedRunner}...");

                    string registry_runner_path = runnerPath.Replace('\\', '_');
                    if ((runner_key.GetSubKeyNames().Contains(registry_runner_path) || IsRunningOnWine()) &&
                        CheckDirectoryStructure(directory))
                    {
                        Console.WriteLine("Running AurieLoader...");
                        
                        if (IsRunningOnWine())
                            arguments += " --debug";

                        RunCommand(directory, loader, arguments);
                    }
                    else
                    {
                        Process.Start(runnerPath);
                    }
                }
            }
            catch (Exception ex)
            {
                ThrowError("Failed to launch the game!\n" + ex.Message);
            }
        }

        [RelayCommand]
        private void OnAddModsButton()
        {
            string modName = runnerBox.SelectedItem.ToString();

            OpenFileDialog openFileDialog = new OpenFileDialog {
                Multiselect = true,
                Filter = "Mod files (*.dll)|*.dll",
                Title = $"Select mods to add to {modName}"
            };

            bool? result = openFileDialog.ShowDialog();

            if (result == true)
            {
                string[] selectedFiles = openFileDialog.FileNames;

                string runner_path = GetRunnerPathFromName(modName);
                string runner_directory = Directory.GetParent(runner_path)?.FullName ?? "";
                string destinationDirectory = Path.Combine(runner_directory, "mods", "Aurie");

                foreach (string file in selectedFiles)
                {
                    string fileName = Path.GetFileName(file);
                    string destinationPath = Path.Combine(destinationDirectory, fileName);

                    try
                    {
                        File.Copy(file, destinationPath, true);
                        Console.WriteLine($"File '{fileName}' copied to '{destinationPath}' successfully.");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error copying file '{fileName}': {ex.Message}");
                    }
                }
                SetModList();
            }
        }

        [RelayCommand]
        private async void OnRemoveModsButton()
        {
            if (modListView.SelectedItems.Count == 0)
            {
                await GetSnackbarPresenter().HideCurrent();
                Snackbar snackbar = new(GetSnackbarPresenter()) {
                    MinHeight = 0,
                    Content = "No mods are selected!",
                    Timeout = System.TimeSpan.FromSeconds(4),
                    Appearance = ControlAppearance.Danger,
                    VerticalContentAlignment = VerticalAlignment.Center
                };
                snackbar.Show();
                System.Media.SystemSounds.Exclamation.Play();
                return;
            }

            var contentDialog = new ContentDialog(GetContentPresenter());

            contentDialog.Title = "Delete Mods";
            contentDialog.Content = "Would you like to delete all of the selected mods?";
            contentDialog.PrimaryButtonText = "Delete";
            contentDialog.CloseButtonText = "Cancel";
            contentDialog.DialogHeight = 270;
            contentDialog.DialogWidth = 340;

            ContentDialogResult result = await contentDialog.ShowAsync();
            if (result == ContentDialogResult.Primary)
            {
                bool modsDeleted = false;
                Console.WriteLine("Deleting all selected mods...");
                foreach (Object selectedItem in modListView.SelectedItems)
                {
                    if (selectedItem is ModItem modItem)
                    {
                        if (modItem.ModName == "AurieCore.dll" || modItem.ModName == "YYToolkit.dll") continue;
                        string modPath = modItem.ModPath;

                        try
                        {
                            File.Delete(modPath);
                            modsDeleted = true;
                            Console.WriteLine($"Deleted {modPath}!");
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"Error deleting file {modPath}: {ex.Message}");
                        }
                    }
                }
                if (modsDeleted)
                {
                    Console.WriteLine("Settings mods!");
                    SetModList();
                }
            }
        }
    }
}
