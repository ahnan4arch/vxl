#ifndef bvpl_global_pca_txx
#define bvpl_global_pca_txx
//:
// \file
// \author Isabel Restrepo
// \date 14-Mar-2011

#include "bvpl_global_pca.h"
#include <bvpl/bvpl_octree/sample/bvpl_pca_basis_sample.h>

#include <bxml/bxml_write.h>
#include <bxml/bxml_read.h>
#include <bxml/bxml_find.h>

#include <boxm/boxm_scene_parser.h>

#include <vnl/vnl_random.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_matrix_fixed.h>
#include <vnl/algo/vnl_symmetric_eigensystem.h>

#include <vul/vul_file.h>
#include <vcl_cassert.h>

#define DEBUG_PROJ

//: Create from xml_file
template <unsigned feature_dim>
bvpl_global_pca<feature_dim>::bvpl_global_pca(const vcl_string &path)
{
  vcl_cout << "Loading pca info from xml-file" << vcl_endl;
  int valid = 0;
  path_out_ = path;
  vcl_ifstream xml_ifs(xml_path().c_str());
  if (!xml_ifs.is_open()){
    vcl_cerr << "Error: bvpl_discover_pca_kernels - could not open xml info file: " << xml_path() << '\n';
    throw;
  }
  bxml_document doc = bxml_read(xml_ifs);
  bxml_element query("pca_global_info");
  bxml_data_sptr root = bxml_find_by_name(doc.root_element(), query);
  if (!root) {
    vcl_cerr << "Error: bvpl_discover_pca_kernels - could not parse xml root\n";
    throw;
  }


  //Parse neighborhood bounding box - units are number of voxels
  bxml_element nbbox_query("neighborhood");
  bxml_data_sptr nbbox_data = bxml_find_by_name(root, nbbox_query);
  bxml_element* nbbox_elm = dynamic_cast<bxml_element*>(nbbox_data.ptr());
  int min_x, min_y, min_z, max_x, max_y, max_z =0;
  nbbox_elm->get_attribute("min_x", min_x);
  nbbox_elm->get_attribute("min_y", min_y);
  nbbox_elm->get_attribute("min_z", min_z);
  nbbox_elm->get_attribute("max_x", max_x);
  nbbox_elm->get_attribute("max_y", max_y);
  nbbox_elm->get_attribute("max_z", max_z);

  nbbox_ = vgl_box_3d<int>(vgl_point_3d<int>(min_x, min_y, min_y), vgl_point_3d<int>(max_x, max_y, max_z));
  vcl_cout << "Neighborhood: " << nbbox_ << vcl_endl;


  //Parse Number of samples
  bxml_element properties_query("properties");
  bxml_data_sptr properties_data = bxml_find_by_name(root, properties_query);
  bxml_element* properties_elm = dynamic_cast<bxml_element*>(properties_data.ptr());
  properties_elm->get_attribute("nsamples", nsamples_);
  unsigned temp_dim;
  properties_elm->get_attribute("feature_dim", temp_dim);
  if (temp_dim!=feature_dim) {
    valid = -10;
  }
  //properties_elm->get_attribute("finest_cell_length", finest_cell_length_);
  properties_elm->get_attribute("training_fraction", training_fraction_);

  unsigned nscenes = 0;
  properties_elm->get_attribute("nscenes", nscenes);

  vcl_cout << "Number of samples: " << nsamples_ << vcl_endl
           << "Feature dimension: " << temp_dim << vcl_endl
  //       << "Finest cell length: " << finest_cell_length_ << vcl_endl
           << "Number of scenes: " << nscenes <<vcl_endl;


  //Parse scenes
  bxml_element scenes_query("scenes");
  bxml_data_sptr scenes_data = bxml_find_by_name(root, scenes_query);

  bxml_element* scenes_elm = dynamic_cast<bxml_element*>(scenes_data.ptr());
  scenes_.reserve(nscenes);
  aux_dirs_.reserve(nscenes);
  finest_cell_length_.reserve(nscenes);
  for (unsigned i =0; i<nscenes; i++) {
    vcl_stringstream scene_ss;
    scene_ss<< "scene" << i;
    vcl_string scene_path = "";
    scenes_elm->get_attribute(scene_ss.str(), scene_path);
    scenes_.push_back(scene_path);


    vcl_stringstream aux_dir_ss;
    aux_dir_ss<< "aux_dir" << i;
    vcl_string aux_dir_path = "";
    scenes_elm->get_attribute(aux_dir_ss.str(), aux_dir_path);
    aux_dirs_.push_back(aux_dir_path);

    vcl_stringstream cell_length_ss;
    cell_length_ss<< "cell_length" << i;
    double cell_length = 0.0;
    scenes_elm->get_attribute(cell_length_ss.str(), cell_length);
    finest_cell_length_.push_back(cell_length);
  }

  //Parse paths and set matrices
  bxml_element paths_query("paths");
  bxml_data_sptr paths_data = bxml_find_by_name(root, paths_query);
  bxml_element* path_elm = dynamic_cast<bxml_element*>(paths_data.ptr());


  vcl_string ifs_path;

  path_elm->get_attribute("pc_path", ifs_path);
  if (ifs_path != pc_path())
    valid = -1;
  else{
    vcl_ifstream ifs(ifs_path.c_str());
    ifs >> pc_;
    if (pc_.size()!=feature_dim*feature_dim)
      valid = -2;
  }

  path_elm->get_attribute("weights_path", ifs_path);
  if (ifs_path != weights_path())
    valid = -3;
  else{
    vcl_ifstream ifs(ifs_path.c_str());
    ifs >> weights_;
    if (weights_.size()!=feature_dim)
      valid = -4;
  }

  path_elm->get_attribute("mean_path", ifs_path);
  if (ifs_path != mean_path())
    valid = -5;
  else
  {
    if ( vul_file::exists(ifs_path))
    {
      vcl_ifstream ifs(ifs_path.c_str());
      ifs >> training_mean_;
    }
    else{
      vcl_cout << " Warning: Mean file is empty" <<vcl_endl;
      training_mean_.fill(0.0);
    }

    path_elm->get_attribute("scatter_path", ifs_path);
    if (ifs_path != scatter_path())
      valid = -6;
    else if (vul_file::exists(ifs_path)){
      vcl_ifstream(ifs_path);
      ifs_path >> scatter_;
    }
    else {
      vcl_cout << " Warning: Scatter file is empty" <<vcl_endl;
      scatter_.fill(0.0);
    }

    if (valid<0){
      vcl_cout << "bvpl_discover_pca_kernels - errors parsing pca_info.xml. Error code: " << valid << vcl_endl;
      xml_write();
    }
  }
}

