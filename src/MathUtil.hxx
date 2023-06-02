#ifndef HARUHI_MATHUTIL_HXX
#define HARUHI_MATHUTIL_HXX

#include <simd/simd.h>

namespace math {

using simd::float3;
using simd::float3x3;
using simd::float4x4;

const float3 add(const float3&, const float3&);
const simd_float4x4 makeIdentity();
float4x4 makePerspective(float, float, float, float);
float4x4 makeXRotate(float);
float4x4 makeYRotate(float);
float4x4 makeZRotate(float);
float4x4 makeTranslate(const float3&);
float4x4 makeScale(const float3&);
float3x3 discardTranslation(const float4x4&);

} // ns math

#endif