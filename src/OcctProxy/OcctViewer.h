// OcctViewer.h - OpenGL 3D viewer bound to a Win32 window handle (HWND).
#pragma once

#include "Common.h"
#include "OcctShape.h"

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_GradientFillMethod.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <WNT_Window.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <NCollection_List.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_SelectionScheme.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <NCollection_Haft.h>

namespace OcctProxy
{
  public enum class DisplayMode
  {
    Wireframe = AIS_WireFrame,
    Shaded    = AIS_Shaded
  };

  public enum class ViewOrientation
  {
    Front, Back, Top, Bottom, Left, Right, Isometric
  };

  /// Encapsulates V3d_Viewer / V3d_View / AIS_InteractiveContext for one window.
  /// Create it from a WinForms control: viewer.Init(control.Handle).
  public ref class OcctViewer
  {
  public:
    OcctViewer() {}
    ~OcctViewer() { this->!OcctViewer(); }
    !OcctViewer()
    {
      if (!myContext().IsNull()) { myContext()->RemoveAll(false); }
      if (!myView().IsNull())    { myView()->Remove(); }
      myContext().Nullify();
      myView().Nullify();
      myViewer().Nullify();
      myDriver().Nullify();
    }

    /// Creates the OpenGL context on the given HWND. Returns false on failure.
    bool Init(System::IntPtr hwnd)
    {
      OCCT_TRY
        Handle(Aspect_DisplayConnection) aDisplay = new Aspect_DisplayConnection();
        Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisplay, false);
        aDriver->ChangeOptions().buffersNoSwap = false;
        if (!aDriver->InitContext())
        {
          return false;
        }
        myDriver() = aDriver;

        myViewer() = new V3d_Viewer(aDriver);
        myViewer()->SetDefaultLights();
        myViewer()->SetLightOn();

        myView() = myViewer()->CreateView();
        Handle(WNT_Window) aWindow = new WNT_Window(reinterpret_cast<HWND>(hwnd.ToPointer()));
        myView()->SetWindow(aWindow);
        if (!aWindow->IsMapped())
        {
          aWindow->Map();
        }
        myView()->SetBgGradientColors(Quantity_Color(0.30, 0.33, 0.40, Quantity_TOC_sRGB),
                                      Quantity_Color(0.08, 0.09, 0.12, Quantity_TOC_sRGB),
                                      Aspect_GradientFillMethod_Vertical, false);
        myView()->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.08, V3d_ZBUFFER);

        myContext() = new AIS_InteractiveContext(myViewer());
        myContext()->SetDisplayMode(AIS_Shaded, false);