//: Init auxiliary scenes and smallest cell length values
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::init()
{
  for (unsigned i = 0; i < aux_dirs_.size(); i++)
  {
    boxm_scene_base_sptr data_scene_base = load_scene(i);
    boxm_scene<boct_tree<short, float> >* data_scene = dynamic_cast<boxm_scene<boct_tree<short, float> >*> (data_scene_base.as_pointer());
    if (!data_scene){
      vcl_cerr << "Error in bvpl_global_pca<feature_dim>::init(): Could not cast data scene\n";
      return;
    }
    double finest_cell_length = data_scene->finest_cell_length();
    finest_cell_length_[i] = finest_cell_length;

    if (!(vul_file::exists(aux_dirs_[i]) && vul_file::is_directory(aux_dirs_[i]))){
      vul_file::make_directory(aux_dirs_[i]);
    }

    {
      vcl_stringstream aux_scene_ss;
      aux_scene_ss << "aux_pca_scene_" << i ;
      vcl_string aux_scene_path = aux_dirs_[i] + "/" + aux_scene_ss.str() + ".xml";
      if (!vul_file::exists(aux_scene_path)){
        vcl_cout<< "Scene: " << aux_scene_path << " does not exist, initializing" << vcl_endl;
        boxm_scene<boct_tree<short, int> > *aux_scene =
        new boxm_scene<boct_tree<short, int> >(data_scene->lvcs(), data_scene->origin(), data_scene->block_dim(), data_scene->world_dim(), data_scene->max_level(), data_scene->init_level());
        aux_scene->set_appearance_model(BOXM_INT);
        aux_scene->set_paths(aux_dirs_[i], aux_scene_ss.str());
        aux_scene->write_scene("/" + aux_scene_ss.str() +  ".xml");
      }
    }

    {
      vcl_stringstream proj_scene_ss;
      proj_scene_ss << "proj_pca_scene_" << i ;
      vcl_string proj_scene_path = aux_dirs_[i] + "/" + proj_scene_ss.str() + ".xml";
      if (!vul_file::exists(proj_scene_path)){
        vcl_cout<< "Scene: " << proj_scene_path << " does not exist, initializing" << vcl_endl;
        typedef boct_tree<short,bvpl_pca_basis_sample<10> > pca_tree_type;
        boxm_scene<pca_tree_type > *proj_scene =
        new boxm_scene<pca_tree_type >(data_scene->lvcs(), data_scene->origin(), data_scene->block_dim(), data_scene->world_dim(), data_scene->max_level(), data_scene->init_level());
        proj_scene->set_appearance_model(BVPL_PCA_BASIS_SAMPLE_10);
        proj_scene->set_paths(aux_dirs_[i], proj_scene_ss.str());
        proj_scene->write_scene("/" + proj_scene_ss.str() +  ".xml");
      }
    }
  }

  xml_write();
}

