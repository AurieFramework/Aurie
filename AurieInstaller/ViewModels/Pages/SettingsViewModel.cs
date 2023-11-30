// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using System.IO;
using Wpf.Ui.Controls;

namespace AurieInstaller.ViewModels.Pages
{
    public partial class SettingsViewModel : ObservableObject, INavigationAware
    {
        private bool _isInitialized = false;

        private static ISnackbarService? _snackbarService;

        public ISnackbarService SnackbarService {
            get => _snackbarService;
            set
            {
                _snackbarService = value;
                OnPropertyChanged(nameof(SnackbarService));
            }
        }

        public static SnackbarPresenter GetSnackbarPresenter()
        {
            return _snackbarService.GetSnackbarPresenter();
        }

        [ObservableProperty]
        private string _appVersion = String.Empty;

        [ObservableProperty]
        private Wpf.Ui.Appearance.ThemeType _currentTheme = Wpf.Ui.Appearance.ThemeType.Unknown;

        public void OnNavigatedTo()
        {
            if (!_isInitialized)
                InitializeViewModel();
        }

        public void OnNavigatedFrom() { }

        private void InitializeViewModel()
        {
            CurrentTheme = Wpf.Ui.Appearance.Theme.GetAppTheme();
            AppVersion = $"AurieManager - {GetAssemblyVersion()}";

            _isInitialized = true;
        }

        private static string GetAssemblyVersion()
        {
            return System.Reflection.Assembly.GetExecutingAssembly().GetName().Version?.ToString()
                ?? String.Empty;
        }

        [RelayCommand]
        private void OnChangeTheme(string parameter)
        {
            switch (parameter)
            {
                case "theme_light":
                    if (CurrentTheme == Wpf.Ui.Appearance.ThemeType.Light)
                        break;

                    Wpf.Ui.Appearance.Theme.Apply(Wpf.Ui.Appearance.ThemeType.Light);
                    CurrentTheme = Wpf.Ui.Appearance.ThemeType.Light;

                    break;

                default:
                    if (CurrentTheme == Wpf.Ui.Appearance.ThemeType.Dark)
                        break;

                    Wpf.Ui.Appearance.Theme.Apply(Wpf.Ui.Appearance.ThemeType.Dark);
                    CurrentTheme = Wpf.Ui.Appearance.ThemeType.Dark;

                    break;
            }
        }

        [RelayCommand]
        private async void OnDeleteConfig()
        {
            const string settings_file_name = "aurie-config.json";
            string app_data_path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Aurie");
            string settings_file_path = Path.Combine(app_data_path, settings_file_name);

            if (File.Exists(settings_file_path))
            {
                File.Delete(settings_file_path);
                Console.WriteLine("Config file deleted!");
            }
            else
            {
                Console.WriteLine("Config file doesn't exist!");
            }
            await GetSnackbarPresenter().HideCurrent();
            Snackbar snackbar = new(GetSnackbarPresenter()) {
                MinHeight = 0,
                Content = "The Aurie config file was deleted successfully!",
                Timeout = System.TimeSpan.FromSeconds(5),
                Appearance = ControlAppearance.Success,
                VerticalContentAlignment = VerticalAlignment.Center
            };
            System.Media.SystemSounds.Hand.Play();
            snackbar.Show();
        }
    }
}
