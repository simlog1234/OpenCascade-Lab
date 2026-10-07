using System.Windows;
using OcctLab.Runtime;

namespace OcctLab.Viewer;

public partial class App : Application
{
    // Runs before the WPF-generated Main(): puts the OCCT DLL folders on PATH
    // before any OcctProxy type gets loaded by MainWindow.
    static App()
    {
        try
        {
            OcctEnvironment.Initialize();
        }
        catch (OcctNotFoundException ex)
        {
            MessageBox.Show(ex.Message, "OCCT not found", MessageBoxButton.OK, MessageBoxImage.Error);
            Environment.Exit(2);
        }
    }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        DispatcherUnhandledException += (_, args) =>
        {
            MessageBox.Show(args.Exception.Message, args.Exception.GetType().Name,
                MessageBoxButton.OK, MessageBoxImage.Error);
            args.Handled = true;
        };
    }
}