//: Compute the scatter matrix of the specified block. A random number of cells are drawn for the calculation
//  The mask block indicates, whether a cell was used for learning the scatter matrix and mean
//  Ramndom samples according to octree structure.
//  Sampling is achieved by generating uniform random cell samples.
//  Since there are more cells where the resolution is finer, then these regions get sampled more often
template <unsigned feature_dim>
bool bvpl_global_pca<feature_dim>::sample_statistics( int scene_id, int block_i, int block_j, int block_k,
                                                      vnl_matrix_fixed<double, feature_dim, feature_dim> &S,
                                                      vnl_vector_fixed<double, feature_dim> &mean,
                                                      unsigned long &nfeature)
{
  typedef boct_tree<short,float> float_tree_type;
  typedef boct_tree_cell<short,float> float_cell_type;

  typedef boct_tree<short,int> int_tree_type;
  typedef boct_tree_cell<short,int> int_cell_type;

  boxm_scene_base_sptr data_scene_base =load_scene(scene_id);
  boxm_scene_base_sptr mask_scene_base =load_aux_scene(scene_id);


  boxm_scene<boct_tree<short, float> >* data_scene = dynamic_cast<boxm_scene<boct_tree<short, float> >*>(data_scene_base.as_pointer());
  boxm_scene<boct_tree<short, int> >* mask_scene = dynamic_cast<boxm_scene<boct_tree<short, int> >*>(mask_scene_base.as_pointer());

  if (!(data_scene && mask_scene))
  {
    vcl_cerr << "Error in bvpl_global_pca<feature_dim>::sample_statistics: Could not cast input scenes\n";
    return false;
  }

  //init variables
  data_scene->unload_active_blocks();
  mask_scene->unload_active_blocks();

  mean.fill(0.0);
  S.fill(0.0);

  vnl_random rng;
  //vnl_random rng(9667566ul);

  //get the cells for this block
  if (!(data_scene->valid_index(block_i, block_j, block_k) && mask_scene->valid_index(block_i, block_j, block_k))){
    vcl_cerr << "In compute_testing_error: Invalid block\n";
    return false;
  }

  data_scene->load_block_and_neighbors(block_i, block_j, block_k);
  mask_scene->load_block(block_i, block_j, block_k);

  //get the trees
  float_tree_type* data_tree = data_scene->get_block(block_i, block_j, block_k)->get_tree();
  int_tree_type* mask_tree = data_tree->template clone_to_type<int>();
  mask_tree->init_cells(0);

  nfeature = 1;

  //2. Sample cells from this tree. The number of samples from this tree depends on the portion of scene cells that live in this tree
  vcl_vector<float_cell_type *> leaf_cells = data_tree->leaf_cells();
  vcl_vector<int_cell_type*> mask_leaves = mask_tree->leaf_cells();

  float tree_ncells = leaf_cells.size();
  unsigned long tree_nsamples = (unsigned long)tree_ncells*training_fraction_;
  double cell_length = finest_cell_length_[scene_id];
  //CAUTION: the neighborhood box was suppossed to be defined as number of regular neighbors
  //convert neighborhood box to scene coordinates
  vgl_point_3d<int> nmin = nbbox_.min_point();
  vgl_point_3d<int> nmax = nbbox_.max_point();

  vcl_cout <<" In block (" << block_i <<", " << block_j << ", " << block_k << "), number of nsamples is: " << tree_nsamples << ", cell length is: " << cell_length << vcl_endl;

  for (unsigned long i=0; i<tree_nsamples; i++)
  {
    unsigned long sample = rng.lrand32(tree_ncells-1);

    boct_tree_cell<short, float> *center_cell = leaf_cells[sample];
    vgl_point_3d<double> center_cell_centroid = data_tree->global_centroid(center_cell);

    vgl_box_3d<double> roi(cell_length*(double)nmin.x(),cell_length*(double)nmin.y(),cell_length*(double)nmin.z(),
                           cell_length*(double)nmax.x(),cell_length*(double)nmax.y(),cell_length*(double)nmax.z());
    roi.set_centroid(center_cell_centroid);

    //if neighborhood is not inclusive we would have missing features
    if (!((data_scene->get_world_bbox()).contains(roi))){
      i--;
      continue;
    }


    //3. Assemble neighborhood as a feature-vector
    vnl_vector_fixed<double, feature_dim> this_feature(0.0);

    unsigned curr_dim = 0;
    for (int z = nbbox_.min_z(); z<=nbbox_.max_z(); z++)
      for (int y = nbbox_.min_y(); y<=nbbox_.max_y(); y++)
        for (int x = nbbox_.min_x(); x<=nbbox_.max_x(); x++)
        {
          vgl_point_3d<double> neighbor_centroid(center_cell_centroid.x() + (double)x*cell_length,
                                                 center_cell_centroid.y() + (double)y*cell_length,
                                                 center_cell_centroid.z() + (double)z*cell_length);

          boct_tree_cell<short,float> *neighbor_cell = data_scene->locate_point_in_memory(neighbor_centroid);


          assert(neighbor_cell !=NULL);
          this_feature[curr_dim] = (double)neighbor_cell->data();
          curr_dim++;
        }

    assert(curr_dim == feature_dim);

    mask_leaves[sample]->set_data(1);

    //increment weights
    double rho = 1.0/(double)nfeature;
    double rho_comp = 1.0 - rho;

    // the difference vector between the sample and the mean
    vnl_vector_fixed<double, feature_dim> diff = this_feature - mean;

    //update the covariance
    S += rho_comp*outer_product(diff,diff);

    //update the mean
    mean += (rho*diff);

    ++nfeature;
    //vcl_cerr << "Feature EVD: " <<this_feature << '\n'
    //         << "Mean Feature EVD: " <<mean_feature << '\n';
  }


  --nfeature;
  // write and release memory
  mask_scene->get_block(block_i, block_j, block_k)->init_tree(mask_tree);
  mask_scene->write_active_block();

  data_scene->unload_active_blocks();
  mask_scene->unload_active_blocks();

  return true;
}


 //: Update mean and scatter, given the mean and scatter of two sets.
 //  Calculation is done according to Chan et al. Updating Formulae and a Pairwise Algorithm for Computing Sample Variances
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::combine_pairwise_statistics( const vnl_vector_fixed<double,feature_dim> &mean1,
                                                                const vnl_matrix_fixed<double,feature_dim,feature_dim> &scatter1,
                                                                double const n1,
                                                                const vnl_vector_fixed<double,feature_dim>  &mean2,
                                                                const  vnl_matrix_fixed<double,feature_dim,feature_dim> &scatter2,
                                                                double const n2,
                                                                vnl_vector_fixed<double,feature_dim> & mean_out,
                                                                vnl_matrix_fixed<double,feature_dim,feature_dim> & scatter_out,
                                                                double &n_out )
{
  n_out = n1+n2;
  mean_out = (n1*mean1 + n2*mean2)* (1.0/n_out);
  vnl_vector_fixed<double, feature_dim> d = mean2 - mean1;
  scatter_out = scatter1 + scatter2 + (n1*n2/n_out)*outer_product(d,d);
}

