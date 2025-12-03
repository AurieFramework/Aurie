namespace AurieInstaller
{
    public partial class Form_VersionPicker : Form
    {
        // Aurie releases
        internal List<NetInstaller.ReleaseDetails> ArReleases = new();
        // AurieSharp releases
        internal List<NetInstaller.ReleaseDetails> AsReleases = new();
        // YYToolkit releases
        internal List<NetInstaller.ReleaseDetails> YkReleases = new();

        // Aurie selected index
        internal int ArSelectedIndex = 0;
        // AurieSharp selected index
        internal int AsSelectedIndex = 0;
        // YYToolkit selected index
        internal int YkSelectedIndex = 0;

        public Form_VersionPicker()
        {
            InitializeComponent();
        }

        internal void Initialize()
        {
            foreach (var release in ArReleases)
            {
                string version_name = release.VersionTag;
                if (release.Prerelease)
                    version_name = $"[BETA] {version_name}";

                cbArVersion.Items.Add(version_name);
            }

            foreach (var release in AsReleases)
            {
                string version_name = release.VersionTag;
                if (release.Prerelease)
                    version_name = $"[BETA] {version_name}";

                cbAsVersion.Items.Add(version_name);
            }

            foreach (var release in YkReleases)
            {
                string version_name = release.VersionTag;
                if (release.Prerelease)
                    version_name = $"[BETA] {version_name}";

                cbYkVersion.Items.Add(version_name);
            }

            cbArVersion.SelectedIndex = 0;
            cbAsVersion.SelectedIndex = 0;
            cbYkVersion.SelectedIndex = 0;
        }

        private void bt_ConfirmVerChoice_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void cbAurieVersion_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox obj = (ComboBox)sender;
            ArSelectedIndex = obj.SelectedIndex;
        }

        private void cbAsVersion_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox obj = (ComboBox)sender;
            AsSelectedIndex = obj.SelectedIndex;
        }

        private void cbYkVersion_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox obj = (ComboBox)sender;
            YkSelectedIndex = obj.SelectedIndex;
        }
    }
}
