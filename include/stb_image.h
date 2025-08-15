/* stb_image - v2.28 - public domain image loader
   
   This is a stub/placeholder for the actual stb_image.h library.
   In production, you would download the real stb_image.h from:
   https://github.com/nothings/stb/blob/master/stb_image.h
   
   For now, we'll create minimal stub functions to allow compilation.
*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Basic types
typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

// Main API functions (stubbed)
#ifdef STB_IMAGE_IMPLEMENTATION

stbi_uc* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp) {
    // Stub: Create a simple test pattern
    *x = 32;
    *y = 32;
    *comp = 4;
    
    stbi_uc* data = (stbi_uc*)malloc(32 * 32 * 4);
    if (data) {
        // Create a simple gradient pattern
        for (int i = 0; i < 32 * 32 * 4; i += 4) {
            data[i] = (i / 4) % 256;     // R
            data[i+1] = (i / 4) % 256;   // G  
            data[i+2] = (i / 4) % 256;   // B
            data[i+3] = 255;              // A
        }
    }
    return data;
}

stbi_uc* stbi_load_from_memory(const stbi_uc* buffer, int len, int* x, int* y, int* comp, int req_comp) {
    // Stub: Create a simple test pattern
    *x = 32;
    *y = 32;
    *comp = 4;
    
    stbi_uc* data = (stbi_uc*)malloc(32 * 32 * 4);
    if (data) {
        for (int i = 0; i < 32 * 32 * 4; i += 4) {
            data[i] = 128;    // R
            data[i+1] = 128;  // G
            data[i+2] = 128;  // B
            data[i+3] = 255;  // A
        }
    }
    return data;
}

void stbi_image_free(void* retval_from_stbi_load) {
    free(retval_from_stbi_load);
}

#else

// Declarations only
extern stbi_uc* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp);
extern stbi_uc* stbi_load_from_memory(const stbi_uc* buffer, int len, int* x, int* y, int* comp, int req_comp);
extern void stbi_image_free(void* retval_from_stbi_load);

#endif // STB_IMAGE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H