static int
OJPEGDecodeRaw(TIFF* tif, uint8* buf, tmsize_t cc)
{
	static const char module[]="OJPEGDecodeRaw";
	OJPEGState* sp=(OJPEGState*)tif->tif_data;
	uint8* m;
	tmsize_t n;
	uint8* oy;
	uint8* ocb;
	uint8* ocr;
	uint8* p;
	uint32 q;
	uint8* r;
	uint8 sx,sy;
	<vul-start>if (cc%sp->bytes_per_line!=0)<vul-end>
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Fractional scanline not read");
		return(0);
	}
	assert(cc>0);
	m=buf;
	n=cc;
	do
	{
		if (sp->subsampling_convert_state==0)
		{
			if (jpeg_read_raw_data_encap(sp,&(sp->libjpeg_jpeg_decompress_struct),sp->subsampling_convert_ycbcrimage,sp->subsampling_ver*8)==0)
				return(0);
		}
		oy=sp->subsampling_convert_ybuf+sp->subsampling_convert_state*sp->subsampling_ver*sp->subsampling_convert_ylinelen;
		ocb=sp->subsampling_convert_cbbuf+sp->subsampling_convert_state*sp->subsampling_convert_clinelen;
		ocr=sp->subsampling_convert_crbuf+sp->subsampling_convert_state*sp->subsampling_convert_clinelen;
		p=m;
		for (q=0; q<sp->subsampling_convert_clinelenout; q++)
		{
			r=oy;
			for (sy=0; sy<sp->subsampling_ver; sy++)
			{
				for (sx=0; sx<sp->subsampling_hor; sx++)
					*p++=*r++;
				r+=sp->subsampling_convert_ylinelen-sp->subsampling_hor;
			}
			oy+=sp->subsampling_hor;
			*p++=*ocb++;
			*p++=*ocr++;
		}
		sp->subsampling_convert_state++;
		if (sp->subsampling_convert_state==sp->subsampling_convert_clines)
			sp->subsampling_convert_state=0;
		m+=sp->bytes_per_line;
		n-=sp->bytes_per_line;
	} while(n>0);
	return(1);
}