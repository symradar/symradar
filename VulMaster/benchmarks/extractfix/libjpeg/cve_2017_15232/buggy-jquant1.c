METHODDEF(void)
quantize_ord_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
                     JSAMPARRAY output_buf, int num_rows)
/* General case, with ordered dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex_ci;
  int *dither;                  /* points to active row of dither matrix */
  int row_index, col_index;     /* current indexes into dither matrix */
  int nc = cinfo->out_color_components;
  int ci;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  

  for (row = 0; row < num_rows; row++) {
    /* Initialize output values to 0 so can process components separately */
    <vul-start>jzero_far((void *) output_buf[row], (size_t) (width * sizeof(JSAMPLE)));<vul-end>
    row_index = cquantize->row_index;
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buf[row] + ci;
      output_ptr = output_buf[row];
      colorindex_ci = cquantize->colorindex[ci];
      dither = cquantize->odither[ci][row_index];
      col_index = 0;

      for (col = width; col > 0; col--) {
        /* Form pixel value + dither, range-limit to 0..MAXJSAMPLE,
         * select output value, accumulate into output code for this pixel.
         * Range-limiting need not be done explicitly, as we have extended
         * the colorindex table to produce the right answers for out-of-range
         * inputs.  The maximum dither is +- MAXJSAMPLE; this sets the
         * required amount of padding.
         */
        *output_ptr += colorindex_ci[GETJSAMPLE(*input_ptr)+dither[col_index]];
        input_ptr += nc;
        output_ptr++;
        col_index = (col_index + 1) & ODITHER_MASK;
      }
    }
    /* Advance row index for next row */
    row_index = (row_index + 1) & ODITHER_MASK;
    cquantize->row_index = row_index;
  }
}