// ShapeFactory.h - primitives, booleans, fillets and transformations.
#pragma once

#include "Common.h"
#include "OcctShape.h"

#include <cmath>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepBuilderAPI_Transform.hxx>

namespace OcctProxy
{
  // Pure native helpers. BRepFilletAPI_* objects carry over-aligned members, and a
  // function holding such a local is compiled as native under /clr - which is only
  // allowed when the function has no managed types in its signature or body.
  namespace Native
  {
    inline TopoDS_Shape FilletAllEdges(const TopoDS_Shape& theShape, double theRadius, bool& theIsDone)
    {
      BRepFilletAPI_MakeFillet aFillet(theShape);
      for (TopExp_Explorer anExp(theShape, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        aFillet.Add(theRadius, TopoDS::Edge(anExp.Current()));
      }
      aFillet.Build();
      theIsDone = aFillet.IsDone();
      return theIsDone ? aFillet.Shape() : TopoDS_Shape();
    }

    inline TopoDS_Shape ChamferAllEdges(const TopoDS_Shape& theShape, double theDistance, bool& theIsDone)
    {
      BRepFilletAPI_MakeChamfer aChamfer(theShape);
      for (TopExp_Explorer anExp(theShape, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        aChamfer.Add(theDistance, TopoDS::Edge(anExp.Current()));
      }
      aChamfer.Build();
      theIsDone = aChamfer.IsDone();
      return theIsDone ? aChamfer.Shape() : TopoDS_Shape();
    }
  }

  /// Static factory exposing the most common OCCT modelling algorithms to .NET.
  /// Lengths are in model units (mm by convention), angles in degrees.
  public ref class ShapeFactory abstract sealed
  {
  public:
    // ---- primitives --------------------------------------------------------

    /// Box with one corner at the origin, extending along +X/+Y/+Z.
    static OcctShape^ MakeBox(double dx, double dy, double dz)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeBox(dx, dy, dz).Shape());
      OCCT_CATCH
    }

    /// Box with one corner at (x, y, z).
    static OcctShape^ MakeBox(double x, double y, double z, double dx, double dy, double dz)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeBox(gp_Pnt(x, y, z), dx, dy, dz).Shape());
      OCCT_CATCH
    }

    /// Cylinder along +Z with its base centred at the origin.
    static OcctShape^ MakeCylinder(double radius, double height)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeCylinder(radius, height).Shape());
      OCCT_CATCH
    }

    /// Cylinder whose base centre is (x, y, z) and whose axis is (dirX, dirY, dirZ).
    static OcctShape^ MakeCylinder(double x, double y, double z,
                                   double dirX, double dirY, double dirZ,
                                   double radius, double height)
    {
      OCCT_TRY
        gp_Ax2 anAxis(gp_Pnt(x, y, z), gp_Dir(dirX, dirY, dirZ));
        return gcnew OcctShape(BRepPrimAPI_MakeCylinder(anAxis, radius, height).Shape());
      OCCT_CATCH
    }

    static OcctShape^ MakeSphere(double radius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeSphere(radius).Shape());
      OCCT_CATCH
    }

    static OcctShape^ MakeSphere(double x, double y, double z, double radius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeSphere(gp_Pnt(x, y, z), radius).Shape());
      OCCT_CATCH
    }

    /// Cone along +Z: radius1 at the base, radius2 at the top (0 for a sharp tip).
    static OcctShape^ MakeCone(double radius1, double radius2, double height)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeCone(radius1, radius2, height).Shape());
      OCCT_CATCH
    }

    /// Torus around +Z: majorRadius from centre to tube centre, minorRadius of the tube.
    static OcctShape^ MakeTorus(double majorRadius, double minorRadius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeTorus(majorRadius, minorRadius).Shape());
      OCCT_CATCH
    }

    // ---- booleans ----------------------------------------------------------

    static OcctShape^ Fuse(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Fuse anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Fuse");
      OCCT_CATCH
    }

    /// Returns a minus b.
    static OcctShape^ Cut(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Cut anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Cut");
      OCCT_CATCH
    }

    static OcctShape^ Common(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Common anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Common");
      OCCT_CATCH
    }

    // ---- local operations --------------------------------------------------

    /// Fillets every edge of the shape with a constant radius.
    static OcctShape^ FilletAllEdges(OcctShape^ shape, double radius)
    {
      CheckArg(shape);
      OCCT_TRY
        bool isDone = false;
        TopoDS_Shape aResult = Native::FilletAllEdges(shape->Native(), radius, isDone);
        if (!isDone)
        {
          throw gcnew OcctException("Fillet failed (radius too large for the geometry?)");
        }
        return gcnew OcctShape(aResult);
      OCCT_CATCH
    }

    /// Chamfers every edge of the shape with a constant distance.
    static OcctShape^ ChamferAllEdges(OcctShape^ shape, double distance)
    {
      CheckArg(shape);
      OCCT_TRY
        bool isDone = false;
        TopoDS_Shape aResult = Native::ChamferAllEdges(shape->Native(), distance, isDone);
        if (!isDone)
        {
          throw gcnew OcctException("Chamfer failed (distance too large for the geometry?)");
        }
        return gcnew OcctShape(aResult);
      OCCT_CATCH
    }

    // ---- transformations ---------------------------------------------------

    static OcctShape^ Translate(OcctShape^ shape, double dx, double dy, double dz)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetTranslation(gp_Vec(dx, dy, dz));
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// Rotates around the axis through (px, py, pz) with direction (dx, dy, dz).
    static OcctShape^ Rotate(OcctShape^ shape,
                             double px, double py, double pz,
                             double dx, double dy, double dz,
                             double angleDegrees)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetRotation(gp_Ax1(gp_Pnt(px, py, pz), gp_Dir(dx, dy, dz)),
                          angleDegrees * 3.14159265358979323846 / 180.0);
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// Uniform scale about the origin.
    static OcctShape^ Scale(OcctShape^ shape, double factor)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetScale(gp_Pnt(0.0, 0.0, 0.0), factor);
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// Groups several shapes into one compound (no boolean operation is performed).
    static OcctShape^ MakeCompound(System::Collections::Generic::IEnumerable<OcctShape^>^ shapes)
    {
      if (shapes == nullptr) throw gcnew System::ArgumentNullException("shapes");
      OCCT_TRY
        BRep_Builder aBuilder;
        TopoDS_Compound aCompound;
        aBuilder.MakeCompound(aCompound);
        for each (OcctShape^ aShape in shapes)
        {
          if (aShape != nullptr && !aShape->IsNull)
          {
            aBuilder.Add(aCompound, aShape->Native());
          }
        }
        return gcnew OcctShape(aCompound);
      OCCT_CATCH
    }

  private:
    static void CheckArg(OcctShape^ shape)
    {
      if (shape == nullptr || shape->IsNull) throw gcnew System::ArgumentException("Shape is null");
    }

    static void CheckArgs(OcctShape^ a, OcctShape^ b)
    {
      if (a == nullptr || a->IsNull) throw gcnew System::ArgumentException("First shape is null");
      if (b == nullptr || b->IsNull) throw gcnew System::ArgumentException("Second shape is null");
    }

    static OcctShape^ FromBoolean(BRepAlgoAPI_BooleanOperation& theOp, const char* theName)
    {
      theOp.Build();
      if (!theOp.IsDone() || theOp.HasErrors())
      {
        throw gcnew OcctException(System::String::Concat("Boolean ", gcnew System::String(theName), " failed"));
      }
      return gcnew OcctShape(theOp.Shape());
    }
  };
}
