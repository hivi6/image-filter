#include <stdio.h>

#include "ap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IDENTITY_KERNEL {0, 0, 0, 0, 1.0, 0, 0, 0, 0}
#define EDGE_KERNEL     {0, -1.0, 0, -1.0, 4.0, -1.0, 0, -1, 0}
#define SHARPEN_KERNEL  {0, -1.0, 0, -1.0, 5.0, -1.0, 0, -1.0, 0}
#define BOX_KERNEL      {1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0}
#define GAUSSIAN_KERNEL {1/16.0, 2/16.0, 1/16.0, 2/16.0, 4/16.0, 2/16.0, 1/16.0, 2/16.0, 1/16.0}

enum filter_type {
  FILTER_IDENTITY     = (1 << 0),
  FILTER_EDGE         = (1 << 1),
  FILTER_SHARPEN      = (1 << 2),
  FILTER_BOX_BLUR     = (1 << 3),
  FILTER_GAUS_BLUR    = (1 << 4),
  FILTER_UNSHARP_MASK = (1 << 5),
};

int parse_filters(const char* filters);

struct image_t {
  int width;
  int height;
  int channels;
  unsigned char *data;
};

struct image_t *image_init(int width, int height, int channels);
struct image_t *image_load(const char *filename);
void image_free(struct image_t *image);
int image_save(struct image_t *image, const char *filename);

struct kernel_t {
  int size;
  float *data;
};

struct kernel_t *kernel_init(const float *data, int size);
void kernel_free(struct kernel_t *kernel);

struct image_t *apply_kernel(struct image_t *input, struct kernel_t *kernel);

int main(int argc, const char* argv[]) {
  struct ap_value *help = ap_value_init(AP_FLAG, "show help", "-h", "--help");
  struct ap_value *verbose = ap_value_init(AP_FLAG, "verbose", "-v", "--verbose");
  struct ap_value *input = ap_value_init(AP_FVALUE, "input file", "-i", "--in");
  struct ap_value *output = ap_value_init(AP_FVALUE, "output file", "-o", "--out");
  struct ap_value *filter = ap_value_init(AP_FVALUE, "filter", "-f", "--filter");

  struct ap_parser *parser = ap_parser_init("image-filter", "Filter image");

  ap_parser_add_argument(parser, help);
  ap_parser_add_argument(parser, verbose);
  ap_parser_add_argument(parser, input);
  ap_parser_add_argument(parser, output);
  ap_parser_add_argument(parser, filter);

  int err = ap_parser_parse(parser, argc, argv);
  if (err) {
    printf("Something went wrong while parsing\n");
    ap_parser_help(parser);
    exit(1);
  }

  if (help->is_exists) {
    ap_parser_help(parser);
    exit(0);
  }

  if (verbose->is_exists) {
    printf("input-file: %s\n", input->value);
    printf("output-file: %s\n", output->value);
    printf("filter: %s\n", filter->value);
  }

  int filter_flags = parse_filters(filter->value);
  if (filter_flags == -1) {
    printf("Something went wrong while parsing filter flag\n");
    printf("filter: %s\n", filter->value);
    exit(1);
  }

  struct image_t *input_image = image_load(input->value);
  if (input_image == NULL) {
    printf("Something went wrong while loading input image\n");
    exit(1);
  }

  float kernel_data[9] = BOX_KERNEL;
  struct kernel_t *kernel = kernel_init(kernel_data, 3);

  struct image_t *output_image = apply_kernel(input_image, kernel);

  if (image_save(output_image, output->value)) {
    printf("Something went wrong while saving image\n");
    printf("output-filename: %s\n", output->value);
    exit(1);
  }

  image_free(output_image);

  kernel_free(kernel);

  image_free(input_image);

  ap_parser_delete(parser);

  ap_value_delete(help);
  ap_value_delete(verbose);
  ap_value_delete(input);
  ap_value_delete(output);
  ap_value_delete(filter);

  return 0;
}

