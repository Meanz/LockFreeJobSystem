#ifndef VECTOR_H
#define VECTOR_H

#include <cmath>
#include <type_traits>

template<typename T>
struct _v2
{
    using length_type = typename std::conditional<std::is_integral<T>::value, float, T>::type;

    T x;
    T y;

    _v2(T _x, T _y) :
        x(_x), y(_y)
    {}

    _v2(T _x) :
        _v2(_x, _x)
    {}

    template<typename U>
    _v2(const _v2<U>& other) :
        x(static_cast<T>(other.x)),
        y(static_cast<T>(other.y))
    { }

    _v2() :
        _v2(0.0f, 0.0f)
    {}

    length_type length_squared() const
    {
        return this->dot(*this);
    }

    length_type length() const
    {
        return sqrt(this->length_squared());
    }

    length_type dot(const _v2<T> &v) const
    {
        return static_cast<length_type>(x) * static_cast<length_type>(v.x) +
               static_cast<length_type>(y) * static_cast<length_type>(v.y);
    }

    _v2<T> lerp(const _v2<T>& other, float lf)
    {
        //Lerp = a + (b - a) * lf;
        T x = x + (other.x - x) * lf;
        T y = y + (other.y - y) * lf;
        return _v2<T>(x, y);
    }

    _v2<T>& operator-=(const _v2<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    _v2<T>& operator+=(const _v2<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    _v2<T>& operator*=(const _v2<T>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    _v2<T>& operator*=(float value)
    {
        x *= value;
        y *= value;
        return *this;
    }

    _v2<T>& operator/=(float value)
    {
        x /= value;
        y /= value;
        return *this;
    }

    _v2<T>& operator/=(int value)
    {
        x /= value;
        y /= value;
        return *this;
    }


    _v2<T> operator/(float value) const
    {
        return _v2<T>(x / value, y / value);
    }

    _v2<T> operator+(const _v2<T>& rhs) const
    {
        return _v2<T>(x + rhs.x, y + rhs.y);
    }

    _v2<T> operator-(const _v2<T>& rhs) const
    {
        return _v2<T>(x - rhs.x,
                      y - rhs.y);
    }

    _v2<T> operator*(const float& rhs) const
    {
        return _v2<T>(x * rhs, y * rhs);
    }

};

template<typename T>
inline _v2<T> operator-(const _v2<T> v)
{
    return _v2<T>(-v.x, -v.y);
}

template<typename T>
struct _v3
{
    using length_type = typename std::conditional<std::is_integral<T>::value, float, T>::type;

    T x;
    T y;
    T z;

    _v3(T _x, T _y, T _z) :
        x(_x), y(_y), z(_z)
    {}

    _v3() :
        _v3(0.0f, 0.0f, 0.0f)
    {}

    length_type length_squared() const
    {
        return this->dot(*this);
    }

    length_type length() const
    {
        return sqrt(this->length_squared());
    }

    float dot(const _v3<T>& other) const
    {
        return static_cast<length_type>(x) * static_cast<length_type>(other.x) +
               static_cast<length_type>(y) * static_cast<length_type>(other.y) +
               static_cast<length_type>(z) * static_cast<length_type>(other.z);
    }

    _v3<T> unit_vector()
    {
        float l = length();
        if(l == 0.0f)
        {
            return *this;
        }
        return _v3<T>(x / l, y / l, z / l);
    }

    _v3<T> lerp(const _v3<T>& other, float lf)
    {
        //Lerp = a + (b - a) * lf;
        T _x = x + (other.x - x) * lf;
        T _y = y + (other.y - y) * lf;
        T _z = z + (other.z - z) * lf;
        return _v3<T>(_x, _y, _z);
    }

    _v3<T>& operator-=(const _v3<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    _v3<T>& operator+=(const _v3<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    _v3<T>& operator*=(const _v3<T>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    _v3<T>& operator/=(float value)
    {
        x /= value;
        y /= value;
        z /= value;
        return *this;
    }

    _v3<T>& operator/=(int value)
    {
        x /= value;
        y /= value;
        z /= value;
        return *this;
    }

    _v3<T> operator+(const _v3<T>& rhs) const
    {
        return _v3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    _v3<T> operator-(const _v3<T>& rhs) const
    {
        return _v3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    _v3<T> operator*(const float rhs) const
    {
        return _v3<T>(x * rhs, y * rhs, z * rhs);
    }

    bool operator==(const _v3<T>& other);
};

template<typename T>
struct _v4
{
    using length_type = typename std::conditional<std::is_integral<T>::value, float, T>::type;

    T x;
    T y;
    T z;
    T w;

    _v4(T _x, T _y, T _z, T _w) :
        x(_x), y(_y), z(_z), w(_w)
    {}

    _v4(_v3<T> inv3) :
        _v4(inv3.x, inv3.y, inv3.z, 0)
    {

    }

    _v4() :
        _v4(0.0f, 0.0f, 0.0f, 0.0f)
    {}

    length_type length_squared() const
    {
        return this->dot(*this);
    }

    length_type length() const
    {
        return sqrt(this->length_squared());
    }

    float dot(const _v3<T>& other)
    {
        return static_cast<length_type>(x) * static_cast<length_type>(other.x) +
               static_cast<length_type>(y) * static_cast<length_type>(other.y) +
               static_cast<length_type>(z) * static_cast<length_type>(other.z) +
               static_cast<length_type>(w) * static_cast<length_type>(other.w);
    }

    _v4<T>& operator-=(const _v4<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    _v4<T>& operator+=(const _v4<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    _v4<T>& operator*=(const _v4<T>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    _v4<T>& operator/=(float value)
    {
        x /= value;
        y /= value;
        z /= value;
        w /= value;
        return *this;
    }

    _v4<T>& operator/=(int value)
    {
        x /= value;
        y /= value;
        z /= value;
        w /= value;
        return *this;
    }

    _v4<T> operator+(const _v4<T>& rhs) const
    {
        return _v4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }

    _v4<T> operator-(const _v4<T>& rhs) const
    {
        return _v4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    _v4<T> operator*(const float& rhs) const
    {
        return _v4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
    }

    bool operator==(const _v4<T>& other);
};


typedef _v2<double> v2d;
typedef _v2<float> v2;
typedef _v2<int> v2i;
typedef _v2<long long> v2ll;
typedef _v3<float> v3;
typedef _v3<int> v3i;
typedef _v4<float> v4;
typedef _v4<int> v4i;

template<>
inline bool _v3<int>::operator==(const _v3<int>& other)
{
    return this->x == other.x && this->y == other.y && this->z == other.z;
}


#endif // VECTOR_H
