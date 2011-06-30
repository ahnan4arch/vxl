// This is brl/bseg/boxm2/cpp/pro/processes/boxm2_cpp_batch_update_processes.cxx
#include <bprb/bprb_func_process.h>
//:
// \file
// \brief  Processes to update the scene using a set of data blocks in a batch mode
//
// \author Ozge C. Ozcanli
// \date May 12, 2011

#include <vcl_fstream.h>
#include <boxm2/io/boxm2_stream_cache.h>
#include <boxm2/io/boxm2_cache.h>
#include <boxm2/boxm2_scene.h>
#include <boxm2/boxm2_block.h>
#include <boxm2/boxm2_data_base.h>
#
//brdb stuff
#include <brdb/brdb_value.h>
#include <boxm2/boxm2_util.h>

#include <boxm2/cpp/algo/boxm2_batch_functors.h>
#include <boxm2/cpp/algo/boxm2_data_serial_iterator.h>


//: create a normalized intensity value in each cell using the segment lengths of all the rays that intersect the cell
namespace boxm2_cpp_create_norm_intensities_process_globals
{
  const unsigned n_inputs_ = 5;
  const unsigned n_outputs_ = 0;
}

bool boxm2_cpp_create_norm_intensities_process_cons(bprb_func_process& pro)
{
  using namespace boxm2_cpp_create_norm_intensities_process_globals;

  //process takes 5 inputs
  // 0) scene
  // 1) cache
  // 2) camera
  // 3) image
  // 4) image identifier
  vcl_vector<vcl_string> input_types_(n_inputs_);
  input_types_[0] = "boxm2_scene_sptr";
  input_types_[1] = "boxm2_cache_sptr";
  input_types_[2] = "vpgl_camera_double_sptr";
  input_types_[3] = "vil_image_view_base_sptr";
  input_types_[4] = "vcl_string";  //image identifier
  // process has 0 output:
  vcl_vector<vcl_string>  output_types_(n_outputs_);

  return pro.set_input_types(input_types_) && pro.set_output_types(output_types_);
}

bool boxm2_cpp_create_norm_intensities_process(bprb_func_process& pro)
{
  using namespace boxm2_cpp_create_norm_intensities_process_globals;

  if ( pro.n_inputs() < n_inputs_ ){
      vcl_cout << pro.name() << ": The input number should be " << n_inputs_<< vcl_endl;
      return false;
  }
  //get the inputs
  unsigned i = 0;
  boxm2_scene_sptr scene =pro.get_input<boxm2_scene_sptr>(i++);
  boxm2_cache_sptr cache= pro.get_input<boxm2_cache_sptr>(i++);
  vpgl_camera_double_sptr cam= pro.get_input<vpgl_camera_double_sptr>(i++);
  vil_image_view_base_sptr in_img=pro.get_input<vil_image_view_base_sptr>(i++);
  vil_image_view_base_sptr float_image=boxm2_util::prepare_input_image(in_img);
  vcl_string identifier = pro.get_input<vcl_string>(i++);

  if (vil_image_view<float> * input_image=dynamic_cast<vil_image_view<float> * > (float_image.ptr()))
  {
    vcl_vector<boxm2_block_id> vis_order=scene->get_vis_blocks(reinterpret_cast<vpgl_generic_camera<double>*>(cam.ptr()));
    if (vis_order.empty())
    {
      vcl_cout<<" None of the blocks are visible from this viewpoint"<<vcl_endl;
      return true;
    }
    boxm2_batch_update_pass0_functor pass0;

    vcl_vector<boxm2_block_id>::iterator id;

    int aux0TypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_AUX0>::prefix());
    int aux1TypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_AUX1>::prefix());
    int numObsTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_NUM_OBS_SINGLE>::prefix());
    for (id = vis_order.begin(); id != vis_order.end(); ++id)
    {
      vcl_cout<<"Block id "<<(*id)<<' ';
      boxm2_block *     blk   = cache->get_block(*id);
      boxm2_block_metadata mdata = scene->get_block_metadata(*id);
      long num_cells = mdata.sub_block_num_.x() * mdata.sub_block_num_.y() * mdata.sub_block_num_.z();
      //: first remove from memory just in case to ensure proper initialization
      cache->remove_data_base(*id,boxm2_data_traits<BOXM2_AUX0>::prefix(identifier));
      cache->remove_data_base(*id,boxm2_data_traits<BOXM2_AUX1>::prefix(identifier));
      cache->remove_data_base(*id,boxm2_data_traits<BOXM2_NUM_OBS_SINGLE>::prefix(identifier));
      //: call get_data_base method with num_bytes > 0 to avoid reading from disc, we want initialized data
      boxm2_data_base *aux0 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX0>::prefix(identifier),num_cells*aux0TypeSize,false);
      boxm2_data_base *aux1 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX1>::prefix(identifier),num_cells*aux1TypeSize,false);
      boxm2_data_base *aux2 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_NUM_OBS_SINGLE>::prefix(identifier),num_cells*numObsTypeSize,false);
      vcl_vector<boxm2_data_base*> datas;
      datas.push_back(aux0);
      datas.push_back(aux1);
      datas.push_back(aux2);
      boxm2_scene_info_wrapper *scene_info_wrapper=new boxm2_scene_info_wrapper();
      scene_info_wrapper->info=scene->get_blk_metadata(*id);

      pass0.init_data(datas,input_image);
      cast_ray_per_block<boxm2_batch_update_pass0_functor>(pass0,
                                                           scene_info_wrapper->info,
                                                           blk,
                                                           cam,
                                                           input_image->ni(),
                                                           input_image->nj());
    }
  }

  return true;
}

