this document describes how to add new image decoder.

1. create a directory which name is your image decoder's name and put source files
 under the directory.
'const struct image_decoder image_decoder' and 'IMGDEC_HEADER' must be declared in
 one of your source files.
see imageviewer.h for the detail of struct image_decoder.

2. add the directory name to apps/plugins/imageviewer/SUBDIR so that the decoder
 is built.
if the decoder is supported by particular targets, surround it with #if directive.
e.g. if the decoder supports color LCD targets only,
#ifdef HAVE_LCD_COLOR
bmp
#endif

3. append appropriate entry to enum image_type in image_decoder.h, decoder_names
 and ext_list in image_decoder.c so that the imageviewer plugin can recognize
 the decoder.
if the decoder is supported by particular targets, surround them with same #if
 directive in SUBDIR.

4. add entry to apps/plugins/viewers.config
 (in format: file_extension,viewer/imageviewer) so that the file with specified 
 file extension will be opened by image viewer plugin.
if the decoder is supported by particular targets, surround it with same #if
 directive in SUBDIR.

5. add entry to apps/plugins/CATEGORIES (in format: decoder_name,viewer) so
 that the build file is copied to viewers directory.
DON'T surround this with #if directive.


notes:
if you need to use greylib functions to draw image, add the functions to
 struct imgdec_api just like gray_bitmap_part because GREY_INFO_STRUCT is
 declared in imageviewer and is not available from the decoder.
