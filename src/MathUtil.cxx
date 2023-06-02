#include "MathUtil.hxx"

namespace math {

using simd::float4;

const float3
add(const float3& a, const float3& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z };
}

const simd_float4x4
makeIdentity() {
  return (simd_float4x4) {
    (float4){ 1., 0., 0., 0. },
    (float4){ 0., 1., 0., 0. },
    (float4){ 0., 0., 1., 0. },
    (float4){ 0., 0., 0., 1. }
  };
}

float4x4
makePerspective(float fov, float asp, float znear, float zfar) {

  float ys = 1. / tanf(fov * 0.5);
  float xs = ys / asp;
  float zs = zfar / (znear - zfar);

  return simd_matrix_from_rows(
    (float4){ xs, 0., 0., 0. },
    (float4){ 0., ys, 0., 0. },
    (float4){ 0., 0., zs, znear * zs },
    (float4){ 0, 0, -1, 0 }
  );
}

float4x4
makeXRotate(float a) {
  return simd_matrix_from_rows(
    (float4){ 1., 0., 0., 0. },
    (float4){ 0., cosf(a), sinf(a), 0. },
    (float4){ 0., -sinf(a), cosf(a), 0. },
    (float4){ 0., 0., 0., 1. }
  );
}

float4x4
makeYRotate(float a) {
  return simd_matrix_from_rows(
    (float4){ cosf(a), 0., sinf(a), 0. },
    (float4){ 0., 1., 0., 0. },
    (float4){ -sinf(a), 0., cosf(a), 0. },
    (float4){ 0., 0., 0., 1. }
  );
}

float4x4
makeZRotate(float a) {
  return simd_matrix_from_rows(
    (float4){ cosf(a), sinf(a), 0., 0. },
    (float4){ -sinf(a), cosf(a), 0., 0. },
    (float4){ 0., 0., 1., 0. },
    (float4){ 0., 0., 0., 1. }
  );
}

float4x4
makeTranslate(const float3& v) {
  const float4 c0 = { 1., 0., 0., 0. };
  const float4 c1 = { 0., 1., 0., 0. };
  const float4 c2 = { 0., 0., 1., 0. };
  const float4 c3 = { v.x, v.y, v.z, 1. };
  return simd_matrix(c0, c1, c2, c3);
}

float4x4
makeScale(const float3& v) {
  return simd_matrix(
    (float4){ v.x, 0, 0, 0 },
    (float4){ 0, v.y, 0, 0 },
    (float4){ 0, 0, v.z, 0 },
    (float4){ 0, 0, 0, 1. }
  );
}

simd::float3x3
discardTranslation(const float4x4& m) {
  return simd_matrix(m.columns[0].xyz, m.columns[1].xyz, m.columns[2].xyz);
}

} // ns math