        myView()->MustBeResized();
        myView()->Redraw();
        return true;
      OCCT_CATCH
    }

    property bool IsInitialized { bool get() { return !myView().IsNull(); } }

    // ---- window plumbing ---------------------------------------------------

    /// Call from the control's Resize event.
    void Resize()
    {
      if (!myView().IsNull()) { myView()->MustBeResized(); }
    }

    /// Call from the control's Paint event.
    void Redraw()
    {
      if (!myView().IsNull()) { myView()->Redraw(); }
    }

    // ---- scene -------------------------------------------------------------

    /// Displays a shape with the given RGB colour (components in 0..1).
    void Display(OcctShape^ shape, double r, double g, double b, bool update)
    {
      if (shape == nullptr || shape->IsNull || myContext().IsNull()) return;
      OCCT_TRY
        Handle(AIS_Shape) anAis = new AIS_Shape(shape->Native());
        anAis->SetColor(Quantity_Color(r, g, b, Quantity_TOC_sRGB));
        myContext()->Display(anAis, update ? true : false);
      OCCT_CATCH
    }

    void Display(OcctShape^ shape)
    {
      Display(shape, 0.75, 0.75, 0.78, true);
    }

    void EraseAll()
    {
      if (myContext().IsNull()) return;
      myContext()->RemoveAll(true);
    }

    void FitAll()
    {
      if (myView().IsNull()) return;
      myView()->FitAll(0.01, false);
      myView()->ZFitAll();
      myView()->Redraw();
    }

    /// Switches every displayed object (and the default) to wireframe or shaded.
    void SetDisplayMode(DisplayMode mode)
    {
      if (myContext().IsNull()) return;
      OCCT_TRY
        const int aMode = static_cast<int>(mode);
        myContext()->SetDisplayMode(aMode, false);
        NCollection_List<Handle(AIS_InteractiveObject)> aList;
        myContext()->DisplayedObjects(aList);
        for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
        {
          myContext()->SetDisplayMode(anIt.Value(), aMode, false);
        }
        myContext()->UpdateCurrentViewer();
      OCCT_CATCH
    }

    void SetBackgroundColor(double r, double g, double b)
    {
      if (myView().IsNull()) return;
      myView()->SetBgGradientStyle(Aspect_GradientFillMethod_None, false);
      myView()->SetBackgroundColor(Quantity_Color(r, g, b, Quantity_TOC_sRGB));
      myView()->Redraw();
    }

    void SetOrientation(ViewOrientation orientation)
    {
      if (myView().IsNull()) return;
      switch (orientation)
      {
        case ViewOrientation::Front:  myView()->SetProj(V3d_Yneg); break;
        case ViewOrientation::Back:   myView()->SetProj(V3d_Ypos); break;
        case ViewOrientation::Top:    myView()->SetProj(V3d_Zpos); break;
        case ViewOrientation::Bottom: myView()->SetProj(V3d_Zneg); break;
        case ViewOrientation::Left:   myView()->SetProj(V3d_Xneg); break;
        case ViewOrientation::Right:  myView()->SetProj(V3d_Xpos); break;
        default:                      myView()->SetProj(V3d_XposYnegZpos); break;
      }
      FitAll();
    }

    // ---- mouse interaction (pixel coordinates of the control) --------------

    void StartRotation(int x, int y)
    {
      if (!myView().IsNull()) { myView()->StartRotation(x, y); }
    }

    void Rotate(int x, int y)
    {
      if (!myView().IsNull()) { myView()->Rotation(x, y); }
    }

    /// Pans by a pixel delta.
    void Pan(int dx, int dy)
    {
      if (!myView().IsNull()) { myView()->Pan(dx, dy); }
    }

    /// Zooms around the given pixel; factor > 1 zooms in, < 1 zooms out.
    void ZoomAt(int x, int y, double factor)
    {
      if (myView().IsNull()) return;
      myView()->StartZoomAtPoint(x, y);
      // ZoomAtPoint derives the coefficient from the pixel delta; translate factor to a delta.
      const int aDelta = static_cast<int>((factor - 1.0) * 100.0);
      myView()->ZoomAtPoint(x, y, x + aDelta, y);
    }

    /// Dynamic highlighting of the entity under the cursor.
    void MoveTo(int x, int y)
    {
      if (myContext().IsNull()) return;
      myContext()->MoveTo(x, y, myView(), true);
    }

    /// Selects the detected (highlighted) entity, replacing the current selection.
    void Select()
    {
      if (myContext().IsNull()) return;
      myContext()->SelectDetected(AIS_SelectionScheme_Replace);
      myContext()->UpdateCurrentViewer();
    }

    /// Toggles the detected entity in the current selection.
    void ShiftSelect()
    {
      if (myContext().IsNull()) return;
      myContext()->SelectDetected(AIS_SelectionScheme_XOR);
      myContext()->UpdateCurrentViewer();
    }

    property int SelectedCount
    {
      int get() { return myContext().IsNull() ? 0 : myContext()->NbSelected(); }
    }

    void SetSelectedColor(double r, double g, double b)
    {
      if (myContext().IsNull()) return;
      Quantity_Color aColor(r, g, b, Quantity_TOC_sRGB);
      for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
      {
        myContext()->SetColor(myContext()->SelectedInteractive(), aColor, false);
      }
      myContext()->UpdateCurrentViewer();
    }

    void RemoveSelected()
    {
      if (myContext().IsNull()) return;
      NCollection_List<Handle(AIS_InteractiveObject)> aList;
      for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
      {
        aList.Append(myContext()->SelectedInteractive());
      }
      myContext()->ClearSelected(false);
      for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
      {
        myContext()->Remove(anIt.Value(), false);
      }
      myContext()->UpdateCurrentViewer();
    }

    /// Saves a screenshot (png/bmp/jpg by extension).
    bool Dump(System::String^ path)
    {
      if (myView().IsNull()) return false;
      OCCT_TRY
        myView()->Redraw();
        return myView()->Dump(ToAscii(path).ToCString()) == true;
      OCCT_CATCH
    }

  private:
    NCollection_Haft<Handle(Graphic3d_GraphicDriver)> myDriver;
    NCollection_Haft<Handle(V3d_Viewer)>              myViewer;
    NCollection_Haft<Handle(V3d_View)>                myView;
    NCollection_Haft<Handle(AIS_InteractiveContext)>  myContext;
  };
}
