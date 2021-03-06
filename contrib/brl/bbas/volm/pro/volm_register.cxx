#include "volm_register.h"

#include <bprb/bprb_macros.h>
#include <bprb/bprb_batch_process_manager.h>
#include <bprb/bprb_func_process.h>
#include "volm_processes.h"
#include <volm/volm_satellite_resources_sptr.h>


void volm_register::register_datatype()
{
  REGISTER_DATATYPE(volm_satellite_resources_sptr);
}

void volm_register::register_process()
{
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_create_satellite_resources_process, "volmCreateSatResourcesProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_save_satellite_resources_process, "volmSaveSatResourcesProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_load_satellite_resources_process, "volmLoadSatResourcesProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_add_satellite_resources_process, "volmAddSatelliteResourcesProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_query_satellite_resources_process, "volmQuerySatelliteResourcesProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_query_satellite_resources_kml_process, "volmQuerySatelliteResourceKmlProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_correct_rational_cameras_ransac_process, "volmCorrectRationalCamerasRANSACProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_pick_nadir_resource_process, "volmPickNadirResProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_pick_nadir_resource_pair_process, "volmPickNadirResPairProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_map_osm_process, "volmMapOSMProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_map_osm_onto_image_process, "volmMapOSMtoImage");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_map_segments_process, "volmMapSegmentsProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_generate_class_map_process, "volmGenerateClassMapProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_generate_color_class_map_process, "volmGenerateColorClassMapProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_update_class_map_process, "volmUpdateClassMapProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_find_res_pair_process, "volmFindResourcePairProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_correct_rational_cameras_ransac_process2, "volmCorrectRationalCamerasRANSACProcess2");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_get_full_path_process, "volmGetFullPathProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_find_satellite_pairs_process, "volmFindSatellitePairsProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_transfer_geoindex_id_to_str_process, "volmTransferGeoIndexIDToStr");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_transfer_geoindex_id_to_str_process2,"volmTransferGeoIndexIDToStr2");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_generate_height_map_from_ply_process, "volmGenerateHeightMapFromPlyProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_refine_bvxm_height_map_process, "volmRefineBvxmHeightMapProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_project_dem_to_sat_img_process, "volmProjectDEMtoSatImgPorcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_upsample_dem_projected_img_process, "volmUpsampleDemImgProcess");
  REG_PROCESS_FUNC_CONS(bprb_func_process, bprb_batch_process_manager, volm_generate_height_map_plot_process, "volmGenerateHeightMapPlotProcess");
}
