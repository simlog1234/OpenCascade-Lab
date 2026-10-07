// OcctShape.h - managed wrapper around TopoDS_Shape.
#pragma once

#include "Common.h"

#include <TopoDS_Shape.hxx>
#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

namespace OcctProxy
{
  /// Axis-aligned bounding box in model coordinates.
  public value struct BoundingBox
  {
    double XMin, YMin, ZMin, XMax, YMax, ZMax;

    property double SizeX { double get() { return XMax - XMin; } }
    property double SizeY { double get() { return YMax - YMin; } }
    property double SizeZ { double get() { return ZMax - ZMin; } }

    virtual System::String^ ToString() override
    {
      return System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
        "[{0:F3}, {1:F3}, {2:F3}] - [{3:F3}, {4:F3}, {5:F3}]", XMin, YMin, ZMin, XMax, YMax, ZMax);
    }
  };

  /// Topological shape type (mirrors TopAbs_ShapeEnum).
  public enum class ShapeType
  {
    Compound  = TopAbs_COMPOUND,
    CompSolid = TopAbs_COMPSOLID,
    Solid     = TopAbs_SOLID,
    Shell     = TopAbs_SHELL,
    Face      = TopAbs_FACE,
    Wire      = TopAbs_WIRE,
    Edge      = TopAbs_EDGE,
    Vertex    = TopAbs_VERTEX,
    Shape     = TopAbs_SHAPE
  };

  /// Managed handle to a native TopoDS_Shape. Owns a heap copy of the shape
  /// (TopoDS_Shape itself is a cheap reference-counted handle, so copying is inexpensive).
  public ref class OcctShape
  {
  internal:
    OcctShape(const TopoDS_Shape& theShape)
      : myShape(new TopoDS_Shape(theShape)) {}

    /// Native access for the other proxy classes.
    const TopoDS_Shape& Native() { return *myShape; }

  public:
    OcctShape() : myShape(new TopoDS_Shape()) {}
    ~OcctShape() { this->!OcctShape(); }
    !OcctShape()
    {
      if (myShape != nullptr)
      {
        delete myShape;
        myShape = nullptr;
      }
    }

    property bool IsNull { bool get() { return myShape == nullptr || myShape->IsNull(); } }

    property ShapeType Type
    {
      ShapeType get() { return IsNull ? ShapeType::Shape : static_cast<ShapeType>(myShape->ShapeType()); }
    }

    property int NbSolids   { int get() { return Count(TopAbs_SOLID); } }
    property int NbShells   { int get() { return Count(TopAbs_SHELL); } }
    property int NbFaces    { int get() { return Count(TopAbs_FACE); } }
    property int NbEdges    { int get() { return Count(TopAbs_EDGE); } }
    property int NbVertices { int get() { return Count(TopAbs_VERTEX); } }

    /// Volume of all solids in the shape (model units^3).
    property double Volume
    {
      double get()
      {
        if (IsNull) return 0.0;
        OCCT_TRY
          GProp_GProps aProps;
          BRepGProp::VolumeProperties(*myShape, aProps);
          return aProps.Mass();
        OCCT_CATCH
      }
    }

    /// Total surface area of all faces (model units^2).
    property double SurfaceArea
    {
      double get()
      {
        if (IsNull) return 0.0;
        OCCT_TRY
          GProp_GProps aProps;
          BRepGProp::SurfaceProperties(*myShape, aProps);
          return aProps.Mass();
        OCCT_CATCH
      }
    }

    /// Volume-weighted centre of mass as (x, y, z).
    array<double>^ CenterOfMass()
    {
      array<double>^ aResult = gcnew array<double>(3);
      if (IsNull) return aResult;
      OCCT_TRY
        GProp_GProps aProps;
        BRepGProp::VolumeProperties(*myShape, aProps);
        gp_Pnt aCenter = aProps.CentreOfMass();
        aResult[0] = aCenter.X();
        aResult[1] = aCenter.Y();
        aResult[2] = aCenter.Z();
        return aResult;
      OCCT_CATCH
    }

    BoundingBox GetBoundingBox()
    {
      BoundingBox aBox = {};
      if (IsNull) return aBox;
      OCCT_TRY
        Bnd_Box aBnd;
        BRepBndLib::Add(*myShape, aBnd, false);
        if (!aBnd.IsVoid())
        {
          aBnd.Get(aBox.XMin, aBox.YMin, aBox.ZMin, aBox.XMax, aBox.YMax, aBox.ZMax);
        }
        return aBox;
      OCCT_CATCH
    }

    /// Runs BRepCheck_Analyzer (topological / geometric validity check).
    bool IsValid()
    {
      if (IsNull) return false;
      OCCT_TRY
        BRepCheck_Analyzer anAnalyzer(*myShape);
        return anAnalyzer.IsValid() == true;
      OCCT_CATCH
    }

    virtual System::String^ ToString() override
    {
      if (IsNull) return "OcctShape(null)";
      return System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
        "OcctShape({0}: {1} solids, {2} faces, {3} edges, {4} vertices)",
        Type.ToString(), NbSolids, NbFaces, NbEdges, NbVertices);
    }

  private:
    int Count(TopAbs_ShapeEnum theType)
    {
      if (IsNull) return 0;
      NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aMap;
      TopExp::MapShapes(*myShape, theType, aMap);
      return aMap.Extent();
    }

    TopoDS_Shape* myShape;
  };
}
