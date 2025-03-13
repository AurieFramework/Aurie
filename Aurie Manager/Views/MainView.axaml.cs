using Aurie_Manager.ViewModels;
using Avalonia.Controls;
using Avalonia.Platform.Storage;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

namespace Aurie_Manager.Views;

public partial class MainView : UserControl
{
    public MainView()
    {
        InitializeComponent();
    }

    private static FilePickerFileType AllExecutables { get; } = new("PE32 / PE32+ Executables")
    {
        Patterns = new[] { "*.exe" }
    };

    private MainViewModel? GetMainViewModel()
    {
        return (MainViewModel?)this.DataContext;
    }

    // Error here: Return type of async method must be void, or Task<T>, or some enumerable types.
    private async Task<string?> SelectGame()
    {
        // Get top level from the current control. Alternatively, you can use Window reference instead.
        TopLevel? top_level = TopLevel.GetTopLevel(this);

        if (top_level is null)
            return null;

        FilePickerOpenOptions open_options = new FilePickerOpenOptions
        {
            AllowMultiple = false,
            Title = "Please select the game executable...",
            FileTypeFilter = new[] { AllExecutables }
        };

        var files_selected = await top_level.StorageProvider.OpenFilePickerAsync(open_options);

        if (files_selected is null) 
            return null;

        if (files_selected.Count() == 0)
            return null;

        return files_selected[0].TryGetLocalPath();
    }

    public async void GameSelectionComboBox_SelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        ComboBox self = this.GetControl<ComboBox>("GameSelectionComboBox");

        // If we selected the last "Add game" pseudo-item 
        if (self.SelectedIndex != self.Items.Count - 1)
            return;

        string? path_selected = await SelectGame();
        string? file_selected = Path.GetFileName(path_selected);

        if (file_selected is not null)
        {
            // Add stuff
            GetMainViewModel()?.m_AddedGames.Add(file_selected);
            self.ItemsSource = GetMainViewModel()?.p_AddedGames;
        }
    }
}
