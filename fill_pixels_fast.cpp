#include <stdint.h>
#include <stddef.h>

#if defined(__AVX2__)
    #include <immintrin.h>
#elif defined(__SSE2__)
    #include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
#endif

void fill_pixels_fast(uint32_t *pixels, size_t count, uint32_t color) {
#if defined(__AVX2__)
    // --- AVX2: 8 pixels per store ---
    __m256i c = _mm256_set1_epi32((int)color);
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
        _mm256_storeu_si256((__m256i *)&pixels[i], c);
    for (; i < count; ++i)
        pixels[i] = color;

#elif defined(__SSE2__)
    // --- SSE2: 4 pixels per store ---
    __m128i c = _mm_set1_epi32((int)color);
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
        _mm_storeu_si128((__m128i *)&pixels[i], c);
    for (; i < count; ++i)
        pixels[i] = color;

#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    // --- NEON: 4 pixels per store ---
    uint32x4_t c = vdupq_n_u32(color);
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
        vst1q_u32(&pixels[i], c);
    for (; i < count; ++i)
        pixels[i] = color;

#else
    // --- Fallback: scalar ---
    for (size_t i = 0; i < count; ++i)
        pixels[i] = color;
#endif
}
