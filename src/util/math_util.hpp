#ifndef MATH_UTIL_HPP
#define MATH_UTIL_HPP

#include <cmath>

#define CONSTANT_HALF_PI          1.57079632679
#define CONSTANT_PI               3.14159265359
#define CONSTANT_ONE_AND_HALF_PI  4.71238898038
#define CONSTANT_E                2.71828182846
#define CONSTANT_TAU              6.28318530717

namespace cobot {

float snap_value(float val, float bound1, float bound2, float threshold);

struct ivec2 {
    int x, y;
};

enum Direction {
    DirNone = 0,
    DirEast = 1,
    DirWest = 2,
    DirSouth = 4,
    DirNorth = 8,
    DirNorthEast = DirNorth | DirEast,
    DirNorthWest = DirNorth | DirWest,
    DirSouthEast = DirSouth | DirEast,
    DirSouthWest = DirSouth | DirWest,
};

constexpr bool direction_is_vertical(Direction dir) { return dir & DirNorth || dir & DirSouth; }
constexpr bool direction_is_horizontal(Direction dir) { return dir & DirWest || dir & DirEast; }

enum Axis {
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_W,
};

struct vec2 {
    float x = 0, y = 0;
    vec2() {}
    vec2(float p_x, float p_y) : x(p_x), y(p_y) {}
    explicit vec2(float p) : x(p), y(p) {}

    vec2 normalized() const
    {
        float mag = sqrt(x*x+y*y);
        return vec2(x/mag,y/mag);
    }

    float magnitude() const {
        return sqrtf(x * x + y * y);
    }

    vec2 rotated(float angle) const {
        float s = std::sinf(angle);
        float c = std::cosf(angle);
        return vec2(x * c - y * s, x * s + y * c);
    }

    void operator+=(const vec2 other)
    {
        x += other.x;
        y += other.y;
    }

    void operator-=(const vec2 other)
    {
        x -= other.x;
        y -= other.y;
    }

    void operator/=(float s)
    {
        x /= s;
        y /= s;
    }