//: Set total scatter matrix, mean, sample, principal components and weights for this class
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::set_up_pca_evd(const vnl_matrix_fixed<double, feature_dim, feature_dim> &S,
                                                  const vnl_vector_fixed<double, feature_dim> &mean,
                                                  const double total_nsamples)
{
  scatter_ = S;
  training_mean_ = mean;
  nsamples_ = total_nsamples;

  // Compute eigenvectors(principal components) and values of S

  vnl_matrix<double> pc_temp;
  vnl_vector<double> w_temp;

  vnl_symmetric_eigensystem_compute(scatter_.as_ref(), pc_temp, w_temp);
  pc_ = pc_temp.fliplr();
  weights_=w_temp.flip();

  //save the newly set vaeiables
  this->xml_write();
}

//: Computes 10-dimensional pca projection at each voxel on the block and saves it
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::project(int scene_id, int block_i, int block_j, int block_k)
{
  typedef boct_tree<short,float> float_tree_type;
  typedef boct_tree_cell<short,float> float_cell_type;

  typedef boct_tree<short,bvpl_pca_basis_sample<10> > pca_tree_type;
  typedef boct_tree_cell<short,bvpl_pca_basis_sample<10> > pca_cell_type;

  boxm_scene_base_sptr data_scene_base =load_scene(scene_id);
  boxm_scene_base_sptr proj_scene_base =load_projection_scene(scene_id);
  boxm_scene_base_sptr aux_scene_base = load_aux_scene(scene_id);

  boxm_scene<boct_tree<short, float> >* data_scene = dynamic_cast<boxm_scene<boct_tree<short, float> >*>(data_scene_base.as_pointer());
  boxm_scene<pca_tree_type>* proj_scene = dynamic_cast<boxm_scene<pca_tree_type>*>(proj_scene_base.as_pointer());
  boxm_scene<boct_tree<short, int> >* aux_scene = dynamic_cast<boxm_scene<boct_tree<short, int> >*> (aux_scene_base.as_pointer());
  if (!(data_scene && proj_scene && aux_scene))
  {
    vcl_cerr << "Error in bvpl_global_pca<feature_dim>::sample_statistics: Could not cast input scenes\n";
    return;
  }

  //init variables
  data_scene->unload_active_blocks();
  proj_scene->unload_active_blocks();
  aux_scene->unload_active_blocks();

  //get the cells for this block
  if (!(data_scene->valid_index(block_i, block_j, block_k) && proj_scene->valid_index(block_i, block_j, block_k) && aux_scene->valid_index(block_i, block_j, block_k))){
    vcl_cerr << "In compute_testing_error: Invalid block\n";
    return;
  }

  data_scene->load_block_and_neighbors(block_i, block_j, block_k);
  proj_scene->load_block(block_i, block_j, block_k);
  aux_scene->load_block(block_i, block_j, block_k);

  //get the trees
  float_tree_type* data_tree = data_scene->get_block(block_i, block_j, block_k)->get_tree();
  pca_tree_type* proj_tree = data_tree->template clone_to_type<bvpl_pca_basis_sample<10> >();
  boct_tree<short, int>* aux_tree = aux_scene->get_block(block_i, block_j, block_k)->get_tree();

  //get leaf cells
  vcl_vector<float_cell_type *> data_leaves = data_tree->leaf_cells();
  vcl_vector<pca_cell_type *> proj_leaves = proj_tree->leaf_cells();
  vcl_vector<boct_tree_cell<short, int> *> aux_leaves = aux_tree->leaf_cells();


  double cell_length = finest_cell_length_[scene_id];

  //CAUTION: the neighborhood box was suppossed to be defined as number of regular neighbors
  //convert neighborhood box to scene coordinates
  vgl_point_3d<int> nmin = nbbox_.min_point();
  vgl_point_3d<int> nmax = nbbox_.max_point();
#ifdef DEBUG_PROJ
  double error = 0.0;
  unsigned long n_valid_cells= 0;
#endif

  for (unsigned long i =0; i<data_leaves.size(); i++)
  {
    float_cell_type* data_cell = data_leaves[i];

    //create a region around the center cell
    vgl_point_3d<double> centroid = data_tree->global_centroid(data_cell);

    //change the coordinates of enpoints to be in global coordinates abd test if they are contained in the scene
    vgl_point_3d<double> min_point_global(centroid.x() + (double)nmin.x()*cell_length, centroid.y() + (double)nmin.y()*cell_length, centroid.z() + (double)nmin.z()*cell_length);
    vgl_point_3d<double> max_point_global(centroid.x() + (double)nmax.x()*cell_length, centroid.y() + (double)nmax.y()*cell_length, centroid.z() + (double)nmax.z()*cell_length);
    if (!(data_scene->locate_point_in_memory(min_point_global) && data_scene->locate_point_in_memory(max_point_global))){
      proj_leaves[i]->set_data(bvpl_pca_basis_sample<10>());
      aux_leaves[i]->set_data(-1);
      continue;
    }

    //3. Assemble neighborhood as a feature-vector
    vnl_vector_fixed<double, feature_dim> this_feature(0.0f);

    unsigned curr_dim = 0;
    for (int z = nbbox_.min_z(); z<=nbbox_.max_z(); z++)
      for (int y = nbbox_.min_y(); y<=nbbox_.max_y(); y++)
        for (int x = nbbox_.min_x(); x<=nbbox_.max_x(); x++)
        {
          vgl_point_3d<double> neighbor_centroid(centroid.x() + (double)x*cell_length,
                                                 centroid.y() + (double)y*cell_length,
                                                 centroid.z() + (double)z*cell_length);

          boct_tree_cell<short,float> *neighbor_cell = data_scene->locate_point_in_memory(neighbor_centroid);


          if (!neighbor_cell){
            vcl_cerr << "Error in bvpl_global_pca<feature_dim>::project\n";
            return;
          }

          this_feature[curr_dim] = (double)neighbor_cell->data();
          curr_dim++;
        }


    if (curr_dim != feature_dim){
      vcl_cerr << "Error in bvpl_global_pca<feature_dim>::project\n";
      return;
    }
    this_feature-=training_mean_;

    //solve for the coefficients
    vnl_vector_fixed<double, feature_dim> a(0.0);
    a = pc_.transpose() * (this_feature);
    bvpl_pca_basis_sample<10> sample(a.extract(10));
    proj_leaves[i]->set_data(sample);

#ifdef DEBUG_PROJ
    //project as a function of number of components
    vnl_vector_fixed<double, feature_dim> feature_approx  = pc_.extract(feature_dim, 10) * sample.pca_projections_;

    //compute error
    error+=(float)((this_feature - feature_approx).squared_magnitude());
    n_valid_cells++;
#endif
  }


  // write and release memory
  proj_scene->get_block(block_i, block_j, block_k)->init_tree(proj_tree);
  proj_scene->write_active_block();
  data_scene->unload_active_blocks();

#ifdef DEBUG_PROJ
  vcl_cout << "Total error in this block: " << error/(double)n_valid_cells << vcl_endl;
#endif
}

