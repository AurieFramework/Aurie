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
using System.Runtime.CompilerServices;

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

        private void DashboardPage_Loaded(object sender, RoutedEventArgs e)
        {
            var ui_elements = new UIElements
            {
                m_InstallButton = install_button,
                m_RunnerBox = runner_box,
                m_DownloadProgressBar = progress_bar,
                m_PlayButton = play_button,
                m_FileNameText = file_name_text,
                m_ModListCanvas = mod_list_canvas,
                m_ModListMask = mod_list_mask,
                m_ModListView = mod_list_view,
                m_AddModsButton = add_mods_button,
                m_RemoveModsButton = remove_mods_button
            };
            ViewModel.SetUIElements(ui_elements);
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
            TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("mod_name");
            SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("mod_toggle_symbol");

            mod_item.m_IsEnabled = true;
            toggle_button.ToolTip = "Disable Mod";
            ApplicationTheme theme = ApplicationThemeManager.GetAppTheme();
            if (theme == ApplicationTheme.Dark)
            {
                text_block.Foreground = Brushes.White;
            }
            else if (theme == ApplicationTheme.Dark)
            {
                text_block.Foreground = Brushes.Black;
            }
            symbol_icon.Symbol = SymbolRegular.Checkmark20;
            mod_item.m_ModPath = ChangeFileExtension(mod_item.m_ModPath);
            mod_item.m_ModName = Path.GetFileName(mod_item.m_ModPath);
        }

        private void ModToggle_Unchecked(object sender, RoutedEventArgs e)
        {
            ToggleButton toggle_button = (ToggleButton)sender;
            Grid grid = FindParent<Grid>(toggle_button);
            ListViewItem list_view_item = FindParent<ListViewItem>(grid);
            ModItem mod_item = (ModItem)list_view_item.DataContext;
            TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("mod_name");
            SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("mod_toggle_symbol");

            mod_item.m_IsEnabled = false;
            toggle_button.ToolTip = "Enable Mod";
            text_block.Foreground = Brushes.Gray;
            symbol_icon.Symbol = SymbolRegular.Dismiss20;
            mod_item.m_ModPath = ChangeFileExtension(mod_item.m_ModPath);
            mod_item.m_ModName = Path.GetFileName(mod_item.m_ModPath);
        }

        private void ModToggle_Loaded(object sender, RoutedEventArgs e)
        {
            ToggleButton toggle_button = (ToggleButton)sender;
            Grid grid = FindParent<Grid>(toggle_button);
            ListViewItem list_view_item = FindParent<ListViewItem>(grid);
            if (list_view_item.DataContext is ModItem mod_item)
            {
                TextBlock text_block = (TextBlock)FindParent<Grid>(toggle_button)?.FindName("mod_name");
                SymbolIcon symbol_icon = (SymbolIcon)toggle_button.FindName("mod_toggle_symbol");

                if (mod_item.m_ModName == "AurieCore.dll" || mod_item.m_ModName == "YYToolkit.dll")
                {
                    toggle_button.ToolTip = $"{mod_item.m_ModName} can't be disabled!";
                    toggle_button.IsEnabled = false;
                    text_block.FontStyle = mod_item.m_IsNative ? FontStyles.Italic : FontStyles.Normal;
                    text_block.FontWeight = FontWeights.Bold;
                    symbol_icon.Symbol = (SymbolRegular)SymbolFilled.Checkmark20;
                    return;
                }

                toggle_button.ToolTip = mod_item.m_IsEnabled ? "Disable Mod" : "Enable Mod";
                symbol_icon.Symbol = mod_item.m_IsEnabled ? SymbolRegular.Checkmark20 : SymbolRegular.Dismiss20;

                SolidColorBrush enabled_brush = ApplicationThemeManager.GetAppTheme() == ApplicationTheme.Dark ? Brushes.White : Brushes.Black;
                text_block.Foreground = mod_item.m_IsEnabled ? enabled_brush : Brushes.Gray;
                text_block.FontStyle = mod_item.m_IsNative ? FontStyles.Italic : FontStyles.Normal;
            }
            else
            {
                Console.WriteLine("ListViewItem.DataContext is not of type ModItem!");
            }
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
            DependencyObject parent_object = VisualTreeHelper.GetParent(child);

            //we've reached the end of the tree
            if (parent_object == null) return null;

            //check if the parent matches the type we're looking for
            if (parent_object is T parent)
                return parent;
            else
                return FindParent<T>(parent_object);
        }

        private void ModListMask_Loaded(object sender, RoutedEventArgs e)
        {
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
    }
}
