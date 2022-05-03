/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <float.h>
#include <iostream>
#include <string>
#include <sstream>

#include "IException.h"
#include "IString.h"
#include "JP2Decoder.h"
#include "JP2Error.h"
#include "openjpeg.h"

using namespace std;

#if ENABLEJP2K
using namespace kdu_core;
using namespace kdu_supp;

#endif

namespace Isis {

  /**
   * Constructs a JPEG2000 decoder object
   *
   * @param jp2file The name of the JP2 file that needs to be decoded.
   *
   */
  JP2Decoder::JP2Decoder(const QString &jp2file) {

#if ENABLEJP2K
    p_jp2File = jp2file;
    p_resolutionLevel = 1;
    // JP2_Source = NULL;

    // Register the Kakadu error handler
    Kakadu_Error = new JP2Error;
    kdu_customize_errors(Kakadu_Error);
#else
    std::string msg = "JPEG2000 has not been enabled with this build of ISIS3";
    throw IException(IException::Programmer, msg, _FILEINFO_);
#endif
  }

  /**
   * Open the JPEG2000 file
   *
   */
  void JP2Decoder::OpenFile() {
#if ENABLEJP2K

    // Open the JP2 file stream
    JP2_Stream = opj_stream_create_default_file_stream(p_jp2File.toLatin1().data(), 1);

    p_decompressor = opj_create_decompress(OPJ_CODEC_JP2);


    opj_read_header(JP2_Stream, p_decompressor, &p_image);

    // Number of components (bands)
    p_numBands = p_image->numcomps;

    if (p_numBands < 1){
      QString msg = "Source file ";
      msg += "[" + p_jp2File + "] empty.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_bandOne = &p_image->comps[0];

    // Pixel data structure
    p_pixelBits = p_bandOne->prec;
    p_pixelBytes = (p_pixelBits >> 3) + ((p_pixelBits % 8) ? 1 : 0);
    if(p_pixelBytes == 3) p_pixelBytes = 4;
    if(p_pixelBits > 16 || p_pixelBytes > 2) {
      QString msg = "The source file has unsupported pixel type ";
      msg += "[" + p_jp2File + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_signedData = p_bandOne->sgnd;

    // Check all bands in the JP2 file to make sure they all have the same
    // dimensions, bit depth, and signedness
    opj_image_comp bandN;
    OPJ_UINT32 pixelBits;
    OPJ_UINT32 signedData;
    for(unsigned int band = 1; band < p_numBands; ++band) {
      bandN = p_image->comps[band];
      pixelBits = bandN.prec;
      signedData = bandN.sgnd;
      if(bandN.w != p_bandOne->w || bandN.h != p_bandOne->h ||
          bandN.x0 != p_bandOne->x0 || bandN.y0 != p_bandOne->y0 ||
          pixelBits != p_pixelBits || signedData != p_signedData) {
        std::string msg = "The source file does not have bands with matching ";
        msg += "characteristics";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    p_strip_height = p_bandOne->w;

    // Get the total available resolution levels and set the effective
    // resolution and image region
    SetResolutionAndRegion();

    // Determine optimum stripe heights for accessing data - the
    // optimum stripe heights are ignored. We are instead reading
    // the file a line at a time.
    // p_stripeHeights = new int[p_numBands];
    // p_maxStripeHeights = new int[p_numBands];
    // p_precisions = new int[p_numBands];
    // p_isSigned = new bool[p_numBands];
    // p_decompressor.get_recommended_stripe_heights(MIN_STRIPE_HEIGHT,
    //     MAX_STRIPE_HEIGHT, p_stripeHeights, p_maxStripeHeights);
    // for(unsigned int i = 0; i < p_numBands; i++) {
    //   p_precisions[i] = p_pixelBits;
    //   p_stripeHeights[i] = 1;
    //   p_isSigned[i] = p_signedData;
    // }
  // }
#endif
  }

  /**
   * Set resolution level of the JPEG2000 file. This class is currently set
   * up to only read a file at full resolution.
   *
   */
  void JP2Decoder::SetResolutionAndRegion() {
#if ENABLEJP2K
    p_bandOne = &p_image->comps[0];
    p_numSamples = p_bandOne->w;
    p_numLines = p_bandOne->h;
#endif
  }

  /**
   * Read data from JP2 file containing 8-bit data
   *
   * @param inbuf The array of pointers to byte buffers that will be used to read
   *              in the image data. One byte buffer is required for each band in
   *              the image. Kakadu reads in a BIL manner. It is up to the calling
   *              routine to do the casting necessary to convert the byte data to
   *              the actual pixel type (UnsignedByte, UnsignedWord, SignedWord).
   *
   */
  void JP2Decoder::Read(unsigned char **inbuf, int line) {
#if ENABLEJP2K
    if (!opj_set_decode_area(p_decompressor, p_image, 0, (OPJ_INT32)line,
                             (OPJ_INT32)p_strip_height, (OPJ_INT32)(line + 1))){
      fprintf(stderr, "ERROR -> failed to set the decoded area\n");
    }

    /* Get the decoded image */
    if (!opj_decode(p_decompressor, JP2_Stream, p_image)){
      fprintf(stderr, "ERROR -> failed to decode image!\n");
    }

    for (int compno = 0; compno < p_image->numcomps; compno++) {
      memcpy(inbuf[compno], p_image->comps[compno].data, p_strip_height);
    }
#endif
  }

  /**
   * Read data from JP2 file containing 16-bit data
   *
   * @param inbuf The array of pointers to byte buffers that will be used to read
   *              in the image data. One byte buffer is required for each band in
   *              the image. Kakadu reads in a BIL manner. It is up to the calling
   *              routine to do the casting necessary to convert the byte data to
   *              the actual pixel type (UnsignedByte, UnsignedWord, SignedWord).
   *
   */
  void JP2Decoder::Read(short int **inbuf, int line) {
#if ENABLEJP2K
    opj_set_decode_area(p_decompressor, p_image, 0, (OPJ_INT32)line,
                             (OPJ_INT32)p_strip_height, (OPJ_INT32)(line + 1));

    /* Get the decoded image */
    opj_decode(p_decompressor, JP2_Stream, p_image);

    for (int compno = 0; compno < p_image->numcomps; compno++) {
      memcpy(inbuf[compno], p_image->comps[compno].data, p_strip_height);
    }
#endif
  }

  /**
   * JP2Decoder destructor
   *
   */
  JP2Decoder::~JP2Decoder() {
#if ENABLEJP2K
    // See kdu_stripe_decompressor::reset documentation:
    // "You should be sure to call this function or finish before destroying the kdu_codestream
    // inteface that was passed to start."
    // i.e. Make sure to finish the decompressor before destroying the kdu_codestream.
    // p_decompressor.finish();
    opj_end_decompress(p_decompressor,  JP2_Stream);

    opj_stream_destroy(JP2_Stream);
    opj_destroy_codec(p_decompressor);
    opj_image_destroy(p_image);

    // if(Kakadu_Error) {
    //   delete Kakadu_Error;
    // }
    
    delete [] p_stripeHeights;
    delete [] p_maxStripeHeights;
    delete [] p_precisions;
    delete [] p_isSigned;
#endif
  }


  bool JP2Decoder::IsJP2(QString filename) {
#if ENABLEJP2K
    jp2_family_src *stream = new jp2_family_src();
    stream->open(filename.toLatin1().data());
    jp2_source *source = new jp2_source();

    bool result = source->open(stream);

    source->close();
    delete source;

    stream->close();
    delete stream;

    return result;
#else
    return (false);
#endif
  }
}
