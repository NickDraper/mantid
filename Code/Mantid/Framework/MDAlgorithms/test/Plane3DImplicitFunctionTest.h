#ifndef MANTID_MDALGORITHMS_MDPLANEIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDPLANEIMPLICITFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include <MantidKernel/Matrix.h>
#include "MantidMDAlgorithms/Plane3DImplicitFunction.h"
#include <boost/shared_ptr.hpp>


using namespace Mantid::MDAlgorithms;


//=====================================================================================
// Functional Tests
//=====================================================================================
class Plane3DImplicitFunctionTest: public CxxTest::TestSuite
{
private:

  Mantid::MDAlgorithms::NormalParameter normal;
  Mantid::MDAlgorithms::OriginParameter origin;
  Mantid::MDAlgorithms::WidthParameter width;
  const double PI;

public:

  Plane3DImplicitFunctionTest() :
    normal(1, 1, 1), origin(2, 3, 4), width(2), PI(3.14159265)
  {
  }

  void testPlaneImplicitFunctionConstruction(void)
  {
    NormalParameter normalParam(1, 0, 0);

    Plane3DImplicitFunction plane(normalParam, origin, width);
    TSM_ASSERT_EQUALS("Normal x component not wired-up correctly", 1, plane.getNormalX());
    TSM_ASSERT_EQUALS("Normal y component not wired-up correctly", 0, plane.getNormalY());
    TSM_ASSERT_EQUALS("Normal z component not wired-up correctly", 0, plane.getNormalZ());
    TSM_ASSERT_EQUALS("Origin x component not wired-up correctly", 2, plane.getOriginX());
    TSM_ASSERT_EQUALS("Origin y component not wired-up correctly", 3, plane.getOriginY());
    TSM_ASSERT_EQUALS("Origin z component not wired-up correctly", 4, plane.getOriginZ());
    TSM_ASSERT_EQUALS("Width component not wired-up correctly", 2, plane.getWidth());

  }

  /** Perform the test with a standard plane */
  bool do_test(double x, double y, double z)
  {
    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary
    Plane3DImplicitFunction plane(tNormal, tOrigin, tWidth);
    Mantid::coord_t coords[3] = {x,y,z};
    return plane.isPointContained(coords);
  }

  void testEvaluateInsidePointReflectNormal() //Test that plane automatically relects normals where necessary.
  {
    NormalParameter tNormal(1, 2, 3);
    NormalParameter rNormal = tNormal.reflect();
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    Plane3DImplicitFunction plane(rNormal, tOrigin, tWidth);

    Mantid::coord_t coords[3] = {0.999,2,3};
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane after the normal was reflected.", plane.isPointContained(coords));
  }

  void testEvaluatePointOutsideForwardPlane()
  {
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !do_test(1.001,2.001,3.001));
  }

  void testEvaluatePointOutsideBackwardPlane()
  {
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.",  !do_test(-1.001,-2.001,-3.001));
  }

  void testEvaluateOnPlanePointDecreaseX()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.",
        do_test(0.999,2,3));
  }

  void testEvaluateOnPlanePointIncreaseX()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.",
        !do_test(1.001,2,3));
  }

  void testEvaluateOnPlanePointDecreaseY()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point y-value.",
        do_test(1,1.999,3));
  }

  void testEvaluateOnPlanePointIncreaseY()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.",
        !do_test(1,2.001,3));
  }

  void testEvaluateOnPlanePointDecreaseZ()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.",
        do_test(1,2,2.999));
  }

  void testEvaluateOnPlanePointIncreaseZ()
  {
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.",
        !do_test(1,2,3.001));
  }


  void testToXML()
  {
    NormalParameter tNormal(1, 0, 0);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(3);
    Plane3DImplicitFunction plane(tNormal, tOrigin, tWidth);
    TSM_ASSERT_EQUALS("The xml generated by this function did not match the expected schema.", "<Function><Type>Plane3DImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>0.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>WidthParameter</Type><Value>3.0000</Value></Parameter></ParameterList></Function>", plane.toXMLString());
  }

  void testEqual()
  {
    NormalParameter n(1, 2, 3);
    OriginParameter o(4, 5, 6);
    WidthParameter width(10);
    Plane3DImplicitFunction A(n, o,  width);
    Plane3DImplicitFunction B(n, o,  width);
    TSM_ASSERT_EQUALS("These two objects should be considered equal.", A, B);
  }

  void testNotEqual()
  {
    NormalParameter n1(1, 2, 3);
    OriginParameter o1(4, 5, 6);
    WidthParameter width1(10);
    NormalParameter n2(0, 0, 0);
    OriginParameter o2(0, 0, 0);
    WidthParameter width2(0);
    Plane3DImplicitFunction A(n1, o1, width1); //Base comparison
    Plane3DImplicitFunction B(n2, o1, width1); //Differ normal only
    Plane3DImplicitFunction C(n1, o2, width1); //Differ origin only
    Plane3DImplicitFunction D(n1, o1, width2); //Differ width only
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, B);
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, C);
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, D);
  }


};

#endif /* MANTID_MDALGORITHMS_MDPLANEIMPLICITFUNCTIONTEST_H_ */

