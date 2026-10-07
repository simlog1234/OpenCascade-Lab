using System.Globalization;
using OcctLab.Runtime;

namespace OcctLab.Cli;

internal static class Program
{
    private static int Main(string[] args)
    {
        // Must run before any OcctProxy type is JIT-compiled (see OcctEnvironment docs).
        try
        {
            OcctEnvironment.Initialize();
        }
        catch (OcctNotFoundException ex)
        {
            Console.Error.WriteLine(ex.Message);
            return 2;
        }

        var outputDir = args.Length > 0 ? args[0] : Path.Combine(Environment.CurrentDirectory, "output");
        try
        {
            Demo.Run(outputDir);
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"FAILED: {ex.GetType().Name}: {ex.Message}");
            return 1;
        }
    }
}
