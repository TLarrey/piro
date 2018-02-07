/*
 * endian.h
 *
 *  Created on: 25-Nov-2013
 *      Author: TLarrey
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <stdio.h>
#include <stdlib.h>
#include "bmptypes.h"


class endian {
public:
	endian();
	virtual ~endian();
	/*
	 * Read the basic types as little-endian values.  The return value will be
	 * zero if successful, EOF, otherwise.
	 */
	int readINT8little(FILE *f, INT8 *i);
	int readINT16little(FILE *f, INT16 *i);
	int readINT32little(FILE *f, INT32 *i);
	int readUINT8little(FILE *f, UINT8 *i);
	int readUINT16little(FILE *f, UINT16 *i);
	int readUINT32little(FILE *f, UINT32 *i);

	/*
	 * Write the basic types as little-endian values.  The return value will be
	 * zero if successful, EOF otherwise.
	 */
	int writeINT8little(FILE *f, INT8 i);
	int writeINT16little(FILE *f, INT16 i);
	int writeINT32little(FILE *f, INT32 i);
	int writeUINT8little(FILE *f, UINT8 i);
	int writeUINT16little(FILE *f, UINT16 i);
	int writeUINT32little(FILE *f, UINT32 i);
};


#endif /* ENDIAN_H_ */
