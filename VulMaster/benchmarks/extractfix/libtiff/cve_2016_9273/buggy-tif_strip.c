/*
 * Compute how many strips are in an image.
 */
uint32
TIFFNumberOfStrips(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	uint32 nstrips;

	nstrips = (td->td_rowsperstrip == (uint32) -1 ? 1 :
	     TIFFhowmany_32(td->td_imagelength, td->td_rowsperstrip));
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
	<vul-start>nstrips = _TIFFMultiply32(tif, nstrips, (uint32)td->td_samplesperpixel,
		    "TIFFNumberOfStrips");<vul-end>
	return (nstrips);
}