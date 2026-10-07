// ShapeIO.h - STEP / IGES / BREP / STL import and export.
#pragma once

#include "Common.h"
#include "OcctShape.h"

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <StlAPI_Writer.hxx>

namespace OcctProxy
{
  /// File import / export helpers. Paths may contain non-ASCII characters.
  public ref class ShapeIO abstract sealed
  {
  public:
    // ---- STEP --------------------------------------------------------------

    /// Writes the shape as STEP AP214 (schema can be "AP203", "AP214" or "AP242").
    static void ExportStep(OcctShape^ shape, System::String^ path, System::String^ schema)
    {
      CheckArg(shape);
      OCCT_TRY
        if (!System::String::IsNullOrEmpty(schema))
        {
          Interface_Static::SetCVal("write.step.schema", ToAscii(schema).ToCString());
        }
        STEPControl_Writer aWriter;
        if (aWriter.Transfer(shape->Native(), STEPControl_AsIs) != IFSelect_RetDone)
        {
          throw gcnew OcctException("STEP transfer failed");
        }
        if (aWriter.Write(ToAscii(path).ToCString()) != IFSelect_RetDone)
        {
          throw gcnew OcctException(System::String::Concat("STEP write failed: ", path));
        }
      OCCT_CATCH
    }

    static void ExportStep(OcctShape^ shape, System::String^ path)
    {
      ExportStep(shape, path, "AP214");
    }

    /// Reads a STEP file; all root entities are merged into one shape (compound if several).
    static OcctShape^ ImportStep(System::String^ path)
    {
      OCCT_TRY
        STEPControl_Reader aReader;
        if (aReader.ReadFile(ToAscii(path).ToCString()) != IFSelect_RetDone)
        {
          throw gcnew OcctException(System::String::Concat("STEP read failed: ", path));
        }
        aReader.TransferRoots();
        TopoDS_Shape aShape = aReader.OneShape();
        if (aShape.IsNull())
        {
          throw gcnew OcctException("STEP file contains no transferable shape");
        }
        return gcnew OcctShape(aShape);
      OCCT_CATCH
    }

    // ---- IGES --------------------------------------------------------------

    static void ExportIges(OcctShape^ shape, System::String^ path)
    {
      CheckArg(shape);
      OCCT_TRY
        IGESControl_Writer aWriter("MM", 0 /* faces mode */);
        if (!aWriter.AddShape(shape->Native()))
        {
          throw gcnew OcctException("IGES transfer failed");
        }
        aWriter.ComputeModel();
        if (!aWriter.Write(ToAscii(path).ToCString()))
        {
          throw gcnew OcctException(System::String::Concat("IGES write failed: ", path));
        }
      OCCT_CATCH
    }

    static OcctShape^ ImportIges(System::String^ path)
    {
      OCCT_TRY
        IGESControl_Reader aReader;
        if (aReader.ReadFile(ToAscii(path).ToCString()) != IFSelect_RetDone)
        {
          throw gcnew OcctException(System::String::Concat("IGES read failed: ", path));
        }
        aReader.TransferRoots();
        TopoDS_Shape aShape = aReader.OneShape();
        if (aShape.IsNull())
        {
          throw gcnew OcctException("IGES file contains no transferable shape");
        }
        return gcnew OcctShape(aShape);
      OCCT_CATCH
    }

    // ---- BREP (OCCT native) ------------------------------------------------

    static void ExportBrep(OcctShape^ shape, System::String^ path)
    {
      CheckArg(shape);
      OCCT_TRY
        if (!BRepTools::Write(shape->Native(), ToAscii(path).ToCString()))
        {
          throw gcnew OcctException(System::String::Concat("BREP write failed: ", path));
        }
      OCCT_CATCH
    }

    static OcctShape^ ImportBrep(System::String^ path)
    {
      OCCT_TRY
        BRep_Builder aBuilder;
        TopoDS_Shape aShape;
        if (!BRepTools::Read(aShape, ToAscii(path).ToCString(), aBuilder))
        {
          throw gcnew OcctException(System::String::Concat("BREP read failed: ", path));
        }
        return gcnew OcctShape(aShape);
      OCCT_CATCH
    }

    // ---- STL (mesh export) -------------------------------------------------

    /// Triangulates the shape (linear deflection in model units) and writes an STL file.
    static void ExportStl(OcctShape^ shape, System::String^ path, double linearDeflection, bool ascii)
    {
      CheckArg(shape);
      OCCT_TRY
        BRepMesh_IncrementalMesh aMesher(shape->Native(), linearDeflection, false, 0.5, true);
        StlAPI_Writer aWriter;
        aWriter.ASCIIMode() = ascii ? true : false;
        if (!aWriter.Write(shape->Native(), ToAscii(path).ToCString()))
        {
          throw gcnew OcctException(System::String::Concat("STL write failed: ", path));
        }
      OCCT_CATCH
    }

    static void ExportStl(OcctShape^ shape, System::String^ path)
    {
      ExportStl(shape, path, 0.1, false);
    }

  private:
    static void CheckArg(OcctShape^ shape)
    {
      if (shape == nullptr || shape->IsNull) throw gcnew System::ArgumentException("Shape is null");
    }
  };
}
