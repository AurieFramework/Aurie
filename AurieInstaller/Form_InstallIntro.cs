using AurieInstaller.Install_Internals;
using System.Diagnostics;

namespace AurieInstaller
{
    public partial class Form_InstallIntro : Form
    {
        public Form_InstallIntro()
        {
            InitializeComponent();
        }
        internal static void ErrorBox(string message)
        {
            MessageBox.Show(message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        internal static async Task<int> PatchGame(string GamePath, string NativeDllPath, string AuriePatcherPath)
        {
            Process p = new Process();
            p.StartInfo.FileName = AuriePatcherPath;
            p.StartInfo.Arguments = $"\"{GamePath}\" \"{NativeDllPath}\" install";

            p.Start();
            await p.WaitForExitAsync();

            return p.ExitCode;
        }

        private async void bt_PickGame_Click(object sender, EventArgs e)
        {
            bool game_selected = FileUtils.SelectFile(
                "Select your game executable...",
                "PE32+ Executables (*.exe)|*.exe",
                out string game_executable
            );

            if (!game_selected)
            {
                ErrorBox("No executable was selected.");
                return;
            }

            if (!File.Exists(game_executable))
            {
                ErrorBox("The selected executable does not exist.");
                return;
            }

            if (!FileUtils.IsFileWritable(game_executable))
            {
                ErrorBox("The selected executable isn't writable. Try re-running as an administrator.");
                return;
            }

            string game_directory = Directory.GetParent(game_executable)?.FullName ?? "";
            if (string.IsNullOrEmpty(game_directory))
            {
                ErrorBox("The selected executable does not have a parent. Move it to a different folder.");
                return;
            }

            Enabled = false;
            {
                var version_picker = new Form_VersionPicker();

                // Create storage for all releases
                NetInstaller.ReleaseDetails aurie_release = new(),
                    auriesharp_release = new(),
                    yytk_release = new();

                // Stage 1: Access GH API to fetch releases
                {
                    var progress_bar = new Form_Progress();
                    progress_bar.Show();

                    progress_bar.lb_Status.Text = "Waiting for GitHub...";
                    progress_bar.pb_Status.Value = 20;

                    var Ar_ReleasesJSON = await NetInstaller.FetchResponseAsync(NetInstaller.BuildGHReleasesApiLink("AurieFramework", "Aurie", false));
                    progress_bar.pb_Status.Value = 30;

                    var As_ReleasesJSON = await NetInstaller.FetchResponseAsync(NetInstaller.BuildGHReleasesApiLink("AurieFramework", "AurieSharp", false));
                    progress_bar.pb_Status.Value = 40;

                    var Yk_ReleasesJSON = await NetInstaller.FetchResponseAsync(NetInstaller.BuildGHReleasesApiLink("AurieFramework", "YYToolkit", false));
                    progress_bar.pb_Status.Value = 50;


                    // Make sure we have valid JSONs for everything.
                    if (Ar_ReleasesJSON is null || As_ReleasesJSON is null || Yk_ReleasesJSON is null)
                    {
                        ErrorBox("Failed to fetch data from GitHub. Check your firewall and network settings.");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    // Try to parse all the JSONs into our ReleaseDetails struct.
                    try
                    {
                        version_picker.ArReleases = NetInstaller.ParseGHReleaseJson(Ar_ReleasesJSON);
                        version_picker.AsReleases = NetInstaller.ParseGHReleaseJson(As_ReleasesJSON);
                        version_picker.YkReleases = NetInstaller.ParseGHReleaseJson(Yk_ReleasesJSON);
                    }
                    catch
                    {
                        ErrorBox("Failed to parse data from GitHub. Check your firewall and network settings.");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    progress_bar.pb_Status.Value = 70;

                    // Versions that we actually want to install.
                    // By default, we use the latest non-beta version.
                    aurie_release = version_picker.ArReleases.First((inst) => !inst.Prerelease);
                    auriesharp_release = version_picker.AsReleases.FirstOrDefault((inst) => !inst.Prerelease);
                    yytk_release = version_picker.YkReleases.FirstOrDefault((inst) => !inst.Prerelease);

                    // Close progress bar, loading is done...
                    progress_bar.pb_Status.Value = 100;
                    progress_bar.Close();
                }

                // If advanced mode, let the user choose the version.
                if (cb_AdvancedMode.Checked)
                {
                    // Initialize the ComboBoxes and other UI elements from the XxReleases variables.
                    version_picker.Initialize();

                    // Check if user clicked "Confirm version"
                    if (version_picker.ShowDialog() != DialogResult.OK)
                    {
                        ErrorBox("The installation was cancelled.");
                        Enabled = true;
                        return;
                    }

                    // Select releases based off that
                    aurie_release = version_picker.ArReleases[version_picker.ArSelectedIndex];
                    auriesharp_release = version_picker.AsReleases[version_picker.AsSelectedIndex];
                    yytk_release = version_picker.YkReleases[version_picker.YkSelectedIndex];
                }

                // Here we go again with the progressbar...
                {
                    var progress_bar = new Form_Progress();
                    progress_bar.Show();

                    progress_bar.lb_Status.Text = "Creating folders...";
                    progress_bar.pb_Status.Value = 10;

                    try
                    {
                        FileUtils.CreateModDirectoryStructure(game_directory);
                    }
                    catch (Exception ex)
                    {
                        ErrorBox($"Cannot create mod directory!\n\n{ex.Message}");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    string[] aurie_required_files = new[]
                    {
                        "auriecore.dll",
                        "auriepatcher.exe"
                    };

                    // If any of the required files aren't in the Aurie release artifacts:
                    if (aurie_required_files.Any(key => !aurie_release.Artifacts.ContainsKey(key)))
                    {
                        ErrorBox($"Aurie {aurie_release.VersionTag} does not contain the required files. Please select a different version.");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    string[] yytk_required_files = new[]
                    {
                        "yytoolkit.dll",
                    };

                    if (yytk_required_files.Any(key => !yytk_release.Artifacts.ContainsKey(key)))
                    {
                        ErrorBox($"YYToolkit {yytk_release.VersionTag} does not contain the required files. Please select a different version.");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    string[] auriesharp_required_files = new[]
                    {
                        "auriesharpcore.dll",
                        "auriesharpinterop.dll",
                        "auriesharpinterop.runtimeconfig.json",
                        "auriesharpmanaged.dll",
                        "ijwhost.dll",
                        "nethost.dll"
                    };

                    // If any of the required files aren't in the AurieSharp release artifacts:
                    if (auriesharp_required_files.Any(key => !auriesharp_release.Artifacts.ContainsKey(key)))
                    {
                        ErrorBox($"AurieSharp {auriesharp_release.VersionTag} does not contain the required files. Please select a different version.");
                        Enabled = true;
                        progress_bar.Close();
                        return;
                    }

                    string mods = Path.Combine(game_directory, "mods");
                    string mods_native = Path.Combine(mods, "native");
                    string mods_aurie = Path.Combine(mods, "aurie");
                    string mods_binaries = Path.Combine(mods, "bin");
                    string mods_managed = Path.Combine(mods, "managed");


                    string auriecore_dll = Path.Combine(mods_native, "auriecore.dll");
                    string auriepatcher_exe = Path.Combine(mods_binaries, "auriepatcher.exe");
                    string yytoolkit_dll = Path.Combine(mods_aurie, "yytoolkit.dll");

                    progress_bar.lb_Status.Text = "Downloading Aurie Core...";
                    progress_bar.pb_Status.Value = 20;
                    await NetInstaller.DownloadFileFromUrl(aurie_release.Artifacts["auriecore.dll"], auriecore_dll);

                    progress_bar.lb_Status.Text = "Downloading Aurie Patcher...";
                    progress_bar.pb_Status.Value = 30;
                    await NetInstaller.DownloadFileFromUrl(aurie_release.Artifacts["auriepatcher.exe"], auriepatcher_exe);

                    progress_bar.lb_Status.Text = "Downloading YYToolkit...";
                    progress_bar.pb_Status.Value = 40;
                    await NetInstaller.DownloadFileFromUrl(yytk_release.Artifacts["yytoolkit.dll"], yytoolkit_dll);

                    if (cb_InstallAs.Checked)
                    {
                        progress_bar.lb_Status.Text = "Downloading AurieSharp Core...";
                        progress_bar.pb_Status.Value = 50;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["auriesharpcore.dll"], Path.Combine(mods_aurie, "auriesharpcore.dll"));

                        progress_bar.lb_Status.Text = "Downloading AurieSharp Interop...";
                        progress_bar.pb_Status.Value = 60;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["auriesharpinterop.dll"], Path.Combine(mods_aurie, "auriesharpinterop.dll"));
                        
                        progress_bar.pb_Status.Value = 65;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["auriesharpinterop.runtimeconfig.json"], Path.Combine(mods_aurie, "auriesharpinterop.runtimeconfig.json"));

                        progress_bar.lb_Status.Text = "Downloading AurieSharp Managed...";
                        progress_bar.pb_Status.Value = 70;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["auriesharpmanaged.dll"], Path.Combine(mods_managed, "auriesharpmanaged.dll"));

                        progress_bar.lb_Status.Text = "Downloading AurieSharp dependencies...";
                        progress_bar.pb_Status.Value = 80;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["ijwhost.dll"], Path.Combine(mods_native, "ijwhost.dll"));

                        progress_bar.lb_Status.Text = "Downloading AurieSharp dependencies...";
                        progress_bar.pb_Status.Value = 85;
                        await NetInstaller.DownloadFileFromUrl(auriesharp_release.Artifacts["nethost.dll"], Path.Combine(mods_native, "nethost.dll"));
                    }

                    progress_bar.lb_Status.Text = "Patching game...";
                    progress_bar.pb_Status.Value = 95;

                    int exit_status = await PatchGame(game_executable, auriecore_dll, auriepatcher_exe);
                    if (exit_status == 0)
                    {
                        MessageBox.Show("Game patched successfully.", "Aurie Installer", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                    else
                    {
                        MessageBox.Show($"Failed to patch the game. Aurie Patcher exited with status {exit_status}.", "Aurie Installer", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                    progress_bar.Close();
                }
            }
            Enabled = true;
        }
    }
}
