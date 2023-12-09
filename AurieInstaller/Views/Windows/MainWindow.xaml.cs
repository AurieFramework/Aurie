// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using AurieInstaller.ViewModels.Pages;
using AurieInstaller.ViewModels.Windows;
using Wpf.Ui;
using Wpf.Ui.Appearance;
using Wpf.Ui.Controls;

namespace AurieInstaller.Views.Windows
{
    public partial class MainWindow
    {
        public MainWindowViewModel ViewModel { get; }

        public MainWindow(
            MainWindowViewModel viewModel,
            INavigationService navigationService,
            IServiceProvider serviceProvider,
            ISnackbarService snackbarService,
            IContentDialogService contentDialogService
        )
        {
            if (SettingsManager.LoadSettings().m_CurrentSelectedTheme is ApplicationTheme app_theme)
            {
                ApplicationThemeManager.Apply(app_theme);
            }
            else
            {
                SystemThemeWatcher.Watch(this);
            }

            _ = new DashboardViewModel() { m_SnackbarService = snackbarService };
            _ = new SettingsViewModel() { SnackbarService = snackbarService };
            _ = new DashboardViewModel() { m_ContentDialogService = contentDialogService };

            ViewModel = viewModel;
            ViewModel.SnackbarService = snackbarService;

            DataContext = this;

            InitializeComponent();

            navigationService.SetNavigationControl(NavigationView);
            snackbarService.SetSnackbarPresenter(SnackbarPresenter);
            contentDialogService.SetContentPresenter(RootContentDialog);

            NavigationView.SetServiceProvider(serviceProvider);
        }
    }
}
