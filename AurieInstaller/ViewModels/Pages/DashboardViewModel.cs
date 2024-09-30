// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using Microsoft.Win32;
using System.IO;
using System.Runtime.InteropServices;
using System.Net.Http;
using System.Text.Json;
using System.Windows.Controls;
using System.Windows.Media;
using Brushes = System.Windows.Media.Brushes;
using System.Diagnostics;
using Wpf.Ui.Controls;
using Button = Wpf.Ui.Controls.Button;
using TextBlock = Wpf.Ui.Controls.TextBlock;
using Wpf.Ui.Appearance;
using System.Collections.ObjectModel;
using Path = System.IO.Path;
using Wpf.Ui;

namespace AurieInstaller.ViewModels.Pages
{
    public class RunnerData
    {
        public required string m_Name { get; set; }
        public required string m_Path { get; set; }
    }

    public class AppSettings
    {
        public List<RunnerData> m_AddedRunners { get; set; } = new List<RunnerData>();
        public string? m_CurrentSelectedRunner { get; set; }
        public ApplicationTheme m_CurrentSelectedTheme { get; set; } = ApplicationTheme.Dark;
    }

    public class SettingsManager
    {
        private const string settings_file_name = "aurie-config.json";
        private static readonly string app_data_path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Aurie");
        private static readonly string settings_file_path = Path.Combine(app_data_path, settings_file_name);

        public static AppSettings LoadSettings()
        {
            if (File.Exists(settings_file_path))
            {
                string json = File.ReadAllText(settings_file_path);
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
            Directory.CreateDirectory(app_data_path);
            File.WriteAllText(settings_file_path, json);
        }
    }

    public partial class DashboardViewModel : ObservableObject
    {
        private static readonly string m_IFEOPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
        private ComboBox? runner_box;
        private Button? install_button;
        private Button? play_button;
        private ProgressBar? download_progress_bar;
        private TextBlock? file_name_text;
        private Canvas? mod_list_canvas;
        private Border? mod_list_mask;
        private ListView? mod_list_view;
        private Button? add_mods_button;
        private Button? remove_mods_button;
        private readonly SettingsManager settings_manager = new();
        private AppSettings settings = SettingsManager.LoadSettings();
        private bool can_install = true;

        private static ISnackbarService? snackbar_service;

        public ISnackbarService m_SnackbarService
        {
            get => snackbar_service;
            set
            {
                snackbar_service = value;
                OnPropertyChanged(nameof(m_SnackbarService));
            }
        }

        private static IContentDialogService? content_dialog_service;

        public IContentDialogService m_ContentDialogService
        {
            get => content_dialog_service;
            set
            {
                content_dialog_service = value;
                OnPropertyChanged(nameof(m_ContentDialogService));
            }
        }

        private ObservableCollection<ModItem> mods;

        public ObservableCollection<ModItem> m_Mods
        {
            get { return mods; }
            set
            {
                if (mods != value)
                {
                    mods = value;
                    OnPropertyChanged(nameof(m_Mods));
                }
            }
        }

        public class ModItem : ObservableObject
        {
            private string? mod_name;
            public string? m_ModPath { get; set; }
            public string? m_ModName
            {
                get { return mod_name; }
                set
                {
                    if (mod_name != value)
                    {
                        mod_name = value;
                        OnPropertyChanged(nameof(m_ModName));
                    }
                }
            }
            public string? m_ModIcon { get; set; }
            public string? m_ModRepo { get; set; }
            public bool m_IsEnabled { get; set; }
            public bool m_HasUpdate { get; set; }
            public bool m_IsNative { get; set; }
        }

        public bool m_CanInstall {
            get { return can_install; }
            set
            {
                if (can_install != value)
                {
                    can_install = value;
                    OnPropertyChanged(nameof(m_CanInstall));
                }
            }
        }

        public string? GetRunnerPathFromName(string name)
        {
            foreach (RunnerData runner in settings.m_AddedRunners)
            {
                if (runner.m_Name == name) return runner.m_Path;
            }
            return null;
        }

        public class UIElements
        {
            public Button? m_InstallButton { get; set; }
            public ComboBox? m_RunnerBox { get; set; }
            public ProgressBar? m_DownloadProgressBar { get; set; }
            public Button? m_PlayButton { get; set; }
            public TextBlock? m_FileNameText { get; set; }
            public Canvas? m_ModListCanvas { get; set; }
            public Border? m_ModListMask { get; set; }
            public ListView? m_ModListView { get; set; }
            public Button? m_AddModsButton { get; set; }
            public Button? m_RemoveModsButton {  get; set; }
        }

