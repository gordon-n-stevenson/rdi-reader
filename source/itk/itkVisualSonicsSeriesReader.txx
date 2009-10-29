#ifndef __itkVisualSonicsSeriesReader_txx
#define __itkVisualSonicsSeriesReader_txx

#include "itkVisualSonicsSeriesReader.h"

#include "itkVisualSonicsImageIO.h"

namespace itk
{


template <class TOutputImage>
VisualSonicsSeriesReader< TOutputImage >
::VisualSonicsSeriesReader()
{
  this->SetImageIO( VisualSonicsImageIO::New() );
}


template <class TOutputImage>
void
VisualSonicsSeriesReader< TOutputImage >
::GenerateOutputInformation(void)
{
  typename TOutputImage::Pointer output = this->GetOutput();

  ImageRegionType largestRegion;
  typename TOutputImage::PointType     origin;

  origin.Fill( 0.0 );

  if ( this->m_FileNames.size() == 0 )
    {
    itkExceptionMacro(<< "At least one filename is required." );
    }

  SpacingType spacing = this->GenerateSubRegions();

  typename SubRegionsVectorType::const_iterator sub_it = m_SubRegionsVector.begin();
  IndexType index = sub_it->GetIndex();
  SizeType  size  = sub_it->GetSize();
  for( sub_it = m_SubRegionsVector.begin();
    sub_it != m_SubRegionsVector.end();
    ++sub_it )
    {
    size[0] += sub_it->GetSize()[0];
    }
  largestRegion.SetIndex( index );
  largestRegion.SetSize(  size  );

  output->SetOrigin( origin );
  output->SetSpacing( spacing );
  output->SetLargestPossibleRegion( largestRegion );

}


template <class TOutputImage>
void
VisualSonicsSeriesReader< TOutputImage >
::EnlargeOutputRequestedRegion( DataObject* output )
{
}


template <class TOutputImage>
void
VisualSonicsSeriesReader< TOutputImage >
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );

  os << indent << "SubRegionsVector Size: " << m_SubRegionsVector.size() << std::endl;
  for( size_t i = 0; i < m_SubRegionsVector.size(); i++ )
    {
    os << indent << "SubRegionsVector[" << i << "]:" << std::endl;
    m_SubRegionsVector[i].Print(os, indent.GetNextIndent() );
    }
}


template <class TOutputImage>
void
VisualSonicsSeriesReader< TOutputImage >
::GenerateData()
{
}


template < class TOutputImage >
typename VisualSonicsSeriesReader< TOutputImage >::SpacingType
VisualSonicsSeriesReader< TOutputImage >
::GenerateSubRegions()
{
  m_SubRegionsVector.resize( this->m_FileNames.size() );

  const int numberOfFiles = static_cast<int>( this->m_FileNames.size());
  const int FileName = ( this->m_ReverseOrder ? numberOfFiles - 1: 0 );

  typename ReaderType::Pointer first_reader = ReaderType::New();
  first_reader->SetFileName( this->m_FileNames[FileName].c_str() );
  first_reader->SetImageIO( this->m_ImageIO );

  first_reader->UpdateOutputInformation();
  SpacingType spacing = first_reader->GetOutput()->GetSpacing();

  m_SubRegionsVector[0] = first_reader->GetOutput()->GetLargestPossibleRegion();

  SizeType first_size  = first_reader->GetOutput()->GetLargestPossibleRegion().GetSize();
  IndexType first_index = first_reader->GetOutput()->GetLargestPossibleRegion().GetIndex();

  SizeType  next_size;
  IndexType next_index;
  ImageRegionType next_region;

  for ( int i = 1; i != numberOfFiles; ++i )
    {
    const int iFileName = ( this->m_ReverseOrder ? numberOfFiles - i - 1: i );

    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( this->m_FileNames[iFileName].c_str() );
    reader->SetImageIO( this->m_ImageIO );

    reader->UpdateOutputInformation();

    next_size  = reader->GetOutput()->GetLargestPossibleRegion().GetSize();
    next_index = reader->GetOutput()->GetLargestPossibleRegion().GetIndex();

    if( next_size[1]  != first_size[1] ||
	next_size[2]  != first_size[2] ||
	next_index[1] != first_index[1] ||
	next_index[2] != first_index[2] )
      {
      itkExceptionMacro( << "Images have inconsistent sizes or indices in the lateral or elevational directions." );
      }
    }


  return spacing;
}

} // end namespace itk

#endif // inclusion guard