#if 0 //Not testsed
//: Computes the projection error (as square magniture) given 10-dimensional pca projection at each voxel on the block
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::projection_error(int scene_id, int block_i, int block_j, int block_k)
{
  typedef boct_tree<short,float> float_tree_type;
  typedef boct_tree_cell<short,float> float_cell_type;

  typedef boct_tree<short,bvpl_pca_basis_sample<10> > pca_tree_type;
  typedef boct_tree_cell<short,bvpl_pca_basis_sample<10> > pca_cell_type;

  boxm_scene_base_sptr proj_scene_base = load_projection_scene(scene_id);
  boxm_scene_base_sptr error_scene_base = load_error_scene(scene_id);

  boxm_scene_base_sptr data_scene_base =load_scene(scene_id);
  boxm_scene<pca_tree_type>* proj_scene = dynamic_cast<boxm_scene<pca_tree_type>*>(proj_scene_base.as_pointer());
  boxm_scene<boct_tree<short, float> >* error_scene = dynamic_cast<boxm_scene<boct_tree<short, float> >*>(error_scene_base.as_pointer());


  if (!(data_scene && error_scene && proj_scene))
  {
    vcl_cerr << "Error in bvpl_global_pca<feature_dim>::sample_statistics: Could not cast input scenes\n";
    return;
  }

  //init variables
  data_scene->unload_active_blocks();
  proj_scene->unload_active_blocks();
  error_scene->unload_active_blocks();

  //get the cells for this block
  if (!(data_scene->valid_index(block_i, block_j, block_k) && proj_scene->valid_index(block_i, block_j, block_k) && error_scene->valid_index(block_i, block_j, block_k))){
    vcl_cerr << "In compute_testing_error: Invalid block\n";
    return;
  }

  data_scene->load_block_and_neighbors(block_i, block_j, block_k);
  proj_scene->load_block(block_i, block_j, block_k);
  error_scene->load_block(block_i, block_j, block_k);

  //get the trees
  float_tree_type* data_tree = data_scene->get_block(block_i, block_j, block_k)->get_tree();
  pca_tree_type* proj_tree = proj_scene->get_block(block_i, block_j, block_k)->get_tree();
  float_tree_type* error_tree = data_tree->template clone();
  error_tree->init_cells(-1.0);

  //get leaf cells
  vcl_vector<float_cell_type *> data_leaves = data_tree->leaf_cells();
  vcl_vector<pca_cell_type *> proj_leaves = proj_tree->leaf_cells();
  vcl_vector<float_cell_type *> error_leaves = error_tree->leaf_cells();

  double cell_length = finest_cell_length_[scene_id];

  //CAUTION: the neighborhood box was suppossed to be defined as number of regular neighbors
  //convert neighborhood box to scene coordinates
  vgl_point_3d<int> nmin = nbbox_.min_point();
  vgl_point_3d<int> nmax = nbbox_.max_point();

  for (unsigned i =0; i<data_leaves.size(); i++)
  {
    float_cell_type* data_cell = data_leaves[i];

    //create a region around the center cell
    vgl_point_3d<double> centroid = data_tree->global_centroid(data_cell);

    //change the coordinates of enpoints to be in global coordinates aad test if they are contained in the scene
    vgl_point_3d<double> min_point_global(centroid.x() + (double)nmin.x()*cell_length, centroid.y() + (double)nmin.y()*cell_length, centroid.z() + (double)nmin.z()*cell_length);
    vgl_point_3d<double> max_point_global(centroid.x() + (double)nmax.x()*cell_length, centroid.y() + (double)nmax.y()*cell_length, centroid.z() + (double)nmax.z()*cell_length);
    if (!(data_scene->locate_point_in_memory(min_point_global) && data_scene->locate_point_in_memory(max_point_global))){
      error_leaves[i]->set_data(-1.0f);
      continue;
    }

    //3. Assemble neighborhood as a feature-vector
    vnl_vector_fixed<double, feature_dim> this_feature(0.0f);

    unsigned curr_dim = 0;
    for (int z = nbbox_.min_z(); z<=nbbox_.max_z(); z++)
      for (int y = nbbox_.min_y(); y<=nbbox_.max_y(); y++)
        for (int x = nbbox_.min_x(); x<=nbbox_.max_x(); x++)
        {
          vgl_point_3d<double> neighbor_centroid(centroid.x() + (double)x*cell_length,
                                                 centroid.y() + (double)y*cell_length,
                                                 centroid.z() + (double)z*cell_length);

          boct_tree_cell<short,float> *neighbor_cell = data_scene->locate_point_in_memory(neighbor_centroid);


          if (!neighbor_cell){
            vcl_cerr << "Error in bvpl_global_pca<feature_dim>::project\n";
            return;
          }

          this_feature[curr_dim] = (double)neighbor_cell->data();
          curr_dim++;
        }


    if (curr_dim != feature_dim){
      vcl_cerr << "Error in bvpl_global_pca<feature_dim>::project\n";
      return;
    }
    this_feature-=training_mean_;

    //get the coefficients
    vnl_vector_fixed<double, feature_dim> a = proj_leaves[i]->data().pca_projections_;

    //project
    vnl_vector_fixed<double, feature_dim> feature_approx  = pc_.extract(feature_dim, 10) * a;

    //compute error
    error_leaves[i]->set_data((float)((this_feature - feature_approx).squared_magnitude()));
  }


  // write and release memory
  errror_scene->get_block(block_i, block_j, block_k)->init_tree(error_tree);
  errror_scene->write_active_block();
  data_scene->unload_active_blocks();
  proj_scene->unload_active_blocks();
}
#endif

