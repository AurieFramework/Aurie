// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using AurieInstaller.ViewModels.Pages;
using System.Drawing;
using Wpf.Ui.Controls;
using System.Windows.Media;
using Brushes = System.Windows.Media.Brushes;

namespace AurieInstaller.Views.Pages
{
    public partial class DashboardPage : INavigableView<DashboardViewModel>
    {
        public DashboardViewModel ViewModel { get; }

        public DashboardPage(DashboardViewModel viewModel)
        {
            ViewModel = viewModel;
            DataContext = this;

            InitializeComponent();
            Loaded += DashboardPage_Loaded;
        }

        private void DashboardPage_Loaded(object sender, System.Windows.RoutedEventArgs e)
        {
            Console.WriteLine("Setting installButton...");
            ViewModel.SetInstallButton(installButton);

            Console.WriteLine("Setting runnerBox...");
            ViewModel.SetRunnerBox(runnerBox);

            Console.WriteLine("Setting playButton...");
            ViewModel.SetPlayButton(playButton);

            Console.WriteLine("Setting snackbarPresenter...");
            ViewModel.SetSnackbarPresenter(snackbarPresenter);

            Console.WriteLine("Setting progressBar...");
            ViewModel.SetProgressBar(progressBar);
        }

        private void runnerBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            ViewModel.OnRunnerChange(sender, e);
        }
    }
}