    void operator*=(float s)
    {
        x *= s;
        y *= s;
    }
};

inline float dot2(vec2 a, vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline float distance2(vec2 a, vec2 b)
{
    return vec2(a.x - b.x, a.y - b.y).magnitude();
}

inline vec2 operator-(const vec2 v)
{ return vec2(-v.x, -v.y); }

inline vec2 operator+(const vec2 a, const vec2 b)
{
    return vec2(a.x + b.x, a.y + b.y);
}
inline vec2 operator-(const vec2 a, const vec2 b)
{
    return vec2(a.x - b.x, a.y - b.y);
}
inline vec2 operator*(vec2 v, float s)
{
    return vec2(v.x * s, v.y * s);
}
inline vec2 operator*(float s, vec2 v)
{
    return vec2(v.x * s, v.y * s);
}
inline vec2 operator/(vec2 v, float s)
{
    return vec2(v.x / s, v.y / s);
}
inline vec2 operator*(vec2 a, vec2 b)
{
    return vec2(a.x * b.x, a.y * b.y);
}

vec2 lerp2(vec2 a, vec2 b, float t);
vec2 reflect2(vec2 incident, vec2 normal);
vec2 get_direction_vector(float angle);

struct vec3 {
    float x = 0;
    float y = 0;
    float z = 0;

    vec3() {}
    vec3(float p_x, float p_y, float p_z) : x(p_x), y(p_y), z(p_z) {}
    explicit vec3(float p) : x(p), y(p), z(p) {}

    vec2 xy() const { return vec2(x, y); }

    vec3 normalized() const
    {
        float mag = sqrt(x*x+y*y+z*z);
        return vec3(x/mag,y/mag,z/mag);
    }

    float magnitude() const {
        return sqrtf(x * x + y * y + z * z);
    }

    void operator+=(const vec3 other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    void operator-=(const vec3 other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }

    void operator/=(float s)
    {
        x /= s;
        y /= s;
        z /= s;
    }

    void operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
    }
};

inline float dot3(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross3(vec3 a, vec3 b)
{
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline vec3 operator-(const vec3 v)
{ return vec3(-v.x, -v.y, -v.z); }

inline vec3 operator+(const vec3 a, const vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline vec3 operator-(const vec3 a, const vec3 b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline vec3 operator*(vec3 v, float s)
{
    return vec3(v.x * s, v.y * s, v.z * s);
}
inline vec3 operator*(float s, vec3 v)
{
    return vec3(v.x * s, v.y * s, v.z * s);
}
inline vec3 operator/(vec3 v, float s)
{
    return vec3(v.x / s, v.y / s, v.z / s);
}
inline vec3 operator*(vec3 a, vec3 b)
{
    return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

struct mat3x3 {
    float m00, m01, m02 = {};
    float m10, m11, m12 = {};
    float m20, m21, m22 = {};
};

vec3 mat3apply(mat3x3* mat, vec3 v);
void mat3mul(mat3x3* dst, mat3x3* left, mat3x3* right);

void get_rotation_x(mat3x3* mat, float angle);
void get_rotation_y(mat3x3* mat, float angle);
void get_rotation_z(mat3x3* mat, float angle);

struct vec4
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;
};

struct mat4x4 {
    float m00, m01, m02, m03 = {};
    float m10, m11, m12, m13 = {};
    float m20, m21, m22, m23 = {};
    float m30, m31, m32, m33 = {};
};

void mat4mul(mat4x4* dst, mat4x4* left, mat4x4* right);

mat4x4 identity_matrix();
mat4x4 orthographic_projection_matrix(float left, float right, float bottom, float top, float near, float far);
mat4x4 camera_matrix(vec2 position, vec2 scale);

struct vec3d {
    double x = 0;
    double y = 0;
    double z = 0;

    vec3d() {}
    vec3d(double x, double y, double z)
        : x(x), y(y), z(z)
    {}
    vec3d(vec3 v)
        : x(v.x), y(v.y), z(v.z)
    {}
};

// @todo quaternions

inline float normalize_angle_radians_f(float x) {
    return std::fmodf(x, CONSTANT_TAU);
}

inline float normalize_angle_degrees_f(float x) {
    return std::fmodf(x, 360.0f);
}

inline double normalize_angle_radians(double x) {
    return std::fmod(x, CONSTANT_TAU);
}

inline double normalize_angle_degrees(double x) {
    return std::fmod(x, 360.0);
}

struct ColorF;

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
    Color() {}
    constexpr Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b), a(0xff) {}
    constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) {}
    Color(const ColorF& color);
};

struct ColorF {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    ColorF() {}
    ColorF(float r, float g, float b) : r(r), g(g), b(b), a(1.0) {}
    ColorF(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
    ColorF(const Color& color);
    explicit ColorF(ColorF col, float nalpha) : r(col.r), g(col.g), b(col.b), a(nalpha) {}
};

ColorF mixColors(ColorF a, ColorF b, float t);

// simple custom complex number
struct Complex {
	float real = 0.0;
	float imaginary = 0.0;

	Complex() {}
    Complex(float r, float i) : real(r), imaginary(i) {}

    float magnitude() const;
    float winding() const;
};

// overloads for complex

inline Complex operator+(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real + rhs.real, lhs.imaginary + rhs.imaginary);
}

inline Complex operator-(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real - rhs.real, lhs.imaginary - rhs.imaginary);
}

inline Complex operator*(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real * rhs.real - lhs.imaginary * rhs.imaginary, lhs.real * rhs.imaginary + lhs.imaginary * rhs.real);
}

struct Rectangle {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rectangle() {}
    Rectangle(vec2 pos, vec2 scale) : x(pos.x), y(pos.y), w(scale.x), h(scale.y) {}
    Rectangle(float p_x, float p_y, float p_w, float p_h)
        : x(p_x), y(p_y), w(p_w), h(p_h)
    {}

    bool contains_top_left(vec2 p) const;
    bool contains_centered(vec2 p) const;
    Rectangle to_top_left() const {
        return Rectangle(x - w / 2, y - h / 2, w, h);
    }
    Rectangle to_center() const {
        return Rectangle(x + w / 2, y + h / 2, w, h);
    }

