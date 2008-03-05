/*! @file   ExtractMetadataBase.h
 *  @brief  base class for holding and extracting VisualSonics Digital RF files and their metadata
 *
 *  @author Matt McCormick (thewtex) <matt@mmmccormick.com>
 *  @date   2008 February 25
 *
 */

#ifndef	EXTRACTMETADATABASE_H
#define EXTRACTMETADATABASE_H

#include <boost/filesystem/convenience.hpp>

namespace bf = boost::filesystem;

#include "rdiParserData.h"

namespace visual_sonics
{
  namespace cxx { class ExtractImage; };

  class ExtractMetadataBase
  {
  public:
    friend class ExtractImageBase;
    friend class cxx::ExtractImage;

    ExtractMetadataBase(const bf::path& in_file_path, 
	      const bf::path& in_file_name);

    virtual ~ExtractMetadataBase(){};

    
  protected:
    //! directory path where the .rdi and .rdb files are located
    bf::path its_in_file_path;
    //! file name of the .rdi and .rdb files minus the .rdi and .rdb
    bf::path its_in_file_name;

    rdiParserData its_rpd;

  private:
    //! parse the .rdi file metadata
    void parse_metadata();

  };
} // namespace visual_sonics


#endif // EXTRACTMETADATABASE_H
