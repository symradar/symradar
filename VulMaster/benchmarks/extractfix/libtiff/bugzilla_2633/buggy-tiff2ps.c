void
PSDataColorContig(FILE* fd, TIFF* tif, uint32 w, uint32 h, int nc)
{
	uint32 row;
	int breaklen = MAXLINE, es = samplesperpixel - nc;
	tsize_t cc;
	unsigned char *tf_buf;
	unsigned char *cp, c;

	(void) w;
	tf_buf = (unsigned char *) _TIFFmalloc(tf_bytesperrow);
	if (tf_buf == NULL) {
		TIFFError(filename, "No space for scanline buffer");
		return;
	}
	for (row = 0; row < h; row++) {
		if (TIFFReadScanline(tif, tf_buf, row, 0) < 0)
			break;
		cp = tf_buf;
		/*
		 * for 16 bits, the two bytes must be most significant
		 * byte first
		 */
		if (bitspersample == 16 && !HOST_BIGENDIAN) {
			PS_FlipBytes(cp, tf_bytesperrow);
		}
		if (alpha) {
			int adjust;
			cc = 0;
			for (; cc < tf_bytesperrow; cc += samplesperpixel) {
				DOBREAK(breaklen, nc, fd);
				/*
				 * For images with alpha, matte against
				 * a white background; i.e.
				 *    Cback * (1 - Aimage)
				 * where Cback = 1.
				 */
				adjust = 255 - cp[nc];
				switch (nc) {
				case 4: c = *cp++ + adjust; PUTHEX(c,fd);
				case 3: c = *cp++ + adjust; PUTHEX(c,fd);
				case 2: c = *cp++ + adjust; PUTHEX(c,fd);
				case 1: c = *cp++ + adjust; PUTHEX(c,fd);
				}
				<vul-start>cp += es;<vul-start>
			}
		} else {
			cc = 0;
			for (; cc < tf_bytesperrow; cc += samplesperpixel) {
				DOBREAK(breaklen, nc, fd);
				switch (nc) {
				case 4: c = *cp++; PUTHEX(c,fd);
				case 3: c = *cp++; PUTHEX(c,fd);
				case 2: c = *cp++; PUTHEX(c,fd);
				case 1: c = *cp++; PUTHEX(c,fd);
				}
				cp += es;
			}
		}
	}
	_TIFFfree((char *) tf_buf);
}