//: Load scene info
template <unsigned feature_dim>
boxm_scene_base_sptr bvpl_global_pca<feature_dim>::load_scene (int scene_id)
{
  if (scene_id<0 || scene_id>((int)scenes_.size() -1))
  {
    vcl_cerr << "Error in bvpl_global_pca::load_scene: Invalid scene id\n";
    return NULL;
  }
  //load scene
  boxm_scene_base_sptr scene_base = new boxm_scene_base();
  boxm_scene_parser scene_parser;
  scene_base->load_scene(scenes_[scene_id], scene_parser);

  //cast scene
  boxm_scene<boct_tree<short, float > > *scene= new boxm_scene<boct_tree<short, float > >();
  if (scene_base->appearence_model() == BOXM_FLOAT){
    scene->load_scene(scene_parser);
    scene_base = scene;
  }
  else {
    vcl_cerr << "Error in bvpl_global_pca::load_scene: Invalid appearance model\n";
    return NULL;
  }

  return scene_base;
}

//: Load auxiliary scene info
template <unsigned feature_dim>
boxm_scene_base_sptr bvpl_global_pca<feature_dim>::load_aux_scene (int scene_id)
{
  if (scene_id<0 || scene_id>((int)scenes_.size() -1))
  {
    vcl_cerr << "Error in bvpl_global_pca::load_aux_scene: Invalid scene id\n";
    return NULL;
  }
  //load scene
  boxm_scene_base_sptr aux_scene_base = new boxm_scene_base();
  boxm_scene_parser aux_parser;
  vcl_stringstream aux_scene_ss;
  aux_scene_ss << aux_dirs_[scene_id] << "/aux_pca_scene_" << scene_id << ".xml";
  aux_scene_base->load_scene(aux_scene_ss.str(), aux_parser);

  //cast scene
  boxm_scene<boct_tree<short, int > > *aux_scene= new boxm_scene<boct_tree<short, int > >();
  if (aux_scene_base->appearence_model() == BOXM_INT){
    aux_scene->load_scene(aux_parser);
    aux_scene_base = aux_scene;
  }
  else {
    vcl_cerr << "Error in bvpl_global_pca::load_aux_scene: Invalid appearance model\n";
    return NULL;
  }

  return aux_scene_base;
}

