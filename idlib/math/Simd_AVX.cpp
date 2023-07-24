#include "../precompiled.h"
#include <immintrin.h>
#pragma hdrstop

#include "Simd_AVX.h"
#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"
#include "Simd_Macros.h"

/*
 							//AVX & SSE IMPLEMTATION\\

Make sure to compile the code with appropriate compiler flags to enable AVX instructions,
such as -mavx for GCC or Clang. This is most important for AVX instructions.

*/

/*
===================
arcSIMD_AVX::
===================
*/
const char * VPCALL arcSIMD_AVX::GetName( void ) const {
	"AVX & SSE"
}

/*
===================
arcSIMD_AVX::DotProduct
===================
*/
float VPCALL arcSIMD_AVX::DotProduct( const float *src1, const float *src2, const int count ) {
	__m256 sum = _mm256_setzero_ps();

	for ( int i = 0; i < size; i += 8 ) {
		__m256 aVec = _mm256_loadu_ps( &src1[i] );
		__m256 bVec = _mm256_loadu_ps( &src2[i] );
		__m256 mul = _mm256_mul_ps( aVec, bVec );
		sum = _mm256_add_ps( sum, mul );
	}

	float result = 0.0f;
    float temp[8];
    _mm256_storeu_ps( temp, sum );
    for ( int i = 0; i < 8; i++ ) {
		result += temp[i];
	}

	return result;
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Sub16( float *dst, const float *src1, const float *src2, const int count ) {
	// Assuming count is a multiple of 16
	for ( int i = 0; i < count; i += 16 ) {
		__m256i aVec = _mm256_loadu_si256((__m256i*)&src1[i]);
		__m256i bVec = _mm256_loadu_si256((__m256i*)&src2[i]);
		__m256i resultVec = _mm256_sub_epi16(aVec, bVec);
		_mm256_storeu_si256((__m256i*)&dst[i], resultVec);
	}
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Add16( float *dst, const float *src1, const float *src2, const int count ) {

}

/*
============
arcSIMD_AVX::DotProduct

dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/
void VPCALL arcSIMD_AVX::Dot( float *dst, const arcVec3 &constant, const arcDrawVert *src, const int count ) {
}

/*
============
arcSIMD_AVX::DotProduct

dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/
void VPCALL arcSIMD_AVX::Dot( float &dot, const float *src1, const float *src2, const int count ) {
    if ( count == 0 || ( constant.length == 0 && src.length == 0 ) ) {

	}
}

/*
============
arcSIMD_AVX::DotProduct

dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/

void VPCALL arcSIMD_AVX::Dot( float &dot, const float *src1, const float *src2, const int count ) {
    if ( count == 0 ) {
        dot = 0.0f;
        return;
    }

    double sum = 0.0;
    int i = 0;
    for ( ; i < count - 7; i += 8 ) {
        sum += src1[i + 0] * src2[i + 0] +
               src1[i + 1] * src2[i + 1] +
               src1[i + 2] * src2[i + 2] +
               src1[i + 3] * src2[i + 3] +
               src1[i + 4] * src2[i + 4] +
               src1[i + 5] * src2[i + 5] +
               src1[i + 6] * src2[i + 6] +
               src1[i + 7] * src2[i + 7];
    }

    for ( ; i < count; i++ ) {
        sum += src1[i] * src2[i];
    }

    dot = static_cast<float>( sum );
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Add( float *dst, const float *src0, const float *src1, const int count ) {
    for ( int i = 0; i < count; i++ ) {
        dst[i] = src0[i] + src1[i];
    }
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Mul( float *dst, const float *src0, const float *src1, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::MulSub( float *dst, const float *src0, const float *src1, const int count ) {

}


/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::MulAdd( float *dst, const float *src0, const float *src1, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Clamp( float *dst, const float *src, const float min, const float max, const int count ) {
+	for (; i < count - 7; i += 8 ) {
+		__m256 srcVec = _mm256_loadu_ps( src + i );
+		__m256 minVec = _mm256_set1_ps( min );
+		__m256 maxVec = _mm256_set1_ps( max );
+		__m256 resultVec = _mm256_min_ps(_mm256_max_ps( srcVec, minVec ), maxVec );
+		_mm256_storeu_ps( dst + i, resultVec );
+	}

	for ( int i = 0; i < count; ++i ) {
		dst[i] = Min( Max( src[i], min ), max );
	}
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::ClampMin( float *dst, const float *src, const float min, const int count ) {
	int i = 0;
	// Process 8 elements at a time using AVX instructions
    for (; i < count - 7; i += 8 ) {
		__m256 srcVec = _mm256_loadu_ps(src + i);
        __m256 minVec = _mm256_set1_ps(min);
		__m256 resultVec = _mm256_max_ps(srcVec, minVec);
        _mm256_storeu_ps(dst + i, resultVec);
	}
	// Process remaining elements individually
	for ( ; i < count; i++ ) {
		dst[i] = Max( src[i], min );
	}
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::ClampMax( float *dst, const float *src, const float max, const int count ) {
int i = 0;
    // Process 8 elements at a time using AVX instructions
    for (; i < count - 7; i += 8) {
        __m256 srcVec = _mm256_loadu_ps(src + i);
        __m256 maxVec = _mm256_set1_ps(max);
        __m256 resultVec = _mm256_min_ps(srcVec, maxVec);
        _mm256_storeu_ps(dst + i, resultVec);
    }
    // Process remaining elements individually
    for (; i < count; i++) {
        dst[i] = Min( src[i], max );
    }
}

/*
===================
arcSIMD_AVX::
===================
*/

void VPCALL arcSIMD_AVX::Zero16( float *dst, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Negate16( float *dst, const int count ) {
    for (int i = 0; i < count; i += 16) {
        __m256 vec = _mm256_loadu_ps(dst + i);
        vec = _mm256_sub_ps(_mm256_setzero_ps(), vec);
        _mm256_storeu_ps(dst + i, vec);
    }
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Copy16( float *dst, const float *src, const int count ) {
    for ( int i = 0; i < count; i += 16 ) {
        __m256 srcVec = _mm256_loadu_ps( &src[i] );
        _mm256_storeu_ps( &dst[i], srcVec );
    }
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Add16( float *dst, const float *src1, const float *src2, const int count ) {
	for ( int i = 0; i < count; i += 8 ) {
		__m256 vec1 = _mm256_loadu_ps( src1 + i );
		__m256 vec2 = _mm256_loadu_ps( src2 + i );
		__m256 sum = _mm256_add_ps( vec1, vec2 );
		_mm256_storeu_ps( dst + i, sum );
	}
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Sub16( float *dst, const float *src1, const float *src2, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::Mul16( float *dst, const float *src1, const float constant, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::AddAssign16( float *dst, const float *src, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::SubAssign16( float *dst, const float *src, const int count ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void arcSIMD_AVX::MulAssign16( float *dst, const float constant, const int count ) {
    for ( int i = 0; i < count; i += 8 ) {
        __m256 c = _mm256_set1_ps( constant );
        __m256 a = _mm256_loadu_ps( dst + i );
        __m256 result = _mm256_mul_ps (a, c );
        _mm256_storeu_ps( dst + i, result );
    }
}

/*
===================
arcSIMD_AVX::
===================
*/
void arcSIMD_AVX::Memcpy( void* dest0, const int val, const int count0 ) {
    // Code to copy memory efficiently and quickly
}

/*
===================
arcSIMD_AVX::
===================
*/
void arcSIMD_AVX::Memset( void* dest0, const int val, const int count0 ) {
    __m256i* dest = (__m256i*)dest0;
    const __m256i value = _mm256_set1_epi32 (val );

    for ( int i = 0; i < count0 / 8; i++ ) {
        _mm256_storeu_si256( dest + i, value );
    }

	if ( count0 % 8 != 0 ) {
		int *lastElement = (int*)( dest + count0 / 8 );
		for ( int i = 0; i < count0 % 8; i++ ) {
			*lastElement++ = val;
		}
	}
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::MatX_LowerTriangularSolve( const arcMatX &L, float *x, const float *b, const int n, int skip = 0 ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::MatX_LowerTriangularSolveTranspose( const arcMatX &L, float *x, const float *b, const int n ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
bool VPCALL arcSIMD_AVX::MatX_LDLTFactor( arcMatX &mat, arcVecX& invDiag, const int n ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int* index, const int numJoints ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::ConvertJointQuatsToJointMats(arcJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::ConvertJointMatsToJointQuats(idJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::TransformJoints( arcJointMat *jointMats, const int* parents, const int firstJoint, const int lastJoint ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::UntransformJoints( arcJointMat *jointMats, const int* parents, const int firstJoint, const int lastJoint ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::TransformVerts( arcDrawVert *verts, const int numVerts, const arcJointMat *joints, const arcVec4* weights, const int* index, const int numWeights ) {
	int i;
	const byte *jointsPtr = ( byte * )joints;
	for ( int j = i = 0; i < numVerts; i++ ) {
		arcVec3 v;
		v = ( *(arcJointMat *) ( jointsPtr + index[j*2+0] ) ) * weights[j];
		while( index[j*2+1] == 0 ) {
			j++;
			v += ( *(arcJointMat *) ( jointsPtr + index[j*2+0] ) ) * weights[j];
		}
		j++;

		verts[i].xyz = v;
	}
}

/*
===================
arcSIMD_AVX::GetCropBounds
===================
*/
static ARC_INLINE void GetCropBounds( const frustumCorners_t &corners, const float *renderMat, arcBounds &bounds ) {
    __m256 cornersX = _mm256_loadu_ps(corners.x);
    __m256 cornersY = _mm256_loadu_ps(corners.y);
    __m256 cornersZ = _mm256_loadu_ps(corners.z);

    __m256 matRows01 = _mm256_loadu_ps(renderMat);
    __m256 matRows23 = _mm256_loadu_ps(renderMat+8);


    __m256 m00 = _mm256_splat_ps(matRows01, 0);
    __m256 m01 = _mm256_splat_ps(matRows01, 1);
    __m256 m02 = _mm256_splat_ps(matRows01, 2);
    __m256 m03 = _mm256_splat_ps(matRows01, 3);
    __m256 outXs = _mm256_add_ps(_mm256_mul_ps(m00, cornersX),
                                 _mm256_mul_ps(m01, cornersY));
    outXs = _mm256_add_ps(
        _mm256_add_ps(outXs, _mm256_mul_ps(m02, cornersZ)), m03);

    __m256 m10 = _mm256_splat_ps(matRows01, 4+0);
    __m256 m11 = _mm256_splat_ps(matRows01, 4+1);
    __m256 m12 = _mm256_splat_ps(matRows01, 4+2);
    __m256 m13 = _mm256_splat_ps(matRows01, 4+3);
    __m256 outYs = _mm256_add_ps(_mm256_mul_ps(m10, cornersX),
                                 _mm256_mul_ps(m11, cornersY));
    outYs = _mm256_add_ps(
        _mm256_add_ps(outYs, _mm256_mul_ps(m12, cornersZ)), m13);

    __m256 m20 = _mm256_splat_ps(matRows23, 0);
    __m256 m21 = _mm256_splat_ps(matRows23, 1);
    __m256 m22 = _mm256_splat_ps(matRows23, 2);
    __m256 m23 = _mm256_splat_ps(matRows23, 3);
    __m256 outZs = _mm256_add_ps(_mm256_mul_ps(m20, cornersX),
                                 _mm256_mul_ps(m21, cornersY));
    outZs = _mm256_add_ps(
        _mm256_add_ps(outZs, _mm256_mul_ps(m22, cornersZ)), m23);

    __m256 m30 = _mm256_splat_ps(matRows23, 4+0);
    __m256 m31 = _mm256_splat_ps(matRows23, 4+1);
    __m256 m32 = _mm256_splat_ps(matRows23, 4+2);
    __m256 m33 = _mm256_splat_ps(matRows23, 4+3);
    __m256 outWs = _mm256_add_ps(_mm256_mul_ps(m30, cornersX),
                                 _mm256_mul_ps(m31, cornersY));
    outWs = _mm256_add_ps(
        _mm256_add_ps(outWs, _mm256_mul_ps(m32, cornersZ)), m33);

    outXs = _mm256_div_ps(outXs, outWs);
    outYs = _mm256_div_ps(outYs, outWs);
    outZs = _mm256_div_ps(outZs, outWs);

    bounds.AddPoint(arcVec3(outXs[0], outYs[0], outZs[0]));
    bounds.AddPoint(arcVec3(outXs[1], outYs[1], outZs[1]));
    bounds.AddPoint(arcVec3(outXs[2], outYs[2], outZs[2]));
    bounds.AddPoint(arcVec3(outXs[3], outYs[3], outZs[3]));
    bounds.AddPoint(arcVec3(outXs[4], outYs[4], outZs[4]));
    bounds.AddPoint(arcVec3(outXs[5], outYs[5], outZs[5]));
    bounds.AddPoint(arcVec3(outXs[6], outYs[6], outZs[6]));
    bounds.AddPoint(arcVec3(outXs[7], outYs[7], outZs[7]));
}
/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::TracePointCull( byte *cullBits, byte& totalOr, const float radius, const arcPlane *planes, const arcDrawVert *verts, const int numVerts ) {
}

void VPCALL arcSIMD_AVX::DecalPointCull( byte *cullBits, const arcPlane* planes, const arcDrawVert *verts, const int numVerts ) {
    for ( int i = 0; i < numVerts; i++ ) {
        cullBits[i] = 0;
        for (int j = 0; j < 6; j++) {
            if ( planes[j].Distance( verts[i].xyz ) < -verts[i].radius ) {
                cullBits[i] |= ( 1 << j );
            }
        }
    }
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::OverlayPointCull( byte *cullBits, arcVec2 *texCoords, const arcPlane *planes, const arcDrawVert *verts, const int numVerts ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::DeriveTriPlanes( arcPlane *planes, const arcDrawVert *verts, const int numVerts, const int* indexes, const int numIndexes ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::DeriveTriPlanes( arcPlane *planes, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::DeriveTangents( arcPlane *planes, arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {

}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::DeriveUnsmoothedTangents( arcDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) {
}

/*
===================
arcSIMD_AVX::
===================
*/
void VPCALL arcSIMD_AVX::NormalizeTangents( arcDrawVert *verts, const int numVerts ) {
}