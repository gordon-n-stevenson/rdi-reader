#ifndef __itkFFTW1DRealToComplexConjugateImageFilter_txx
#define __itkFFTW1DRealToComplexConjugateImageFilter_txx

#include "itkFFTW1DRealToComplexConjugateImageFilter.h"
#include "itkFFTRealToComplexConjugateImageFilter.txx"
#include <iostream>
#include "itkIndent.h"
#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkImageLinearIteratorWithIndex.h"
#include "itkMetaDataObject.h"

namespace itk
{
/** TODO:  There should be compile time type checks so that
           if only USE_FFTW1DF is defined, then only floats are valid.
           and if USE_FFTW1DD is defined, then only doubles are valid.
*/

template <typename TPixel, unsigned int Dimension>
void
FFTW1DRealToComplexConjugateImageFilter<TPixel,Dimension>::
GenerateData()
{
  // get pointers to the input and output
  typename TInputImageType::ConstPointer  inputPtr  = this->GetInput();
  typename TOutputImageType::Pointer      outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // allocate output buffer memory
  outputPtr->SetBufferedRegion( outputPtr->GetRequestedRegion() );
  outputPtr->Allocate();

  const typename TInputImageType::SizeType&   inputSize
    = inputPtr->GetRequestedRegion().GetSize();
  const typename TOutputImageType::SizeType&   outputSize
    = outputPtr->GetRequestedRegion().GetSize();

  // figure out sizes
  // size of input and output aren't the same which is handled in the superclass,
  // sort of.
  // the input size and output size only differ in the fastest moving dimension
  unsigned int total_inputSize = inputSize[this->m_Direction];
  unsigned int total_outputSize = outputSize[this->m_Direction];

  if(this->m_PlanComputed)            // if we've already computed a plan
    {
    // if the image sizes aren't the same,
    // we have to compute the plan again
    if(this->m_LastImageSize != total_inputSize)
      {
      delete [] this->m_InputBuffer;
      delete [] this->m_OutputBuffer;
      FFTW1DProxyType::DestroyPlan(this->m_Plan);
      this->m_PlanComputed = false;
      }
    }
  if(!this->m_PlanComputed)
    {
    try
      {
      this->m_InputBuffer = new TPixel[total_inputSize];
      this->m_OutputBuffer =
        new typename FFTW1DProxyType::ComplexType[total_outputSize];
      }
    catch( std::bad_alloc & )
      {
      itkExceptionMacro("Problem allocating memory for internal computations");
      }
    this->m_LastImageSize = total_inputSize;
    this->m_Plan = FFTW1DProxyType::Plan_dft_r2c_1d(inputSize[this->m_Direction],
                                         this->m_InputBuffer,
                                         this->m_OutputBuffer,
                                         FFTW_ESTIMATE);
    this->m_PlanComputed = true;
    }

  typedef itk::ImageLinearConstIteratorWithIndex< TInputImageType >  InputIteratorType;
  typedef itk::ImageLinearIteratorWithIndex< TOutputImageType >      OutputIteratorType;
  InputIteratorType inputIt( inputPtr, inputPtr->GetRequestedRegion() );
  // the output region should be the same as the input region in the non-fft directions
  OutputIteratorType outputIt( outputPtr, outputPtr->GetRequestedRegion() );

  inputIt.SetDirection(this->m_Direction);
  outputIt.SetDirection(this->m_Direction);

  TPixel* inputBufferIt = this->m_InputBuffer;
  typename FFTW1DProxyType::ComplexType* outputBufferIt = this->m_OutputBuffer;

  // for every fft line
  for( inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd();
    outputIt.NextLine(), inputIt.NextLine() )
    {
    // copy the input line into our buffer
    inputIt.GoToBeginOfLine();
    inputBufferIt = this->m_InputBuffer;
    while( !inputIt.IsAtEndOfLine() )
      {
      *inputBufferIt = inputIt.Get();
      ++inputIt;
      ++inputBufferIt;
      }

    // do the transform
    FFTW1DProxyType::Execute(this->m_Plan);

    // copy the output from the buffer into our line
    outputBufferIt = this->m_OutputBuffer;
    outputIt.GoToBeginOfLine();
    while( !outputIt.IsAtEndOfLine() )
      {
      outputIt.Set( *(reinterpret_cast<typename OutputIteratorType::PixelType*>(outputBufferIt)) );
      ++outputIt;
      ++outputBufferIt;
      }
    }
}

template < class TPixel , unsigned int Dimension >
void
FFTW1DRealToComplexConjugateImageFilter < TPixel , Dimension >
::GenerateOutputInformation()
{
  // call the ImageToImageFilter's implementation of this method
  // the SuperClass has stuff in it that really should be the FFTW classes
  typedef ImageToImageFilter< Image< TPixel , Dimension >,
	                                  Image< std::complex< TPixel > , Dimension > >
					    ImageToImageFilterType;

  this->ImageToImageFilterType::GenerateOutputInformation();

  // get pointers to the input and output
  typename TInputImageType::ConstPointer  inputPtr  = this->GetInput();
  typename TOutputImageType::Pointer      outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  //
  // This is all based on the same function in itk::ShrinkImageFilter
  // ShrinkImageFilter also modifies the image spacing, but spacing
  // will be ignored
  const typename TInputImageType::SizeType&   inputSize
    = inputPtr->GetLargestPossibleRegion().GetSize();
  const typename TInputImageType::IndexType&  inputStartIndex
    = inputPtr->GetLargestPossibleRegion().GetIndex();

  typename TOutputImageType::SizeType     outputSize;
  typename TOutputImageType::IndexType    outputStartIndex;

  for (unsigned int i = 0; i < TOutputImageType::ImageDimension; i++)
    {
    outputSize[i] = inputSize[i];
    outputStartIndex[i] = inputStartIndex[i];
    }
  //
  // in 4.3.4 of the FFTW documentation, they indicate the size of
  // of a real-to-complex FFT is N * N ... + (N /2+1)
  //                              1   2        d
  // complex numbers.
  // static_cast prob. not necessary but want to make sure integer
  // division is used.
  unsigned int direction = this->m_Direction;
  outputSize[direction] = static_cast<unsigned int>(inputSize[direction])/2 + 1;

  //
  // the halving of the input size hides the actual size of the input.
  // to get the same size image out of the IFFT, need to send it as
  // Metadata.
  typedef typename TOutputImageType::SizeType::SizeValueType SizeScalarType;
  itk::MetaDataDictionary &OutputDic=outputPtr->GetMetaDataDictionary();
  itk::EncapsulateMetaData<SizeScalarType>(OutputDic,
                                       std::string("FFT_Actual_RealImage_Size"),
                                                     inputSize[direction]);
  typename TOutputImageType::RegionType outputLargestPossibleRegion;
  outputLargestPossibleRegion.SetSize( outputSize );
  outputLargestPossibleRegion.SetIndex( outputStartIndex );

  outputPtr->SetLargestPossibleRegion( outputLargestPossibleRegion );
}

template < class TPixel , unsigned int Dimension >
void
FFTW1DRealToComplexConjugateImageFilter < TPixel , Dimension >
::GenerateInputRequestedRegion()
{
  // call the ImageToImageFilter's implementation of this method
  // the SuperClass has stuff in it that really should be the FFTW classes
  typedef ImageToImageFilter< Image< TPixel , Dimension >,
	                                  Image< std::complex< TPixel > , Dimension > >
					    ImageToImageFilterType;
  // call the superclass' implementation of this method
  this->ImageToImageFilterType::GenerateInputRequestedRegion();


  // get pointers to the inputs
  typename TInputImageType::Pointer inputPtr  =
    const_cast<TInputImageType *> (this->GetInput());
  typename TOutputImageType::Pointer outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // we need to compute the input requested region (size and start index)
  typedef const typename TOutputImageType::SizeType& OutputSizeType;
  OutputSizeType outputRequestedRegionSize =
    outputPtr->GetRequestedRegion().GetSize();
  typedef const typename TOutputImageType::IndexType& OutputIndexType;
  OutputIndexType outputRequestedRegionStartIndex =
    outputPtr->GetRequestedRegion().GetIndex();

  typename TInputImageType::SizeType  inputRequestedRegionSize;
  typename TInputImageType::IndexType inputRequestedRegionStartIndex;

  // the regions other than the fft direction are fine
  for (unsigned int i = 0; i < TInputImageType::ImageDimension; i++)
    {
    inputRequestedRegionSize[i] = outputRequestedRegionSize[i];
    inputRequestedRegionStartIndex[i] = outputRequestedRegionStartIndex[i];
    }

  // we but need all of the input in the fft direction
  unsigned int direction = this->m_Direction;
  const typename TInputImageType::SizeType& inputLargeSize =
    inputPtr->GetLargestPossibleRegion().GetSize();
  inputRequestedRegionSize[direction] = inputLargeSize[direction];
  const typename TInputImageType::IndexType& inputLargeIndex =
    inputPtr->GetLargestPossibleRegion().GetIndex();
  inputRequestedRegionStartIndex[direction] = inputLargeIndex[direction];

  typename TInputImageType::RegionType inputRequestedRegion;
  inputRequestedRegion.SetSize( inputRequestedRegionSize );
  inputRequestedRegion.SetIndex( inputRequestedRegionStartIndex );

  inputPtr->SetRequestedRegion( inputRequestedRegion );
}


template < class TPixel , unsigned int Dimension >
void
FFTW1DRealToComplexConjugateImageFilter < TPixel , Dimension >
::EnlargeOutputRequestedRegion(DataObject *output)
{
  TOutputImageType* outputPtr = dynamic_cast<TOutputImageType*>( output );

  // we need to enlarge the region in the fft direction to the
  // largest possible in that direction
  typedef const typename TOutputImageType::SizeType& ConstOutputSizeType;
  ConstOutputSizeType requestedSize =
    outputPtr->GetRequestedRegion().GetSize();
  ConstOutputSizeType outputLargeSize =
    outputPtr->GetLargestPossibleRegion().GetSize();
  typedef const typename TOutputImageType::IndexType& ConstOutputIndexType;
  ConstOutputIndexType requestedIndex =
    outputPtr->GetRequestedRegion().GetIndex();
  ConstOutputIndexType outputLargeIndex =
    outputPtr->GetLargestPossibleRegion().GetIndex();

  typename TOutputImageType::SizeType enlargedSize;
  typename TOutputImageType::IndexType enlargedIndex;

  for( unsigned int i = 0; i < TOutputImageType::ImageDimension; i++ )
    {
    enlargedSize[i] = requestedSize[i];
    enlargedIndex[i] = requestedIndex[i];
    }
  enlargedSize[this->m_Direction] = outputLargeSize[this->m_Direction];
  enlargedIndex[this->m_Direction] = outputLargeIndex[this->m_Direction];

  typename TOutputImageType::RegionType enlargedRegion;
  enlargedRegion.SetSize( enlargedSize );
  enlargedRegion.SetIndex( enlargedIndex );
  outputPtr->SetRequestedRegion( enlargedRegion );
}

template <typename TPixel,unsigned int Dimension>
bool
FFTW1DRealToComplexConjugateImageFilter<TPixel,Dimension>::
FullMatrix()
{
  return false;
}

} // namespace itk

#endif //_itkFFTW1DRealToComplexConjugateImageFilter_txx
