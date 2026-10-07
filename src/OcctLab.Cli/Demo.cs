using System.Globalization;
using OcctLab.Runtime;
using OcctProxy;

namespace OcctLab.Cli;

/// <summary>
/// Headless modelling walkthrough: primitives -> boolean -> fillet -> measure -> export -> re-import.
/// Everything here is plain C#; the OCCT calls go through the C++/CLI OcctProxy assembly.
/// </summary>
internal static class Demo
{
    public static void Run(string outputDir)
    {
        var inv = CultureInfo.InvariantCulture;
        OcctInfo.SetConsoleOutput(false);   // silence OCCT's STEP transfer statistics on stdout
        Console.WriteLine($"OCCT version      : {OcctInfo.Version}");
        Console.WriteLine($"OCCT binaries     : {OcctEnvironment.RuntimeDirectories[0]}  ({OcctEnvironment.Configuration})");
        Console.WriteLine();

        // 1. Primitives -------------------------------------------------------
        using var box = ShapeFactory.MakeBox(100, 60, 40);
        using var hole = ShapeFactory.MakeCylinder(50, 30, -5, 0, 0, 1, radius: 15, height: 50);
        Console.WriteLine($"Box               : {box}");
        Console.WriteLine($"Cylinder          : {hole}");

        // 2. Boolean + fillet -------------------------------------------------
        using var plate = ShapeFactory.Cut(box, hole);
        using var rounded = ShapeFactory.FilletAllEdges(plate, 3.0);
        Console.WriteLine($"Box - Cylinder    : {plate}");
        Console.WriteLine($"Filleted (r=3)    : {rounded}");
        Console.WriteLine();

        // 3. Measurements -----------------------------------------------------
        var bbox = rounded.GetBoundingBox();
        var com = rounded.CenterOfMass();
        Console.WriteLine($"Valid (BRepCheck) : {rounded.IsValid()}");
        Console.WriteLine($"Volume            : {rounded.Volume.ToString("F3", inv)}  (box {box.Volume.ToString("F1", inv)}, hole {(Math.PI * 15 * 15 * 40).ToString("F1", inv)})");
        Console.WriteLine($"Surface area      : {rounded.SurfaceArea.ToString("F3", inv)}");
        Console.WriteLine($"Bounding box      : {bbox}");
        Console.WriteLine($"Center of mass    : ({com[0].ToString("F3", inv)}, {com[1].ToString("F3", inv)}, {com[2].ToString("F3", inv)})");
        Console.WriteLine();

        // 4. Export -----------------------------------------------------------
        Directory.CreateDirectory(outputDir);
        var stepPath = Path.Combine(outputDir, "plate.step");
        var brepPath = Path.Combine(outputDir, "plate.brep");
        var stlPath = Path.Combine(outputDir, "plate.stl");
        ShapeIO.ExportStep(rounded, stepPath);
        ShapeIO.ExportBrep(rounded, brepPath);
        ShapeIO.ExportStl(rounded, stlPath, linearDeflection: 0.05, ascii: false);
        foreach (var p in new[] { stepPath, brepPath, stlPath })
        {
            Console.WriteLine($"Exported          : {p}  ({new FileInfo(p).Length:N0} bytes)");
        }
        Console.WriteLine();

        // 5. Round-trip check -------------------------------------------------
        using var reloaded = ShapeIO.ImportStep(stepPath);
        var delta = Math.Abs(reloaded.Volume - rounded.Volume);
        Console.WriteLine($"Re-imported STEP  : {reloaded}");
        Console.WriteLine($"Volume difference : {delta.ToString("E2", inv)}  -> {(delta < 1e-3 ? "OK" : "MISMATCH")}");

        // 6. Error handling: OCCT failures surface as OcctException ---------
        try
        {
            ShapeFactory.FilletAllEdges(box, 1000.0);
        }
        catch (OcctException ex)
        {
            Console.WriteLine($"Expected failure  : {ex.Message}");
        }
    }
}
