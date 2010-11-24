/*
    This file is part of darktable,
    copyright (c) 2009--2010 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif
#include "common/imageio_pfm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

dt_imageio_retval_t dt_imageio_open_pfm(dt_image_t *img, const char *filename)
{
  const char *ext = filename + strlen(filename);
  while(*ext != '.' && ext > filename) ext--;
  if(strncmp(ext, ".pfm", 4) && strncmp(ext, ".PFM", 4) && strncmp(ext, ".Pfm", 4)) return DT_IMAGEIO_FILE_CORRUPTED;
  FILE *f = fopen(filename, "rb");
  if(!f) return DT_IMAGEIO_FILE_CORRUPTED;
  int ret = 0;
  int cols = 3;
  char head[2] = {'X', 'X'};
  ret = fscanf(f, "%c%c\n", head, head+1);
  if(ret != 2 || head[0] != 'P') goto error_corrupt;
  if(head[1] == 'F') cols = 3;
  else if(head[1] == 'f') cols = 1;
  else goto error_corrupt;
  ret = fscanf(f, "%d %d\n%*[^\n]", &img->width, &img->height);
  if(ret != 2) goto error_corrupt;
  ret = fread(&ret, sizeof(char), 1, f);
  if(ret != 1) goto error_corrupt;

  if(dt_image_alloc(img, DT_IMAGE_FULL)) goto error_cache_full;
  dt_image_check_buffer(img, DT_IMAGE_FULL, 4*img->width*img->height*sizeof(float));
  if(cols == 3)
  {
    ret = fread(img->pixels, 3*sizeof(float), img->width*img->height, f);
    for(int i=img->width*img->height-1;i>=0;i--) for(int c=0;c<3;c++) img->pixels[4*i+c] = fmaxf(0.0f, fminf(10000.0, img->pixels[3*i+c]));
  }
  else for(int j=0; j < img->height; j++)
    for(int i=0; i < img->width; i++)
    {
      ret = fread(img->pixels + 4*(img->width*j + i), sizeof(float), 1, f);
      img->pixels[4*(img->width*j + i) + 2] =
      img->pixels[4*(img->width*j + i) + 1] =
      img->pixels[4*(img->width*j + i) + 0];
    }
  float *line = (float *)malloc(sizeof(float)*4*img->width);
  for(int j=0; j < img->height/2; j++)
  {
    memcpy(line,                                         img->pixels + img->width*j*4,                 4*sizeof(float)*img->width);
    memcpy(img->pixels + img->width*j*4,                 img->pixels + img->width*(img->height-1-j)*4, 4*sizeof(float)*img->width);
    memcpy(img->pixels + img->width*(img->height-1-j)*4, line,                                         4*sizeof(float)*img->width);
  }
  free(line);
  dt_image_release(img, DT_IMAGE_FULL, 'w');
  fclose(f);
  return DT_IMAGEIO_OK;

error_corrupt:
  fclose(f);
  return DT_IMAGEIO_FILE_CORRUPTED;
error_cache_full:
  fclose(f);
  return DT_IMAGEIO_CACHE_FULL;
}

dt_imageio_retval_t dt_imageio_open_pfm_preview(dt_image_t *img, const char *filename)
{
  const char *ext = filename + strlen(filename);
  while(*ext != '.' && ext > filename) ext--;
  if(strncmp(ext, ".pfm", 4) && strncmp(ext, ".PFM", 4) && strncmp(ext, ".Pfm", 4)) return DT_IMAGEIO_FILE_CORRUPTED;
  FILE *f = fopen(filename, "rb");
  if(!f) return DT_IMAGEIO_FILE_CORRUPTED;
  int ret = 0;
  int cols = 3;
  char head[2] = {'X', 'X'};
  ret = fscanf(f, "%c%c\n", head, head+1);
  if(ret != 2 || head[0] != 'P') goto error_corrupt;
  if(head[1] == 'F') cols = 3;
  else if(head[1] == 'f') cols = 1;
  else goto error_corrupt;
  ret = fscanf(f, "%d %d\n%*[^\n]\n", &img->width, &img->height);
  if(ret != 2) goto error_corrupt;

  float *buf = (float *)malloc(4*sizeof(float)*img->width*img->height);
  if(!buf) goto error_corrupt;
  if(cols == 3)
  {
    ret = fread(buf, 3*sizeof(float), img->width*img->height, f);
    for(int i=img->width*img->height-1;i>=0;i--) for(int c=0;c<3;c++) img->pixels[4*i+c] = fmaxf(0.0f, fminf(10000.0f, img->pixels[3*i+c]));
  }
  else for(int j=0; j < img->height; j++)
    for(int i=0; i < img->width; i++)
    {
      ret = fread(buf + 4*(img->width*j + i), sizeof(float), 1, f);
      buf[4*(img->width*j + i) + 2] =
      buf[4*(img->width*j + i) + 1] =
      buf[4*(img->width*j + i) + 0];
    }
  float *line = (float *)malloc(sizeof(float)*4*img->width);
  for(int j=0; j < img->height/2; j++)
  {
    memcpy(line,                                 buf + img->width*j*4,                 4*sizeof(float)*img->width);
    memcpy(buf + img->width*j*4,                 buf + img->width*(img->height-1-j)*4, 4*sizeof(float)*img->width);
    memcpy(buf + img->width*(img->height-1-j)*4, line,                                 4*sizeof(float)*img->width);
  }
  free(line);
  dt_imageio_retval_t retv = dt_image_raw_to_preview(img, buf);
  free(buf);
  fclose(f);
  return retv;

error_corrupt:
  fclose(f);
  return DT_IMAGEIO_FILE_CORRUPTED;
}