        public void SetUIElements(UIElements UIElements)
        {
            install_button = UIElements.m_InstallButton;
            runner_box = UIElements.m_RunnerBox;
            download_progress_bar = UIElements.m_DownloadProgressBar;
            play_button = UIElements.m_PlayButton;
            file_name_text = UIElements.m_FileNameText;
            mod_list_canvas = UIElements.m_ModListCanvas;
            mod_list_mask = UIElements.m_ModListMask;
            mod_list_view = UIElements.m_ModListView;
            add_mods_button = UIElements.m_AddModsButton;
            remove_mods_button = UIElements.m_RemoveModsButton;

            if (install_button != null)
            {
                Console.WriteLine("install_button found!");
            }

            if (add_mods_button != null)
            {
                Console.WriteLine("add_mods_button found!");
                add_mods_button.Visibility = Visibility.Hidden;
            }

            if (remove_mods_button != null)
            {
                Console.WriteLine("remove_mods_button found!");
                remove_mods_button.Visibility = Visibility.Hidden;
            }

            if (runner_box != null)
            {
                Console.WriteLine("runner_box found!");
                settings = SettingsManager.LoadSettings();
                runner_box.Items.Clear();
                runner_box.SelectedItem = null;
                runner_box.Text = "Select a game...";
                runner_box.IsEnabled = false;
                install_button.Visibility = Visibility.Hidden;

                foreach (RunnerData runner in settings.m_AddedRunners)
                {
                    runner_box.Items.Add(runner.m_Name);
                }

                if (runner_box.Items.Count > 0)
                {
                    runner_box.IsEnabled = true;
                    install_button.Visibility = Visibility.Visible;
                }

                if (settings.m_CurrentSelectedRunner != null)
                {
                    runner_box.SelectedItem = settings.m_CurrentSelectedRunner;
                }
            }

            if (download_progress_bar != null)
            {
                Console.WriteLine("progress_bar found!");
                download_progress_bar.Visibility = Visibility.Hidden;
            }

            if (play_button != null)
            {
                Console.WriteLine("play_button found!");
                play_button.Visibility = Visibility.Hidden;
                if (runner_box.Items.Count > 0)
                {
                    play_button.Visibility = Visibility.Visible;
                }
            }

            if (file_name_text != null)
            {
                Console.WriteLine("file_name_text found!");
                file_name_text.Visibility = Visibility.Hidden;
            }

            if (mod_list_mask != null)
            {
                Console.WriteLine("mod_list_mask found!");
                mod_list_mask.Visibility = Visibility.Hidden;
                ApplicationTheme theme = ApplicationThemeManager.GetAppTheme();
                if (theme == ApplicationTheme.Dark)
                {
                    mod_list_mask.Background = (SolidColorBrush)new BrushConverter().ConvertFromString("#202020");
                }
                else if (theme == ApplicationTheme.Light)
                {
                    mod_list_mask.Background = (SolidColorBrush)new BrushConverter().ConvertFromString("#fafafa");
                }
            }

            if (mod_list_view != null)
            {
                Console.WriteLine("mod_list_view found!");
                mod_list_view.Visibility = Visibility.Hidden;
                SetModList("SetUIElements");
            }
        }

        public static SnackbarPresenter GetSnackbarPresenter()
        {
            return snackbar_service.GetSnackbarPresenter();
        }

        public ContentPresenter GetContentPresenter()
        {
            return content_dialog_service.GetContentPresenter();
        }