//: Load auxiliary scene info
template <unsigned feature_dim>
boxm_scene_base_sptr bvpl_global_pca<feature_dim>::load_projection_scene (int scene_id)
{
  if (scene_id<0 || scene_id>((int)scenes_.size() -1))
  {
    vcl_cerr << "Error in bvpl_global_pca::load_projection_scene: Invalid scene id\n";
    return NULL;
  }
  //load scene
  boxm_scene_base_sptr proj_scene_base = new boxm_scene_base();
  boxm_scene_parser proj_parser;
  vcl_stringstream proj_scene_ss;
  proj_scene_ss << aux_dirs_[scene_id] << "/proj_pca_scene_" << scene_id << ".xml";
  proj_scene_base->load_scene(proj_scene_ss.str(), proj_parser);

  //cast scene
  typedef boct_tree<short,bvpl_pca_basis_sample<10> > pca_tree_type;
  boxm_scene<pca_tree_type > *proj_scene= new boxm_scene<pca_tree_type >();
  if (proj_scene_base->appearence_model() == BVPL_PCA_BASIS_SAMPLE_10){
    proj_scene->load_scene(proj_parser);
    proj_scene_base = proj_scene;
  }
  else {
    vcl_cerr << "Error in bvpl_global_pca::load_proj_scene: Invalid appearance model\n";
    return NULL;
  }

  return proj_scene_base;
}

