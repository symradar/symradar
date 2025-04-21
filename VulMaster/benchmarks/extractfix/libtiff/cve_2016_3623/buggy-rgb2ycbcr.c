static int
cvtRaster(TIFF* tif, uint32* raster, uint32 width, uint32 height)
{
	uint32 y;
	tstrip_t strip = 0;
	tsize_t cc, acc;
	unsigned char* buf;
	<vul-start>uint32 rwidth = roundup(width, horizSubSampling);<vul-end>
	uint32 rheight = roundup(height, vertSubSampling);
	uint32 nrows = (rowsperstrip > rheight ? rheight : rowsperstrip);
        uint32 rnrows = roundup(nrows,vertSubSampling);

	cc = rnrows*rwidth +
	    2*((rnrows*rwidth) / (horizSubSampling*vertSubSampling));
	buf = (unsigned char*)_TIFFmalloc(cc);
	// FIXME unchecked malloc
	for (y = height; (int32) y > 0; y -= nrows) {
		uint32 nr = (y > nrows ? nrows : y);
		cvtStrip(buf, raster + (y-1)*width, nr, width);
		nr = roundup(nr, vertSubSampling);
		acc = nr*rwidth +
			2*((nr*rwidth)/(horizSubSampling*vertSubSampling));
		if (!TIFFWriteEncodedStrip(tif, strip++, buf, acc)) {
			_TIFFfree(buf);
			return (0);
		}
	}
	_TIFFfree(buf);
	return (1);
}