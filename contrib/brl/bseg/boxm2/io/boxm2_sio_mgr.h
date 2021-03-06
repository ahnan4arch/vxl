#ifndef boxm2_sio_mgr_h_
#define boxm2_sio_mgr_h_
//:
// \file
// \brief Loads blocks and data from ID's and data_types with blocking.
//  If file is not available, will return null.
#include <boxm2/boxm2_block.h>
#include <boxm2/basic/boxm2_block_id.h>
#include <boxm2/boxm2_data_traits.h>
#include <boxm2/boxm2_data.h>
#include <vul/vul_file.h>
#include <vcl_iostream.h>

//: enabling to allow different filesystems to load blocks
typedef enum {LOCAL=0, HDFS} BOXM2_IO_FS_TYPE;

//: disk level storage class.
//  handles all of the synchronous IO read and write requests
class boxm2_sio_mgr
{
  public:
    //: loads block from disk
    static boxm2_block*   load_block(vcl_string dir, boxm2_block_id block_id,
                                     BOXM2_IO_FS_TYPE fs_type=LOCAL);

    static boxm2_block* load_block(vcl_string dir, boxm2_block_id block_id,
                                   boxm2_block_metadata data, BOXM2_IO_FS_TYPE fs_type=LOCAL);

    //: saves block to disk
    static void save_block(vcl_string dir, boxm2_block* block);

    //: load data from disk
    template <boxm2_data_type data_type>
    static boxm2_data<data_type> *  load_block_data(vcl_string dir, boxm2_block_id block_id);

    //: load data generically
    // loads a generic boxm2_data_base* from disk (given data_type string prefix)
    static boxm2_data_base*  load_block_data_generic(vcl_string dir, boxm2_block_id id,
                                                     vcl_string data_type, BOXM2_IO_FS_TYPE fs_type=LOCAL);

    //: saves data to disk
    template <boxm2_data_type data_type>
    static void save_block_data(vcl_string dir, boxm2_block_id block_id, boxm2_data<data_type> * block_data );

    //: saves data generically
    // generically saves data_base * to disk (given prefix)
    static void save_block_data_base(vcl_string dir, boxm2_block_id block_id, boxm2_data_base* data, vcl_string prefix);

  private:
    static char* load_from_hdfs(vcl_string filepath, unsigned long &numBytes);
};

template <boxm2_data_type data_type>
boxm2_data<data_type> *  boxm2_sio_mgr::load_block_data(vcl_string dir, boxm2_block_id block_id)
{
    return (boxm2_data<data_type>*)
           boxm2_sio_mgr::load_block_data_generic(dir, block_id, boxm2_data_traits<data_type>::prefix());

#if 0
    // file name
    vcl_string filename = dir + boxm2_data_traits<data_type>::prefix() + "_" + block_id.to_string() + ".bin";

    //get file size
    unsigned long numBytes=vul_file::size(filename);

    //Read bytes into stream
    char * bytes = new char[numBytes];
    vcl_ifstream myFile (filename.c_str(), vcl_ios::in | vcl_ios::binary);
    myFile.read(bytes, numBytes);
    if (!myFile) {
        vcl_cerr<<"boxm2_sio_mgr::load_data cannot read file "<<filename<<vcl_endl;
        return NULL;
    }

    //instantiate new block
    return new boxm2_data<data_type>(bytes,numBytes,block_id);
#endif // 0
}

//: saves block to disk
template <boxm2_data_type data_type>
void boxm2_sio_mgr::save_block_data(vcl_string dir, boxm2_block_id block_id, boxm2_data<data_type> * block_data )
{
    vcl_string filename = dir + boxm2_data_traits<data_type>::prefix() + "_" + block_id.to_string() + ".bin";

    char * bytes = block_data->data_buffer();
    vcl_ofstream myFile (filename.c_str(), vcl_ios::out | vcl_ios::binary);
    myFile.write(bytes, block_data->buffer_length());
    myFile.close();
    return;
}

#endif // boxm2_sio_mgr_h_
