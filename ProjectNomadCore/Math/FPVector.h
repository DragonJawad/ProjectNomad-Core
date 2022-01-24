#pragma once

#include <iostream>

#include "FixedPoint.h"
#include "FPMath.h"

namespace ProjectNomad {
    class FPVector {
    public:
        fp x;
        fp y;
        fp z;

        FPVector() : x(0), y(0), z(0) {}
        FPVector(const fp& val): x(val), y(val), z(val) {}
        FPVector(fp x, fp y, fp z) : x(x), y(y), z(z) {}

        static FPVector zero() {
            return {fp{0}, fp{0}, fp{0}};
        }

        static FPVector forward() {
            return {fp{1}, fp{0}, fp{0}};
        }

        static FPVector right() {
            return {fp{0}, fp{1}, fp{0}};
        }

        static FPVector up() {
            return {fp{0}, fp{0}, fp{1}};
        }

        static fp distanceSq(FPVector from, FPVector to) {
            return (to - from).getLengthSquared();
        }

        static fp distance(FPVector from, FPVector to) {
            return (to - from).getLength();
        }

        static FPVector direction(FPVector from, FPVector to) {
            return (to - from).normalized();
        }

        fp getLength() const {
            fp preSqrt = x * x + y * y + z * z;
            return sqrt(preSqrt);
        }

        fp getLengthSquared() const {
            return x * x + y * y + z * z;
        }

        FPVector operator-() const {
            return {-x, -y, -z};
        }

        FPVector operator+(const FPVector& v) const {
            return {x + v.x, y + v.y, z + v.z};
        }

        FPVector operator+=(const FPVector& v) {
            x += v.x;
            y += v.y;
            z += v.z;

            return *this;
        }

        FPVector operator-(const FPVector& v) const {
            return {x - v.x, y - v.y, z - v.z};
        }

        FPVector operator-=(const FPVector& v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;

            return *this;
        }

        FPVector operator*(fp value) const {
            return {x * value, y * value, z * value};
        }

        FPVector operator/(fp value) const {
            return {x / value, y / value, z / value};
        }

        fp operator[](int i) const {
            switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                // TODO: Throw error!
                return FPMath::minLimit();
            }
        }

        FPVector normalized() const {
            auto length = getLength();

            // IDEA: Likely far better to log and/or assert as likely unexpected scenario
            // IDEA: Somehow use LengthSq for this check instead while still being efficient (eg, take square after)
            if (length == fp{0}) {
                return zero();
            }

            return *this / length;
        }

        void normalize() {
            *this = normalized();
        }

        fp dot(const FPVector& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        FPVector cross(const FPVector& other) const {
            FPVector result;

            result.x = y * other.z - z * other.y;
            result.y = z * other.x - x * other.z;
            result.z = x * other.y - y * other.x;

            return result;
        }

        std::string toString() const {
            auto floatX = (float)x;
            auto floatY = (float)y;
            auto floatZ = (float)z;

            return "x: " + std::to_string(floatX)
                + " | y: " + std::to_string(floatY)
                + " | z: " + std::to_string(floatZ);
        }
    };

    inline bool operator==(const FPVector& lhs, const FPVector& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    inline bool operator!=(const FPVector& lhs, const FPVector& rhs) {
        return !(lhs == rhs);
    }

    inline FPVector operator*(fp lhs, const FPVector& rhs) {
        return rhs * lhs;
    }

    inline std::ostream& operator<<(std::ostream& os, const FPVector& vector) {
        os << "FPVector<" << static_cast<float>(vector.x) << ", "
                        << static_cast<float>(vector.y) << ", "
                        << static_cast<float>(vector.z) << ">";
        return os;
    }

}