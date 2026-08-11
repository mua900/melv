#include "math_util.hpp"

namespace melv
{

float Complex::magnitude() const
{
    return sqrtf(real * real + imaginary * imaginary);
}

float Complex::winding() const
{
    return atan2f(imaginary, real);
}

float snap_value(float val, float bound1, float bound2, float threshold)
{
    if (fabsf(val - bound1) <= threshold) {
        val = bound1;
    }
    else if (fabsf(val - bound2) <= threshold) {
        val = bound2;
    }
    else if (fabsf(val - (bound1 + bound2) / 2) <= threshold) {
        val = (bound1 + bound2) / 2;
    }

    return val;
}

Color::Color(const Colorf& color) {
    float coef = 255.0;
    r = int(color.r * coef);
    g = int(color.g * coef);
    b = int(color.b * coef);
    a = int(color.a * coef);
}

Colorf::Colorf(const Color& color) {
    float coef = 1.0 / 255.0;
    r = (float)color.r * coef;
    g = (float)color.g * coef;
    b = (float)color.b * coef;
    a = (float)color.a * coef;
}

Colorf mixColors(Colorf a, Colorf b, float t)
{
    return Colorf(
        melv::lerp(a.r, b.r, t),
        melv::lerp(a.g, b.g, t),
        melv::lerp(a.b, b.b, t),
        melv::lerp(a.a, b.a, t)
    );
}

Colorf hexToColor(u32 color)
{
    Colorf result = {};

    u32 mask = 0xFF;
    result.r = float(color & mask) / 255;
    color >>= 8;
    result.g = float(color & mask) / 255;
    color >>= 8;
    result.b = float(color & mask) / 255;
    color >>= 8;
    result.a = float(color & mask) / 255;

    return result;
}

u32 colorToHex(Colorf color)
{
    u32 result = 0;
    result |= u32(color.r * 255) << 0;
    result |= u32(color.g * 255) << 8;
    result |= u32(color.b * 255) << 16;
    result |= u32(color.a * 255) << 24;
    return result;
}

vec2 lerp2(vec2 a, vec2 b, float t)
{
    return vec2(melv::lerp(a.x, b.x, t), melv::lerp(a.y, b.y, t));
}

vec2 reflect2(vec2 incident, vec2 normal)
{
    return incident - 2.0f * dot2(normal, incident) * normal;
}

vec2 get_direction_vector(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    return vec2(c, s);
}


mat4x4 identity_matrix()
{
    return mat4x4{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

vec3 mat3apply(mat3x3* mat, vec3 v)
{
    return vec3(
        mat->m00 * v.x + mat->m01 * v.y + mat->m02 * v.z,
        mat->m10 * v.x + mat->m11 * v.y + mat->m12 * v.z,
        mat->m20 * v.x + mat->m21 * v.y + mat->m22 * v.z
    );
}

void mat3mul(mat3x3* dst, mat3x3* left, mat3x3* right)
{
    mat3x3 result = {
        left->m00 * right->m00 + left->m01 * right->m10 + left->m02 * right->m20,
        left->m00 * right->m01 + left->m01 * right->m11 + left->m02 * right->m21,
        left->m00 * right->m02 + left->m01 * right->m12 + left->m02 * right->m22,

        left->m10 * right->m00 + left->m11 * right->m10 + left->m12 * right->m20,
        left->m10 * right->m01 + left->m11 * right->m11 + left->m12 * right->m21,
        left->m10 * right->m02 + left->m11 * right->m12 + left->m12 * right->m22,

        left->m20 * right->m00 + left->m21 * right->m10 + left->m22 * right->m20,
        left->m20 * right->m01 + left->m21 * right->m11 + left->m22 * right->m21,
        left->m20 * right->m02 + left->m21 * right->m12 + left->m22 * right->m22,
    };

    *dst = result;
}

void get_rotation_x(mat3x3* mat, float angle)
{
    *mat = {
        1, 0,               0,
        0, std::cos(angle), std::sin(angle),
        0,-std::sin(angle), std::cos(angle),
    };
}

void get_rotation_y(mat3x3* mat, float angle)
{
    *mat = {
        std::cos(angle), 0,-std::sin(angle),
        0,               1, 0,
        std::sin(angle), 0, std::cos(angle),
    };
}

void get_rotation_z(mat3x3* mat, float angle)
{
    *mat = {
        std::cos(angle), std::sin(angle), 0,
        -std::sin(angle), std::cos(angle), 0,
        0,               0,               1,
    };
}

void mat4mul(mat4x4* dst, mat4x4* left, mat4x4* right)
{
    mat4x4 result = {
        left->m00 * right->m00 + left->m01 * right->m10 + left->m02 * right->m20 + left->m03 * right->m30,
        left->m00 * right->m01 + left->m01 * right->m11 + left->m02 * right->m21 + left->m03 * right->m31,
        left->m00 * right->m02 + left->m01 * right->m12 + left->m02 * right->m22 + left->m03 * right->m32,
        left->m00 * right->m03 + left->m01 * right->m13 + left->m02 * right->m23 + left->m03 * right->m33,

        left->m10 * right->m00 + left->m11 * right->m10 + left->m12 * right->m20 + left->m13 * right->m30,
        left->m10 * right->m01 + left->m11 * right->m11 + left->m12 * right->m21 + left->m13 * right->m31,
        left->m10 * right->m02 + left->m11 * right->m12 + left->m12 * right->m22 + left->m13 * right->m32,
        left->m10 * right->m03 + left->m11 * right->m13 + left->m12 * right->m23 + left->m13 * right->m33,

        left->m20 * right->m00 + left->m21 * right->m10 + left->m22 * right->m20 + left->m23 * right->m30,
        left->m20 * right->m01 + left->m21 * right->m11 + left->m22 * right->m21 + left->m23 * right->m31,
        left->m20 * right->m02 + left->m21 * right->m12 + left->m22 * right->m22 + left->m23 * right->m32,
        left->m20 * right->m03 + left->m21 * right->m13 + left->m22 * right->m23 + left->m23 * right->m33,

        left->m30 * right->m00 + left->m31 * right->m10 + left->m32 * right->m20 + left->m33 * right->m30,
        left->m30 * right->m01 + left->m31 * right->m11 + left->m32 * right->m21 + left->m33 * right->m31,
        left->m30 * right->m02 + left->m31 * right->m12 + left->m32 * right->m22 + left->m33 * right->m32,
        left->m30 * right->m03 + left->m31 * right->m13 + left->m32 * right->m23 + left->m33 * right->m33,
    };

    *dst = result;
}

mat4x4 orthographic_projection_matrix(float left, float right, float bottom, float top, float near, float far)
{
    return mat4x4{
        2.0f / (right - left),  0,                      0,                      -(right + left) / (right - left),
        0,                      2.0f / (top - bottom),  0,                      -(top + bottom) / (top - bottom),
        0,                      0,                      -2.0f / (far - near),   -(far + near) / (far - near),
        0,                      0,                      0,                      1.0
    };
}

mat4x4 camera_matrix(vec2 position, vec2 scale)
{
    return mat4x4{
        scale.x, 0,       0, -position.x,
        0,       scale.y, 0, -position.y,
        0,       0,       1, 0,
        0,       0,       0, 1
    };
}

// @todo
void mat4_translate(mat4x4 *mat, vec3 translation)
{
    mat->m03 += translation.x;
    mat->m13 += translation.y;
    mat->m23 += translation.z;
}

// @todo
void mat4_scale(mat4x4* mat, float scale)
{
    mat->m33 *= scale;
}

void print_mat4(mat4x4* mat)
{
    printf("%f %f %f %f\n", mat->m00, mat->m01, mat->m02, mat->m03);
    printf("%f %f %f %f\n", mat->m10, mat->m11, mat->m12, mat->m13);
    printf("%f %f %f %f\n", mat->m20, mat->m21, mat->m22, mat->m23);
    printf("%f %f %f %f\n", mat->m30, mat->m31, mat->m32, mat->m33);
}

RectPoints get_rotated_points(Rectangle rect, float angle)
{
    float s = std::sinf(angle);
    float c = std::cosf(angle);
    float hw = rect.w / 2;
    float hh = rect.h / 2;

    vec2 diag0 = vec2(hw * c - hh * s, hw * s + hh * c);
    vec2 diag1 = vec2(-hw * c - hh * s, -hw * s + hh * c);

	RectPoints quad;
	quad.p[QuadTopLeft]     = vec2(rect.x + diag1.x, rect.y - diag1.y);
	quad.p[QuadTopRight]    = vec2(rect.x + diag0.x, rect.y - diag0.y);
	quad.p[QuadBottomLeft]  = vec2(rect.x - diag0.x, rect.y + diag0.y);
	quad.p[QuadBottomRight] = vec2(rect.x - diag1.x, rect.y + diag1.y);

	return quad;
}

Rectangle merge_volumes(Rectangle v1, Rectangle v2)
{
    Rectangle res = {};
    vec2 p = vec2(v1.x, v1.y) - vec2(v1.w / 2, v1.h / 2);
    vec2 q = vec2(v2.x, v2.y) - vec2(v2.w / 2, v2.h / 2);
    vec2 r = vec2(v1.x, v1.y) + vec2(v1.w / 2, v1.h / 2);
    vec2 w = vec2(v2.x, v2.y) + vec2(v2.w / 2, v2.h / 2);

    vec2 min = vec2(melv::min(p.x, q.x), melv::min(p.y, q.y));
    vec2 max = vec2(melv::max(r.x, w.x), melv::max(r.y, w.y));

    res.x = (min.x + max.x) / 2;
    res.y = (min.y + max.y) / 2;
    res.w = max.x - min.x;
    res.h = max.y - min.y;

    return res;
}

vec2 Rectangle::get_point_at_direction(Direction dir) const {
    switch (dir) {
    case DirNone:      return vec2(x, y);  // ???
    case DirEast:      return vec2(x + w / 2, y);
    case DirWest:      return vec2(x - w / 2, y);
    case DirSouth:     return vec2(x, y - h / 2);
    case DirNorth:     return vec2(x, y + h / 2);
    case DirNorthEast: return vec2(x + w / 2, y + h / 2);
    case DirNorthWest: return vec2(x - w / 2, y + h / 2);
    case DirSouthEast: return vec2(x + w / 2, y - h / 2);
    case DirSouthWest: return vec2(x - w / 2, y - h / 2);
    default:           panic("Invalid direction");
    }
}

bool Rectangle::contains_top_left(vec2 p) const
{
    return p.x >= x && p.x <= x + w &&
        p.y >= y && p.y <= y + h;
}

bool Rectangle::contains_centered(vec2 p) const
{
    return p.x >= x - w / 2 && p.x <= x + w / 2 &&
        p.y >= y - h / 2 && p.y <= y + h / 2;
}

float lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

float clamp(float a, float b, float x)
{
    return (a > x) ? a : (b < x) ? b : x;
}

float smoothstep(float a, float b, float x)
{
    float t = melv::clamp(0, 1, (x - a) / (b - a));
    return t * t * (3.0 - 2.0 * t);
}

}  // namespace
