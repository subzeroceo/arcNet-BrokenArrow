#include "../ParallelJobList_JobHeaders.h"
#include "../math/Math.h"
#include "../math/Vector.h"
#include "../math/Matrix.h"
#include "../math/Rotation.h"
#include "../math/Plane.h"
#include "../bv/Sphere.h"
#include "../bv/Bounds.h"
#include "RenderMatrix.h"

// FIXME:	it would be nice if all render matrices were 16-byte aligned
//			so there is no need for unaligned loads and stores everywhere

#ifdef _lint
#undef ARC_WIN_X86_SSE2_INTRIN
#endif

//lint -e438	// the non-SSE code isn't lint friendly, either
//lint -e550

#define RENDER_MATRIX_INVERSE_EPSILON		1e-16f	// JDC: changed from 1e-14f to allow full wasteland parallel light projections to invert
#define RENDER_MATRIX_INFINITY				1e30f	// NOTE: cannot initiaize a vec_float4 with anMath::INFINITY on the SPU
#define RENDER_MATRIX_PROJECTION_EPSILON	0.1f

#define CLIP_SPACE_OGL		// the OpenGL clip space Z is in the range [-1, 1]

/*
================================================================================================

Constant render matrices

================================================================================================
*/

// identity matrix
ALIGNTYPE16 const anGLMatrix renderMatrix_identity(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

// convert from our coordinate system (looking down X) to OpenGL's coordinate system (looking down -Z)
ALIGNTYPE16 const anGLMatrix renderMatrix_flipToOpenGL(
	 0.0f, -1.0f, 0.0f, 0.0f,
	 0.0f,  0.0f, 1.0f, 0.0f,
	-1.0f,  0.0f, 0.0f, 0.0f,
	 0.0f,  0.0f, 0.0f, 1.0f
);

// OpenGL -1 to 1.
ALIGNTYPE16 const anGLMatrix renderMatrix_windowSpaceToClipSpace(
	2.0f, 0.0f, 0.0f, -1.0f,
	0.0f, 2.0f, 0.0f, -1.0f,
	0.0f, 0.0f, 2.0f, -1.0f,
	0.0f, 0.0f, 0.0f,  1.0f
);

/*
================================================================================================

SIMD constants

================================================================================================
*/

#ifdef ARC_WIN_X86_SSE2_INTRIN

static const __m128i vector_int_1							= _mm_set1_epi32( 1 );
static const __m128i vector_int_4							= _mm_set1_epi32( 4 );
static const __m128i vector_int_0123						= _mm_set_epi32( 3, 2, 1, 0 );
static const __m128 vector_float_mask0						= __m128c( _mm_set1_epi32( 1<<0 ) );
static const __m128 vector_float_mask1						= __m128c( _mm_set1_epi32( 1<<1 ) );
static const __m128 vector_float_mask2						= __m128c( _mm_set1_epi32( 1<<2 ) );
static const __m128 vector_float_mask3						= __m128c( _mm_set1_epi32( 1<<3 ) );
static const __m128 vector_float_mask4						= __m128c( _mm_set1_epi32( 1<<4 ) );
static const __m128 vector_float_mask5						= __m128c( _mm_set1_epi32( 1<<5 ) );
static const __m128 vector_float_sign_bit					= __m128c( _mm_set1_epi32( IEEE_FLT_SIGN_MASK ) );
static const __m128 vector_float_abs_mask					= __m128c( _mm_set1_epi32( ~IEEE_FLT_SIGN_MASK ) );
static const __m128 vector_float_keep_last					= __m128c( _mm_set_epi32( -1, 0, 0, 0 ) );
static const __m128 vector_float_inverse_epsilon			= { RENDER_MATRIX_INVERSE_EPSILON, RENDER_MATRIX_INVERSE_EPSILON, RENDER_MATRIX_INVERSE_EPSILON, RENDER_MATRIX_INVERSE_EPSILON };
static const __m128 vector_float_smallest_non_denorm		= { 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f };
static const __m128 vector_float_pos_infinity				= { RENDER_MATRIX_INFINITY, RENDER_MATRIX_INFINITY, RENDER_MATRIX_INFINITY, RENDER_MATRIX_INFINITY };
static const __m128 vector_float_neg_infinity				= { -RENDER_MATRIX_INFINITY, -RENDER_MATRIX_INFINITY, -RENDER_MATRIX_INFINITY, -RENDER_MATRIX_INFINITY };
static const __m128 vector_float_zero						= { 0.0f, 0.0f, 0.0f, 0.0f };
static const __m128 vector_float_half						= { 0.5f, 0.5f, 0.5f, 0.5f };
static const __m128 vector_float_neg_half					= { -0.5f, -0.5f, -0.5f, -0.5f };
static const __m128 vector_float_one						= { 1.0f, 1.0f, 1.0f, 1.0f };
static const __m128 vector_float_pos_one					= { +1.0f, +1.0f, +1.0f, +1.0f };
static const __m128 vector_float_neg_one					= { -1.0f, -1.0f, -1.0f, -1.0f };
static const __m128 vector_float_last_one					= { 0.0f, 0.0f, 0.0f, 1.0f };

#endif

/*
================================================================================================

Box definition

================================================================================================
*/

/*
            4----{E}---5
 +         /|         /|
 Z      {H} {I}    {F} |
 -     /    |     /   {J}
      7--{G}-----6     |
      |     |    |     |
     {L}    0----|-{A}-1
      |    /    {K}   /       -
      | {D}      | {B}       Y
      |/         |/         +
      3---{C}----2

	    - X +
*/

static const short boxPolygonVertices[6][4] = {
	{ 0, 3, 7, 4 },	// neg-X
	{ 0, 1, 5, 4 },	// neg-Y
	{ 0, 1, 2, 3 },	// neg-Z
	{ 1, 2, 6, 5 },	// pos-X
	{ 2, 3, 7, 6 },	// pos-Y
	{ 4, 5, 6, 7 }	// pos-Z
};

static const short boxEdgeVertices[12][2] = {
	/* A = */ { 0, 1 }, /* B = */ { 1, 2 }, /* C = */ { 2, 3 }, /* D = */ { 3, 0 },	// bottom
	/* E = */ { 4, 5 }, /* F = */ { 5, 6 }, /* G = */ { 6, 7 }, /* H = */ { 7, 4 },	// top
	/* I = */ { 0, 4 }, /* J = */ { 1, 5 }, /* K = */ { 2, 6 }, /* L = */ { 3, 7 }	// sides
};

static int boxEdgePolygons[12][2] = {
	/* A = */ { 1, 2 }, /* B = */ { 3, 2 }, /* C = */ { 4, 2 }, /* D = */ { 0, 2 },	// bottom
	/* E = */ { 1, 5 }, /* F = */ { 3, 5 }, /* G = */ { 4, 5 }, /* H = */ { 0, 5 }, // top
	/* I = */ { 0, 1 }, /* J = */ { 3, 1 }, /* K = */ { 3, 4 }, /* L = */ { 0, 4 }	// sides
};

/*
#include <Windows.h>

class idCreateBoxFrontPolygonsForFrontBits {
public:
	idCreateBoxFrontPolygonsForFrontBits() {
		for ( int i = 0; i < 64; i++ ) {
			int frontPolygons[7] = { 0 };
			int numFrontPolygons = 0;
			char bits[7] = { 0 };
			for ( int j = 0; j < 6; j++ ) {
				if ( ( i & ( 1 << j ) ) != 0 ) {
					frontPolygons[numFrontPolygons++] = j;
					bits[5 - j] = '1';
				} else {
					bits[5 - j] = '0';
				}
			}
			const char *comment = ( ( i & ( i >> 3 ) & 7 ) != 0 ) ? " invalid" : "";
			if ( i == 0 ) {
				comment = " inside the box, every polygon is considered front facing";
				numFrontPolygons = 6;
				for ( int j = 0; j < 6; j++ ) {
					frontPolygons[j] = j;
				}
			}
			char buffer[1024];
			sprintf( buffer, "{ { %d, %d, %d, %d, %d, %d, %d }, %d }, // %s = %d%s\n",
								frontPolygons[0], frontPolygons[1], frontPolygons[2], frontPolygons[3],
								frontPolygons[4], frontPolygons[5], frontPolygons[6],
								numFrontPolygons, bits, i, comment );
			OutputDebugString( buffer );
		}
	}
} createBoxFrontPolygonsForFrontBits;
*/

// make sure this is a power of two for fast addressing an array of these without integer multiplication
static const struct frontPolygons_t {
	byte	indices[7];
	byte	count;
} boxFrontPolygonsForFrontBits[64] = {
	{ { 0, 1, 2, 3, 4, 5, 0 }, 6 }, // 000000 = 0 inside the box, every polygon is considered front facing
	{ { 0, 0, 0, 0, 0, 0, 0 }, 1 }, // 000001 = 1
	{ { 1, 0, 0, 0, 0, 0, 0 }, 1 }, // 000010 = 2
	{ { 0, 1, 0, 0, 0, 0, 0 }, 2 }, // 000011 = 3
	{ { 2, 0, 0, 0, 0, 0, 0 }, 1 }, // 000100 = 4
	{ { 0, 2, 0, 0, 0, 0, 0 }, 2 }, // 000101 = 5
	{ { 1, 2, 0, 0, 0, 0, 0 }, 2 }, // 000110 = 6
	{ { 0, 1, 2, 0, 0, 0, 0 }, 3 }, // 000111 = 7
	{ { 3, 0, 0, 0, 0, 0, 0 }, 1 }, // 001000 = 8
	{ { 0, 3, 0, 0, 0, 0, 0 }, 2 }, // 001001 = 9 invalid
	{ { 1, 3, 0, 0, 0, 0, 0 }, 2 }, // 001010 = 10
	{ { 0, 1, 3, 0, 0, 0, 0 }, 3 }, // 001011 = 11 invalid
	{ { 2, 3, 0, 0, 0, 0, 0 }, 2 }, // 001100 = 12
	{ { 0, 2, 3, 0, 0, 0, 0 }, 3 }, // 001101 = 13 invalid
	{ { 1, 2, 3, 0, 0, 0, 0 }, 3 }, // 001110 = 14
	{ { 0, 1, 2, 3, 0, 0, 0 }, 4 }, // 001111 = 15 invalid
	{ { 4, 0, 0, 0, 0, 0, 0 }, 1 }, // 010000 = 16
	{ { 0, 4, 0, 0, 0, 0, 0 }, 2 }, // 010001 = 17
	{ { 1, 4, 0, 0, 0, 0, 0 }, 2 }, // 010010 = 18 invalid
	{ { 0, 1, 4, 0, 0, 0, 0 }, 3 }, // 010011 = 19 invalid
	{ { 2, 4, 0, 0, 0, 0, 0 }, 2 }, // 010100 = 20
	{ { 0, 2, 4, 0, 0, 0, 0 }, 3 }, // 010101 = 21
	{ { 1, 2, 4, 0, 0, 0, 0 }, 3 }, // 010110 = 22 invalid
	{ { 0, 1, 2, 4, 0, 0, 0 }, 4 }, // 010111 = 23 invalid
	{ { 3, 4, 0, 0, 0, 0, 0 }, 2 }, // 011000 = 24
	{ { 0, 3, 4, 0, 0, 0, 0 }, 3 }, // 011001 = 25 invalid
	{ { 1, 3, 4, 0, 0, 0, 0 }, 3 }, // 011010 = 26 invalid
	{ { 0, 1, 3, 4, 0, 0, 0 }, 4 }, // 011011 = 27 invalid
	{ { 2, 3, 4, 0, 0, 0, 0 }, 3 }, // 011100 = 28
	{ { 0, 2, 3, 4, 0, 0, 0 }, 4 }, // 011101 = 29 invalid
	{ { 1, 2, 3, 4, 0, 0, 0 }, 4 }, // 011110 = 30 invalid
	{ { 0, 1, 2, 3, 4, 0, 0 }, 5 }, // 011111 = 31 invalid
	{ { 5, 0, 0, 0, 0, 0, 0 }, 1 }, // 100000 = 32
	{ { 0, 5, 0, 0, 0, 0, 0 }, 2 }, // 100001 = 33
	{ { 1, 5, 0, 0, 0, 0, 0 }, 2 }, // 100010 = 34
	{ { 0, 1, 5, 0, 0, 0, 0 }, 3 }, // 100011 = 35
	{ { 2, 5, 0, 0, 0, 0, 0 }, 2 }, // 100100 = 36 invalid
	{ { 0, 2, 5, 0, 0, 0, 0 }, 3 }, // 100101 = 37 invalid
	{ { 1, 2, 5, 0, 0, 0, 0 }, 3 }, // 100110 = 38 invalid
	{ { 0, 1, 2, 5, 0, 0, 0 }, 4 }, // 100111 = 39 invalid
	{ { 3, 5, 0, 0, 0, 0, 0 }, 2 }, // 101000 = 40
	{ { 0, 3, 5, 0, 0, 0, 0 }, 3 }, // 101001 = 41 invalid
	{ { 1, 3, 5, 0, 0, 0, 0 }, 3 }, // 101010 = 42
	{ { 0, 1, 3, 5, 0, 0, 0 }, 4 }, // 101011 = 43 invalid
	{ { 2, 3, 5, 0, 0, 0, 0 }, 3 }, // 101100 = 44 invalid
	{ { 0, 2, 3, 5, 0, 0, 0 }, 4 }, // 101101 = 45 invalid
	{ { 1, 2, 3, 5, 0, 0, 0 }, 4 }, // 101110 = 46 invalid
	{ { 0, 1, 2, 3, 5, 0, 0 }, 5 }, // 101111 = 47 invalid
	{ { 4, 5, 0, 0, 0, 0, 0 }, 2 }, // 110000 = 48
	{ { 0, 4, 5, 0, 0, 0, 0 }, 3 }, // 110001 = 49
	{ { 1, 4, 5, 0, 0, 0, 0 }, 3 }, // 110010 = 50 invalid
	{ { 0, 1, 4, 5, 0, 0, 0 }, 4 }, // 110011 = 51 invalid
	{ { 2, 4, 5, 0, 0, 0, 0 }, 3 }, // 110100 = 52 invalid
	{ { 0, 2, 4, 5, 0, 0, 0 }, 4 }, // 110101 = 53 invalid
	{ { 1, 2, 4, 5, 0, 0, 0 }, 4 }, // 110110 = 54 invalid
	{ { 0, 1, 2, 4, 5, 0, 0 }, 5 }, // 110111 = 55 invalid
	{ { 3, 4, 5, 0, 0, 0, 0 }, 3 }, // 111000 = 56
	{ { 0, 3, 4, 5, 0, 0, 0 }, 4 }, // 111001 = 57 invalid
	{ { 1, 3, 4, 5, 0, 0, 0 }, 4 }, // 111010 = 58 invalid
	{ { 0, 1, 3, 4, 5, 0, 0 }, 5 }, // 111011 = 59 invalid
	{ { 2, 3, 4, 5, 0, 0, 0 }, 4 }, // 111100 = 60 invalid
	{ { 0, 2, 3, 4, 5, 0, 0 }, 5 }, // 111101 = 61 invalid
	{ { 1, 2, 3, 4, 5, 0, 0 }, 5 }, // 111110 = 62 invalid
	{ { 0, 1, 2, 3, 4, 5, 0 }, 6 }, // 111111 = 63 invalid
};

/*
#include <Windows.h>

class idCreateBoxSilhouetteEdgesForFrontBits {
public:
	idCreateBoxSilhouetteEdgesForFrontBits() {
		for ( int i = 0; i < 64; i++ ) {
			int silhouetteEdges[12] = { 0 };
			int numSilhouetteEdges = 0;

			for ( int j = 0; j < 12; j++ ) {
				if ( i == 0 || ( ( i >> boxEdgePolygons[j][0] ) & 1 ) != ( ( i >> boxEdgePolygons[j][1] ) & 1 ) ) {
					silhouetteEdges[numSilhouetteEdges++] = j;
				}
			}

			char bits[7] = { 0 };
			for ( int j = 0; j < 6; j++ ) {
				if ( ( i & ( 1 << j ) ) != 0 ) {
					bits[5 - j] = '1';
				} else {
					bits[5 - j] = '0';
				}
			}
			const char * comment = ( ( i & ( i >> 3 ) & 7 ) != 0 ) ? " invalid" : "";
			if ( i == 0 ) {
				comment = " inside the box, every edge is considered part of the silhouette";
			}
			char buffer[1024];
			sprintf( buffer, "{ { %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d }, %2d }, // %s = %d%s\n",
								silhouetteEdges[0], silhouetteEdges[1], silhouetteEdges[2], silhouetteEdges[3],
								silhouetteEdges[4], silhouetteEdges[5], silhouetteEdges[6], silhouetteEdges[7],
								silhouetteEdges[8], silhouetteEdges[9], silhouetteEdges[10], silhouetteEdges[11],
								numSilhouetteEdges, bits, i, comment );
			OutputDebugString( buffer );
		}
	}
} createBoxSilhouetteEdgesForFrontBits;
*/

// make sure this is a power of two for fast addressing an array of these without integer multiplication
static const struct silhouetteEdges_t {
	byte	indices[12];
	int32	count;
} boxSilhouetteEdgesForFrontBits[64] = {
	{ {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 }, 12 }, // 000000 = 0 inside the box, every edge is considered part of the silhouette
	{ {  3,  7,  8, 11,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 000001 = 1
	{ {  0,  4,  8,  9,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 000010 = 2
	{ {  0,  3,  4,  7,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 000011 = 3
	{ {  0,  1,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 000100 = 4
	{ {  0,  1,  2,  7,  8, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 000101 = 5
	{ {  1,  2,  3,  4,  8,  9,  0,  0,  0,  0,  0,  0 },  6 }, // 000110 = 6
	{ {  1,  2,  4,  7,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 000111 = 7
	{ {  1,  5,  9, 10,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 001000 = 8
	{ {  1,  3,  5,  7,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 001001 = 9 invalid
	{ {  0,  1,  4,  5,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 001010 = 10
	{ {  0,  1,  3,  4,  5,  7, 10, 11,  0,  0,  0,  0 },  8 }, // 001011 = 11 invalid
	{ {  0,  2,  3,  5,  9, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 001100 = 12
	{ {  0,  2,  5,  7,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 001101 = 13 invalid
	{ {  2,  3,  4,  5,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 001110 = 14
	{ {  2,  4,  5,  7, 10, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 001111 = 15 invalid
	{ {  2,  6, 10, 11,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 010000 = 16
	{ {  2,  3,  6,  7,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 010001 = 17
	{ {  0,  2,  4,  6,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 010010 = 18 invalid
	{ {  0,  2,  3,  4,  6,  7,  9, 10,  0,  0,  0,  0 },  8 }, // 010011 = 19 invalid
	{ {  0,  1,  3,  6, 10, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 010100 = 20
	{ {  0,  1,  6,  7,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 010101 = 21
	{ {  1,  3,  4,  6,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 010110 = 22 invalid
	{ {  1,  4,  6,  7,  9, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 010111 = 23 invalid
	{ {  1,  2,  5,  6,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 011000 = 24
	{ {  1,  2,  3,  5,  6,  7,  8,  9,  0,  0,  0,  0 },  8 }, // 011001 = 25 invalid
	{ {  0,  1,  2,  4,  5,  6,  8, 11,  0,  0,  0,  0 },  8 }, // 011010 = 26 invalid
	{ {  0,  1,  2,  3,  4,  5,  6,  7,  0,  0,  0,  0 },  8 }, // 011011 = 27 invalid
	{ {  0,  3,  5,  6,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 011100 = 28
	{ {  0,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0 },  6 }, // 011101 = 29 invalid
	{ {  3,  4,  5,  6,  8, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 011110 = 30 invalid
	{ {  4,  5,  6,  7,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 011111 = 31 invalid
	{ {  4,  5,  6,  7,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 100000 = 32
	{ {  3,  4,  5,  6,  8, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 100001 = 33
	{ {  0,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0 },  6 }, // 100010 = 34
	{ {  0,  3,  5,  6,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 100011 = 35
	{ {  0,  1,  2,  3,  4,  5,  6,  7,  0,  0,  0,  0 },  8 }, // 100100 = 36 invalid
	{ {  0,  1,  2,  4,  5,  6,  8, 11,  0,  0,  0,  0 },  8 }, // 100101 = 37 invalid
	{ {  1,  2,  3,  5,  6,  7,  8,  9,  0,  0,  0,  0 },  8 }, // 100110 = 38 invalid
	{ {  1,  2,  5,  6,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 100111 = 39 invalid
	{ {  1,  4,  6,  7,  9, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 101000 = 40
	{ {  1,  3,  4,  6,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 101001 = 41 invalid
	{ {  0,  1,  6,  7,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 101010 = 42
	{ {  0,  1,  3,  6, 10, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 101011 = 43 invalid
	{ {  0,  2,  3,  4,  6,  7,  9, 10,  0,  0,  0,  0 },  8 }, // 101100 = 44 invalid
	{ {  0,  2,  4,  6,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 101101 = 45 invalid
	{ {  2,  3,  6,  7,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 101110 = 46 invalid
	{ {  2,  6, 10, 11,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 101111 = 47 invalid
	{ {  2,  4,  5,  7, 10, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 110000 = 48
	{ {  2,  3,  4,  5,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 110001 = 49
	{ {  0,  2,  5,  7,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 110010 = 50 invalid
	{ {  0,  2,  3,  5,  9, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 110011 = 51 invalid
	{ {  0,  1,  3,  4,  5,  7, 10, 11,  0,  0,  0,  0 },  8 }, // 110100 = 52 invalid
	{ {  0,  1,  4,  5,  8, 10,  0,  0,  0,  0,  0,  0 },  6 }, // 110101 = 53 invalid
	{ {  1,  3,  5,  7,  8,  9, 10, 11,  0,  0,  0,  0 },  8 }, // 110110 = 54 invalid
	{ {  1,  5,  9, 10,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 110111 = 55 invalid
	{ {  1,  2,  4,  7,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 111000 = 56
	{ {  1,  2,  3,  4,  8,  9,  0,  0,  0,  0,  0,  0 },  6 }, // 111001 = 57 invalid
	{ {  0,  1,  2,  7,  8, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 111010 = 58 invalid
	{ {  0,  1,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 111011 = 59 invalid
	{ {  0,  3,  4,  7,  9, 11,  0,  0,  0,  0,  0,  0 },  6 }, // 111100 = 60 invalid
	{ {  0,  4,  8,  9,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 111101 = 61 invalid
	{ {  3,  7,  8, 11,  0,  0,  0,  0,  0,  0,  0,  0 },  4 }, // 111110 = 62 invalid
	{ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  0 }, // 111111 = 63 invalid
};

/*
#include <Windows.h>

class idCreateBoxSilhouetteVerticesForFrontBits {
public:
	idCreateBoxSilhouetteVerticesForFrontBits() {
		for ( int i = 0; i < 64; i++ ) {
			int silhouetteEdges[12] = { 0 };
			int numSilhouetteEdges = 0;

			for ( int j = 0; j < 12; j++ ) {
				if ( i == 0 || ( ( i >> boxEdgePolygons[j][0] ) & 1 ) != ( ( i >> boxEdgePolygons[j][1] ) & 1 ) ) {
					silhouetteEdges[numSilhouetteEdges++] = j;
				}
			}

			int silhouetteVertices[8] = { 0 };
			int numSilhouetteVertices = 0;

			int vertex = boxEdgeVertices[silhouetteEdges[0]][0];
			for ( int j = 0; j < 7; j++ ) {
				int newVertex = -1;
				for ( int j = 0; j < numSilhouetteEdges; j++ ) {
					if ( silhouetteEdges[j] == -1 ) {
						continue;
					}
					if ( boxEdgeVertices[silhouetteEdges[j]][0] == vertex ) {
						newVertex = boxEdgeVertices[silhouetteEdges[j]][1];
						silhouetteEdges[j] = -1;
						break;
					} else if ( boxEdgeVertices[silhouetteEdges[j]][1] == vertex ) {
						newVertex = boxEdgeVertices[silhouetteEdges[j]][0];
						silhouetteEdges[j] = -1;
						break;
					}
				}
				if ( newVertex == -1 ) {
					break;
				}
				silhouetteVertices[numSilhouetteVertices++] = newVertex;
				vertex = newVertex;
			}

			char bits[7] = { 0 };
			for ( int j = 0; j < 6; j++ ) {
				if ( ( i & ( 1 << j ) ) != 0 ) {
					bits[5 - j] = '1';
				} else {
					bits[5 - j] = '0';
				}
			}
			const char *comment = ( ( i & ( i >> 3 ) & 7 ) != 0 ) ? " invalid" : "";
			if ( i == 0 ) {
				comment = " inside the box, no silhouette";
			}
			char buffer[1024];
			sprintf( buffer, "{ { %d, %d, %d, %d, %d, %d, %d }, %d }, // %s = %d%s\n",
								silhouetteVertices[0], silhouetteVertices[1], silhouetteVertices[2], silhouetteVertices[3],
								silhouetteVertices[4], silhouetteVertices[5], silhouetteVertices[6], numSilhouetteVertices, bits, i, comment );
			OutputDebugString( buffer );
		}
	}
} createBoxSilhouetteVerticesForFrontBits;
*/

// make sure this is a power of two for fast addressing an array of these without integer multiplication
static const struct silhouetteVertices_t {
	byte	indices[7];
	byte	count;
} boxSilhouetteVerticesForFrontBits[64] = {
	{ { 1, 2, 3, 0, 4, 5, 6 }, 7 }, // 000000 = 0 inside the box, no vertex is considered part of the silhouette
	{ { 0, 4, 7, 3, 0, 0, 0 }, 4 }, // 000001 = 1
	{ { 1, 5, 4, 0, 0, 0, 0 }, 4 }, // 000010 = 2
	{ { 1, 5, 4, 7, 3, 0, 0 }, 6 }, // 000011 = 3
	{ { 1, 2, 3, 0, 0, 0, 0 }, 4 }, // 000100 = 4
	{ { 1, 2, 3, 7, 4, 0, 0 }, 6 }, // 000101 = 5
	{ { 2, 3, 0, 4, 5, 1, 0 }, 6 }, // 000110 = 6
	{ { 2, 3, 7, 4, 5, 1, 0 }, 6 }, // 000111 = 7
	{ { 2, 6, 5, 1, 0, 0, 0 }, 4 }, // 001000 = 8
	{ { 2, 6, 5, 1, 0, 0, 0 }, 4 }, // 001001 = 9 invalid
	{ { 1, 2, 6, 5, 4, 0, 0 }, 6 }, // 001010 = 10
	{ { 1, 2, 6, 5, 4, 7, 3 }, 7 }, // 001011 = 11 invalid
	{ { 1, 5, 6, 2, 3, 0, 0 }, 6 }, // 001100 = 12
	{ { 1, 5, 6, 2, 3, 7, 4 }, 7 }, // 001101 = 13 invalid
	{ { 3, 0, 4, 5, 6, 2, 0 }, 6 }, // 001110 = 14
	{ { 3, 7, 4, 5, 6, 2, 0 }, 6 }, // 001111 = 15 invalid
	{ { 3, 7, 6, 2, 0, 0, 0 }, 4 }, // 010000 = 16
	{ { 3, 0, 4, 7, 6, 2, 0 }, 6 }, // 010001 = 17
	{ { 1, 5, 4, 0, 0, 0, 0 }, 4 }, // 010010 = 18 invalid
	{ { 1, 5, 4, 7, 6, 2, 3 }, 7 }, // 010011 = 19 invalid
	{ { 1, 2, 6, 7, 3, 0, 0 }, 6 }, // 010100 = 20
	{ { 1, 2, 6, 7, 4, 0, 0 }, 6 }, // 010101 = 21
	{ { 2, 6, 7, 3, 0, 4, 5 }, 7 }, // 010110 = 22 invalid
	{ { 2, 6, 7, 4, 5, 1, 0 }, 6 }, // 010111 = 23 invalid
	{ { 2, 3, 7, 6, 5, 1, 0 }, 6 }, // 011000 = 24
	{ { 2, 3, 0, 4, 7, 6, 5 }, 7 }, // 011001 = 25 invalid
	{ { 1, 2, 3, 7, 6, 5, 4 }, 7 }, // 011010 = 26 invalid
	{ { 1, 2, 3, 0, 0, 0, 0 }, 4 }, // 011011 = 27 invalid
	{ { 1, 5, 6, 7, 3, 0, 0 }, 6 }, // 011100 = 28
	{ { 1, 5, 6, 7, 4, 0, 0 }, 6 }, // 011101 = 29 invalid
	{ { 0, 4, 5, 6, 7, 3, 0 }, 6 }, // 011110 = 30 invalid
	{ { 5, 6, 7, 4, 0, 0, 0 }, 4 }, // 011111 = 31 invalid
	{ { 5, 6, 7, 4, 0, 0, 0 }, 4 }, // 100000 = 32
	{ { 0, 4, 5, 6, 7, 3, 0 }, 6 }, // 100001 = 33
	{ { 1, 5, 6, 7, 4, 0, 0 }, 6 }, // 100010 = 34
	{ { 1, 5, 6, 7, 3, 0, 0 }, 6 }, // 100011 = 35
	{ { 1, 2, 3, 0, 0, 0, 0 }, 4 }, // 100100 = 36 invalid
	{ { 1, 2, 3, 7, 6, 5, 4 }, 7 }, // 100101 = 37 invalid
	{ { 2, 3, 0, 4, 7, 6, 5 }, 7 }, // 100110 = 38 invalid
	{ { 2, 3, 7, 6, 5, 1, 0 }, 6 }, // 100111 = 39 invalid
	{ { 2, 6, 7, 4, 5, 1, 0 }, 6 }, // 101000 = 40
	{ { 2, 6, 7, 3, 0, 4, 5 }, 7 }, // 101001 = 41 invalid
	{ { 1, 2, 6, 7, 4, 0, 0 }, 6 }, // 101010 = 42
	{ { 1, 2, 6, 7, 3, 0, 0 }, 6 }, // 101011 = 43 invalid
	{ { 1, 5, 4, 7, 6, 2, 3 }, 7 }, // 101100 = 44 invalid
	{ { 1, 5, 4, 0, 0, 0, 0 }, 4 }, // 101101 = 45 invalid
	{ { 3, 0, 4, 7, 6, 2, 0 }, 6 }, // 101110 = 46 invalid
	{ { 3, 7, 6, 2, 0, 0, 0 }, 4 }, // 101111 = 47 invalid
	{ { 3, 7, 4, 5, 6, 2, 0 }, 6 }, // 110000 = 48
	{ { 3, 0, 4, 5, 6, 2, 0 }, 6 }, // 110001 = 49
	{ { 1, 5, 6, 2, 3, 7, 4 }, 7 }, // 110010 = 50 invalid
	{ { 1, 5, 6, 2, 3, 0, 0 }, 6 }, // 110011 = 51 invalid
	{ { 1, 2, 6, 5, 4, 7, 3 }, 7 }, // 110100 = 52 invalid
	{ { 1, 2, 6, 5, 4, 0, 0 }, 6 }, // 110101 = 53 invalid
	{ { 2, 6, 5, 1, 0, 0, 0 }, 4 }, // 110110 = 54 invalid
	{ { 2, 6, 5, 1, 0, 0, 0 }, 4 }, // 110111 = 55 invalid
	{ { 2, 3, 7, 4, 5, 1, 0 }, 6 }, // 111000 = 56
	{ { 2, 3, 0, 4, 5, 1, 0 }, 6 }, // 111001 = 57 invalid
	{ { 1, 2, 3, 7, 4, 0, 0 }, 6 }, // 111010 = 58 invalid
	{ { 1, 2, 3, 0, 0, 0, 0 }, 4 }, // 111011 = 59 invalid
	{ { 1, 5, 4, 7, 3, 0, 0 }, 6 }, // 111100 = 60 invalid
	{ { 1, 5, 4, 0, 0, 0, 0 }, 4 }, // 111101 = 61 invalid
	{ { 0, 4, 7, 3, 0, 0, 0 }, 4 }, // 111110 = 62 invalid
	{ { 0, 0, 0, 0, 0, 0, 0 }, 0 }, // 111111 = 63 invalid
};

/*
========================
GetBoxFrontBits

front bits:
  bit 0 = neg-X is front facing
  bit 1 = neg-Y is front facing
  bit 2 = neg-Z is front facing
  bit 3 = pos-X is front facing
  bit 4 = pos-Y is front facing
  bit 5 = pos-Z is front facing
========================
*/
#ifdef ARC_WIN_X86_SSE2_INTRIN

static int GetBoxFrontBits_SSE2( const __m128 & b0, const __m128 & b1, const __m128 & viewOrigin ) {
	const __m128 dir0 = _mm_sub_ps( viewOrigin, b0 );
	const __m128 dir1 = _mm_sub_ps( b1, viewOrigin );
	const __m128 d0 = _mm_cmplt_ps( dir0, _mm_setzero_ps() );
	const __m128 d1 = _mm_cmplt_ps( dir1, _mm_setzero_ps() );

	int frontBits = _mm_movemask_ps( d0 ) | ( _mm_movemask_ps( d1 ) << 3 );
	return frontBits;
}
#else

static int GetBoxFrontBits_Generic( const anBounds & bounds, const anVec3 & viewOrigin ) {
	anVec3 dir0 = viewOrigin - bounds[0];
	anVec3 dir1 = bounds[1] - viewOrigin;
	int frontBits = 0;
	frontBits |= IEEE_FLT_SIGNBITSET( dir0.x ) << 0;
	frontBits |= IEEE_FLT_SIGNBITSET( dir0.y ) << 1;
	frontBits |= IEEE_FLT_SIGNBITSET( dir0.z ) << 2;
	frontBits |= IEEE_FLT_SIGNBITSET( dir1.x ) << 3;
	frontBits |= IEEE_FLT_SIGNBITSET( dir1.y ) << 4;
	frontBits |= IEEE_FLT_SIGNBITSET( dir1.z ) << 5;
	return frontBits;
}
#endif

/*
================================================================================================

anGLMatrix implementation

================================================================================================
*/

/*
========================
anGLMatrix::CreateFromOriginAxis
========================
*/
void anGLMatrix::CreateFromOriginAxis( const anVec3 & origin, const anMat3 & axis, anGLMatrix & out ) {
	out[0][0] = axis[0][0];
	out[0][1] = axis[1][0];
	out[0][2] = axis[2][0];
	out[0][3] = origin[0];

	out[1][0] = axis[0][1];
	out[1][1] = axis[1][1];
	out[1][2] = axis[2][1];
	out[1][3] = origin[1];

	out[2][0] = axis[0][2];
	out[2][1] = axis[1][2];
	out[2][2] = axis[2][2];
	out[2][3] = origin[2];

	out[3][0] = 0.0f;
	out[3][1] = 0.0f;
	out[3][2] = 0.0f;
	out[3][3] = 1.0f;
}

/*
========================
anGLMatrix::CreateFromOriginAxisScale
========================
*/
void anGLMatrix::CreateFromOriginAxisScale( const anVec3 & origin, const anMat3 & axis, const anVec3 & scale, anGLMatrix & out ) {
	out[0][0] = axis[0][0] * scale[0];
	out[0][1] = axis[1][0] * scale[1];
	out[0][2] = axis[2][0] * scale[2];
	out[0][3] = origin[0];

	out[1][0] = axis[0][1] * scale[0];
	out[1][1] = axis[1][1] * scale[1];
	out[1][2] = axis[2][1] * scale[2];
	out[1][3] = origin[1];

	out[2][0] = axis[0][2] * scale[0];
	out[2][1] = axis[1][2] * scale[1];
	out[2][2] = axis[2][2] * scale[2];
	out[2][3] = origin[2];

	out[3][0] = 0.0f;
	out[3][1] = 0.0f;
	out[3][2] = 0.0f;
	out[3][3] = 1.0f;
}

/*
========================
anGLMatrix::CreateViewMatrix

Our axis looks down positive +X, render matrix looks down -Z.
========================
*/
void anGLMatrix::CreateViewMatrix( const anVec3 & origin, const anMat3 & axis, anGLMatrix & out ) {
	out[0][0] = -axis[1][0];
	out[0][1] = -axis[1][1];
	out[0][2] = -axis[1][2];
	out[0][3] = origin[0] * axis[1][0] + origin[1] * axis[1][1] + origin[2] * axis[1][2];

	out[1][0] = axis[2][0];
	out[1][1] = axis[2][1];
	out[1][2] = axis[2][2];
	out[1][3] = -( origin[0] * axis[2][0] + origin[1] * axis[2][1] + origin[2] * axis[2][2] );

	out[2][0] = -axis[0][0];
	out[2][1] = -axis[0][1];
	out[2][2] = -axis[0][2];
	out[2][3] = origin[0] * axis[0][0] + origin[1] * axis[0][1] + origin[2] * axis[0][2];

	out[3][0] = 0.0f;
	out[3][1] = 0.0f;
	out[3][2] = 0.0f;
	out[3][3] = 1.0f;
}

/*
========================
anGLMatrix::CreateProjectionMatrix

If zFar == 0, an infinite far plane will be used.
========================
*/
void anGLMatrix::CreateProjectionMatrix( float xMin, float xMax, float yMin, float yMax, float zNear, float zFar, anGLMatrix & out ) {
	const float width  = xMax - xMin;
	const float height = yMax - yMin;

	out[0][0] = 2.0f * zNear / width;
	out[0][1] = 0.0f;
	out[0][2] = ( xMax + xMin ) / width;	// normally 0
	out[0][3] = 0.0f;

	out[1][0] = 0.0f;
	out[1][1] = 2.0f * zNear / height;
	out[1][2] = ( yMax + yMin ) / height;	// normally 0
	out[1][3] = 0.0f;

	if ( zFar <= zNear ) {
		// this is the far-plane-at-infinity formulation
		out[2][0] = 0.0f;
		out[2][1] = 0.0f;
		out[2][2] = -1.0f;
#if defined( CLIP_SPACE_D3D )
		// the D3D clip space Z is in range [0,1] instead of [-1,1]
		out[2][3] = -zNear;
#else
		out[2][3] = -2.0f * zNear;
#endif
	} else {
		out[2][0] = 0.0f;
		out[2][1] = 0.0f;
#if defined( CLIP_SPACE_D3D )
		// the D3D clip space Z is in range [0,1] instead of [-1,1]
		out[2][2] = -( zFar ) / ( zFar - zNear );
		out[2][3] = -( zFar * zNear ) / ( zFar - zNear );
#else
		out[2][2] = -( zFar + zNear ) / ( zFar - zNear );
		out[2][3] = -( 2.0f * zFar * zNear ) / ( zFar - zNear );
#endif
	}

	out[3][0] = 0.0f;
	out[3][1] = 0.0f;
	out[3][2] = -1.0f;
	out[3][3] = 0.0f;
}

/*
========================
anGLMatrix::CreateProjectionMatrixFov

xOffset and yOffset should be in the -1 to 1 range for sub-pixel accumulation jitter.
xOffset can also be used for eye separation when rendering stereo.
========================
*/
void anGLMatrix::CreateProjectionMatrixFov( float xFovDegrees, float yFovDegrees, float zNear, float zFar, float xOffset, float yOffset, anGLMatrix & out ) {
	float xMax = zNear * anMath::Tan( DEG2RAD( xFovDegrees ) * 0.5f );
	float xMin = -xMax;

	float yMax = zNear * anMath::Tan( DEG2RAD( yFovDegrees ) * 0.5f );
	float yMin = -yMax;

	xMin += xOffset;
	xMax += xOffset;

	yMin += yOffset;
	yMax += yOffset;

	CreateProjectionMatrix( xMin, xMax, yMin, yMax, zNear, zFar, out );
}

/*
========================
anGLMatrix::OffsetScaleForBounds

Add the offset to the center of the bounds and scale for the width of the bounds.
The result matrix will transform the unit-cube to exactly cover the bounds.
========================
*/
void anGLMatrix::OffsetScaleForBounds( const anGLMatrix &src, const anBounds &bounds, anGLMatrix &out ) {
	assert( &src != &out );

#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 b0 = _mm_loadu_bounds_0( bounds );
	__m128 b1 = _mm_loadu_bounds_1( bounds );

	__m128 offset = _mm_mul_ps( _mm_add_ps( b1, b0 ), vector_float_half );
	__m128 scale  = _mm_mul_ps( _mm_sub_ps( b1, b0 ), vector_float_half );

	scale = _mm_or_ps( scale, vector_float_last_one );

	__m128 a0 = _mm_loadu_ps( src.m + 0*4 );
	__m128 a1 = _mm_loadu_ps( src.m + 1*4 );
	__m128 a2 = _mm_loadu_ps( src.m + 2*4 );
	__m128 a3 = _mm_loadu_ps( src.m + 3*4 );

	__m128 d0 = _mm_mul_ps( a0, offset );
	__m128 d1 = _mm_mul_ps( a1, offset );
	__m128 d2 = _mm_mul_ps( a2, offset );
	__m128 d3 = _mm_mul_ps( a3, offset );

	__m128 s0 = _mm_unpacklo_ps( d0, d2 );		// a0, c0, a1, c1
	__m128 s1 = _mm_unpackhi_ps( d0, d2 );		// a2, c2, a3, c3
	__m128 s2 = _mm_unpacklo_ps( d1, d3 );		// b0, d0, b1, d1
	__m128 s3 = _mm_unpackhi_ps( d1, d3 );		// b2, d2, b3, d3

	__m128 t0 = _mm_unpacklo_ps( s0, s2 );		// a0, b0, c0, d0
	__m128 t1 = _mm_unpackhi_ps( s0, s2 );		// a1, b1, c1, d1
	__m128 t2 = _mm_unpacklo_ps( s1, s3 );		// a2, b2, c2, d2

	t0 = _mm_add_ps( t0, t1 );
	t0 = _mm_add_ps( t0, t2 );

	__m128 n0 = _mm_and_ps( _mm_splat_ps( t0, 0 ), vector_float_keep_last );
	__m128 n1 = _mm_and_ps( _mm_splat_ps( t0, 1 ), vector_float_keep_last );
	__m128 n2 = _mm_and_ps( _mm_splat_ps( t0, 2 ), vector_float_keep_last );
	__m128 n3 = _mm_and_ps( _mm_splat_ps( t0, 3 ), vector_float_keep_last );

	a0 = _mm_madd_ps( a0, scale, n0 );
	a1 = _mm_madd_ps( a1, scale, n1 );
	a2 = _mm_madd_ps( a2, scale, n2 );
	a3 = _mm_madd_ps( a3, scale, n3 );

	_mm_storeu_ps( out.m + 0*4, a0 );
	_mm_storeu_ps( out.m + 1*4, a1 );
	_mm_storeu_ps( out.m + 2*4, a2 );
	_mm_storeu_ps( out.m + 3*4, a3 );
#else
	const anVec3 offset = ( bounds[1] + bounds[0] ) * 0.5f;
	const anVec3 scale = ( bounds[1] - bounds[0] ) * 0.5f;

	out[0][0] = src[0][0] * scale[0];
	out[0][1] = src[0][1] * scale[1];
	out[0][2] = src[0][2] * scale[2];
	out[0][3] = src[0][3] + src[0][0] * offset[0] + src[0][1] * offset[1] + src[0][2] * offset[2];

	out[1][0] = src[1][0] * scale[0];
	out[1][1] = src[1][1] * scale[1];
	out[1][2] = src[1][2] * scale[2];
	out[1][3] = src[1][3] + src[1][0] * offset[0] + src[1][1] * offset[1] + src[1][2] * offset[2];

	out[2][0] = src[2][0] * scale[0];
	out[2][1] = src[2][1] * scale[1];
	out[2][2] = src[2][2] * scale[2];
	out[2][3] = src[2][3] + src[2][0] * offset[0] + src[2][1] * offset[1] + src[2][2] * offset[2];

	out[3][0] = src[3][0] * scale[0];
	out[3][1] = src[3][1] * scale[1];
	out[3][2] = src[3][2] * scale[2];
	out[3][3] = src[3][3] + src[3][0] * offset[0] + src[3][1] * offset[1] + src[3][2] * offset[2];

#endif
}

/*
========================
anGLMatrix::InverseOffsetScaleForBounds

Subtract the offset to the center of the bounds and inverse scale for the width of the bounds.
The result matrix will transform the bounds to exactly cover the unit-cube.
========================
*/
void anGLMatrix::InverseOffsetScaleForBounds( const anGLMatrix & src, const anBounds & bounds, anGLMatrix & out ) {
	assert( &src != &out );

#ifdef ARC_WIN_X86_SSE2_INTRIN

	__m128 b0 = _mm_loadu_bounds_0( bounds );
	__m128 b1 = _mm_loadu_bounds_1( bounds );

	__m128 offset = _mm_mul_ps( _mm_add_ps( b1, b0 ), vector_float_neg_half );
	__m128 scale  = _mm_mul_ps( _mm_sub_ps( b0, b1 ), vector_float_neg_half );

	scale = _mm_max_ps( scale, vector_float_smallest_non_denorm );

	__m128 rscale = _mm_rcp32_ps( scale );

	offset = _mm_mul_ps( offset, rscale );

	__m128 d0 = _mm_and_ps( _mm_splat_ps( offset, 0 ), vector_float_keep_last );
	__m128 d1 = _mm_and_ps( _mm_splat_ps( offset, 1 ), vector_float_keep_last );
	__m128 d2 = _mm_and_ps( _mm_splat_ps( offset, 2 ), vector_float_keep_last );

	__m128 a0 = _mm_loadu_ps( src.m + 0*4 );
	__m128 a1 = _mm_loadu_ps( src.m + 1*4 );
	__m128 a2 = _mm_loadu_ps( src.m + 2*4 );
	__m128 a3 = _mm_loadu_ps( src.m + 3*4 );

	a0 = _mm_madd_ps( a0, _mm_splat_ps( rscale, 0 ), d0 );
	a1 = _mm_madd_ps( a1, _mm_splat_ps( rscale, 1 ), d1 );
	a2 = _mm_madd_ps( a2, _mm_splat_ps( rscale, 2 ), d2 );

	_mm_storeu_ps( out.m + 0*4, a0 );
	_mm_storeu_ps( out.m + 1*4, a1 );
	_mm_storeu_ps( out.m + 2*4, a2 );
	_mm_storeu_ps( out.m + 3*4, a3 );

#else

	const anVec3 offset = -0.5f * ( bounds[1] + bounds[0] );
	const anVec3 scale = 2.0f / ( bounds[1] - bounds[0] );

	out[0][0] = scale[0] * src[0][0];
	out[0][1] = scale[0] * src[0][1];
	out[0][2] = scale[0] * src[0][2];
	out[0][3] = scale[0] * ( src[0][3] + offset[0] );

	out[1][0] = scale[1] * src[1][0];
	out[1][1] = scale[1] * src[1][1];
	out[1][2] = scale[1] * src[1][2];
	out[1][3] = scale[1] * ( src[1][3] + offset[1] );

	out[2][0] = scale[2] * src[2][0];
	out[2][1] = scale[2] * src[2][1];
	out[2][2] = scale[2] * src[2][2];
	out[2][3] = scale[2] * ( src[2][3] + offset[2] );

	out[3][0] = src[3][0];
	out[3][1] = src[3][1];
	out[3][2] = src[3][2];
	out[3][3] = src[3][3];
#endif
}

/*
========================
anGLMatrix::Transpose
========================
*/
void anGLMatrix::Transpose( const anGLMatrix & src, anGLMatrix & out ) {
	assert( &src != &out );
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 a0 = _mm_loadu_ps( src.m + 0*4 );
	const __m128 a1 = _mm_loadu_ps( src.m + 1*4 );
	const __m128 a2 = _mm_loadu_ps( src.m + 2*4 );
	const __m128 a3 = _mm_loadu_ps( src.m + 3*4 );

	const __m128 r0 = _mm_unpacklo_ps( a0, a2 );
	const __m128 r1 = _mm_unpackhi_ps( a0, a2 );
	const __m128 r2 = _mm_unpacklo_ps( a1, a3 );
	const __m128 r3 = _mm_unpackhi_ps( a1, a3 );

	const __m128 t0 = _mm_unpacklo_ps( r0, r2 );
	const __m128 t1 = _mm_unpackhi_ps( r0, r2 );
	const __m128 t2 = _mm_unpacklo_ps( r1, r3 );
	const __m128 t3 = _mm_unpackhi_ps( r1, r3 );

	_mm_storeu_ps( out.m + 0*4, t0 );
	_mm_storeu_ps( out.m + 1*4, t1 );
	_mm_storeu_ps( out.m + 2*4, t2 );
	_mm_storeu_ps( out.m + 3*4, t3 );

#else

	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[i][j] = src[j][i];
		}
	}
#endif
}

/*
========================
anGLMatrix::Multiply
========================
*/
void anGLMatrix::Multiply( const anGLMatrix & a, const anGLMatrix & b, anGLMatrix & out ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 a0 = _mm_loadu_ps( a.m + 0*4 );
	__m128 a1 = _mm_loadu_ps( a.m + 1*4 );
	__m128 a2 = _mm_loadu_ps( a.m + 2*4 );
	__m128 a3 = _mm_loadu_ps( a.m + 3*4 );

	__m128 b0 = _mm_loadu_ps( b.m + 0*4 );
	__m128 b1 = _mm_loadu_ps( b.m + 1*4 );
	__m128 b2 = _mm_loadu_ps( b.m + 2*4 );
	__m128 b3 = _mm_loadu_ps( b.m + 3*4 );

	__m128 t0 = _mm_mul_ps( _mm_splat_ps( a0, 0 ), b0 );
	__m128 t1 = _mm_mul_ps( _mm_splat_ps( a1, 0 ), b0 );
	__m128 t2 = _mm_mul_ps( _mm_splat_ps( a2, 0 ), b0 );
	__m128 t3 = _mm_mul_ps( _mm_splat_ps( a3, 0 ), b0 );

	t0 = _mm_madd_ps( _mm_splat_ps( a0, 1 ), b1, t0 );
	t1 = _mm_madd_ps( _mm_splat_ps( a1, 1 ), b1, t1 );
	t2 = _mm_madd_ps( _mm_splat_ps( a2, 1 ), b1, t2 );
	t3 = _mm_madd_ps( _mm_splat_ps( a3, 1 ), b1, t3 );

	t0 = _mm_madd_ps( _mm_splat_ps( a0, 2 ), b2, t0 );
	t1 = _mm_madd_ps( _mm_splat_ps( a1, 2 ), b2, t1 );
	t2 = _mm_madd_ps( _mm_splat_ps( a2, 2 ), b2, t2 );
	t3 = _mm_madd_ps( _mm_splat_ps( a3, 2 ), b2, t3 );

	t0 = _mm_madd_ps( _mm_splat_ps( a0, 3 ), b3, t0 );
	t1 = _mm_madd_ps( _mm_splat_ps( a1, 3 ), b3, t1 );
	t2 = _mm_madd_ps( _mm_splat_ps( a2, 3 ), b3, t2 );
	t3 = _mm_madd_ps( _mm_splat_ps( a3, 3 ), b3, t3 );

	_mm_storeu_ps( out.m + 0*4, t0 );
	_mm_storeu_ps( out.m + 1*4, t1 );
	_mm_storeu_ps( out.m + 2*4, t2 );
	_mm_storeu_ps( out.m + 3*4, t3 );
#else
/*	for ( int i = 0 ; i < 4 ; i++ ) {
		for ( int j = 0 ; j < 4 ; j++ ) {
			out.m[ i * 4 + j ] =
				a.m[ i * 4 + 0 ] * b.m[ 0 * 4 + j ] +
				a.m[ i * 4 + 1 ] * b.m[ 1 * 4 + j ] +
				a.m[ i * 4 + 2 ] * b.m[ 2 * 4 + j ] +
				a.m[ i * 4 + 3 ] * b.m[ 3 * 4 + j ];
		}
	}*/

	out.m[0*4+0] = a.m[0*4+0]*b.m[0*4+0] + a.m[0*4+1]*b.m[1*4+0] + a.m[0*4+2]*b.m[2*4+0] + a.m[0*4+3]*b.m[3*4+0];
	out.m[0*4+1] = a.m[0*4+0]*b.m[0*4+1] + a.m[0*4+1]*b.m[1*4+1] + a.m[0*4+2]*b.m[2*4+1] + a.m[0*4+3]*b.m[3*4+1];
	out.m[0*4+2] = a.m[0*4+0]*b.m[0*4+2] + a.m[0*4+1]*b.m[1*4+2] + a.m[0*4+2]*b.m[2*4+2] + a.m[0*4+3]*b.m[3*4+2];
	out.m[0*4+3] = a.m[0*4+0]*b.m[0*4+3] + a.m[0*4+1]*b.m[1*4+3] + a.m[0*4+2]*b.m[2*4+3] + a.m[0*4+3]*b.m[3*4+3];

	out.m[1*4+0] = a.m[1*4+0]*b.m[0*4+0] + a.m[1*4+1]*b.m[1*4+0] + a.m[1*4+2]*b.m[2*4+0] + a.m[1*4+3]*b.m[3*4+0];
	out.m[1*4+1] = a.m[1*4+0]*b.m[0*4+1] + a.m[1*4+1]*b.m[1*4+1] + a.m[1*4+2]*b.m[2*4+1] + a.m[1*4+3]*b.m[3*4+1];
	out.m[1*4+2] = a.m[1*4+0]*b.m[0*4+2] + a.m[1*4+1]*b.m[1*4+2] + a.m[1*4+2]*b.m[2*4+2] + a.m[1*4+3]*b.m[3*4+2];
	out.m[1*4+3] = a.m[1*4+0]*b.m[0*4+3] + a.m[1*4+1]*b.m[1*4+3] + a.m[1*4+2]*b.m[2*4+3] + a.m[1*4+3]*b.m[3*4+3];

	out.m[2*4+0] = a.m[2*4+0]*b.m[0*4+0] + a.m[2*4+1]*b.m[1*4+0] + a.m[2*4+2]*b.m[2*4+0] + a.m[2*4+3]*b.m[3*4+0];
	out.m[2*4+1] = a.m[2*4+0]*b.m[0*4+1] + a.m[2*4+1]*b.m[1*4+1] + a.m[2*4+2]*b.m[2*4+1] + a.m[2*4+3]*b.m[3*4+1];
	out.m[2*4+2] = a.m[2*4+0]*b.m[0*4+2] + a.m[2*4+1]*b.m[1*4+2] + a.m[2*4+2]*b.m[2*4+2] + a.m[2*4+3]*b.m[3*4+2];
	out.m[2*4+3] = a.m[2*4+0]*b.m[0*4+3] + a.m[2*4+1]*b.m[1*4+3] + a.m[2*4+2]*b.m[2*4+3] + a.m[2*4+3]*b.m[3*4+3];

	out.m[3*4+0] = a.m[3*4+0]*b.m[0*4+0] + a.m[3*4+1]*b.m[1*4+0] + a.m[3*4+2]*b.m[2*4+0] + a.m[3*4+3]*b.m[3*4+0];
	out.m[3*4+1] = a.m[3*4+0]*b.m[0*4+1] + a.m[3*4+1]*b.m[1*4+1] + a.m[3*4+2]*b.m[2*4+1] + a.m[3*4+3]*b.m[3*4+1];
	out.m[3*4+2] = a.m[3*4+0]*b.m[0*4+2] + a.m[3*4+1]*b.m[1*4+2] + a.m[3*4+2]*b.m[2*4+2] + a.m[3*4+3]*b.m[3*4+2];
	out.m[3*4+3] = a.m[3*4+0]*b.m[0*4+3] + a.m[3*4+1]*b.m[1*4+3] + a.m[3*4+2]*b.m[2*4+3] + a.m[3*4+3]*b.m[3*4+3];
#endif
}

/*
========================
anGLMatrix::Inverse

inverse( M ) = ( 1 / determinant( M ) ) * transpose( cofactor( M ) )

This code is based on the code written by C�dric Lallain, published on "Cell Performance"
(by Mike Acton) and released under the BSD 3-Clause ( "BSD New" or "BSD Simplified" ) license.
https://code.google.com/p/cellperformance-snippets/

Note that large parallel lights can have very small values in the projection matrix,
scaling tens of thousands of world units down to a 0-1 range, so the determinants
can get really, really small.
========================
*/
bool anGLMatrix::Inverse( const anGLMatrix & src, anGLMatrix & out ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 r0 = _mm_loadu_ps( src.m + 0 * 4 );
	const __m128 r1 = _mm_loadu_ps( src.m + 1 * 4 );
	const __m128 r2 = _mm_loadu_ps( src.m + 2 * 4 );
	const __m128 r3 = _mm_loadu_ps( src.m + 3 * 4 );

	// rXuY = row X rotated up by Y floats.
	const __m128 r0u1 = _mm_perm_ps( r0, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r0u2 = _mm_perm_ps( r0, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r0u3 = _mm_perm_ps( r0, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 r1u1 = _mm_perm_ps( r1, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r1u2 = _mm_perm_ps( r1, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r1u3 = _mm_perm_ps( r1, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 r2u1 = _mm_perm_ps( r2, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r2u2 = _mm_perm_ps( r2, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r2u3 = _mm_perm_ps( r2, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 r3u1 = _mm_perm_ps( r3, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r3u2 = _mm_perm_ps( r3, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r3u3 = _mm_perm_ps( r3, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 m_r2u2_r3u3      						= _mm_mul_ps( r2u2, r3u3 );
	const __m128 m_r1u1_r2u2_r3u3 						= _mm_mul_ps( r1u1, m_r2u2_r3u3 );
	const __m128 m_r2u3_r3u1							= _mm_mul_ps( r2u3, r3u1 );
	const __m128 a_m_r1u2_r2u3_r3u1_m_r1u1_r2u2_r3u3	= _mm_madd_ps( r1u2, m_r2u3_r3u1, m_r1u1_r2u2_r3u3 );
	const __m128 m_r2u1_r3u2							= _mm_perm_ps( m_r2u2_r3u3, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 pos_part_det3x3_r0						= _mm_madd_ps( r1u3, m_r2u1_r3u2, a_m_r1u2_r2u3_r3u1_m_r1u1_r2u2_r3u3 );
	const __m128 m_r2u3_r3u2							= _mm_mul_ps( r2u3, r3u2 );
	const __m128 m_r1u1_r2u3_r3u2						= _mm_mul_ps( r1u1, m_r2u3_r3u2 );
	const __m128 m_r2u1_r3u3							= _mm_perm_ps( m_r2u3_r3u1, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 a_m_r1u2_r2u1_r3u3_m_r1u1_r2u3_r3u2	= _mm_madd_ps( r1u2, m_r2u1_r3u3, m_r1u1_r2u3_r3u2 );
	const __m128 m_r2u2_r3u1							= _mm_perm_ps( m_r2u3_r3u2, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 neg_part_det3x3_r0						= _mm_madd_ps( r1u3, m_r2u2_r3u1, a_m_r1u2_r2u1_r3u3_m_r1u1_r2u3_r3u2 );
	const __m128 det3x3_r0 								= _mm_sub_ps( pos_part_det3x3_r0, neg_part_det3x3_r0 );

	const __m128 m_r0u1_r2u2_r3u3 						= _mm_mul_ps( r0u1, m_r2u2_r3u3 );
	const __m128 a_m_r0u2_r2u3_r3u1_m_r0u1_r2u2_r3u3	= _mm_madd_ps( r0u2, m_r2u3_r3u1, m_r0u1_r2u2_r3u3 );
	const __m128 pos_part_det3x3_r1						= _mm_madd_ps( r0u3, m_r2u1_r3u2, a_m_r0u2_r2u3_r3u1_m_r0u1_r2u2_r3u3 );
	const __m128 m_r0u1_r2u3_r3u2						= _mm_mul_ps( r0u1, m_r2u3_r3u2 );
	const __m128 a_m_r0u2_r2u1_r3u3_m_r0u1_r2u3_r3u2	= _mm_madd_ps( r0u2, m_r2u1_r3u3, m_r0u1_r2u3_r3u2 );
	const __m128 neg_part_det3x3_r1						= _mm_madd_ps( r0u3, m_r2u2_r3u1, a_m_r0u2_r2u1_r3u3_m_r0u1_r2u3_r3u2 );
	const __m128 det3x3_r1 								= _mm_sub_ps( pos_part_det3x3_r1, neg_part_det3x3_r1 );

	const __m128 m_r0u1_r1u2      						= _mm_mul_ps( r0u1, r1u2 );
	const __m128 m_r0u1_r1u2_r2u3 						= _mm_mul_ps( m_r0u1_r1u2, r2u3 );
	const __m128 m_r0u2_r1u3							= _mm_perm_ps( m_r0u1_r1u2, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 a_m_r0u2_r1u3_r2u1_m_r0u1_r1u2_r2u3	= _mm_madd_ps( m_r0u2_r1u3, r2u1, m_r0u1_r1u2_r2u3 );
	const __m128 m_r0u3_r1u1							= _mm_mul_ps( r0u3, r1u1 );
	const __m128 pos_part_det3x3_r3						= _mm_madd_ps( m_r0u3_r1u1, r2u2, a_m_r0u2_r1u3_r2u1_m_r0u1_r1u2_r2u3 );
	const __m128 m_r0u1_r1u3							= _mm_perm_ps( m_r0u3_r1u1, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 m_r0u1_r1u3_r2u2						= _mm_mul_ps( m_r0u1_r1u3, r2u2 );
	const __m128 m_r0u2_r1u1							= _mm_mul_ps( r0u2, r1u1 );
	const __m128 a_m_r0u2_r1u1_r2u3_m_r0u1_r1u3_r2u2	= _mm_madd_ps( m_r0u2_r1u1, r2u3, m_r0u1_r1u3_r2u2 );
	const __m128 m_r0u3_r1u2							= _mm_perm_ps( m_r0u2_r1u1, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 neg_part_det3x3_r3						= _mm_madd_ps( m_r0u3_r1u2, r2u1, a_m_r0u2_r1u1_r2u3_m_r0u1_r1u3_r2u2 );
	const __m128 det3x3_r3 								= _mm_sub_ps( pos_part_det3x3_r3, neg_part_det3x3_r3 );

	const __m128 m_r0u1_r1u2_r3u3 						= _mm_mul_ps( m_r0u1_r1u2, r3u3 );
	const __m128 a_m_r0u2_r1u3_r3u1_m_r0u1_r1u2_r3u3	= _mm_madd_ps( m_r0u2_r1u3, r3u1, m_r0u1_r1u2_r3u3 );
	const __m128 pos_part_det3x3_r2						= _mm_madd_ps( m_r0u3_r1u1, r3u2, a_m_r0u2_r1u3_r3u1_m_r0u1_r1u2_r3u3 );
	const __m128 m_r0u1_r1u3_r3u2						= _mm_mul_ps( m_r0u1_r1u3, r3u2 );
	const __m128 a_m_r0u2_r1u1_r3u3_m_r0u1_r1u3_r3u2	= _mm_madd_ps( m_r0u2_r1u1, r3u3, m_r0u1_r1u3_r3u2 );
	const __m128 neg_part_det3x3_r2						= _mm_madd_ps( m_r0u3_r1u2, r3u1, a_m_r0u2_r1u1_r3u3_m_r0u1_r1u3_r3u2 );
	const __m128 det3x3_r2 								= _mm_sub_ps( pos_part_det3x3_r2, neg_part_det3x3_r2 );

	const __m128 c_zero		= _mm_setzero_ps();
	const __m128 c_mask		= _mm_cmpeq_ps( c_zero, c_zero );
	const __m128 c_signmask	= _mm_castsi128_ps( _mm_slli_epi32( _mm_castps_si128( c_mask ), 31 ) );
	const __m128 c_znzn		= _mm_unpacklo_ps( c_zero, c_signmask );
	const __m128 c_nznz		= _mm_unpacklo_ps( c_signmask, c_zero );

	const __m128 cofactor_r0 = _mm_xor_ps( det3x3_r0, c_znzn );
	const __m128 cofactor_r1 = _mm_xor_ps( det3x3_r1, c_nznz );
	const __m128 cofactor_r2 = _mm_xor_ps( det3x3_r2, c_znzn );
	const __m128 cofactor_r3 = _mm_xor_ps( det3x3_r3, c_nznz );

	const __m128 dot0	= _mm_mul_ps( r0, cofactor_r0 );
	const __m128 dot1 	= _mm_add_ps( dot0, _mm_perm_ps( dot0, _MM_SHUFFLE( 2, 1, 0, 3 ) ) );
	const __m128 det	= _mm_add_ps( dot1, _mm_perm_ps( dot1, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	const __m128 absDet	= _mm_andnot_ps( c_signmask, det );
	if ( _mm_movemask_ps( _mm_cmplt_ps( absDet, vector_float_inverse_epsilon ) ) & 15 ) {
		return false;
	}

	const __m128 rcpDet	= _mm_rcp32_ps( det );

	const __m128 hi_part_r0_r2 = _mm_unpacklo_ps( cofactor_r0, cofactor_r2 );
	const __m128 lo_part_r0_r2 = _mm_unpackhi_ps( cofactor_r0, cofactor_r2 );
	const __m128 hi_part_r1_r3 = _mm_unpacklo_ps( cofactor_r1, cofactor_r3 );
	const __m128 lo_part_r1_r3 = _mm_unpackhi_ps( cofactor_r1, cofactor_r3 );

	const __m128 adjoint_r0    = _mm_unpacklo_ps( hi_part_r0_r2, hi_part_r1_r3 );
	const __m128 adjoint_r1    = _mm_unpackhi_ps( hi_part_r0_r2, hi_part_r1_r3 );
	const __m128 adjoint_r2    = _mm_unpacklo_ps( lo_part_r0_r2, lo_part_r1_r3 );
	const __m128 adjoint_r3    = _mm_unpackhi_ps( lo_part_r0_r2, lo_part_r1_r3 );

	_mm_storeu_ps( out.m + 0 * 4, _mm_mul_ps( adjoint_r0, rcpDet ) );
	_mm_storeu_ps( out.m + 1 * 4, _mm_mul_ps( adjoint_r1, rcpDet ) );
	_mm_storeu_ps( out.m + 2 * 4, _mm_mul_ps( adjoint_r2, rcpDet ) );
	_mm_storeu_ps( out.m + 3 * 4, _mm_mul_ps( adjoint_r3, rcpDet ) );
#else
	const int FRL = 4;

	// 84+4+16 = 104 multiplications
	//			   1 division

	// 2x2 sub-determinants required to calculate 4x4 determinant
	const float det2_01_01 = src.m[0*FRL+0] * src.m[1*FRL+1] - src.m[0*FRL+1] * src.m[1*FRL+0];
	const float det2_01_02 = src.m[0*FRL+0] * src.m[1*FRL+2] - src.m[0*FRL+2] * src.m[1*FRL+0];
	const float det2_01_03 = src.m[0*FRL+0] * src.m[1*FRL+3] - src.m[0*FRL+3] * src.m[1*FRL+0];
	const float det2_01_12 = src.m[0*FRL+1] * src.m[1*FRL+2] - src.m[0*FRL+2] * src.m[1*FRL+1];
	const float det2_01_13 = src.m[0*FRL+1] * src.m[1*FRL+3] - src.m[0*FRL+3] * src.m[1*FRL+1];
	const float det2_01_23 = src.m[0*FRL+2] * src.m[1*FRL+3] - src.m[0*FRL+3] * src.m[1*FRL+2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	const float det3_201_012 = src.m[2*FRL+0] * det2_01_12 - src.m[2*FRL+1] * det2_01_02 + src.m[2*FRL+2] * det2_01_01;
	const float det3_201_013 = src.m[2*FRL+0] * det2_01_13 - src.m[2*FRL+1] * det2_01_03 + src.m[2*FRL+3] * det2_01_01;
	const float det3_201_023 = src.m[2*FRL+0] * det2_01_23 - src.m[2*FRL+2] * det2_01_03 + src.m[2*FRL+3] * det2_01_02;
	const float det3_201_123 = src.m[2*FRL+1] * det2_01_23 - src.m[2*FRL+2] * det2_01_13 + src.m[2*FRL+3] * det2_01_12;

	const float det = ( - det3_201_123 * src.m[3*FRL+0] + det3_201_023 * src.m[3*FRL+1] - det3_201_013 * src.m[3*FRL+2] + det3_201_012 * src.m[3*FRL+3] );

	if ( anMath::Fabs( det ) < RENDER_MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	const float rcpDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	const float det2_03_01 = src.m[0*FRL+0] * src.m[3*FRL+1] - src.m[0*FRL+1] * src.m[3*FRL+0];
	const float det2_03_02 = src.m[0*FRL+0] * src.m[3*FRL+2] - src.m[0*FRL+2] * src.m[3*FRL+0];
	const float det2_03_03 = src.m[0*FRL+0] * src.m[3*FRL+3] - src.m[0*FRL+3] * src.m[3*FRL+0];
	const float det2_03_12 = src.m[0*FRL+1] * src.m[3*FRL+2] - src.m[0*FRL+2] * src.m[3*FRL+1];
	const float det2_03_13 = src.m[0*FRL+1] * src.m[3*FRL+3] - src.m[0*FRL+3] * src.m[3*FRL+1];
	const float det2_03_23 = src.m[0*FRL+2] * src.m[3*FRL+3] - src.m[0*FRL+3] * src.m[3*FRL+2];

	const float det2_13_01 = src.m[1*FRL+0] * src.m[3*FRL+1] - src.m[1*FRL+1] * src.m[3*FRL+0];
	const float det2_13_02 = src.m[1*FRL+0] * src.m[3*FRL+2] - src.m[1*FRL+2] * src.m[3*FRL+0];
	const float det2_13_03 = src.m[1*FRL+0] * src.m[3*FRL+3] - src.m[1*FRL+3] * src.m[3*FRL+0];
	const float det2_13_12 = src.m[1*FRL+1] * src.m[3*FRL+2] - src.m[1*FRL+2] * src.m[3*FRL+1];
	const float det2_13_13 = src.m[1*FRL+1] * src.m[3*FRL+3] - src.m[1*FRL+3] * src.m[3*FRL+1];
	const float det2_13_23 = src.m[1*FRL+2] * src.m[3*FRL+3] - src.m[1*FRL+3] * src.m[3*FRL+2];

	// remaining 3x3 sub-determinants
	const float det3_203_012 = src.m[2*FRL+0] * det2_03_12 - src.m[2*FRL+1] * det2_03_02 + src.m[2*FRL+2] * det2_03_01;
	const float det3_203_013 = src.m[2*FRL+0] * det2_03_13 - src.m[2*FRL+1] * det2_03_03 + src.m[2*FRL+3] * det2_03_01;
	const float det3_203_023 = src.m[2*FRL+0] * det2_03_23 - src.m[2*FRL+2] * det2_03_03 + src.m[2*FRL+3] * det2_03_02;
	const float det3_203_123 = src.m[2*FRL+1] * det2_03_23 - src.m[2*FRL+2] * det2_03_13 + src.m[2*FRL+3] * det2_03_12;

	const float det3_213_012 = src.m[2*FRL+0] * det2_13_12 - src.m[2*FRL+1] * det2_13_02 + src.m[2*FRL+2] * det2_13_01;
	const float det3_213_013 = src.m[2*FRL+0] * det2_13_13 - src.m[2*FRL+1] * det2_13_03 + src.m[2*FRL+3] * det2_13_01;
	const float det3_213_023 = src.m[2*FRL+0] * det2_13_23 - src.m[2*FRL+2] * det2_13_03 + src.m[2*FRL+3] * det2_13_02;
	const float det3_213_123 = src.m[2*FRL+1] * det2_13_23 - src.m[2*FRL+2] * det2_13_13 + src.m[2*FRL+3] * det2_13_12;

	const float det3_301_012 = src.m[3*FRL+0] * det2_01_12 - src.m[3*FRL+1] * det2_01_02 + src.m[3*FRL+2] * det2_01_01;
	const float det3_301_013 = src.m[3*FRL+0] * det2_01_13 - src.m[3*FRL+1] * det2_01_03 + src.m[3*FRL+3] * det2_01_01;
	const float det3_301_023 = src.m[3*FRL+0] * det2_01_23 - src.m[3*FRL+2] * det2_01_03 + src.m[3*FRL+3] * det2_01_02;
	const float det3_301_123 = src.m[3*FRL+1] * det2_01_23 - src.m[3*FRL+2] * det2_01_13 + src.m[3*FRL+3] * det2_01_12;

	out.m[0*FRL+0] = - det3_213_123 * rcpDet;
	out.m[1*FRL+0] = + det3_213_023 * rcpDet;
	out.m[2*FRL+0] = - det3_213_013 * rcpDet;
	out.m[3*FRL+0] = + det3_213_012 * rcpDet;

	out.m[0*FRL+1] = + det3_203_123 * rcpDet;
	out.m[1*FRL+1] = - det3_203_023 * rcpDet;
	out.m[2*FRL+1] = + det3_203_013 * rcpDet;
	out.m[3*FRL+1] = - det3_203_012 * rcpDet;

	out.m[0*FRL+2] = + det3_301_123 * rcpDet;
	out.m[1*FRL+2] = - det3_301_023 * rcpDet;
	out.m[2*FRL+2] = + det3_301_013 * rcpDet;
	out.m[3*FRL+2] = - det3_301_012 * rcpDet;

	out.m[0*FRL+3] = - det3_201_123 * rcpDet;
	out.m[1*FRL+3] = + det3_201_023 * rcpDet;
	out.m[2*FRL+3] = - det3_201_013 * rcpDet;
	out.m[3*FRL+3] = + det3_201_012 * rcpDet;
#endif
	return true;
}

/*
========================
anGLMatrix::InverseByTranspose
========================
*/
void anGLMatrix::InverseByTranspose( const anGLMatrix & src, anGLMatrix & out ) {
	assert( &src != &out );
	assert( src.IsAffineTransform( 0.01f ) );

	out[0][0] = src[0][0];
	out[1][0] = src[0][1];
	out[2][0] = src[0][2];
	out[3][0] = 0.0f;
	out[0][1] = src[1][0];
	out[1][1] = src[1][1];
	out[2][1] = src[1][2];
	out[3][1] = 0.0f;
	out[0][2] = src[2][0];
	out[1][2] = src[2][1];
	out[2][2] = src[2][2];
	out[3][2] = 0.0f;
	out[0][3] = -( src[0][0] * src[0][3] + src[1][0] * src[1][3] + src[2][0] * src[2][3] );
	out[1][3] = -( src[0][1] * src[0][3] + src[1][1] * src[1][3] + src[2][1] * src[2][3] );
	out[2][3] = -( src[0][2] * src[0][3] + src[1][2] * src[1][3] + src[2][2] * src[2][3] );
	out[3][3] = 1.0f;
}

/*
========================
anGLMatrix::InverseByDoubles

This should never be used at run-time.
This is only for tools where more precision is needed.
========================
*/
bool anGLMatrix::InverseByDoubles( const anGLMatrix & src, anGLMatrix & out ) {
	const int FRL = 4;

	// 84+4+16 = 104 multiplications
	//			   1 division

	// 2x2 sub-determinants required to calculate 4x4 determinant
	const double det2_01_01 = (double)src.m[0*FRL+0] * (double)src.m[1*FRL+1] - (double)src.m[0*FRL+1] * (double)src.m[1*FRL+0];
	const double det2_01_02 = (double)src.m[0*FRL+0] * (double)src.m[1*FRL+2] - (double)src.m[0*FRL+2] * (double)src.m[1*FRL+0];
	const double det2_01_03 = (double)src.m[0*FRL+0] * (double)src.m[1*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[1*FRL+0];
	const double det2_01_12 = (double)src.m[0*FRL+1] * (double)src.m[1*FRL+2] - (double)src.m[0*FRL+2] * (double)src.m[1*FRL+1];
	const double det2_01_13 = (double)src.m[0*FRL+1] * (double)src.m[1*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[1*FRL+1];
	const double det2_01_23 = (double)src.m[0*FRL+2] * (double)src.m[1*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[1*FRL+2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	const double det3_201_012 = (double)src.m[2*FRL+0] * det2_01_12 - (double)src.m[2*FRL+1] * det2_01_02 + (double)src.m[2*FRL+2] * det2_01_01;
	const double det3_201_013 = (double)src.m[2*FRL+0] * det2_01_13 - (double)src.m[2*FRL+1] * det2_01_03 + (double)src.m[2*FRL+3] * det2_01_01;
	const double det3_201_023 = (double)src.m[2*FRL+0] * det2_01_23 - (double)src.m[2*FRL+2] * det2_01_03 + (double)src.m[2*FRL+3] * det2_01_02;
	const double det3_201_123 = (double)src.m[2*FRL+1] * det2_01_23 - (double)src.m[2*FRL+2] * det2_01_13 + (double)src.m[2*FRL+3] * det2_01_12;

	const double det = ( - det3_201_123 * (double)src.m[3*FRL+0] + det3_201_023 * (double)src.m[3*FRL+1] - det3_201_013 * (double)src.m[3*FRL+2] + det3_201_012 * (double)src.m[3*FRL+3] );

	const double rcpDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	const double det2_03_01 = (double)src.m[0*FRL+0] * (double)src.m[3*FRL+1] - (double)src.m[0*FRL+1] * (double)src.m[3*FRL+0];
	const double det2_03_02 = (double)src.m[0*FRL+0] * (double)src.m[3*FRL+2] - (double)src.m[0*FRL+2] * (double)src.m[3*FRL+0];
	const double det2_03_03 = (double)src.m[0*FRL+0] * (double)src.m[3*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[3*FRL+0];
	const double det2_03_12 = (double)src.m[0*FRL+1] * (double)src.m[3*FRL+2] - (double)src.m[0*FRL+2] * (double)src.m[3*FRL+1];
	const double det2_03_13 = (double)src.m[0*FRL+1] * (double)src.m[3*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[3*FRL+1];
	const double det2_03_23 = (double)src.m[0*FRL+2] * (double)src.m[3*FRL+3] - (double)src.m[0*FRL+3] * (double)src.m[3*FRL+2];

	const double det2_13_01 = (double)src.m[1*FRL+0] * (double)src.m[3*FRL+1] - (double)src.m[1*FRL+1] * (double)src.m[3*FRL+0];
	const double det2_13_02 = (double)src.m[1*FRL+0] * (double)src.m[3*FRL+2] - (double)src.m[1*FRL+2] * (double)src.m[3*FRL+0];
	const double det2_13_03 = (double)src.m[1*FRL+0] * (double)src.m[3*FRL+3] - (double)src.m[1*FRL+3] * (double)src.m[3*FRL+0];
	const double det2_13_12 = (double)src.m[1*FRL+1] * (double)src.m[3*FRL+2] - (double)src.m[1*FRL+2] * (double)src.m[3*FRL+1];
	const double det2_13_13 = (double)src.m[1*FRL+1] * (double)src.m[3*FRL+3] - (double)src.m[1*FRL+3] * (double)src.m[3*FRL+1];
	const double det2_13_23 = (double)src.m[1*FRL+2] * (double)src.m[3*FRL+3] - (double)src.m[1*FRL+3] * (double)src.m[3*FRL+2];

	// remaining 3x3 sub-determinants
	const double det3_203_012 = (double)src.m[2*FRL+0] * det2_03_12 - (double)src.m[2*FRL+1] * det2_03_02 + (double)src.m[2*FRL+2] * det2_03_01;
	const double det3_203_013 = (double)src.m[2*FRL+0] * det2_03_13 - (double)src.m[2*FRL+1] * det2_03_03 + (double)src.m[2*FRL+3] * det2_03_01;
	const double det3_203_023 = (double)src.m[2*FRL+0] * det2_03_23 - (double)src.m[2*FRL+2] * det2_03_03 + (double)src.m[2*FRL+3] * det2_03_02;
	const double det3_203_123 = (double)src.m[2*FRL+1] * det2_03_23 - (double)src.m[2*FRL+2] * det2_03_13 + (double)src.m[2*FRL+3] * det2_03_12;

	const double det3_213_012 = (double)src.m[2*FRL+0] * det2_13_12 - (double)src.m[2*FRL+1] * det2_13_02 + (double)src.m[2*FRL+2] * det2_13_01;
	const double det3_213_013 = (double)src.m[2*FRL+0] * det2_13_13 - (double)src.m[2*FRL+1] * det2_13_03 + (double)src.m[2*FRL+3] * det2_13_01;
	const double det3_213_023 = (double)src.m[2*FRL+0] * det2_13_23 - (double)src.m[2*FRL+2] * det2_13_03 + (double)src.m[2*FRL+3] * det2_13_02;
	const double det3_213_123 = (double)src.m[2*FRL+1] * det2_13_23 - (double)src.m[2*FRL+2] * det2_13_13 + (double)src.m[2*FRL+3] * det2_13_12;

	const double det3_301_012 = (double)src.m[3*FRL+0] * det2_01_12 - (double)src.m[3*FRL+1] * det2_01_02 + (double)src.m[3*FRL+2] * det2_01_01;
	const double det3_301_013 = (double)src.m[3*FRL+0] * det2_01_13 - (double)src.m[3*FRL+1] * det2_01_03 + (double)src.m[3*FRL+3] * det2_01_01;
	const double det3_301_023 = (double)src.m[3*FRL+0] * det2_01_23 - (double)src.m[3*FRL+2] * det2_01_03 + (double)src.m[3*FRL+3] * det2_01_02;
	const double det3_301_123 = (double)src.m[3*FRL+1] * det2_01_23 - (double)src.m[3*FRL+2] * det2_01_13 + (double)src.m[3*FRL+3] * det2_01_12;

	out.m[0*FRL+0] = ( float )( - det3_213_123 * rcpDet );
	out.m[1*FRL+0] = ( float )( + det3_213_023 * rcpDet );
	out.m[2*FRL+0] = ( float )( - det3_213_013 * rcpDet );
	out.m[3*FRL+0] = ( float )( + det3_213_012 * rcpDet );

	out.m[0*FRL+1] = ( float )( + det3_203_123 * rcpDet );
	out.m[1*FRL+1] = ( float )( - det3_203_023 * rcpDet );
	out.m[2*FRL+1] = ( float )( + det3_203_013 * rcpDet );
	out.m[3*FRL+1] = ( float )( - det3_203_012 * rcpDet );

	out.m[0*FRL+2] = ( float )( + det3_301_123 * rcpDet );
	out.m[1*FRL+2] = ( float )( - det3_301_023 * rcpDet );
	out.m[2*FRL+2] = ( float )( + det3_301_013 * rcpDet );
	out.m[3*FRL+2] = ( float )( - det3_301_012 * rcpDet );

	out.m[0*FRL+3] = ( float )( - det3_201_123 * rcpDet );
	out.m[1*FRL+3] = ( float )( + det3_201_023 * rcpDet );
	out.m[2*FRL+3] = ( float )( - det3_201_013 * rcpDet );
	out.m[3*FRL+3] = ( float )( + det3_201_012 * rcpDet );

	return true;
}

/*
========================
DeterminantIsNegative
========================
*/
#ifdef ARC_WIN_X86_SSE2_INTRIN
void DeterminantIsNegative( bool & negativeDeterminant, const __m128 & r0, const __m128 & r1, const __m128 & r2, const __m128 & r3 ) {
	const __m128 r1u1 = _mm_perm_ps( r1, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r1u2 = _mm_perm_ps( r1, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r1u3 = _mm_perm_ps( r1, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 r2u2 = _mm_perm_ps( r2, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r2u3 = _mm_perm_ps( r2, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 r3u1 = _mm_perm_ps( r3, _MM_SHUFFLE( 2, 1, 0, 3 ) );
	const __m128 r3u2 = _mm_perm_ps( r3, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 r3u3 = _mm_perm_ps( r3, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 m_r2u2_r3u3      						= _mm_mul_ps( r2u2, r3u3 );
	const __m128 m_r1u1_r2u2_r3u3 						= _mm_mul_ps( r1u1, m_r2u2_r3u3 );
	const __m128 m_r2u3_r3u1							= _mm_mul_ps( r2u3, r3u1 );
	const __m128 a_m_r1u2_r2u3_r3u1_m_r1u1_r2u2_r3u3	= _mm_madd_ps( r1u2, m_r2u3_r3u1, m_r1u1_r2u2_r3u3 );
	const __m128 m_r2u1_r3u2							= _mm_perm_ps( m_r2u2_r3u3, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 pos_part_det3x3_r0						= _mm_madd_ps( r1u3, m_r2u1_r3u2, a_m_r1u2_r2u3_r3u1_m_r1u1_r2u2_r3u3 );
	const __m128 m_r2u3_r3u2							= _mm_mul_ps( r2u3, r3u2 );
	const __m128 m_r1u1_r2u3_r3u2						= _mm_mul_ps( r1u1, m_r2u3_r3u2 );
	const __m128 m_r2u1_r3u3							= _mm_perm_ps( m_r2u3_r3u1, _MM_SHUFFLE( 1, 0, 3, 2 ) );
	const __m128 a_m_r1u2_r2u1_r3u3_m_r1u1_r2u3_r3u2	= _mm_madd_ps( r1u2, m_r2u1_r3u3, m_r1u1_r2u3_r3u2 );
	const __m128 m_r2u2_r3u1							= _mm_perm_ps( m_r2u3_r3u2, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 neg_part_det3x3_r0						= _mm_madd_ps( r1u3, m_r2u2_r3u1, a_m_r1u2_r2u1_r3u3_m_r1u1_r2u3_r3u2 );
	const __m128 det3x3_r0 								= _mm_sub_ps( pos_part_det3x3_r0, neg_part_det3x3_r0 );

	const __m128 c_zero		= _mm_setzero_ps();
	const __m128 c_mask		= _mm_cmpeq_ps( c_zero, c_zero );
	const __m128 c_signmask	= _mm_castsi128_ps( _mm_slli_epi32( _mm_castps_si128( c_mask ), 31 ) );
	const __m128 c_znzn		= _mm_unpacklo_ps( c_zero, c_signmask );

	const __m128 cofactor_r0 = _mm_xor_ps( det3x3_r0, c_znzn );

	const __m128 dot0	= _mm_mul_ps( r0, cofactor_r0 );
	const __m128 dot1 	= _mm_add_ps( dot0, _mm_perm_ps( dot0, _MM_SHUFFLE( 2, 1, 0, 3 ) ) );
	const __m128 det	= _mm_add_ps( dot1, _mm_perm_ps( dot1, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	const __m128 result	= _mm_cmpgt_ps( c_zero, det );

	negativeDeterminant	= _mm_movemask_ps( result ) & 1;
}
#else

void DeterminantIsNegative( bool & negativeDeterminant, const float * row0, const float * row1, const float * row2, const float * row3 ) {
	// 2x2 sub-determinants required to calculate 4x4 determinant
	const float det2_01_01 = row0[0] * row1[1] - row0[1] * row1[0];
	const float det2_01_02 = row0[0] * row1[2] - row0[2] * row1[0];
	const float det2_01_03 = row0[0] * row1[3] - row0[3] * row1[0];
	const float det2_01_12 = row0[1] * row1[2] - row0[2] * row1[1];
	const float det2_01_13 = row0[1] * row1[3] - row0[3] * row1[1];
	const float det2_01_23 = row0[2] * row1[3] - row0[3] * row1[2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	const float det3_201_012 = row2[0] * det2_01_12 - row2[1] * det2_01_02 + row2[2] * det2_01_01;
	const float det3_201_013 = row2[0] * det2_01_13 - row2[1] * det2_01_03 + row2[3] * det2_01_01;
	const float det3_201_023 = row2[0] * det2_01_23 - row2[2] * det2_01_03 + row2[3] * det2_01_02;
	const float det3_201_123 = row2[1] * det2_01_23 - row2[2] * det2_01_13 + row2[3] * det2_01_12;

	const float det = ( - det3_201_123 * row3[0] + det3_201_023 * row3[1] - det3_201_013 * row3[2] + det3_201_012 * row3[3] );

	negativeDeterminant = ( det < 0.0f );
}
#endif

/*
========================
anGLMatrix::CopyMatrix
========================
*/
void anGLMatrix::CopyMatrix( const anGLMatrix & matrix, anVec4 & row0, anVec4 & row1, anVec4 & row2, anVec4 & row3 ) {
	assert_16_byte_aligned( row0.ToFloatPtr() );
	assert_16_byte_aligned( row1.ToFloatPtr() );
	assert_16_byte_aligned( row2.ToFloatPtr() );
	assert_16_byte_aligned( row3.ToFloatPtr() );

#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 r0 = _mm_loadu_ps( matrix.m + 0 * 4 );
	const __m128 r1 = _mm_loadu_ps( matrix.m + 1 * 4 );
	const __m128 r2 = _mm_loadu_ps( matrix.m + 2 * 4 );
	const __m128 r3 = _mm_loadu_ps( matrix.m + 3 * 4 );

	_mm_store_ps( row0.ToFloatPtr(), r0 );
	_mm_store_ps( row1.ToFloatPtr(), r1 );
	_mm_store_ps( row2.ToFloatPtr(), r2 );
	_mm_store_ps( row3.ToFloatPtr(), r3 );
#else
	memcpy( row0.ToFloatPtr(), matrix[0], sizeof( anVec4 ) );
	memcpy( row1.ToFloatPtr(), matrix[1], sizeof( anVec4 ) );
	memcpy( row2.ToFloatPtr(), matrix[2], sizeof( anVec4 ) );
	memcpy( row3.ToFloatPtr(), matrix[3], sizeof( anVec4 ) );
#endif
}

#if 0
/*
========================
LocalViewOriginFromMVP
========================
*/
static anVec3 LocalViewOriginFromMVP( const anGLMatrix & mvp ) {
	const float nearX = mvp[3][0] + mvp[2][0];
	const float nearY = mvp[3][1] + mvp[2][1];
	const float nearZ = mvp[3][2] + mvp[2][2];
	const float s = anMath::InvSqrt( nearX * nearX + nearY * nearY + nearZ * nearZ );

	anGLMatrix inverseMVP;
	anGLMatrix::Inverse( mvp, inverseMVP );
	const float invW = 1.0f / inverseMVP[3][3];
	const float x = ( inverseMVP[0][3] - nearX * s ) * invW;
	const float y = ( inverseMVP[1][3] - nearY * s ) * invW;
	const float z = ( inverseMVP[2][3] - nearZ * s ) * invW;

	return anVec3( x, y, z );
}

#endif

/*
========================
LocalNearClipCenterFromMVP

Based on whether the depth range is [0,1] or [-1,1], either transform (0,0,0) or (0,0,-1) with the inverse MVP.
========================
*/
static anVec3 LocalNearClipCenterFromMVP( const anGLMatrix & mvp ) {
	anGLMatrix inverseMVP;
	anGLMatrix::Inverse( mvp, inverseMVP );
#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
	const float x = inverseMVP[0][3];
	const float y = inverseMVP[1][3];
	const float z = inverseMVP[2][3];
	const float w = inverseMVP[3][3];
#else
	const float x = inverseMVP[0][3] - inverseMVP[0][2];
	const float y = inverseMVP[1][3] - inverseMVP[1][2];
	const float z = inverseMVP[2][3] - inverseMVP[2][2];
	const float w = inverseMVP[3][3] - inverseMVP[3][2];
#endif
	const float invW = 1.0f / w;
	return anVec3( x * invW, y * invW, z * invW );
}

#ifdef ARC_WIN_X86_SSE2_INTRIN

/*
========================
ClipHomogeneousPolygonToSide

Clips a polygon with homogeneous coordinates to the axis aligned plane[axis] = sign * offset.
========================
*/
static void ClipHomogeneousPolygonToSide_SSE2( anVec4 *__restrict newPoints, anVec4 __restrict points, int &numPoints, const int axis, const __m128 &sign, const __m128 &offset ) {
	assert( newPoints != points );

	const __m128 side = _mm_mul_ps( sign, offset );
	__m128i mask = _mm_sub_epi32( vector_int_0123, _mm_shuffle_epi32( _mm_cvtsi32_si128( numPoints ), 0 ) );
	__m128i index = _mm_setzero_si128();

	ALIGNTYPE16 unsigned short indices[16 * 2];
	ALIGNTYPE16 float clipFractions[16];

	int localNumPoint = numPoints;

	for ( int i = 0; i < localNumPoint; i += 4 ) {
		const int i0 = ( i + 0 ) & ( ( i + 0 - localNumPoint ) >> 31 );
		const int i1 = ( i + 1 ) & ( ( i + 1 - localNumPoint ) >> 31 );
		const int i2 = ( i + 2 ) & ( ( i + 2 - localNumPoint ) >> 31 );
		const int i3 = ( i + 3 ) & ( ( i + 3 - localNumPoint ) >> 31 );
		const int i4 = ( i + 4 ) & ( ( i + 4 - localNumPoint ) >> 31 );

		const __m128 p0A = _mm_load_ss( &points[i0][axis] );
		const __m128 p1A = _mm_load_ss( &points[i1][axis] );
		const __m128 p2A = _mm_load_ss( &points[i2][axis] );
		const __m128 p3A = _mm_load_ss( &points[i3][axis] );
		const __m128 p4A = _mm_load_ss( &points[i4][axis] );

		const __m128 p0W = _mm_load_ss( &points[i0][3] );
		const __m128 p1W = _mm_load_ss( &points[i1][3] );
		const __m128 p2W = _mm_load_ss( &points[i2][3] );
		const __m128 p3W = _mm_load_ss( &points[i3][3] );
		const __m128 p4W = _mm_load_ss( &points[i4][3] );

		const __m128 t0 = _mm_unpacklo_ps( p0A, p2A );
		const __m128 t1 = _mm_unpacklo_ps( p1A, p3A );
		const __m128 pa0 = _mm_unpacklo_ps( t0, t1 );
		const __m128 pa1 = _mm_sld_ps( pa0, p4A, 4 );

		const __m128 r0 = _mm_unpacklo_ps( p0W, p2W );
		const __m128 r1 = _mm_unpacklo_ps( p1W, p3W );
		const __m128 pw0 = _mm_unpacklo_ps( r0, r1 );
		const __m128 pw1 = _mm_sld_ps( pw0, p4W, 4 );

		{
			const __m128 bside0 = _mm_cmpgt_ps( _mm_mul_ps( offset, pw0 ), _mm_mul_ps( sign, pa0 ) );
			const __m128 bside1 = _mm_cmpgt_ps( _mm_mul_ps( offset, pw1 ), _mm_mul_ps( sign, pa1 ) );
			const __m128i side0 = _mm_and_si128( __m128c( bside0 ), vector_int_1 );
			const __m128i side1 = _mm_and_si128( __m128c( bside1 ), vector_int_1 );
			const __m128i xorSide = _mm_xor_si128( side0, side1 );
			const __m128i interleavedSide0 = _mm_unpacklo_epi32( side0, xorSide );
			const __m128i interleavedSide1 = _mm_unpackhi_epi32( side0, xorSide );
			const __m128i packedSide = _mm_packs_epi32( interleavedSide0, interleavedSide1 );
			const __m128i packedMaskedSide = _mm_and_si128( packedSide, _mm_srai_epi32( mask, 31 ) );

			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide,  2 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide,  4 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide,  6 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide,  8 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide, 10 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide, 12 ) );
			index = _mm_add_epi16( index, _mm_slli_si128( packedMaskedSide, 14 ) );

			_mm_store_si128( (__m128i *)&indices[i * 2], index );

			mask = _mm_add_epi32( mask, vector_int_4 );
			index = _mm_add_epi16( index, packedMaskedSide );
			index = _mm_shufflehi_epi16( index, _MM_SHUFFLE( 3, 3, 3, 3 ) );
			index = _mm_shuffle_epi32( index, _MM_SHUFFLE( 3, 3, 3, 3 ) );
		}{
			const __m128 d0 = _mm_nmsub_ps( pw0, side, pa0 );
			const __m128 d1 = _mm_nmsub_ps( pw1, side, pa1 );
			const __m128 delta = _mm_sub_ps( d0, d1 );
			const __m128 deltaAbs = _mm_and_ps( delta, vector_float_abs_mask );
			const __m128 clamp = _mm_cmpgt_ps( vector_float_smallest_non_denorm, deltaAbs );
			const __m128 deltaClamped = _mm_sel_ps( delta, vector_float_one, clamp );
			const __m128 fraction = _mm_mul_ps( d0, _mm_rcp32_ps( deltaClamped ) );
			const __m128 fractionClamped0 = _mm_sel_ps( fraction, vector_float_one, clamp );
			const __m128 fractionClamped1 = _mm_max_ps( fractionClamped0, vector_float_zero );
			const __m128 fractionClamped2 = _mm_min_ps( fractionClamped1, vector_float_one );

			_mm_store_ps( &clipFractions[i], fractionClamped2 );
		}
	}

	numPoints = _mm_cvtsi128_si32( index ) & 0xFFFF;

	for ( int i = 0; i < localNumPoint; i += 4 ) {
		const int i0 = ( i + 0 ) & ( ( i + 0 - localNumPoint ) >> 31 );
		const int i1 = ( i + 1 ) & ( ( i + 1 - localNumPoint ) >> 31 );
		const int i2 = ( i + 2 ) & ( ( i + 2 - localNumPoint ) >> 31 );
		const int i3 = ( i + 3 ) & ( ( i + 3 - localNumPoint ) >> 31 );
		const int i4 = ( i + 4 ) & ( ( i + 4 - localNumPoint ) >> 31 );

		const __m128 p0 = _mm_load_ps( points[i0].ToFloatPtr() );
		const __m128 p1 = _mm_load_ps( points[i1].ToFloatPtr() );
		const __m128 p2 = _mm_load_ps( points[i2].ToFloatPtr() );
		const __m128 p3 = _mm_load_ps( points[i3].ToFloatPtr() );
		const __m128 p4 = _mm_load_ps( points[i4].ToFloatPtr() );

		const __m128 fraction = _mm_load_ps( &clipFractions[i] );

		const __m128 c0 = _mm_madd_ps( _mm_splat_ps( fraction, 0 ), _mm_sub_ps( p1, p0 ), p0 );
		const __m128 c1 = _mm_madd_ps( _mm_splat_ps( fraction, 1 ), _mm_sub_ps( p2, p1 ), p1 );
		const __m128 c2 = _mm_madd_ps( _mm_splat_ps( fraction, 2 ), _mm_sub_ps( p3, p2 ), p2 );
		const __m128 c3 = _mm_madd_ps( _mm_splat_ps( fraction, 3 ), _mm_sub_ps( p4, p3 ), p3 );

		_mm_store_ps( newPoints[indices[i * 2 + 0]].ToFloatPtr(), p0 );
		_mm_store_ps( newPoints[indices[i * 2 + 1]].ToFloatPtr(), c0 );
		_mm_store_ps( newPoints[indices[i * 2 + 2]].ToFloatPtr(), p1 );
		_mm_store_ps( newPoints[indices[i * 2 + 3]].ToFloatPtr(), c1 );
		_mm_store_ps( newPoints[indices[i * 2 + 4]].ToFloatPtr(), p2 );
		_mm_store_ps( newPoints[indices[i * 2 + 5]].ToFloatPtr(), c2 );
		_mm_store_ps( newPoints[indices[i * 2 + 6]].ToFloatPtr(), p3 );
		_mm_store_ps( newPoints[indices[i * 2 + 7]].ToFloatPtr(), c3 );
	}
}

/*
========================
ClipHomogeneousPolygonToUnitCube

Clips a polygon with homogeneous coordinates to all six axis aligned unit cube planes.
========================
*/
static int ClipHomogeneousPolygonToUnitCube_SSE2( anVec4 * points, int numPoints ) {
	assert( numPoints < 16 - 6 );
	ALIGNTYPE16 anVec4 newPoints[16 * 2];

#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
	ClipHomogeneousPolygonToSide_SSE2( newPoints, points, numPoints, 2, vector_float_neg_one, vector_float_zero );	// near
#else
	ClipHomogeneousPolygonToSide_SSE2( newPoints, points, numPoints, 2, vector_float_neg_one, vector_float_one );	// near
#endif
	ClipHomogeneousPolygonToSide_SSE2( points, newPoints, numPoints, 2, vector_float_pos_one, vector_float_one );	// far
	ClipHomogeneousPolygonToSide_SSE2( newPoints, points, numPoints, 1, vector_float_neg_one, vector_float_one );	// bottom
	ClipHomogeneousPolygonToSide_SSE2( points, newPoints, numPoints, 1, vector_float_pos_one, vector_float_one );	// top
	ClipHomogeneousPolygonToSide_SSE2( newPoints, points, numPoints, 0, vector_float_neg_one, vector_float_one );	// left
	ClipHomogeneousPolygonToSide_SSE2( points, newPoints, numPoints, 0, vector_float_pos_one, vector_float_one );	// right
	return numPoints;
}
#else

/*
========================
ClipHomogeneousLineToSide

Clips a line with homogeneous coordinates to the axis aligned plane[axis] = side.
========================
*/
static anVec4 ClipHomogeneousLineToSide( const anVec4 & p0, const anVec4 & p1, int axis, float side ) {
	const float d0 = p0.w * side - p0[axis];
	const float d1 = p1.w * side - p1[axis];
	const float delta = d0 - d1;
	const float f = anMath::Fabs( delta ) > anMath::FLT_SMALLEST_NON_DENORMAL ? ( d0 / delta ) : 1.0f;
	const float c = anMath::ClampFloat( 0.0f, 1.0f, f );
	return p0 + c * ( p1 - p0 );
}

/*
========================
ClipHomogeneousPolygonToSide

Clips a polygon with homogeneous coordinates to the axis aligned plane[axis] = sign * offset.
========================
*/
static int ClipHomogeneousPolygonToSide_Generic( anVec4 * __restrict newPoints, anVec4 * __restrict points, int numPoints, int axis, float sign, float offset ) {
	assert( newPoints != points );
	assert( numPoints < 16 );
	int sides[16];

	const float side = sign * offset;

	// calculate the plane side for each original point and calculate all potential new points
	for ( int i = 0; i < numPoints; i++ ) {
		int j = ( i + 1 ) & ( ( i + 1 - numPoints ) >> 31 );
		sides[i] = sign * points[i][axis] < offset * points[i].w;
		newPoints[i * 2 + 0] = points[i];
		newPoints[i * 2 + 1] = ClipHomogeneousLineToSide( points[i], points[j], axis, side );
	};

	// repeat the first side at the end to avoid having to wrap around
	sides[numPoints] = sides[0];

	// compact the array of points
	int numNewPoints = 0;
	for ( int i = 0; i < numPoints; i++ ) {
		if ( sides[i + 0] != 0 ) {
			newPoints[numNewPoints++] = newPoints[i * 2 + 0];
		}
		if ( ( sides[i + 0] ^ sides[i + 1] ) != 0 ) {
			newPoints[numNewPoints++] = newPoints[i * 2 + 1];
		}
	}

	assert( numNewPoints <= 16 );
	return numNewPoints;
}

/*
========================
ClipHomogeneousPolygonToUnitCube

Clips a polygon with homogeneous coordinates to all six axis aligned unit cube planes.
========================
*/
static int ClipHomogeneousPolygonToUnitCube_Generic( anVec4 * points, int numPoints ) {
	assert( numPoints < 16 - 6 );
	ALIGNTYPE16 anVec4 newPoints[2 * 16];	// the C clip code temporarily doubles the points

#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
	numPoints = ClipHomogeneousPolygonToSide_Generic( newPoints, points, numPoints, 2, -1.0f, 0.0f );	// near
#else
	numPoints = ClipHomogeneousPolygonToSide_Generic( newPoints, points, numPoints, 2, -1.0f, 1.0f );	// near
#endif
	numPoints = ClipHomogeneousPolygonToSide_Generic( points, newPoints, numPoints, 2, +1.0f, 1.0f );	// far
	numPoints = ClipHomogeneousPolygonToSide_Generic( newPoints, points, numPoints, 1, -1.0f, 1.0f );	// bottom
	numPoints = ClipHomogeneousPolygonToSide_Generic( points, newPoints, numPoints, 1, +1.0f, 1.0f );	// top
	numPoints = ClipHomogeneousPolygonToSide_Generic( newPoints, points, numPoints, 0, -1.0f, 1.0f );	// left
	numPoints = ClipHomogeneousPolygonToSide_Generic( points, newPoints, numPoints, 0, +1.0f, 1.0f );	// right
	return numPoints;
}
#endif

/*
========================
anGLMatrix::ProjectedFullyClippedBounds

Calculates the bounds of the given bounding box projected with the given Model View Projection (MVP) matrix.
If 'windowSpace' is true then the calculated bounds along each axis are moved and clamped to the [0, 1] range.

The given bounding box is first fully clipped to the MVP to get the smallest projected bounds.

Note that this code assumes the MVP matrix has an infinite far clipping plane. When the far plane is at
infinity the bounds are never far clipped and it is sufficient to test whether or not the center of the
near clip plane is inside the bounds to calculate the correct minimum Z. If the far plane is not at
infinity then this code would also have to test for the view frustum being completely contained inside
the given bounds in which case the projected bounds should be set to fully cover the view frustum.
========================
*/
void anGLMatrix::ProjectedFullyClippedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 mvp0 = _mm_loadu_ps( mvp[0] );
	const __m128 mvp1 = _mm_loadu_ps( mvp[1] );
	const __m128 mvp2 = _mm_loadu_ps( mvp[2] );
	const __m128 mvp3 = _mm_loadu_ps( mvp[3] );

	const __m128 t0 = _mm_unpacklo_ps( mvp0, mvp2 );	// mvp[0][0], mvp[2][0], mvp[0][1], mvp[2][1]
	const __m128 t1 = _mm_unpackhi_ps( mvp0, mvp2 );	// mvp[0][2], mvp[2][2], mvp[0][3], mvp[2][3]
	const __m128 t2 = _mm_unpacklo_ps( mvp1, mvp3 );	// mvp[1][0], mvp[3][0], mvp[1][1], mvp[3][1]
	const __m128 t3 = _mm_unpackhi_ps( mvp1, mvp3 );	// mvp[1][2], mvp[3][2], mvp[1][3], mvp[3][3]

	const __m128 mvpX = _mm_unpacklo_ps( t0, t2 );		// mvp[0][0], mvp[1][0], mvp[2][0], mvp[3][0]
	const __m128 mvpY = _mm_unpackhi_ps( t0, t2 );		// mvp[0][1], mvp[1][1], mvp[2][1], mvp[3][1]
	const __m128 mvpZ = _mm_unpacklo_ps( t1, t3 );		// mvp[0][2], mvp[1][2], mvp[2][2], mvp[3][2]
	const __m128 mvpW = _mm_unpackhi_ps( t1, t3 );		// mvp[0][3], mvp[1][3], mvp[2][3], mvp[3][3]

	const __m128 b0 = _mm_loadu_bounds_0( bounds );
	const __m128 b1 = _mm_loadu_bounds_1( bounds );

	const __m128 b0X = _mm_splat_ps( b0, 0 );
	const __m128 b0Y = _mm_splat_ps( b0, 1 );
	const __m128 b0Z = _mm_splat_ps( b0, 2 );

	const __m128 b1X = _mm_splat_ps( b1, 0 );
	const __m128 b1Y = _mm_splat_ps( b1, 1 );
	const __m128 b1Z = _mm_splat_ps( b1, 2 );

	const __m128 p0 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 p1 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 p2 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 p3 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 p4 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 p5 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 p6 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 p7 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );

	ALIGNTYPE16 anVec4 projectedPoints[8];
	_mm_store_ps( projectedPoints[0].ToFloatPtr(), p0 );
	_mm_store_ps( projectedPoints[1].ToFloatPtr(), p1 );
	_mm_store_ps( projectedPoints[2].ToFloatPtr(), p2 );
	_mm_store_ps( projectedPoints[3].ToFloatPtr(), p3 );
	_mm_store_ps( projectedPoints[4].ToFloatPtr(), p4 );
	_mm_store_ps( projectedPoints[5].ToFloatPtr(), p5 );
	_mm_store_ps( projectedPoints[6].ToFloatPtr(), p6 );
	_mm_store_ps( projectedPoints[7].ToFloatPtr(), p7 );

	ALIGNTYPE16 anVec4 clippedPoints[6 * 16];
	int numClippedPoints = 0;
	for ( int i = 0; i < 6; i++ ) {
		_mm_store_ps( clippedPoints[numClippedPoints + 0].ToFloatPtr(), _mm_load_ps( projectedPoints[boxPolygonVertices[i][0]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 1].ToFloatPtr(), _mm_load_ps( projectedPoints[boxPolygonVertices[i][1]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 2].ToFloatPtr(), _mm_load_ps( projectedPoints[boxPolygonVertices[i][2]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 3].ToFloatPtr(), _mm_load_ps( projectedPoints[boxPolygonVertices[i][3]].ToFloatPtr() ) );
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_SSE2( &clippedPoints[numClippedPoints], 4 );
	}

	// repeat the first clipped point at the end to get a multiple of 4 clipped points
	const __m128 point0 = _mm_load_ps( clippedPoints[0].ToFloatPtr() );
	for ( int i = numClippedPoints; ( i & 3 ) != 0; i++ ) {
		_mm_store_ps( clippedPoints[i].ToFloatPtr(), point0 );
	}

	// test if the center of the near clip plane is inside the given bounding box
	const anVec3 localNearClipCenter = LocalNearClipCenterFromMVP( mvp );
	const bool inside = bounds.Expand( RENDER_MATRIX_PROJECTION_EPSILON ).ContainsPoint( localNearClipCenter );

	__m128 minX = vector_float_pos_infinity;
	__m128 minY = vector_float_pos_infinity;
	__m128 minZ = inside ? vector_float_neg_one : vector_float_pos_infinity;

	__m128 maxX = vector_float_neg_infinity;
	__m128 maxY = vector_float_neg_infinity;
	__m128 maxZ = vector_float_neg_infinity;

	for ( int i = 0; i < numClippedPoints; i += 4 ) {
		const __m128 cp0 = _mm_load_ps( clippedPoints[i + 0].ToFloatPtr() );
		const __m128 cp1 = _mm_load_ps( clippedPoints[i + 1].ToFloatPtr() );
		const __m128 cp2 = _mm_load_ps( clippedPoints[i + 2].ToFloatPtr() );
		const __m128 cp3 = _mm_load_ps( clippedPoints[i + 3].ToFloatPtr() );

		const __m128 s0 = _mm_unpacklo_ps( cp0, cp2 );		// cp0[0], cp2[0], cp0[1], cp2[1]
		const __m128 s1 = _mm_unpackhi_ps( cp0, cp2 );		// cp0[2], cp2[2], cp0[3], cp2[3]
		const __m128 s2 = _mm_unpacklo_ps( cp1, cp3 );		// cp1[0], cp3[0], cp1[1], cp3[1]
		const __m128 s3 = _mm_unpackhi_ps( cp1, cp3 );		// cp1[2], cp3[2], cp1[3], cp3[3]

		const __m128 cpX = _mm_unpacklo_ps( s0, s2 );		// cp0[0], cp1[0], cp2[0], cp3[0]
		const __m128 cpY = _mm_unpackhi_ps( s0, s2 );		// cp0[1], cp1[1], cp2[1], cp3[1]
		const __m128 cpZ = _mm_unpacklo_ps( s1, s3 );		// cp0[2], cp1[2], cp2[2], cp3[2]
		const __m128 cpW = _mm_unpackhi_ps( s1, s3 );		// cp0[3], cp1[3], cp2[3], cp3[3]

		const __m128 rW = _mm_rcp32_ps( cpW );
		const __m128 rX = _mm_mul_ps( cpX, rW );
		const __m128 rY = _mm_mul_ps( cpY, rW );
		const __m128 rZ = _mm_mul_ps( cpZ, rW );

		minX = _mm_min_ps( minX, rX );
		minY = _mm_min_ps( minY, rY );
		minZ = _mm_min_ps( minZ, rZ );

		maxX = _mm_max_ps( maxX, rX );
		maxY = _mm_max_ps( maxY, rY );
		maxZ = _mm_max_ps( maxZ, rZ );
	}

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	if ( windowSpace ) {
		minX = _mm_madd_ps( minX, vector_float_half, vector_float_half );
		maxX = _mm_madd_ps( maxX, vector_float_half, vector_float_half );

		minY = _mm_madd_ps( minY, vector_float_half, vector_float_half );
		maxY = _mm_madd_ps( maxY, vector_float_half, vector_float_half );

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minZ = _mm_madd_ps( minZ, vector_float_half, vector_float_half );
		maxZ = _mm_madd_ps( maxZ, vector_float_half, vector_float_half );
#endif

		minX = _mm_max_ps( _mm_min_ps( minX, vector_float_one ), vector_float_zero );
		maxX = _mm_max_ps( _mm_min_ps( maxX, vector_float_one ), vector_float_zero );

		minY = _mm_max_ps( _mm_min_ps( minY, vector_float_one ), vector_float_zero );
		maxY = _mm_max_ps( _mm_min_ps( maxY, vector_float_one ), vector_float_zero );

		minZ = _mm_max_ps( _mm_min_ps( minZ, vector_float_one ), vector_float_zero );
		maxZ = _mm_max_ps( _mm_min_ps( maxZ, vector_float_one ), vector_float_zero );
	}

	_mm_store_ss( & projected[0].x, minX );
	_mm_store_ss( & projected[0].y, minY );
	_mm_store_ss( & projected[0].z, minZ );

	_mm_store_ss( & projected[1].x, maxX );
	_mm_store_ss( & projected[1].y, maxY );
	_mm_store_ss( & projected[1].z, maxZ );
#else
	const anVec3 points[8] = {
		anVec3( bounds[0][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[1][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[1][2] )
	};

	anVec4 projectedPoints[8];
	for ( int i = 0; i < 8; i++ ) {
		const anVec3 & v = points[i];
		projectedPoints[i].x = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2] + mvp[0][3];
		projectedPoints[i].y = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2] + mvp[1][3];
		projectedPoints[i].z = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
		projectedPoints[i].w = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];
	}

	anVec4 clippedPoints[6 * 16];
	int numClippedPoints = 0;
	for ( int i = 0; i < 6; i++ ) {
		clippedPoints[numClippedPoints + 0] = projectedPoints[boxPolygonVertices[i][0]];
		clippedPoints[numClippedPoints + 1] = projectedPoints[boxPolygonVertices[i][1]];
		clippedPoints[numClippedPoints + 2] = projectedPoints[boxPolygonVertices[i][2]];
		clippedPoints[numClippedPoints + 3] = projectedPoints[boxPolygonVertices[i][3]];
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_Generic( &clippedPoints[numClippedPoints], 4 );
	}

	// test if the center of the near clip plane is inside the given bounding box
	const anVec3 localNearClipCenter = LocalNearClipCenterFromMVP( mvp );
	const bool inside = bounds.Expand( RENDER_MATRIX_PROJECTION_EPSILON ).ContainsPoint( localNearClipCenter );

	for ( int i = 0; i < 3; i++ ) {
		projected[0][i] = RENDER_MATRIX_INFINITY;
		projected[1][i] = - RENDER_MATRIX_INFINITY;
	}
	if ( inside ) {
		projected[0][2] = -1.0f;
	}

	for ( int i = 0; i < numClippedPoints; i++ ) {
		const anVec4 & c = clippedPoints[i];

		assert( c.w > anMath::FLT_SMALLEST_NON_DENORMAL );

		const float rw = 1.0f / c.w;

		const float px = c.x * rw;
		const float py = c.y * rw;
		const float pz = c.z * rw;

		projected[0][0] = Min( projected[0][0], px );
		projected[0][1] = Min( projected[0][1], py );
		projected[0][2] = Min( projected[0][2], pz );

		projected[1][0] = Max( projected[1][0], px );
		projected[1][1] = Max( projected[1][1], py );
		projected[1][2] = Max( projected[1][2], pz );
	}

	if ( windowSpace ) {
		// convert to window coords
		projected[0][0] = projected[0][0] * 0.5f + 0.5f;
		projected[1][0] = projected[1][0] * 0.5f + 0.5f;

		projected[0][1] = projected[0][1] * 0.5f + 0.5f;
		projected[1][1] = projected[1][1] * 0.5f + 0.5f;

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		projected[0][2] = projected[0][2] * 0.5f + 0.5f;
		projected[1][2] = projected[1][2] * 0.5f + 0.5f;
#endif

		// clamp to [0, 1] range
		projected[0][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][0] );
		projected[1][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][0] );

		projected[0][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][1] );
		projected[1][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][1] );

		projected[0][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][2] );
		projected[1][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][2] );
	}

#endif
}

/*
========================
PointInsideInfiniteShadow

Returns true if the 'localPoint' is inside the infinite shadow volume cast
from the given occluder bounding box and the given light position.
========================
*/
static bool PointInsideInfiniteShadow( const anBounds &occluderBounds, const anVec3 &localLightOrigin, const anVec3 &localPoint, const float epsilon ) {
	// Expand the bounds with an epsilon.
	const anBounds expandedBounds = occluderBounds.Expand( epsilon );

	// If the point is inside the bounding box then the point
	// is also inside the shadow projection.
	if ( expandedBounds.ContainsPoint( localPoint ) ) {
		return true;
	}

	// If the light is inside the bounding box then the shadow is projected
	// in all directions and any point is inside the infinte shadow projection.
	if ( expandedBounds.ContainsPoint( localPoint ) ) {
		return true;
	}

	// If the line from localLightOrigin to localPoint intersects the
	// bounding box then the point is inside the infinite shadow projection.
	if ( expandedBounds.LineIntersection( localLightOrigin, localPoint ) ) {
		return true;
	}

	// The point is definitely not inside the projected shadow.
	return false;
}

/*
========================
anGLMatrix::GetFrustumPlanes

Normally the clip space extends from -1.0 to 1.0 on each axis, but by setting 'zeroToOne'
to true, the clip space will extend from 0.0 to 1.0 on each axis for a light projection matrix.
========================
*/
void anGLMatrix::GetFrustumPlanes( anPlane planes[6], const anGLMatrix & frustum, bool zeroToOne, bool normalize ) {
	// FIXME:	need to know whether or not this is a D3D MVP.
	//			We cannot just assume that it's an D3D MVP matrix when
	//			zeroToOne = false and CLIP_SPACE_D3D is defined because
	//			this code may be called for non-MVP matrices.
	const bool isZeroOneZ = false;

	if ( zeroToOne ) {
		// left: inside(p) = p * frustum[0] > 0
		planes[0][0] = frustum[0][0];
		planes[0][1] = frustum[0][1];
		planes[0][2] = frustum[0][2];
		planes[0][3] = frustum[0][3];

		// bottom: inside(p) = p * frustum[1] > 0
		planes[2][0] = frustum[1][0];
		planes[2][1] = frustum[1][1];
		planes[2][2] = frustum[1][2];
		planes[2][3] = frustum[1][3];

		// near: inside(p) = p * frustum[2] > 0
		planes[4][0] = frustum[2][0];
		planes[4][1] = frustum[2][1];
		planes[4][2] = frustum[2][2];
		planes[4][3] = frustum[2][3];
	} else {
		// left: inside(p) = p * frustum[0] > - ( p * frustum[3] )
		planes[0][0] = frustum[3][0] + frustum[0][0];
		planes[0][1] = frustum[3][1] + frustum[0][1];
		planes[0][2] = frustum[3][2] + frustum[0][2];
		planes[0][3] = frustum[3][3] + frustum[0][3];

		// bottom: inside(p) = p * frustum[1] > -( p * frustum[3] )
		planes[2][0] = frustum[3][0] + frustum[1][0];
		planes[2][1] = frustum[3][1] + frustum[1][1];
		planes[2][2] = frustum[3][2] + frustum[1][2];
		planes[2][3] = frustum[3][3] + frustum[1][3];

		// near: inside(p) = p * frustum[2] > -( p * frustum[3] )
		planes[4][0] = isZeroOneZ ? ( frustum[2][0] ) : ( frustum[3][0] + frustum[2][0] );
		planes[4][1] = isZeroOneZ ? ( frustum[2][1] ) : ( frustum[3][1] + frustum[2][1] );
		planes[4][2] = isZeroOneZ ? ( frustum[2][2] ) : ( frustum[3][2] + frustum[2][2] );
		planes[4][3] = isZeroOneZ ? ( frustum[2][3] ) : ( frustum[3][3] + frustum[2][3] );
	}

	// right: inside(p) = p * frustum[0] < p * frustum[3]
	planes[1][0] = frustum[3][0] - frustum[0][0];
	planes[1][1] = frustum[3][1] - frustum[0][1];
	planes[1][2] = frustum[3][2] - frustum[0][2];
	planes[1][3] = frustum[3][3] - frustum[0][3];

	// top: inside(p) = p * frustum[1] < p * frustum[3]
	planes[3][0] = frustum[3][0] - frustum[1][0];
	planes[3][1] = frustum[3][1] - frustum[1][1];
	planes[3][2] = frustum[3][2] - frustum[1][2];
	planes[3][3] = frustum[3][3] - frustum[1][3];

	// far: inside(p) = p * frustum[2] < p * frustum[3]
	planes[5][0] = frustum[3][0] - frustum[2][0];
	planes[5][1] = frustum[3][1] - frustum[2][1];
	planes[5][2] = frustum[3][2] - frustum[2][2];
	planes[5][3] = frustum[3][3] - frustum[2][3];

	// optionally normalize the planes
	if ( normalize ) {
		for ( int i = 0; i < 6; i++ ) {
			float s = anMath::InvSqrt( planes[i].Normal().LengthSqr() );
			planes[i][0] *= s;
			planes[i][1] *= s;
			planes[i][2] *= s;
			planes[i][3] *= s;
		}
	}
}

/*
========================
anGLMatrix::GetFrustumCorners
========================
*/
void anGLMatrix::GetFrustumCorners( frustumCorners_t & corners, const anGLMatrix & frustumTransform, const anBounds & frustumBounds ) {
	assert_16_byte_aligned( &corners );

#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 mvp0 = _mm_loadu_ps( frustumTransform[0] );
	__m128 mvp1 = _mm_loadu_ps( frustumTransform[1] );
	__m128 mvp2 = _mm_loadu_ps( frustumTransform[2] );
	__m128 mvp3 = _mm_loadu_ps( frustumTransform[3] );

	__m128 b0 = _mm_loadu_bounds_0( frustumBounds );
	__m128 b1 = _mm_loadu_bounds_1( frustumBounds );

	// take the four points on the X-Y plane
	__m128 vxy = _mm_unpacklo_ps( b0, b1 );						// min X, max X, min Y, max Y
	__m128 vx = _mm_perm_ps( vxy, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// min X, max X, min X, max X
	__m128 vy = _mm_perm_ps( vxy, _MM_SHUFFLE( 3, 3, 2, 2 ) );	// min Y, min Y, max Y, max Y

	__m128 vz0 = _mm_splat_ps( b0, 2 );							// min Z, min Z, min Z, min Z
	__m128 vz1 = _mm_splat_ps( b1, 2 );							// max Z, max Z, max Z, max Z

	// compute four partial X,Y,Z,W values
	__m128 parx = _mm_splat_ps( mvp0, 3 );
	__m128 pary = _mm_splat_ps( mvp1, 3 );
	__m128 parz = _mm_splat_ps( mvp2, 3 );
	__m128 parw = _mm_splat_ps( mvp3, 3 );

	parx = _mm_madd_ps( vx, _mm_splat_ps( mvp0, 0 ), parx );
	pary = _mm_madd_ps( vx, _mm_splat_ps( mvp1, 0 ), pary );
	parz = _mm_madd_ps( vx, _mm_splat_ps( mvp2, 0 ), parz );
	parw = _mm_madd_ps( vx, _mm_splat_ps( mvp3, 0 ), parw );

	parx = _mm_madd_ps( vy, _mm_splat_ps( mvp0, 1 ), parx );
	pary = _mm_madd_ps( vy, _mm_splat_ps( mvp1, 1 ), pary );
	parz = _mm_madd_ps( vy, _mm_splat_ps( mvp2, 1 ), parz );
	parw = _mm_madd_ps( vy, _mm_splat_ps( mvp3, 1 ), parw );

	__m128 mvp0Z = _mm_splat_ps( mvp0, 2 );
	__m128 mvp1Z = _mm_splat_ps( mvp1, 2 );
	__m128 mvp2Z = _mm_splat_ps( mvp2, 2 );
	__m128 mvp3Z = _mm_splat_ps( mvp3, 2 );

	__m128 x0 = _mm_madd_ps( vz0, mvp0Z, parx );
	__m128 y0 = _mm_madd_ps( vz0, mvp1Z, pary );
	__m128 z0 = _mm_madd_ps( vz0, mvp2Z, parz );
	__m128 w0 = _mm_madd_ps( vz0, mvp3Z, parw );

	__m128 x1 = _mm_madd_ps( vz1, mvp0Z, parx );
	__m128 y1 = _mm_madd_ps( vz1, mvp1Z, pary );
	__m128 z1 = _mm_madd_ps( vz1, mvp2Z, parz );
	__m128 w1 = _mm_madd_ps( vz1, mvp3Z, parw );

	__m128 s0 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w0 );
	__m128 s1 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w1 );

	w0 = _mm_sel_ps( w0, vector_float_one, s0 );
	w1 = _mm_sel_ps( w1, vector_float_one, s1 );

	__m128 rw0 = _mm_rcp32_ps( w0 );
	__m128 rw1 = _mm_rcp32_ps( w1 );

	x0 = _mm_mul_ps( x0, rw0 );
	y0 = _mm_mul_ps( y0, rw0 );
	z0 = _mm_mul_ps( z0, rw0 );

	x1 = _mm_mul_ps( x1, rw1 );
	y1 = _mm_mul_ps( y1, rw1 );
	z1 = _mm_mul_ps( z1, rw1 );

	_mm_store_ps( corners.x + 0, x0 );
	_mm_store_ps( corners.x + 4, x1 );
	_mm_store_ps( corners.y + 0, y0 );
	_mm_store_ps( corners.y + 4, y1 );
	_mm_store_ps( corners.z + 0, z0 );
	_mm_store_ps( corners.z + 4, z1 );

#else

	anVec3 v;
	for ( int x = 0; x < 2; x++ ) {
		v[0] = frustumBounds[x][0];
		for ( int y = 0; y < 2; y++ ) {
			v[1] = frustumBounds[y][1];
			for ( int z = 0; z < 2; z++ ) {
				v[2] = frustumBounds[z][2];

				float tx = v[0] * frustumTransform[0][0] + v[1] * frustumTransform[0][1] + v[2] * frustumTransform[0][2] + frustumTransform[0][3];
				float ty = v[0] * frustumTransform[1][0] + v[1] * frustumTransform[1][1] + v[2] * frustumTransform[1][2] + frustumTransform[1][3];
				float tz = v[0] * frustumTransform[2][0] + v[1] * frustumTransform[2][1] + v[2] * frustumTransform[2][2] + frustumTransform[2][3];
				float tw = v[0] * frustumTransform[3][0] + v[1] * frustumTransform[3][1] + v[2] * frustumTransform[3][2] + frustumTransform[3][3];

				assert( tw > anMath::FLT_SMALLEST_NON_DENORMAL );

				float rw = 1.0f / tw;

				corners.x[(z<<2)|(y<<1)|(x<<0)] = tx * rw;
				corners.y[(z<<2)|(y<<1)|(x<<0)] = ty * rw;
				corners.z[(z<<2)|(y<<1)|(x<<0)] = tz * rw;
			}
		}
	}

#endif
}

/*
========================
anGLMatrix::CullFrustumCornersToPlane
========================
*/
frustumCull_t anGLMatrix::CullFrustumCornersToPlane( const frustumCorners_t & corners, const anPlane & plane ) {
	assert_16_byte_aligned( &corners );

#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 vp = _mm_loadu_ps( plane.ToFloatPtr() );

	__m128 x0 = _mm_load_ps( corners.x + 0 );
	__m128 y0 = _mm_load_ps( corners.y + 0 );
	__m128 z0 = _mm_load_ps( corners.z + 0 );

	__m128 x1 = _mm_load_ps( corners.x + 4 );
	__m128 y1 = _mm_load_ps( corners.y + 4 );
	__m128 z1 = _mm_load_ps( corners.z + 4 );

	__m128 p0 = _mm_splat_ps( vp, 0 );
	__m128 p1 = _mm_splat_ps( vp, 1 );
	__m128 p2 = _mm_splat_ps( vp, 2 );
	__m128 p3 = _mm_splat_ps( vp, 3 );

	__m128 d0 = _mm_madd_ps( x0, p0, _mm_madd_ps( y0, p1, _mm_madd_ps( z0, p2, p3 ) ) );
	__m128 d1 = _mm_madd_ps( x1, p0, _mm_madd_ps( y1, p1, _mm_madd_ps( z1, p2, p3 ) ) );

	int b0 = _mm_movemask_ps( d0 );
	int b1 = _mm_movemask_ps( d1 );

	unsigned int front = ( (unsigned int) -( ( b0 & b1 ) ^ 15 ) ) >> 31;
	unsigned int back = ( (unsigned int) -( b0 | b1 ) ) >> 31;

	compile_time_assert( FRUSTUM_CULL_FRONT == 1 );
	compile_time_assert( FRUSTUM_CULL_BACK == 2 );
	compile_time_assert( FRUSTUM_CULL_CROSS == 3 );

	return (frustumCull_t) ( front | ( back << 1 ) );

#else

	bool front = false;
	bool back = false;
	for ( int i = 0; i < 8; i++ ) {
		const float d = corners.x[i] * plane[0] + corners.y[i] * plane[1] + corners.z[i] * plane[2] + plane[3];
		if ( d >= 0.0f ) {
		    front = true;
		} else if ( d <= 0.0f ) {
		    back = true;
		}
		if ( back && front ) {
			return FRUSTUM_CULL_CROSS;
		}
	}
	if ( front ) {
		return FRUSTUM_CULL_FRONT;
	} else {
		return FRUSTUM_CULL_BACK;
	}

#endif
}