#if 0
//: Load auxiliary scene info
template <unsigned feature_dim>
boxm_scene_base_sptr bvpl_global_pca<feature_dim>::load_error_scene (int scene_id)
{
  if (scene_id<0 || scene_id>((int)scenes_.size() -1))
  {
    vcl_cerr << "Error in bvpl_global_pca::load_error_scene: Invalid scene id\n";
    return NULL;
  }
  //load scene
  boxm_scene_base_sptr error_scene_base = new boxm_scene_base();
  boxm_scene_parser error_parser;
  vcl_stringstream error_scene_ss;
  error_scene_ss << aux_dirs_[scene_id] << "/error_pca_scene_" << scene_id << ".xml";
  error_scene_base->load_scene(error_scene_ss.str(), error_parser);

  //cast scene
  typedef boct_tree<short,bvpl_pca_basis_sample<10> > pca_tree_type;
  boxm_scene<pca_tree_type > *error_scene= new boxm_scene<pca_tree_type >();
  if (error_scene_base->appearence_model() == BVPL_PCA_BASIS_SAMPLE_10){
    error_scene->load_scene(error_parser);
    error_scene_base = error_scene;
  }
  else {
    vcl_cerr << "Error in bvpl_global_pca::load_error_scene: Invalid appearance model\n";
    return NULL;
  }

  return error_scene_base;
}
#endif

//: Write this class to xml file
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::xml_write()
{
  bxml_document doc;
  bxml_element *root = new bxml_element("pca_global_info");
  doc.set_root_element(root);
  root->append_text("\n");

  bxml_element* paths = new bxml_element("paths");
  paths->append_text("\n");
  paths->set_attribute("pc_path", pc_path() );
  paths->set_attribute("weights_path", weights_path() );
  paths->set_attribute("mean_path", mean_path());
  paths->set_attribute("scatter_path", scatter_path());
  root->append_data(paths);
  root->append_text("\n");

  bxml_element* neighborhood = new bxml_element("neighborhood");
  neighborhood->append_text("\n");
  neighborhood->set_attribute("min_x", nbbox_.min_point().x());
  neighborhood->set_attribute("min_y", nbbox_.min_point().y());
  neighborhood->set_attribute("min_z", nbbox_.min_point().z());
  neighborhood->set_attribute("max_x", nbbox_.max_point().x());
  neighborhood->set_attribute("max_y", nbbox_.max_point().y());
  neighborhood->set_attribute("max_z", nbbox_.max_point().z());
  root->append_data(neighborhood);
  root->append_text("\n");

  bxml_element* properties = new bxml_element("properties");
  properties->append_text("\n");
  properties->set_attribute("nsamples", nsamples_);
  properties->set_attribute("feature_dim", feature_dim);
  //properties->set_attribute("finest_cell_length", finest_cell_length_);
  properties->set_attribute("nscenes", scenes_.size());
  properties->set_attribute("training_fraction", training_fraction_);

  root->append_data(properties);
  root->append_text("\n");

  //write the scenes
  bxml_element* scenes_elm = new bxml_element("scenes");
  scenes_elm->append_text("\n");

  for (unsigned i =0; i<scenes_.size(); i++) {
    vcl_stringstream scene_ss;
    scene_ss<< "scene" << i;
    vcl_stringstream aux_dir_ss;
    aux_dir_ss<< "aux_dir" << i;
    vcl_stringstream cell_length_ss;
    cell_length_ss<< "cell_length" << i;
    scenes_elm->set_attribute(scene_ss.str(), scenes_[i]);
    scenes_elm->set_attribute(aux_dir_ss.str(), aux_dirs_[i]);
    scenes_elm->set_attribute(cell_length_ss.str(), finest_cell_length_[i]);
  }

  root->append_data(scenes_elm);
  root->append_text("\n");

  //write to disk
  vcl_ofstream os(xml_path().c_str());
  bxml_write(os, doc);
  os.close();

  //: Write pca main matrices -other matrices aren't class variables and should have been written during computation time

  write_pca_matrices();
}

//: Write a PCA file
template <unsigned feature_dim>
void bvpl_global_pca<feature_dim>::write_pca_matrices()
{
  vcl_ofstream pc_ofs(pc_path().c_str());
  pc_ofs.precision(15);
  vcl_ofstream weights_ofs(weights_path().c_str());
  weights_ofs.precision(15);
  vcl_ofstream mean_ofs(mean_path().c_str());
  mean_ofs.precision(15);
  vcl_ofstream scatter_ofs(scatter_path().c_str());
  scatter_ofs.precision(15);

  pc_ofs << pc_;
  pc_ofs.close();
  weights_ofs << weights_;
  weights_ofs.close();
  mean_ofs << training_mean_;
  mean_ofs.close();
  scatter_ofs << scatter_;
  scatter_ofs.close();
}

#define BVPL_GLOBAL_PCA(feature_dim) \
template class bvpl_global_pca<feature_dim>;

#endif // bvpl_global_pca_txx