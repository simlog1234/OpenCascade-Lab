using System.Globalization;
using System.IO;
using System.Windows;
using Microsoft.Win32;
using OcctProxy;

namespace OcctLab.Viewer;

public partial class MainWindow : Window
{
    private readonly OcctViewControl _view = new();
    private readonly List<OcctShape> _shapes = new();
    private readonly Random _random = new();

    private OcctViewer Viewer => _view.Viewer;

    public MainWindow()
    {
        InitializeComponent();
        ViewHost.Child = _view;
        _view.SelectionChanged += (_, _) => Status($"Selected: {Viewer.SelectedCount}");
        VersionText.Text = $"OCCT {OcctInfo.Version}";
        Loaded += (_, _) => OnDemo(this, new RoutedEventArgs());
    }

    // ---- model ---------------------------------------------------------------

    private void OnBox(object sender, RoutedEventArgs e) =>
        Run(() => Add(ShapeFactory.MakeBox(100, 60, 40), "Box"));

    private void OnCylinder(object sender, RoutedEventArgs e) =>
        Run(() => Add(ShapeFactory.MakeCylinder(20, 80), "Cylinder"));

    private void OnSphere(object sender, RoutedEventArgs e) =>
        Run(() => Add(ShapeFactory.MakeSphere(30), "Sphere"));

    private void OnTorus(object sender, RoutedEventArgs e) =>
        Run(() => Add(ShapeFactory.MakeTorus(40, 10), "Torus"));

    private void OnDemo(object sender, RoutedEventArgs e) => Run(() =>
    {
        using var box = ShapeFactory.MakeBox(100, 60, 40);
        using var hole = ShapeFactory.MakeCylinder(50, 30, -5, 0, 0, 1, radius: 15, height: 50);
        using var plate = ShapeFactory.Cut(box, hole);
        var rounded = ShapeFactory.FilletAllEdges(plate, 3.0);
        Add(rounded, "Demo plate", 0.85, 0.55, 0.20);
    });

    private void Add(OcctShape shape, string label, double r = -1, double g = -1, double b = -1)
    {
        if (r < 0) { r = 0.4 + _random.NextDouble() * 0.5; g = 0.4 + _random.NextDouble() * 0.5; b = 0.4 + _random.NextDouble() * 0.5; }
        _shapes.Add(shape);
        Viewer.Display(shape, r, g, b, update: false);
        Viewer.FitAll();
        Status($"{label}: {shape}  volume={shape.Volume.ToString("F2", CultureInfo.InvariantCulture)}");
    }

    // ---- file ----------------------------------------------------------------

    private void OnImport(object sender, RoutedEventArgs e) => Run(() =>
    {
        var dlg = new OpenFileDialog
        {
            Filter = "CAD files|*.step;*.stp;*.iges;*.igs;*.brep;*.rle|STEP (*.step;*.stp)|*.step;*.stp|IGES (*.iges;*.igs)|*.iges;*.igs|BREP (*.brep)|*.brep|All files|*.*"
        };
        if (dlg.ShowDialog(this) != true) return;

        var ext = Path.GetExtension(dlg.FileName).ToLowerInvariant();
        var shape = ext switch
        {
            ".step" or ".stp" => ShapeIO.ImportStep(dlg.FileName),
            ".iges" or ".igs" => ShapeIO.ImportIges(dlg.FileName),
            _ => ShapeIO.ImportBrep(dlg.FileName)
        };
        Add(shape, Path.GetFileName(dlg.FileName));
    });

    private void OnExportStep(object sender, RoutedEventArgs e) => Run(() =>
    {
        if (_shapes.Count == 0) { Status("Nothing to export"); return; }
        var dlg = new SaveFileDialog { Filter = "STEP (*.step)|*.step", FileName = "scene.step" };
        if (dlg.ShowDialog(this) != true) return;

        using var compound = ShapeFactory.MakeCompound(_shapes);
        ShapeIO.ExportStep(compound, dlg.FileName);
        Status($"Exported {_shapes.Count} shape(s) to {dlg.FileName}");
    });

    private void OnScreenshot(object sender, RoutedEventArgs e) => Run(() =>
    {
        var dlg = new SaveFileDialog { Filter = "PNG (*.png)|*.png", FileName = "view.png" };
        if (dlg.ShowDialog(this) != true) return;
        Status(Viewer.Dump(dlg.FileName) ? $"Saved {dlg.FileName}" : "Screenshot failed");
    });

    // ---- view ----------------------------------------------------------------

    private void OnFitAll(object sender, RoutedEventArgs e) => Viewer.FitAll();
    private void OnWireframe(object sender, RoutedEventArgs e) => Viewer.SetDisplayMode(DisplayMode.Wireframe);
    private void OnShaded(object sender, RoutedEventArgs e) => Viewer.SetDisplayMode(DisplayMode.Shaded);
    private void OnIso(object sender, RoutedEventArgs e) => Viewer.SetOrientation(ViewOrientation.Isometric);
    private void OnFront(object sender, RoutedEventArgs e) => Viewer.SetOrientation(ViewOrientation.Front);
    private void OnTop(object sender, RoutedEventArgs e) => Viewer.SetOrientation(ViewOrientation.Top);
    private void OnRight(object sender, RoutedEventArgs e) => Viewer.SetOrientation(ViewOrientation.Right);

    // ---- edit ----------------------------------------------------------------

    private void OnColorSelected(object sender, RoutedEventArgs e) =>
        Viewer.SetSelectedColor(_random.NextDouble(), _random.NextDouble(), _random.NextDouble());

    private void OnDeleteSelected(object sender, RoutedEventArgs e)
    {
        // The viewer only knows AIS objects; the OcctShape list is kept for export and is
        // intentionally left untouched here (a real app would map AIS objects back to shapes).
        Viewer.RemoveSelected();
        Status("Removed selected object(s) from the view");
    }

    private void OnClear(object sender, RoutedEventArgs e)
    {
        Viewer.EraseAll();
        foreach (var s in _shapes) s.Dispose();
        _shapes.Clear();
        Status("Cleared");
    }

    // ---- helpers -------------------------------------------------------------

    private void Run(Action action)
    {
        try
        {
            action();
        }
        catch (OcctException ex)
        {
            Status("OCCT error: " + ex.Message);
            MessageBox.Show(this, ex.Message, "OCCT error", MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    private void Status(string text) => StatusText.Text = text;
}
