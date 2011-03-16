#ifndef boxm2_ocl_processes_h_
#define boxm2_ocl_processes_h_

#include <bprb/bprb_func_process.h>
#include <bprb/bprb_macros.h>

//the init functions
DECLARE_FUNC_CONS(boxm2_create_opencl_cache_process);
DECLARE_FUNC_CONS(boxm2_ocl_render_expected_image_process);
DECLARE_FUNC_CONS(boxm2_ocl_render_expected_depth_process);
DECLARE_FUNC_CONS(boxm2_ocl_update_histogram_process);
DECLARE_FUNC_CONS(boxm2_ocl_query_hist_data_process);
DECLARE_FUNC_CONS(boxm2_ocl_batch_probability_process);
DECLARE_FUNC_CONS(boxm2_ocl_query_cell_data_process);
#endif