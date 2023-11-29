// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT was not distributed with this file, You can obtain one at https://opensource.org/licenses/MIT.
// Copyright (C) Leszek Pomianowski and WPF UI Contributors.
// All Rights Reserved.

using AurieInstaller.ViewModels.Pages;
using System.Drawing;
using Wpf.Ui.Controls;
using System.Windows.Media;
using Brushes = System.Windows.Media.Brushes;
using static AurieInstaller.ViewModels.Pages.DashboardViewModel;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using TextBlock = System.Windows.Controls.TextBlock;
using Wpf.Ui.Appearance;
using System.Diagnostics;
using System.IO;
using System.Windows.Data;

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

        private bool is_initialized = false;

        private void DashboardPage_Loaded(object sender, RoutedEventArgs e)
        {
            if (is_initialized == true) return;
            var uiElements = new UIElements
            {
                InstallButton = installButton,
                RunnerBox = runnerBox,
                DownloadProgressBar = progressBar,
                PlayButton = playButton,
                FileNameText = fileNameText,
                ModListCanvas = modListCanvas,
                ModListMask = modListMask,
                ModListView = modListView,
                AddModsButton = addModsButton,
                RemoveModsButton = removeModsButton
            };
            ViewModel.SetUIElements(uiElements);
            is_initialized = true;
        }

        private void ToggleButton_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            Console.WriteLine($"DataContext changed. New type: {e.NewValue?.GetType().FullName}");
        }

        private void RunnerBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ViewModel.OnRunnerChange(sender, e);
        }

        private void ModToggle_Checked(object sender, RoutedEventArgs e)
        {
            ToggleButton toggle_button = (ToggleButton)sender;
            Grid grid = FindParent<Grid>(toggle_button);
            ListViewItem list_view_item = FindParent<ListViewItem>(grid);
            ModItem mod_item = (ModItem)list_view_item.DataContext;
            TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("modName");
            SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("modToggleSymbol");

            mod_item.IsEnabled = true;
            toggle_button.ToolTip = "Disable Mod";
            ThemeType theme = Theme.GetAppTheme();
            if (theme == ThemeType.Dark)
            {
                text_block.Foreground = Brushes.White;
            }
            else if (theme == ThemeType.Light)
            {
                text_block.Foreground = Brushes.Black;
            }
            symbol_icon.Symbol = Wpf.Ui.Common.SymbolRegular.Checkmark20;
            mod_item.ModPath = ChangeFileExtension(mod_item.ModPath);
            mod_item.ModName = Path.GetFileName(mod_item.ModPath);
        }

        private void ModToggle_Unchecked(object sender, RoutedEventArgs e)
        {
            ToggleButton toggle_button = (ToggleButton)sender;
            Grid grid = FindParent<Grid>(toggle_button);
            ListViewItem list_view_item = FindParent<ListViewItem>(grid);
            ModItem mod_item = (ModItem)list_view_item.DataContext;
            TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("modName");
            SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("modToggleSymbol");

            mod_item.IsEnabled = false;
            toggle_button.ToolTip = "Enable Mod";
            text_block.Foreground = Brushes.Gray;
            symbol_icon.Symbol = Wpf.Ui.Common.SymbolRegular.Dismiss20;
            mod_item.ModPath = ChangeFileExtension(mod_item.ModPath);
            mod_item.ModName = Path.GetFileName(mod_item.ModPath);
        }

        private void ModToggle_Loaded(object sender, RoutedEventArgs e)
        {
            ToggleButton toggle_button = (ToggleButton)sender;
            ModItem mod_item = (ModItem)toggle_button.DataContext;
            TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("modName");
            SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("modToggleSymbol");

            if (mod_item.ModName == "AurieCore.dll" || mod_item.ModName == "YYToolkit.dll")
            {
                toggle_button.ToolTip = $"{mod_item.ModName} can't be disabled!";
                toggle_button.IsEnabled = false;
                text_block.FontStyle = mod_item.IsNative ? FontStyles.Italic : FontStyles.Normal;
                text_block.FontWeight = FontWeights.Bold;
                symbol_icon.Symbol = (Wpf.Ui.Common.SymbolRegular)Wpf.Ui.Common.SymbolFilled.Checkmark20;
                return;
            }

            toggle_button.ToolTip = mod_item.IsEnabled ? "Disable Mod" : "Enable Mod";
            symbol_icon.Symbol = mod_item.IsEnabled ? Wpf.Ui.Common.SymbolRegular.Checkmark20 : Wpf.Ui.Common.SymbolRegular.Dismiss20;

            SolidColorBrush enabled_brush = Theme.GetAppTheme() == ThemeType.Dark ? Brushes.White : Brushes.Black;
            text_block.Foreground = mod_item.IsEnabled ? enabled_brush : Brushes.Gray;
            text_block.FontStyle = mod_item.IsNative ? FontStyles.Italic : FontStyles.Normal;
        }

        static string? ChangeFileExtension(string current_file_path)
        {
            try
            {
                string file_name = Path.GetFileName(current_file_path);
                string new_file_path = "";

                if (file_name.EndsWith(".dll"))
                {
                    new_file_path = Path.Combine(Path.GetDirectoryName(current_file_path) ?? "", $"{file_name}.disabled");
                }
                else if (file_name.EndsWith(".dll.disabled"))
                {
                    new_file_path = Path.Combine(Path.GetDirectoryName(current_file_path) ?? "", file_name.Replace(".disabled", ""));
                }
                File.Move(current_file_path, new_file_path);
                return new_file_path;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error renaming file: {ex.Message}");
                return null;
            }
        }

        public static T? FindParent<T>(DependencyObject child) where T : DependencyObject
        {
            //get parent item
            DependencyObject parentObject = VisualTreeHelper.GetParent(child);

            //we've reached the end of the tree
            if (parentObject == null) return null;

            //check if the parent matches the type we're looking for
            if (parentObject is T parent)
                return parent;
            else
                return FindParent<T>(parentObject);
        }

        private void ModListMask_Loaded(object sender, RoutedEventArgs e)
        {
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
    }
}
