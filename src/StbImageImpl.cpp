// The one translation unit that compiles stb_image's implementation.
// Every other file includes "stb_image.h" WITHOUT this define; defining it
// twice causes "multiple definition" linker errors.
//
// The STBI_ONLY_* switches drop decoders we never use (GIF, PSD, HDR, ...),
// which keeps compile time and executable size down.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#include "stb_image.h"
