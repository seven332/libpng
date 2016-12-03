#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>

#ifdef PNG_FREESTANDING_TESTS
#  include <png.h>
#else
#  include "../../png.h"
#endif

#define DIR_PNG_SUITE "contrib/pngsuite-new/"
#define EXTENSION_PNG ".png"

static int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return 0;
    }
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) {
        return 0;
    }
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static void png_skip_rows(png_structrp png_ptr, png_uint_32 num_rows, png_bytep temp) {
    png_uint_32 i;
    for (i = 0; i < num_rows; ++i) {
        png_read_row(png_ptr, temp, NULL);
    }
}

static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    printf("ERROR: %s\n", error_msg);
}

static void user_warn_fn(png_structp png_ptr, png_const_charp error_msg) {
    //printf("WARN: %s\n", error_msg);
}

static void user_read_fn(png_structp png_ptr,
        png_bytep data, png_size_t length) {
    FILE* file = png_get_io_ptr(png_ptr);
    fread(data, length, 1, file);
}

static void user_seek_fn(png_structp png_ptr, png_uint_32 offset) {
    FILE* file = png_get_io_ptr(png_ptr);
    fseek(file, offset, SEEK_SET);
}

png_byte* fullPngPixels(char* filename, png_uint_32* width, png_uint_32* height) {

    FILE* file;

    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 origWidth, origHeight;
    int bitDepth, colorType, interlaceType;
    png_byte* pixels = NULL;
    png_byte** rows = NULL;
    int number_passes;
    int i, j;

    file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            NULL, &user_error_fn, &user_warn_fn);
    info_ptr = png_create_info_struct(png_ptr);
    if (png_ptr == NULL) {
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_read_fn(png_ptr, file, &user_read_fn);
    png_set_seek_fn(png_ptr, &user_seek_fn);

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
            &colorType, &interlaceType, NULL, NULL);

    //printf("colorType = %d\n", colorType);
    //printf("bitDepth = %d\n", bitDepth);
    printf("interlaceType = %d\n", interlaceType);

    png_set_expand(png_ptr);
    if (bitDepth == 16) {
        png_set_scale_16(png_ptr);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY ||
            colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    if (!(colorType & PNG_COLOR_MASK_ALPHA)) {
        png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    }

    *width = origWidth;
    *height = origHeight;
    pixels = malloc(origWidth * origHeight * 4);
    rows = malloc(origHeight * sizeof(png_byte*));
    for (i = 0; i < origHeight; i++) {
        rows[i] = pixels + (i * origWidth * 4);
    }

    png_read_image(png_ptr, rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

/*
    for (j = 0; j < origHeight; j++) {
        for (i = 0; i < origWidth; i++) {
            printf("%02x %02x %02x %02x, ", pixels[(origHeight * j + i) * 4 + 0], pixels[(origHeight * j + i) * 4 + 1], pixels[(origHeight * j + i) * 4 + 2], pixels[(origHeight * j + i) * 4 + 3]);
        }
        printf("\n");
    }
*/

    fclose(file);
    free(rows);
    return pixels;
}



png_byte* partPngPixels(char* filename, png_uint_32 x, png_uint_32 y,
        png_uint_32 width, png_uint_32 height) {

    FILE* file;

    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 origWidth, origHeight;
    int bitDepth, colorType, interlaceType;
    int number_passes;
    int i, j;
    int actualTop;
    png_byte* pixels = NULL;
    png_byte* row = NULL;

    file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            NULL, &user_error_fn, &user_warn_fn);
    info_ptr = png_create_info_struct(png_ptr);
    if (png_ptr == NULL) {
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_read_fn(png_ptr, file, &user_read_fn);
    png_set_seek_fn(png_ptr, &user_seek_fn);

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
            &colorType, &interlaceType, NULL, NULL);

    png_set_expand(png_ptr);
    if (bitDepth == 16) {
        png_set_scale_16(png_ptr);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY ||
            colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    if (!(colorType & PNG_COLOR_MASK_ALPHA)) {
        png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    }

    png_build_index(png_ptr);

    number_passes = png_set_interlace_handling(png_ptr);

    png_set_interlaced_pass(png_ptr, 0);
    png_read_update_info(png_ptr, info_ptr);


    pixels = malloc(width * height * 4);
    row = malloc(origWidth * 4);


    for (i = 0; i < number_passes; i++) {
        actualTop = y;

        png_configure_decoder(png_ptr, &actualTop, i);
        png_skip_rows(png_ptr, y - actualTop, row);

        for (j = 0; j < height; j++) {
            memcpy(row + (x * 4), pixels + (width * j * 4), width * 4);
            png_read_row(png_ptr, row, NULL);
            memcpy(pixels + (width * j * 4), row + (x * 4), width * 4);
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(file);
    free(row);

/*
    printf("Start from %d\n", y);
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            printf("%02x %02x %02x %02x, ", pixels[(height * j + i) * 4 + 0], pixels[(height * j + i) * 4 + 1], pixels[(height * j + i) * 4 + 2], pixels[(height * j + i) * 4 + 3]);
        }
        printf("\n");
    }
*/

    return pixels;
}

int comparePixels(png_byte* full, int fullWidth, png_byte* part, int x, int y, int w, int h) {
    int i;

    if (y == 0) {
    //printf("row %d\n", y);
    //printf("%x, %x, %x, %x\n", full[0], full[1], full[2], full[3]);
    //printf("%x, %x, %x, %x\n", part[0], part[1], part[2], part[3]);
    }

    for (i = 0; i < h; i++) {
        if (memcmp(part + (w * i * 4), full + ((fullWidth * (y + i) + x) * 4), w * 4)) {
            return 1;
        }
    }
    return 0;
}

int testPng(char* filename) {
    png_uint_32 fullWidth, fullHeight;
    png_byte* fullPixels = NULL;
    png_byte* partPixels = NULL;
    int x, y, width, height;
    int current;
    int error = 0;

    fullPixels = fullPngPixels(filename, &fullWidth, &fullHeight);
    if (fullPixels == NULL) {
        printf("fullPixels == null\n");
        error = 1;
        goto end;
    }

    //printf("%d, %d\n", fullWidth, fullHeight);


    for (x = 0; x < fullWidth; ++x) {
        for (y = 0; y < fullHeight; ++y) {
            for (width = 1; x + width <= fullWidth; width++) {
                  //for (height = 1; y + height <= fullHeight; height++) {
height = fullHeight - y;


        partPixels = partPngPixels(filename, x, y, width, height);
        if (partPixels != NULL) {
            current = comparePixels(fullPixels, fullWidth, partPixels, x, y, width, height);
            if (current) {
                error += current;
                printf("FAILED: %s %d, %d, %d, %d\n", filename, x, y, width, height);
            }
            free(partPixels);
        } else {
            printf("partPngPixels failed\n");
        }

                  //}
            }
        }
    }


/*
    x = 0;
    width = fullWidth;
    for (y = 0; y < fullHeight; y++) {
        height = fullHeight - y;
        partPixels = partPngPixels(filename, x, y, width, height);
        if (partPixels != NULL) {

            current = comparePixels(fullPixels, fullWidth, partPixels, x, y, width, height);

            if (current) {
                error += current;
                //printf("FAILED: %s row %d\n", filename, y);
            }

            free(partPixels);
        } else {
            printf("partPngPixels failed");
        }
    }
*/

end:
    free(fullPixels);
    return error;
}


int main(int argc, char **argv) {
    char filename[256];
    int errors = 0;
    int current;
    DIR *dp;
    struct dirent *ep;

    dp = opendir(DIR_PNG_SUITE);
    if (dp != NULL) {
      while (ep = readdir(dp)) {
          if (endsWith(ep->d_name, EXTENSION_PNG)) {
              printf("%s\n", ep->d_name);

              strcpy(filename, DIR_PNG_SUITE);
              strcat(filename, ep->d_name);
              current = testPng(filename);
              if (current) {
                  errors += current;
                  printf("FAILED: %s\n", filename);
              }
          }
      }
      closedir(dp);
    } else {
        errors++;
        perror("Couldn't open the directory");
    }

    return errors != 0;
}