        private void SetModList(string Source)
        {
            Console.WriteLine($"CALLING SETMODLIST! SOURCE: {Source}");
            m_Mods ??= new ObservableCollection<ModItem>();
            m_Mods.Clear();
            List<ModItem> temp_mods = new();
            temp_mods.Clear();
            if (runner_box == null || runner_box.SelectedItem == null) return;
            
            try
            {
                string runner_path = GetRunnerPathFromName(runner_box.SelectedItem.ToString());
                string runner_directory = Directory.GetParent(runner_path)?.FullName ?? "";
                string[] aurie_files = Directory.GetFiles(Path.Combine(runner_directory, "mods", "Aurie"), "*.dll")
                    .Union(Directory.GetFiles(Path.Combine(runner_directory, "mods", "Aurie"), "*.dll.disabled"))
                    .ToArray();
                string[] native_files = Directory.GetFiles(Path.Combine(runner_directory, "mods", "Native"), "*.dll")
                    .Union(Directory.GetFiles(Path.Combine(runner_directory, "mods", "Native"), "*.dll.disabled"))
                    .ToArray();

                foreach (string file in aurie_files)
                {
                    string mod_name = Path.GetFileName(file);
 
                    bool mod_enabled = !mod_name.Contains(".dll.disabled");
                    ModItem mod = new() {
                        m_ModPath = file,
                        m_ModName = mod_name,
                        m_IsEnabled = mod_enabled,
                        m_HasUpdate = false,
                        m_IsNative = false
                    };
                    temp_mods.Add(mod);
                }

                foreach (string file in native_files)
                {
                    string mod_name = Path.GetFileName(file);
                    bool mod_enabled = !mod_name.Contains(".dll.disabled");
                    ModItem mod = new() {
                        m_ModPath = file,
                        m_ModName = mod_name,
                        m_IsEnabled = mod_enabled,
                        m_HasUpdate = false,
                        m_IsNative = true
                    };
                    temp_mods.Add(mod);
                }

                int yytk_index = temp_mods.FindIndex(mod => mod.m_ModName == "YYToolkit.dll");
                ModItem yytk_mod = temp_mods[yytk_index];
                temp_mods.RemoveAt(yytk_index);
                temp_mods.Insert(0, yytk_mod);

                int aurie_index = temp_mods.FindIndex(mod => mod.m_ModName == "AurieCore.dll");
                ModItem aurie_mod = temp_mods[aurie_index];
                temp_mods.RemoveAt(aurie_index);
                temp_mods.Insert(0, aurie_mod);

                foreach (ModItem mod in temp_mods)
                {
                    m_Mods.Add(mod);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }

            add_mods_button.Visibility = Visibility.Visible;
            remove_mods_button.Visibility = Visibility.Visible;
            if (m_Mods.Count > 0)
            {
                mod_list_canvas.Visibility = Visibility.Visible;
                mod_list_view.Visibility = Visibility.Visible;
                mod_list_mask.Visibility = Visibility.Visible;
                remove_mods_button.IsEnabled = true;

                if (m_Mods.Count > 6)
                {
                    mod_list_canvas.Width = 300;
                    mod_list_view.Width = 300;
                }
                else
                {
                    mod_list_canvas.Width = 286;
                    mod_list_view.Width = 286;
                }
            }
            else
            {
                mod_list_canvas.Width = 286;
                mod_list_view.Width = 286;
                remove_mods_button.IsEnabled = false;
            }
        }

        internal static void ThrowError(string Message)
        {
            Snackbar snackbar = new(GetSnackbarPresenter()) {
                MinHeight = 0,
                Content = Message,
                Timeout = TimeSpan.FromSeconds(4),
                Appearance = ControlAppearance.Danger,
                VerticalContentAlignment = VerticalAlignment.Center
            };
            snackbar.Show();
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

        internal static Dictionary<string, string> GetDownloadUrls(string Json, string[] FileNames)
        {
            Dictionary<string, string> download_urls = new();

            using (JsonDocument doc = JsonDocument.Parse(Json))
            {
                JsonElement root = doc.RootElement;

                if (root.TryGetProperty("assets", out JsonElement assets_array) && assets_array.ValueKind == JsonValueKind.Array)
                {
                    foreach (JsonElement asset in assets_array.EnumerateArray())
                    {
                        string? asset_name = asset.GetProperty("name").GetString();
                        string? download_url = asset.GetProperty("browser_download_url").GetString();

                        // Skip null entries
                        if (asset_name is null || download_url is null)
                            continue;

                        if (FileNames.Contains(asset_name))
                        {
                            download_urls[asset_name] = download_url;
                        }
                    }
                }
            }

            return download_urls;
        }

        [RelayCommand]
        private void OnBrowseRunner()
        {
            OpenFileDialog runner_selection_dialog = PickGame("Select the game executable...", "Game Executables|*.exe");
            // Make it not a nullable (thank you Issue!)
            bool filename_selected = runner_selection_dialog.ShowDialog() ?? false;
            if (!filename_selected)
                return;

            // Extract the required info from the dialog
            string runner_path = runner_selection_dialog.FileName;
            string runner_name = runner_selection_dialog.SafeFileName;

            if (!runner_box.Items.Contains(runner_name))
            {
                RunnerData new_runner = new() {
                    m_Name = runner_name,
                    m_Path = runner_path
                };
                settings.m_AddedRunners.Add(new_runner);
                runner_box.Items.Add(runner_name);
            }
            runner_box.IsEnabled = true;
            install_button.Visibility = Visibility.Visible;
            play_button.Visibility = Visibility.Visible;
            runner_box.SelectedItem = runner_name;
            settings.m_CurrentSelectedRunner = runner_name;

            settings_manager.SaveSettings(settings);
            SetModList("OnBrowseRunner");
        }

        public void OnRunnerChange(object sender, SelectionChangedEventArgs e)
        {
            if (runner_box != null && runner_box.SelectedItem != null)
            {
                string selected_runner_name = (string)runner_box.SelectedItem;
                settings.m_CurrentSelectedRunner = selected_runner_name;
                settings_manager.SaveSettings(settings);
                Console.WriteLine(selected_runner_name);

                using (RegistryKey? runner_key = GetRunnerKey(selected_runner_name, true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    // Set up the UseFilter registry key
                    runner_key.SetValue("IsAurieInstallerKey", 1, RegistryValueKind.DWord);
                    runner_key.SetValue("UseFilter", 1, RegistryValueKind.DWord);

                    string runner_path = GetRunnerPathFromName(selected_runner_name);
                    string runner_directory = Path.GetDirectoryName(runner_path);
                    string registry_runner_path = runner_path.Replace('\\', '_');
                    // Check if the framework is already initialized
                    // Registry check is optional when running Wine
                    if ((runner_key.GetSubKeyNames().Contains(registry_runner_path) || IsRunningOnWine()) &&
                        CheckDirectoryStructure(runner_directory))
                    {
                        can_install = false;
                        install_button.Content = "Uninstall Aurie";
                        install_button.BorderBrush = Brushes.Red;
                        Console.WriteLine($"{selected_runner_name} already has Aurie installed!");
                    }
                    else
                    {
                        can_install = true;
                        install_button.Content = "Install Aurie";
                        install_button.BorderBrush = Brushes.Green;
                        Console.WriteLine($"{selected_runner_name} doesn't have Aurie installed yet!");
                    }
                    SetModList("OnRunnerChange");
                }
            }
        }

        [RelayCommand]
        private async void OnInstallButton()
        {
            if (runner_box.SelectedItem == null)
            {
                Console.WriteLine("No runner currently selected!");
                return;
            }
            string runner_name = (string)runner_box.SelectedItem;
            string runner_path = GetRunnerPathFromName(runner_name) ?? "";

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
                    if (can_install == true)
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

                            Dictionary<string, string[]> file_names = new()
                            {
                                { "Aurie", new string[] { "AurieLoader.exe", "AurieCore.dll" } },
                                { "YYToolkit", new string[] { "YYToolkit.dll" } }
                            };

                            Dictionary<string, string> local_file_paths = new()
                            {
                                { "AurieLoader.exe", Path.Combine(runner_directory, "mods", "AurieLoader.exe") },
                                { "AurieCore.dll", Path.Combine(runner_directory, "mods", "Native", "AurieCore.dll") },
                                { "YYToolkit.dll", Path.Combine(runner_directory, "mods", "Aurie", "YYToolkit.dll") }
                            };

                            using (HttpClient client = new())
                            {
                                file_name_text.Text = "";
                                int total_files = local_file_paths.Count;
                                int downloaded_files = 0;

                                foreach (string repo in repos)
                                {
                                    client.DefaultRequestHeaders.UserAgent.ParseAdd("Aurie");
                                    client.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github.v3+json");

                                    string api_url = $"https://api.github.com/repos/{owner}/{repo}/releases/latest";
                                    HttpResponseMessage response = await client.GetAsync(api_url);

                                    if (response.IsSuccessStatusCode)
                                    {
                                        string response_content = await response.Content.ReadAsStringAsync();
                                        file_names.TryGetValue(repo, out string[] file_names_array);
                                        Dictionary<string, string> download_urls = GetDownloadUrls(response_content, file_names_array);

                                        if (download_urls.Count > 0)
                                        {
                                            foreach (string file_name in file_names_array)
                                            {
                                                if (download_urls.TryGetValue(file_name, out string download_url))
                                                {
                                                    HttpResponseMessage file_response = await client.GetAsync(download_url);

                                                    if (file_response.IsSuccessStatusCode)
                                                    {
                                                        long total_file_size = file_response.Content.Headers.ContentLength ?? 0;
                                                        long bytes_downloaded = 0;
                                                        byte[] buffer = new byte[4096];
                                                        Console.WriteLine($"Writing to '{local_file_paths[file_name]}'...");
                                                        file_name_text.Text = $"Downloading {file_name}...";
                                                        download_progress_bar.Visibility = Visibility.Visible;
                                                        file_name_text.Visibility = Visibility.Visible;
                                                        using (Stream stream = await file_response.Content.ReadAsStreamAsync())
                                                        {
                                                            using (FileStream file_stream = new FileStream(local_file_paths[file_name], FileMode.Create, FileAccess.Write))
                                                            {
                                                                int bytesRead;
                                                                while ((bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length)) > 0)
                                                                {
                                                                    await file_stream.WriteAsync(buffer, 0, bytesRead);

                                                                    bytes_downloaded += bytesRead;

                                                                    int file_progress = (int)((double)bytes_downloaded / total_file_size * 100);
                                                                    download_progress_bar.Value = (int)(100.0 / total_files) * downloaded_files + (int)((double)file_progress / total_files);
                                                                }
                                                            }
                                                        }
                                                        downloaded_files++;
                                                        Console.WriteLine($"File '{file_name}' downloaded to '{local_file_paths[file_name]}' successfully.");
                                                    } else Console.WriteLine($"Failed to download file '{file_name}'. Status code: {file_response.StatusCode}");
                                                } else Console.WriteLine($"Failed to get download URL for file '{file_name}'.");
                                            }
                                        } else Console.WriteLine("No download URLs found for the specified files.");
                                    } else Console.WriteLine($"Failed to get latest release information. Status code: {response.StatusCode}");
                                }
                            }

                            download_progress_bar.Value = 100;
                            download_progress_bar.Visibility = Visibility.Hidden;
                            file_name_text.Visibility = Visibility.Hidden;

                            await GetSnackbarPresenter().HideCurrent();
                            Snackbar snackbar = new(GetSnackbarPresenter()) {
                                MinHeight = 0,
                                Content = "Aurie Framework was installed successfully!",
                                Timeout = System.TimeSpan.FromSeconds(4),
                                Appearance = ControlAppearance.Success,
                                VerticalContentAlignment = VerticalAlignment.Center
                            };
                            snackbar.Show();
                            can_install = false;
                            install_button.Content = "Uninstall Aurie";
                            install_button.BorderBrush = Brushes.Red;
                            SetModList("OnInstallButton");
                        }
                    }
                    else if (can_install == false)
                    {
                        runner_key.DeleteSubKeyTree(registry_runner_path);

                        // If there's no more stuff registered for this runner key, remove the runner key completely
                        if (runner_key.GetSubKeyNames().Length == 0)
                        {
                            using RegistryKey? ifeo = GetIFEOKey(true);
                            ifeo?.DeleteSubKeyTree(runner_name);
                        }

                        RunnerData removed_runner = new()
                        {
                            m_Name = runner_name,
                            m_Path = runner_path
                        };

                        // Remove all entries for this runner from the added runners
                        settings.m_AddedRunners.RemoveAll(runner => runner.m_Path == removed_runner.m_Path);
                        settings.m_CurrentSelectedRunner = "";
                        runner_box.Items.Remove(runner_box.SelectedItem);
                        runner_box.SelectedItem = null;

                        // If the runner directory exists
                        string runner_directory = Path.GetDirectoryName(removed_runner.m_Path) ?? "";
                        if (Path.Exists(runner_directory))
                        {
                            string mods_directory = Path.Combine(runner_directory, "mods") ?? "";

                            string aurie_directory = Path.Combine(runner_directory, "mods", "aurie");
                            string native_directory = Path.Combine(runner_directory, "mods", "native");
                            string loader_file = Path.Combine(runner_directory, "mods", "AurieLoader.exe");

                            if (Directory.Exists(aurie_directory))
                                Directory.Delete(aurie_directory, true);

                            if (Directory.Exists(native_directory))
                                Directory.Delete(native_directory, true);

                            if (File.Exists(loader_file))
                                File.Delete(loader_file);

                            // If no more files remain
                            if (Directory.GetFileSystemEntries(mods_directory).Count() == 0)
                            {
                                Directory.Delete(mods_directory, true);
                            }
                        }

                        runner_box.Text = "Select a game...";

                        if (runner_box.Items.IsEmpty)
                        {
                            runner_box.IsEnabled = false;
                        }

                        play_button.Visibility = Visibility.Hidden;
                        add_mods_button.Visibility = Visibility.Hidden;
                        remove_mods_button.Visibility = Visibility.Hidden;
                        install_button.Visibility = Visibility.Hidden;

                        settings_manager.SaveSettings(settings);

                        await GetSnackbarPresenter().HideCurrent();
                        Snackbar snackbar = new(GetSnackbarPresenter()) {
                            MinHeight = 0,
                            Content = "Aurie Framework was uninstalled successfully!",
                            Timeout = TimeSpan.FromSeconds(4),
                            Appearance = ControlAppearance.Success,
                            VerticalContentAlignment = VerticalAlignment.Center
                        };
                        snackbar.Show();
                        can_install = true;
                        install_button.Content = "Install Aurie";
                        install_button.BorderBrush = Brushes.Green;
                        SetModList("OnInstallButton");
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
                FileName = Path.Combine(directory, command),
                Arguments = arguments.ToString(),
                WorkingDirectory = directory.ToString(),
                UseShellExecute = false,
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
            string runner_path = GetRunnerPathFromName(settings.m_CurrentSelectedRunner);

            if (runner_path == null)
                return;

            string directory = Path.GetDirectoryName(runner_path);
            string loader = $"mods\\AurieLoader.exe";
            string arguments = $"\"{Path.GetFileName(runner_path)}\"";

            try
            {
                using (RegistryKey? runner_key = GetRunnerKey(Path.GetFileName(runner_path), true))
                {
                    if (runner_key is null)
                    {
                        ThrowError("Failed to get runner IFEO key!");
                        return;
                    }

                    Console.WriteLine($"Starting {settings.m_CurrentSelectedRunner}...");

                    string registry_runner_path = runner_path.Replace('\\', '_');
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
                        Process.Start(runner_path);
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
            string mod_name = runner_box.SelectedItem.ToString();

            OpenFileDialog open_file_dialog = new OpenFileDialog {
                Multiselect = true,
                Filter = "Mod files (*.dll)|*.dll",
                Title = $"Select mods to add to {mod_name}"
            };

            bool? result = open_file_dialog.ShowDialog();

            if (result == true)
            {
                string[] selected_files = open_file_dialog.FileNames;

                string runner_path = GetRunnerPathFromName(mod_name);
                string runner_directory = Directory.GetParent(runner_path)?.FullName ?? "";
                string destination_directory = Path.Combine(runner_directory, "mods", "Aurie");

                foreach (string file in selected_files)
                {
                    string file_name = Path.GetFileName(file);
                    string destination_path = Path.Combine(destination_directory, file_name);

                    try
                    {
                        File.Copy(file, destination_path, true);
                        Console.WriteLine($"File '{file_name}' copied to '{destination_path}' successfully.");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error copying file '{file_name}': {ex.Message}");
                    }
                }
                SetModList("OnAddModsButton");
            }
        }

        [RelayCommand]
        private async void OnRemoveModsButton()
        {
            if (mod_list_view.SelectedItems.Count == 0)
            {
                await GetSnackbarPresenter().HideCurrent();
                Snackbar snackbar = new(GetSnackbarPresenter()) {
                    MinHeight = 0,
                    Content = "No mods are selected!",
                    Timeout = TimeSpan.FromSeconds(4),
                    Appearance = ControlAppearance.Danger,
                    VerticalContentAlignment = VerticalAlignment.Center
                };
                snackbar.Show();
                return;
            }

            var content_dialog = new ContentDialog(GetContentPresenter()) {
                Title = "Delete Mods",
                Content = "Would you like to delete all of the selected mods?",
                PrimaryButtonText = "Delete",
                CloseButtonText = "Cancel",
                DialogHeight = 270,
                DialogWidth = 340
            };

            ContentDialogResult result = await content_dialog.ShowAsync();
            if (result == ContentDialogResult.Primary)
            {
                bool mods_deleted = false;
                Console.WriteLine("Deleting all selected mods...");
                foreach (object selected_item in mod_list_view.SelectedItems)
                {
                    if (selected_item is ModItem mod_item)
                    {
                        if (mod_item.m_ModName == "AurieCore.dll" || mod_item.m_ModName == "YYToolkit.dll") continue;
                        string mod_path = mod_item.m_ModPath;

                        try
                        {
                            File.Delete(mod_path);
                            mods_deleted = true;
                            Console.WriteLine($"Deleted {mod_path}!");
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"Error deleting file {mod_path}: {ex.Message}");
                        }
                    }
                }
                if (mods_deleted)
                {
                    Console.WriteLine("Settings mods!");
                    SetModList("OnRemoveModsButton");
                }
            }
        }
    }
}
