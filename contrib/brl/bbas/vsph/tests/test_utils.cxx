#include <testlib/testlib_test.h>
#include <vsph/vsph_unit_sphere.h>
#include <vsph/vsph_sph_point_2d.h>
#include <vsph/vsph_sph_box_2d.h>
#include <vsph/vsph_utils.h>
#include <vgl/vgl_polygon.h>
#include <bpgl/bpgl_camera_utils.h>
#include <vpl/vpl.h>


static void test_utils()
{
  unsigned ni = 1280, nj = 720;
  double nid = static_cast<double>(ni), njd = static_cast<double>(nj);
  double heading = 180.0, tilt = 90.0, roll =0.0, altitude = 1.0;
  double top_fov = 10.0;
  double right_fov = 17.47;
  vpgl_perspective_camera<double> cam =
    bpgl_camera_utils::camera_from_kml(ni, nj, right_fov, top_fov,altitude,
                                       heading, tilt, roll);

    double elevation = 0.0, azimuth = 0.0;
  // ==============   test ray direction on unit sphere ======
  vsph_utils::ray_spherical_coordinates(cam, 640.0, 360.0, elevation,
                                        azimuth, "degrees");
  double er = vcl_fabs(elevation - (180.0-tilt));
  er += vcl_fabs(azimuth - (90.0 -heading));
  TEST_NEAR("ray spherical coordinates", er, 0.0, 0.001);

  // ================= test project poly onto sphere =======
  // remove roll for poly angle tests
  roll = 0.0;
  vpgl_perspective_camera<double> cam0 =
    bpgl_camera_utils::camera_from_kml(nid, njd, right_fov, top_fov,
                                       altitude, heading, tilt, roll);

  vgl_point_2d<double> p0(640.0, 0.0), p1(1280.0, 0.0);
  vgl_point_2d<double> p2(1280.0, 360.0), p3(640.0, 360.0);//<==principal pt
  vcl_vector<vgl_point_2d<double> > sheet;
  sheet.push_back(p0);  sheet.push_back(p1);
  sheet.push_back(p2);  sheet.push_back(p3);
  vgl_polygon<double> image_poly, sph_poly;
  image_poly.push_back(sheet);
  vcl_cout << "in image\n" << image_poly << '\n';
  sph_poly =
   vsph_utils::project_poly_onto_unit_sphere(cam0, image_poly, "degrees");
  vcl_cout << "on sphere\n" << sph_poly << '\n';
  vgl_point_2d<double> sp0 = sph_poly[0][0];
  vgl_point_2d<double> sp2 = sph_poly[0][2];
  vgl_point_2d<double> sp3 = sph_poly[0][3];
  //x is elevation and y is azimuth
  double t_fov = vcl_fabs(sp0.x()-sp3.x());
  double r_fov = vcl_fabs(sp2.y()-sp3.y());
  // shouldn't expect exact recovery since focal length is
  // the average of top and right fov
  er = vcl_fabs(t_fov-top_fov) + vcl_fabs(r_fov-right_fov);
  TEST_NEAR("spherical polygon ", er, 0.0, 1.0);//1 degree

#if 0
  // just to get an idea about the ray and display on the unit sphere
  double u[5]={0.0, 1280.0, 1280.0, 0.0, 640.0};
  double v[5]={0.0, 0.0, 720.0, 720.0, 360.0};
  vsph_sph_box_2d box;
  for (int i = 0; i<5; ++i){
    vsph_utils::ray_spherical_coordinates(cam, u[i], v[i], elevation, azimuth, "radians");
    vcl_cout << '(' << u[i] << ' ' << v[i] << "):=(" << elevation << ' ' << azimuth << ")\n";
    vsph_sph_point_2d pt(elevation, azimuth);
    box.add(pt);
  }
  box.print(vcl_cout, false);
  vsph_sph_point_2d tp(1.59548,-1.43553);
  bool in = box.contains(tp);
  vsl_b_ifstream is("c:/Users/mundy/VisionSystems/Finder/VolumetricQuery/unit_sphere_2.vsl");
  vsph_unit_sphere_sptr usph_ptr;
  vsl_b_read(is, usph_ptr);
  vcl_string display_path = "c:/Users/mundy/VisionSystems/Finder/VolumetricQuery/box_on_sphere.wrl";
  vcl_vector<float> ndef(3, 0.5f);
  vcl_vector<vcl_vector<float> > reg_color(usph_ptr->size(), ndef);
  vsph_unit_sphere::const_iterator sit = usph_ptr->begin();
  vcl_vector<float> c(3, 0.0f);
  c[0] = 1.0f;
  unsigned idx = 0;
  for (; sit != usph_ptr->end(); ++sit, ++idx){
    const vsph_sph_point_2d & sp = *sit;

    if (box.contains(sp)) {
      if (sp.phi_<-0.305|| sp.phi_>0.305) {
        sp.print(vcl_cout); vcl_cout << '\n';
      }
      reg_color[idx]= c;
    }
  }
  usph_ptr->display_region_color(display_path, reg_color);
#endif
}

TESTMAIN(test_utils);