    Direction on_edge(vec2 position, float d) const
    {
        vec2 topLeft = get_top_left();
        vec2 relative = position - topLeft;

        bool left = std::fabsf(relative.x) < d;
        bool right = std::fabsf(relative.x - w) < d;
        bool down = std::fabsf(relative.y) < d;
        bool up = std::fabsf(relative.y - h) < d;

        bool horizontal = topLeft.x - d <= position.x && topLeft.x + w + d >= position.x;
        bool vertical = topLeft.y - d <= position.y && topLeft.y + h + d >= position.y;

        Direction west = (left && vertical) ? DirWest : DirNone;
        Direction east = (right && vertical) ? DirEast : DirNone;
        Direction south = (down && horizontal) ? DirSouth : DirNone;
        Direction north = (up && horizontal) ? DirNorth : DirNone;

        return Direction(west | east | south | north);
    }

    vec2 get_point_at_direction(Direction dir) const;
    
    vec2 get_position() const {
        return vec2(x, y);
    }

    vec2 get_scale() const {
        return vec2(w, h);
    }

    vec2 get_top_left() const {
        return vec2(x - w / 2, y - h / 2);
    }
    vec2 get_center() const {
        return vec2(x + w / 2, y +h / 2);
    }
};

Rectangle merge_volumes(Rectangle v1, Rectangle v2);

enum QuadVertexPosition {
    QuadTopLeft = 0,
    QuadTopRight = 1,
    QuadBottomLeft = 2,
    QuadBottomRight = 3,
};

struct Quad {
    vec2 vertices[4];

    Quad() {}
    Quad(vec2 tl, vec2 tr, vec2 bl, vec2 br)
    {
        vertices[0] = tl;
        vertices[1] = tr;
        vertices[2] = bl;
        vertices[3] = br;
    }
};

Quad get_rotated_points(Rectangle rect, float angle);

struct RectangleRot {
    float x = {};
    float y = {};
    float w = {};
    float h = {};
    float rot = {};

    RectangleRot() {}
    RectangleRot(vec2 pos, vec2 sca, float ori)
        :
        x(pos.x), y(pos.y), w(sca.x), h(sca.y), rot(ori)
    {}
    RectangleRot(float x, float y, float w, float h, float rotation)
        :
        x(x), y(y), w(w), h(h), rot(rotation)
    {}

    Quad get_points() const {
        return get_rotated_points(Rectangle(x,y,w,h), rot);
    }
};

#define COLOR_WHITE ((Color){0xff,0xff,0xff,0xff})
#define COLOR_BLACK ((Color){0,0,0,0xff})
#define COLOR_RED   ((Color){0xff,0,0,0xff})
#define COLOR_GREEN ((Color){0,0xff,0,0xff})
#define COLOR_BLUE  ((Color){0,0,0xff,0xff})

#define COLOR_ARG(color) color.r,color.g,color.b,color.a

constexpr float RADIAN_TO_DEGREE_F = 360.0f / CONSTANT_TAU;
constexpr float DEGREE_TO_RADIAN_F = CONSTANT_TAU / 360.0f;
constexpr double RADIAN_TO_DEGREE = 360.0 / CONSTANT_TAU;
constexpr double DEGREE_TO_RADIAN = CONSTANT_TAU / 360.0;

constexpr int max(int x, int y) { return x > y ? x : y; }
constexpr int min(int x, int y) { return x > y ? y : x; }

float lerp(float a, float b, float t);
float clamp(float a, float b, float x);
float smoothstep(float a, float b, float x);

constexpr float radian_to_degree_f(float angle) { return angle * RADIAN_TO_DEGREE_F; }
constexpr float degree_to_radian_f(float angle) { return angle * DEGREE_TO_RADIAN_F; }
constexpr double radian_to_degree(double angle) { return angle * RADIAN_TO_DEGREE; }
constexpr double degree_to_radian(double angle) { return angle * DEGREE_TO_RADIAN; }

}  // namespace

#endif // MATH_UTIL_HPP