int parse_filters(const char* filter) {
  if (filter == NULL) {
    return -1;
  }

  int res = 0;
  int start = 0;
  const char *type[6] = {"identity", "edge", "sharpen", 
                         "box-blur", "gaussian-blur", "unsharp-masking" };

  for (int i = 0; filter[i]; i++) {
    if (filter[i] == ',' || filter[i + 1] == 0) {
      int f = -1;

      int len = i - start;
      if (filter[i + 1] == 0)
        len++;
      
      for (int j = 0; j < 6; j++) {
        if (strncmp(filter + start, type[j], len) == 0)
          f = (1 << j);
      }

      if (f == -1) {
        return -1;
      }
      
      res |= f;
      start = i + 1;
    }
  }

  return res;
}

struct image_t *image_init(int width, int height, int channels) {
  struct image_t *image = malloc(sizeof(struct image_t));
  if (image == NULL) {
    return NULL;
  }

  image->width = width;
  image->height = height;
  image->channels = channels;
  image->data = malloc(width * height * channels * sizeof(char));
  return image;
}

struct image_t *image_load(const char *filename) {
  if (filename == NULL) {
    return NULL;
  };

  int width, height, channels;
  char *data;
  data = stbi_load(filename, &width, &height, &channels, 3);
  if (data == NULL) {
    return NULL;
  }

  struct image_t *image = malloc(sizeof(struct image_t));
  if (image == NULL) {
    stbi_image_free(data);
    return NULL;
  }

  image->width = width;
  image->height = height;
  image->channels = channels;
  image->data = data;
  return image;
}

void image_free(struct image_t *image) {
  if (image == NULL) {
    return;
  }

  stbi_image_free(image->data);
  free(image);
}



struct kernel_t *kernel_init(const float *data, int size) {
  if (data == NULL) {
    return NULL;
  }

  struct kernel_t *kernel = malloc(sizeof(struct kernel_t));
  if (kernel == NULL) {
    return NULL;
  }

  kernel->data = malloc(size * size * sizeof(float));
  if (kernel->data == NULL) {
    free(kernel);
    return NULL;
  }

  kernel->size = size;
  for (int i = 0; i < size * size; i++) {
    kernel->data[i] = data[i];
  }
  return kernel;
}

void kernel_free(struct kernel_t *kernel) {
  if (kernel == NULL) {
    return;
  }

  free(kernel->data);
  free(kernel);
}

int image_save(struct image_t *image, const char *filename) {
  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    return -1;
  }

  fprintf(file, "P6\n%d %d\n255\n", image->width, image->height);
  fwrite(image->data, sizeof(char), image->width * image->height * image->channels, file);
  fclose(file);

  return 0;
}

struct image_t *apply_kernel(struct image_t *input, struct kernel_t *kernel) {
  struct image_t *result = image_init(input->width, input->height, input->channels);
  if (result == NULL) {
    return NULL;
  }

  for (int y = 0; y < input->height; y++) {
    for (int x = 0; x < input->width; x++) {
      for (int z = 0; z < input->channels; z++) {
        float sum = 0.0f;

        for (int ky = 0; ky < kernel->size; ky++) {
          for (int kx = 0; kx < kernel->size; kx++) {

            int img_x = x - (kernel->size / 2 - kx);
            int img_y = y - (kernel->size / 2 - ky);

            unsigned char pixel = 0;
            if (img_x < 0 || img_x >= input->width || 
                img_y < 0 || img_y >= input->height || 
                z < 0 || z >= input->channels) {
              pixel = 0;
            }
            else {
              pixel = input->data[(img_y * input->width + img_x) * input->channels + z];
            }
            sum += pixel * 
              kernel->data[ky * kernel->size + kx];
          }
        }

        result->data[(y * result->width + x) * result->channels + z] = (char)sum;
      }
    }
  }

  return result;
}
