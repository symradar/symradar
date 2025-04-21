/*
 * The module selection routine for BMP format output.
 */

GLOBAL(djpeg_dest_ptr)
jinit_write_bmp(j_decompress_ptr cinfo, boolean is_os2,
                boolean use_inversion_array)
{
  bmp_dest_ptr dest;
  JDIMENSION row_width;

  /* Create module interface object, fill in method pointers */
  dest = (bmp_dest_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                sizeof(bmp_dest_struct));
  dest->pub.start_output = start_output_bmp;
  dest->pub.finish_output = finish_output_bmp;
  dest->pub.calc_buffer_dimensions = NULL;
  dest->is_os2 = is_os2;

  if (cinfo->out_color_space == JCS_GRAYSCALE) {
    dest->pub.put_pixel_rows = put_gray_rows;
  } else if (IsExtRGB(cinfo->out_color_space)) {
    if (cinfo->quantize_colors)
      dest->pub.put_pixel_rows = put_gray_rows;
    else
      dest->pub.put_pixel_rows = put_pixel_rows;
  } else if (cinfo->out_color_space == JCS_RGB565 ||
             cinfo->out_color_space == JCS_CMYK) {
    <vul-start>dest->pub.put_pixel_rows = put_pixel_rows;<vul-end>
  } else {
    ERREXIT(cinfo, JERR_BMP_COLORSPACE);
  }

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Determine width of rows in the BMP file (padded to 4-byte boundary). */
  if (cinfo->out_color_space == JCS_RGB565) {
    row_width = cinfo->output_width * 2;
    dest->row_width = dest->data_width = cinfo->output_width * 3;
    while ((row_width & 3) != 0) row_width++;
  } else if (!cinfo->quantize_colors &&
             (IsExtRGB(cinfo->out_color_space) ||
              cinfo->out_color_space == JCS_CMYK)) {
    row_width = cinfo->output_width * cinfo->output_components;
    dest->row_width = dest->data_width = cinfo->output_width * 3;
  } else {
    row_width = cinfo->output_width * cinfo->output_components;
    dest->row_width = dest->data_width = row_width;
  }
  while ((dest->row_width & 3) != 0) dest->row_width++;
  dest->pad_bytes = (int)(dest->row_width - dest->data_width);


  if (use_inversion_array) {
    /* Allocate space for inversion array, prepare for write pass */
    dest->whole_image = (*cinfo->mem->request_virt_sarray)
      ((j_common_ptr)cinfo, JPOOL_IMAGE, FALSE,
       dest->row_width, cinfo->output_height, (JDIMENSION)1);
    dest->cur_output_row = 0;
    if (cinfo->progress != NULL) {
      cd_progress_ptr progress = (cd_progress_ptr)cinfo->progress;
      progress->total_extra_passes++; /* count file input as separate pass */
    }
  } else {
    dest->iobuffer = (JSAMPLE *)(*cinfo->mem->alloc_small)
      ((j_common_ptr)cinfo, JPOOL_IMAGE, dest->row_width);
  }
  dest->use_inversion_array = use_inversion_array;

  /* Create decompressor output buffer. */
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr)cinfo, JPOOL_IMAGE, row_width, (JDIMENSION)1);
  dest->pub.buffer_height = 1;

  return (djpeg_dest_ptr)dest;
}