//: create a normalized intensity value in each cell using the segment lengths of all the rays that intersect the cell
namespace boxm2_cpp_create_aux_data_process_globals
{
  const unsigned n_inputs_ = 5;
  const unsigned n_outputs_ = 0;
}

bool boxm2_cpp_create_aux_data_process_cons(bprb_func_process& pro)
{
  using namespace boxm2_cpp_create_aux_data_process_globals;

  //process takes 5 inputs
  // 0) scene
  // 1) cache
  // 2) camera
  // 3) image
  // 4) image identifier
  vcl_vector<vcl_string> input_types_(n_inputs_);
  input_types_[0] = "boxm2_scene_sptr";
  input_types_[1] = "boxm2_cache_sptr";
  input_types_[2] = "vpgl_camera_double_sptr";
  input_types_[3] = "vil_image_view_base_sptr";
  input_types_[4] = "vcl_string";  //image identifier
  // process has 0 output:
  vcl_vector<vcl_string>  output_types_(n_outputs_);

  return pro.set_input_types(input_types_) && pro.set_output_types(output_types_);
}

bool boxm2_cpp_create_aux_data_process(bprb_func_process& pro)
{
  using namespace boxm2_cpp_create_aux_data_process_globals;

  if ( pro.n_inputs() < n_inputs_ ){
    vcl_cout << pro.name() << ": The input number should be " << n_inputs_<< vcl_endl;
    return false;
  }
  //get the inputs
  unsigned i = 0;
  boxm2_scene_sptr scene =pro.get_input<boxm2_scene_sptr>(i++);
  boxm2_cache_sptr cache= pro.get_input<boxm2_cache_sptr>(i++);
  vpgl_camera_double_sptr cam= pro.get_input<vpgl_camera_double_sptr>(i++);
  vil_image_view_base_sptr in_img=pro.get_input<vil_image_view_base_sptr>(i++);
  vil_image_view_base_sptr float_image=boxm2_util::prepare_input_image(in_img);
  vcl_string identifier = pro.get_input<vcl_string>(i++);

  if (vil_image_view<float> * input_image=dynamic_cast<vil_image_view<float> * > (float_image.ptr()))
  {
    vcl_vector<boxm2_block_id> vis_order=scene->get_vis_blocks(reinterpret_cast<vpgl_generic_camera<double>*>(cam.ptr()));
    if (vis_order.empty())
    {
      vcl_cout<<" None of the blocks are visible from this viewpoint"<<vcl_endl;
      return true;
    }

    bool foundDataType = false;

    vcl_string data_type;
    vcl_string num_obs_type;
    vcl_vector<vcl_string> apps = scene->appearances();
    int appTypeSize;
    for (unsigned int i=0; i<apps.size(); ++i) {
      if ( apps[i] == boxm2_data_traits<BOXM2_MOG3_GREY>::prefix() )
      {
          data_type = apps[i];
          foundDataType = true;
          appTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_MOG3_GREY>::prefix());
      }
      else if ( apps[i] == boxm2_data_traits<BOXM2_MOG3_GREY_16>::prefix() )
      {
          data_type = apps[i];
          foundDataType = true;
          appTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_MOG3_GREY_16>::prefix());
      }
    }
    if (!foundDataType) {
      vcl_cout<<"boxm2_cpp_create_aux_data_process ERROR: scene doesn't have BOXM2_MOG3_GREY or BOXM2_MOG3_GREY_16 data type"<<vcl_endl;
      return false;
    }

    boxm2_batch_update_pass1_functor pass1;

    vil_image_view<float> pre_inf_img(input_image->ni(),input_image->nj());
    vil_image_view<float> vis_inf_img(input_image->ni(),input_image->nj());

    vcl_vector<boxm2_block_id>::iterator id;

    int aux0TypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_AUX0>::prefix());
    for (id = vis_order.begin(); id != vis_order.end(); ++id)
    {
      vcl_cout<<"Block id "<<(*id)<<' ';
      boxm2_block *     blk   = cache->get_block(*id);
      boxm2_data_base *  alph  = cache->get_data_base(*id,boxm2_data_traits<BOXM2_ALPHA>::prefix(),0,false);
      boxm2_data_base *  mog   = cache->get_data_base(*id,data_type,0,false);

      boxm2_block_metadata mdata = scene->get_block_metadata(*id);

      //: call get_data_base method with num_bytes = 0 to read from disc
      boxm2_data_base *aux0 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX0>::prefix(identifier));
      boxm2_data_base *aux1 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX1>::prefix(identifier));

      vcl_vector<boxm2_data_base*> datas;
      datas.push_back(aux0);
      datas.push_back(aux1);
      datas.push_back(alph);
      datas.push_back(mog);
      boxm2_scene_info_wrapper *scene_info_wrapper=new boxm2_scene_info_wrapper();
      scene_info_wrapper->info=scene->get_blk_metadata(*id);

      pass1.init_data(datas,&pre_inf_img,&vis_inf_img);
      cast_ray_per_block<boxm2_batch_update_pass1_functor>(pass1,
                                                           scene_info_wrapper->info,
                                                           blk,
                                                           cam,
                                                           input_image->ni(),
                                                           input_image->nj());
    }
    //: now run pass 2 to compute cell averages of pre, post, and vis
    boxm2_batch_update_pass2_functor pass2;
    int auxTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_AUX>::prefix());
    for (id = vis_order.begin(); id != vis_order.end(); ++id)
    {
      vcl_cout<<"Block id "<<(*id)<<' ';
      boxm2_block *     blk   = cache->get_block(*id);
      boxm2_data_base *  alph  = cache->get_data_base(*id,boxm2_data_traits<BOXM2_ALPHA>::prefix(),0,false);
      boxm2_data_base *  mog   = cache->get_data_base(*id,data_type,0,false);

      boxm2_block_metadata mdata = scene->get_block_metadata(*id);

      //: call get_data_base method with num_bytes = 0 to read from disc
      boxm2_data_base *aux0 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX0>::prefix(identifier));
      boxm2_data_base *aux1 = cache->get_data_base(*id,boxm2_data_traits<BOXM2_AUX1>::prefix(identifier));

      //: generate aux in a writable mode
      boxm2_data_base *aux = cache->get_data_base(*id, boxm2_data_traits<BOXM2_AUX>::prefix(identifier),aux0->buffer_length()/aux0TypeSize*auxTypeSize,false);

      vcl_vector<boxm2_data_base*> datas;
      datas.push_back(aux0);
      datas.push_back(aux1);
      datas.push_back(alph);
      datas.push_back(mog);
      datas.push_back(aux);
      boxm2_scene_info_wrapper *scene_info_wrapper=new boxm2_scene_info_wrapper();
      scene_info_wrapper->info=scene->get_blk_metadata(*id);

      pass2.init_data(datas,&pre_inf_img);
      cast_ray_per_block<boxm2_batch_update_pass2_functor>(pass2,
                                                           scene_info_wrapper->info,
                                                           blk,
                                                           cam,
                                                           input_image->ni(),
                                                           input_image->nj());
    }
  }

  return true;
}

