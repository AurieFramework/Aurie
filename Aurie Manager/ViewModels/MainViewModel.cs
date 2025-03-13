using Avalonia.Controls;
using Avalonia.Platform.Storage;
using DynamicData;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using System.Threading.Tasks;

namespace Aurie_Manager.ViewModels;

public class MainViewModel : ViewModelBase
{
    public uint p_WindowWidth => 480;
    public uint p_WindowHeight => 360;
    public string p_WindowTitle => "Aurie Manager v2.0.0";

    public ObservableCollection<string> m_AddedGames = new ObservableCollection<string>();
    public ObservableCollection<string> p_AddedGames
    {
        get
        {
            ObservableCollection<string> collection = m_AddedGames;
            collection.Add("+ Add a game...");
            return collection;
        }
        set
        {
            ObservableCollection<string> collection = value;
            collection.Remove("+ Add a game...");
            m_AddedGames = collection;
        }
    }

    public MainViewModel()
    {
        m_AddedGames = new ObservableCollection<string>
        {
            "test game 1",
            "test game 2"
        };
    }
}
