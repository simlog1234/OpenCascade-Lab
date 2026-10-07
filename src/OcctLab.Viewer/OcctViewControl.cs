using System.Windows.Forms;
using OcctProxy;
// WPF (System.Windows.*) usings are global in this project; pin the WinForms/GDI types we mean.
using Color = System.Drawing.Color;
using Control = System.Windows.Forms.Control;
using MouseEventArgs = System.Windows.Forms.MouseEventArgs;
using Point = System.Drawing.Point;

namespace OcctLab.Viewer;

/// <summary>
/// WinForms control that owns an <see cref="OcctViewer"/> (OpenGL view on this control's HWND).
/// Mouse: left-drag = rotate, middle/right-drag = pan, wheel = zoom, click = select, Shift+click = multi-select.
/// </summary>
public sealed class OcctViewControl : Control
{
    private Point _last;
    private bool _rotating;
    private bool _panning;
    private bool _moved;

    public OcctViewer Viewer { get; } = new();

    public event EventHandler? SelectionChanged;

    public OcctViewControl()
    {
        SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.Opaque | ControlStyles.Selectable, true);
        SetStyle(ControlStyles.OptimizedDoubleBuffer | ControlStyles.SupportsTransparentBackColor, false);
        BackColor = Color.Black;
        TabStop = true;
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        if (DesignMode || Viewer.IsInitialized) return;
        if (!Viewer.Init(Handle))
        {
            throw new InvalidOperationException("Failed to create the OCCT OpenGL view (is a GPU/OpenGL driver available?).");
        }
    }

    protected override void OnPaintBackground(PaintEventArgs pevent)
    {
        // OCCT draws the whole client area.
    }

    protected override void OnPaint(PaintEventArgs e) => Viewer.Redraw();

    protected override void OnResize(EventArgs e)
    {
        base.OnResize(e);
        Viewer.Resize();
        Invalidate();
    }

    protected override void OnMouseDown(MouseEventArgs e)
    {
        base.OnMouseDown(e);
        Focus();
        _last = e.Location;
        _moved = false;
        if (e.Button == MouseButtons.Left)
        {
            _rotating = true;
            Viewer.StartRotation(e.X, e.Y);
        }
        else if (e.Button is MouseButtons.Middle or MouseButtons.Right)
        {
            _panning = true;
        }
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        base.OnMouseMove(e);
        if (_rotating)
        {
            Viewer.Rotate(e.X, e.Y);
            _moved = true;
        }
        else if (_panning)
        {
            Viewer.Pan(e.X - _last.X, _last.Y - e.Y);
            _last = e.Location;
            _moved = true;
        }
        else
        {
            Viewer.MoveTo(e.X, e.Y);
        }
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
        base.OnMouseUp(e);
        if (e.Button == MouseButtons.Left && !_moved)
        {
            Viewer.MoveTo(e.X, e.Y);
            if ((ModifierKeys & Keys.Shift) == Keys.Shift) Viewer.ShiftSelect();
            else Viewer.Select();
            SelectionChanged?.Invoke(this, EventArgs.Empty);
        }
        _rotating = false;
        _panning = false;
    }

    protected override void OnMouseWheel(MouseEventArgs e)
    {
        base.OnMouseWheel(e);
        Viewer.ZoomAt(e.X, e.Y, e.Delta > 0 ? 1.15 : 1.0 / 1.15);
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            Viewer.Dispose();
        }
        base.Dispose(disposing);
    }
}