//: run batch update
namespace boxm2_cpp_batch_update_process_globals
{
  const unsigned n_inputs_ = 4;
  const unsigned n_outputs_ = 0;
}

bool boxm2_cpp_batch_update_process_cons(bprb_func_process& pro)
{
  using namespace boxm2_cpp_batch_update_process_globals;

  //process takes 4 inputs
  // 0) scene
  // 1) cache
  // 2) stream cache
  // 3) the pre-computed sigma normalizer table, for fast access to normalizer values given number of images
  vcl_vector<vcl_string> input_types_(n_inputs_);
  input_types_[0] = "boxm2_scene_sptr";
  input_types_[1] = "boxm2_cache_sptr";
  input_types_[2] = "boxm2_stream_cache_sptr";
  input_types_[3] = "bsta_sigma_normalizer_sptr";
  // process has 0 output:
  vcl_vector<vcl_string>  output_types_(n_outputs_);

  return pro.set_input_types(input_types_) && pro.set_output_types(output_types_);
}

bool boxm2_cpp_batch_update_process(bprb_func_process& pro)
{
  using namespace boxm2_cpp_batch_update_process_globals;

  if ( pro.n_inputs() < n_inputs_ ){
      vcl_cout << pro.name() << ": The input number should be " << n_inputs_<< vcl_endl;
      return false;
  }
  //get the inputs
  unsigned i = 0;
  boxm2_scene_sptr scene =pro.get_input<boxm2_scene_sptr>(i++);
  boxm2_cache_sptr cache= pro.get_input<boxm2_cache_sptr>(i++);
  boxm2_stream_cache_sptr str_cache = pro.get_input<boxm2_stream_cache_sptr>(i++);
  bsta_sigma_normalizer_sptr n_table = pro.get_input<bsta_sigma_normalizer_sptr>(i++);

  vcl_string data_type;
  bool foundDataType = false;
  vcl_vector<vcl_string> apps = scene->appearances();
  int appTypeSize;
  for (unsigned int i=0; i<apps.size(); ++i) {
    if ( apps[i] == boxm2_data_traits<BOXM2_MOG3_GREY>::prefix() )
    {
      data_type = apps[i];
      foundDataType = true;
      appTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_MOG3_GREY>::prefix());
    }
    else if ( apps[i] == boxm2_data_traits<BOXM2_MOG3_GREY_16>::prefix() )
    {
#if 0
      data_type = apps[i];
      foundDataType = true;
      appTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_MOG3_GREY_16>::prefix());
#endif
      vcl_cout << "In boxm2_cpp_batch_update_process ERROR: datatype BOXM2_MOG3_GREY_16 not implemented!\n";
      return false;
    }
  }
  if (!foundDataType) {
    vcl_cout<<"boxm2_cpp_batch_update_process ERROR: scene doesn't have BOXM2_MOG3_GREY or BOXM2_MOG3_GREY_16 data type"<<vcl_endl;
    return false;
  }
  vcl_cout << "Normalization factor for 0: " << n_table->normalization_factor_int(0) << '\n'
           << "Normalization factor for 1: " << n_table->normalization_factor_int(1) << '\n'
           << "Normalization factor for 20: " << n_table->normalization_factor_int(20) << '\n'
           << "Normalization factor for 40: " << n_table->normalization_factor_int(40) << '\n'
           << "Normalization factor for 60: " << n_table->normalization_factor_int(60) << '\n'
           << "Normalization factor for 80: " << n_table->normalization_factor_int(80) << vcl_endl;

  // assumes that the data of each image has been created in the data models previously
  int alphaTypeSize = (int)boxm2_data_info::datasize(boxm2_data_traits<BOXM2_ALPHA>::prefix());
  // iterate the scene block by block and write to output
  vcl_vector<boxm2_block_id> blk_ids = scene->get_block_ids();
  vcl_vector<boxm2_block_id>::iterator id;
  id = blk_ids.begin();
  for (id = blk_ids.begin(); id != blk_ids.end(); ++id) {
    boxm2_block     *  blk   = cache->get_block(*id);
    // pass num_bytes = 0 to make sure disc is read if not already in memory
    boxm2_data_base *  alph  = cache->get_data_base(*id,boxm2_data_traits<BOXM2_ALPHA>::prefix(),0,false);
    boxm2_data_base *  mog  = cache->get_data_base(*id,data_type,0,false);

    vcl_cout << "buffer length of alpha: " << alph->buffer_length() << '\n'
             << "buffer length of mog: " << mog->buffer_length() << vcl_endl;

    boxm2_batch_update_functor data_functor;
    data_functor.init_data(alph, mog, str_cache, n_table, float(blk->sub_block_dim().x()), blk->max_level());
    int data_buff_length = (int) (alph->buffer_length()/alphaTypeSize);
    vcl_cout << "data_buff_length: " << data_buff_length << '\n'
             << "data_buff_length from mog: " << (int) (mog->buffer_length()/appTypeSize) << vcl_endl;

    boxm2_data_serial_iterator<boxm2_batch_update_functor>(data_buff_length,data_functor);
  }

  return true;
}
