/*
 * bmpReader.h
 *
 *  Created on: 25-Nov-2013
 *      Author: TLarrey
 */

#ifndef BMPREADER_H_
#define BMPREADER_H_
#include "endian.h"

class bmpReader {
public:
	bmpReader();
	virtual ~bmpReader();
	/*
	 * Mid-level functions
	 */
	int readBitmapFileHeader(FILE *fp, BITMAPFILEHEADER *bfh);
	int readBitmapArrayHeader(FILE *fp, BITMAPARRAYHEADER *bah);
	int readBitmapHeader(FILE *fp, BITMAPHEADER *bh);
	int readRgb(FILE *fp, RGB *rgb, int numBytes);
	int readColorTable(FILE *fp, RGB *rgb, int numEntries, int numBytesPerEntry);

	int readBitsUncompressed(FILE *fp, RGB *image, int width, int height,
				 int depth, RGB* colorTable);
	int readMaskBitsUncompressed(FILE *fp, char *image, int width, int height);

	void reflectYRGB(RGB *image, int width, int height);
	void reflectYchar(char *image, int width, int height);

	/*
	 * High level functions.
	 */
	int readSingleImageBMP(FILE *fp, RGB **argb, UINT32 *width, UINT32 *height);
	int readSingleImageICOPTR(FILE *fp, char **xorMask, char **andMask,
			          UINT32 *width, UINT32 *height);
	int readSingleImageColorICOPTR(FILE *fp, RGB **argb, char **xorMask,
				       char **andMask, UINT32 *width, UINT32 *height);
	int readMultipleImage(FILE *fp, RGB ***argbs, char ***xorMasks,
			      char ***andMasks, UINT32 **widths, UINT32 **heights,
			      int *imageCount);

private:
	endian endianConverter;
};

#endif /* BMPREADER_H_ */
