#ifndef __BV_BOUNDS_H__
#define __BV_BOUNDS_H__

/*
===============================================================================

        Axis Aligned Bounding Box

===============================================================================
*/

class arcBounds {
public:
  arcBounds(void);
  explicit arcBounds(const arcVec3 &mins, const arcVec3 &maxs);
  explicit arcBounds(const arcVec3 &point);

  const arcVec3 &operator[](const int index) const;
  arcVec3 &operator[](const int index);
  arcBounds operator+( const arcVec3 &r ) const; // returns translated bounds
  arcBounds &operator+=( const arcVec3 &r );     // translate the bounds
  arcBounds operator*( const arcMat3 &r ) const; // returns rotated bounds
  arcBounds &operator*=( const arcMat3 &r );     // rotate the bounds
  arcBounds operator+(const arcBounds &a) const;
  arcBounds &operator+=(const arcBounds &a);
  arcBounds operator-(const arcBounds &a) const;
  arcBounds &operator-=(const arcBounds &a);

  bool Compare(const arcBounds &a) const; // exact compare, no epsilon
  bool Compare(const arcBounds &a,
               const float epsilon) const;   // compare with epsilon
  bool operator==(const arcBounds &a) const; // exact compare, no epsilon
  bool operator!=(const arcBounds &a) const; // exact compare, no epsilon

  void Clear(); // inside out bounds
  void Zero();  // single point at origin

  float ShortestDistance(const arcVec3 &point) const;

  arcVec3 GetCenter() const; // returns center of bounds
  float GetRadius() const;   // returns the radius relative to the bounds origin
  float GetRadius(const arcVec3 &center)
      const;               // returns the radius relative to the given center
  float GetVolume() const; // returns the volume of the bounds
  bool IsCleared() const;  // returns true if bounds are inside out

  void AddPointToBounds(const arcVec3 &v, arcVec3 &mins, arcVec3 &maxs);

  bool AddPoint(
      const arcVec3 &v); // add the point, returns true if the bounds expanded
  bool AddBounds(const arcBounds
                     &a); // add the bounds, returns true if the bounds expanded
  arcBounds Intersect(const arcBounds &a)
      const; // return intersection of this bounds with the given bounds
  arcBounds &IntersectSelf(
      const arcBounds &a); // intersect this bounds with the given bounds
  arcBounds Expand(const float d)
      const; // return bounds expanded in all directions with the given value
  arcBounds &ExpandSelf(
      const float d); // expand bounds in all directions with the given value
  arcBounds
  Translate(const arcVec3 &translation) const; // return translated bounds
  arcBounds &TranslateSelf(const arcVec3 &translation); // translate this bounds
  arcBounds Rotate(const arcMat3 &rotation) const;      // return rotated bounds
  arcBounds &RotateSelf(const arcMat3 &rotation);       // rotate this bounds

  float PlaneDistance(const arcPlane &plane) const;
  int PlaneSide(const arcPlane &plane, const float epsilon = ON_EPSILON) const;

  bool ContainsPoint(const arcVec3 &p) const;      // includes touching
  bool IntersectsBounds(const arcBounds &a) const; // includes touching
  bool LineIntersection(const arcVec3 &start, const arcVec3 &end) const;
  // intersection point is start + dir * scale
  bool RayIntersection(const arcVec3 &start, const arcVec3 &dir,
                       float &scale) const;

  // most tight bounds for the given transformed bounds
  void FromTransformedBounds(const arcBounds &bounds, const arcVec3 &origin,
                             const arcMat3 &axis);
  // most tight bounds for a point set
  void FromPoints(const arcVec3 *points, const int numPoints);
  // most tight bounds for a translation
  void FromPointTranslation(const arcVec3 &point, const arcVec3 &translation);
  void FromBoundsTranslation(const arcBounds &bounds, const arcVec3 &origin,
                             const arcMat3 &axis, const arcVec3 &translation);
  // most tight bounds for a rotation
  void FromPointRotation(const arcVec3 &point, const arcRotate &rotation);
  void FromBoundsRotation(const arcBounds &bounds, const arcVec3 &origin,
                          const arcMat3 &axis, const arcRotate &rotation);

  void ToPoints(arcVec3 points[8]) const;
  ARCSphere ToSphere(void) const;
  // float			LongestDistance( const ARVec3 &point ) const;

  void AxisProjection(const arcVec3 &dir, float &min, float &max) const;
  void AxisProjection(const arcVec3 &origin, const arcMat3 &axis,
                      const arcVec3 &dir, float &min, float &max) const;

  int GetDimension() const;

  const float *ToFloatPtr() const;
  float *ToFloatPtr();

private:
  arcVec3 b[2];
};

extern arcBounds bounds_zero;
extern arcBounds bounds_zeroOneCube;
extern arcBounds bounds_unitCube;

ARC_INLINE arcBounds::arcBounds(void) {}

ARC_INLINE arcBounds::arcBounds(const arcVec3 &mins, const arcVec3 &maxs) {
  b[0] = mins;
  b[1] = maxs;
}

ARC_INLINE arcBounds::arcBounds(const arcVec3 &point) {
  b[0] = point;
  b[1] = point;
}

ARC_INLINE const arcVec3 &arcBounds::operator[]( const int index ) const {
  return b[index];
}

ARC_INLINE arcVec3 &arcBounds::operator[](const int index) { return b[index]; }

ARC_INLINE arcBounds arcBounds::operator+( const arcVec3 &r ) const {
  return arcBounds(b[0] + t, b[1] + t);
}

ARC_INLINE arcBounds &arcBounds::operator+=( const arcVec3 &r ) {
  b[0] += t;
  b[1] += t;
  return *this;
}

ARC_INLINE arcBounds arcBounds::operator*( const arcMat3 &r ) const {
  arcBounds bounds.FromTransformedBounds(*this, vec3_origin, r);
  return bounds;
}

ARC_INLINE arcBounds &arcBounds::operator*=( const arcMat3 &r ) {
  this->FromTransformedBounds(*this, vec3_origin, r);
  return *this;
}

ARC_INLINE arcBounds arcBounds::operator+(const arcBounds &a) const {
  arcBounds newBounds = *this;
  newBounds.AddBounds(a);
  return newBounds;
}

ARC_INLINE arcBounds &arcBounds::operator+=(const arcBounds &a) {
  arcBounds::AddBounds(a);
  return *this;
}

ARC_INLINE arcBounds arcBounds::operator-(const arcBounds &a) const {
  assert(b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
         b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
         b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2]);
  return arcBounds(
      arcVec3(b[0][0] + a.b[1][0], b[0][1] + a.b[1][1], b[0][2] + a.b[1][2]),
      arcVec3(b[1][0] + a.b[0][0], b[1][1] + a.b[0][1], b[1][2] + a.b[0][2]));
}

ARC_INLINE arcBounds &arcBounds::operator-=(const arcBounds &a) {
  assert(b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
         b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
         b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2]);
  b[0] += a.b[1];
  b[1] += a.b[0];
  return *this;
}

ARC_INLINE bool arcBounds::Compare(const arcBounds &a) const {
  return (b[0].Compare(a.b[0]) && b[1].Compare(a.b[1]));
}

ARC_INLINE bool arcBounds::Compare(const arcBounds &a,
                                   const float epsilon) const {
  return (b[0].Compare(a.b[0], epsilon) && b[1].Compare(a.b[1], epsilon));
}

ARC_INLINE bool arcBounds::operator==(const arcBounds &a) const {
  return Compare(a);
}

ARC_INLINE bool arcBounds::operator!=(const arcBounds &a) const {
  return !Compare(a);
}

ARC_INLINE void arcBounds::Clear() {
  b[0][0] = b[0][1] = b[0][2] = arcMath::INFINITY;
  b[1][0] = b[1][1] = b[1][2] = -arcMath::INFINITY;
}

ARC_INLINE void ClearBounds(arcVec3 &mins, arcVec3 &maxs) {
  mins[0] = mins[1] = mins[2] = 99999;  //= arcMath::INFINITY;
  maxs[0] = maxs[1] = maxs[2] = -99999; //= -arcMath::INFINITY;
}

ARC_INLINE void arcBounds::Zero() {
  b[0][0] = b[0][1] = b[0][2] = b[1][0] = b[1][1] = b[1][2] = 0;
}

ARC_INLINE arcVec3 arcBounds::GetCenter() const {
  return arcVec3((b[1][0] + b[0][0]) * 0.5f, (b[1][1] + b[0][1]) * 0.5f,
                 (b[1][2] + b[0][2]) * 0.5f);
}

ARC_INLINE float arcBounds::GetVolume() const {
  if (b[0][0] >= b[1][0] || b[0][1] >= b[1][1] || b[0][2] >= b[1][2]) {
    return 0.0f;
  }
  return ((b[1][0] - b[0][0]) * (b[1][1] - b[0][1]) * (b[1][2] - b[0][2]));
}

ARC_INLINE bool arcBounds::IsCleared() const { return b[0][0] > b[1][0]; }

ARCC_INLINE void arcBounds::AddPointToBounds(const arcVec3 &v, arcVec3 &mins,
                                             arcVec3 &maxs) {
  for (int i = 0; i < 3; i++) {
    float val = v[i];
    if (val < mins[i]) {
      mins[i] = val;
    }

    if (val > maxs[i]) {
      maxs[i] = val;
    }
  }
}

ARCC_INLINE bool arcBounds::AddPoint(const arcVec3 &v) {
  bool expanded = false;
  if (v[0] < b[0][0]) {
    b[0][0] = v[0];
    expanded = true;
  }
  if (v[0] > b[1][0]) {
    b[1][0] = v[0];
    expanded = true;
  }
  if (v[1] < b[0][1]) {
    b[0][1] = v[1];
    expanded = true;
  }
  if (v[1] > b[1][1]) {
    b[1][1] = v[1];
    expanded = true;
  }
  if (v[2] < b[0][2]) {
    b[0][2] = v[2];
    expanded = true;
  }
  if (v[2] > b[1][2]) {
    b[1][2] = v[2];
    expanded = true;
  }
  return expanded;
}

ARC_INLINE bool arcBounds::AddBounds(const arcBounds &a) {
  bool expanded = false;
  if (a.b[0][0] < b[0][0]) {
    b[0][0] = a.b[0][0];
    expanded = true;
  }
  if (a.b[0][1] < b[0][1]) {
    b[0][1] = a.b[0][1];
    expanded = true;
  }
  if (a.b[0][2] < b[0][2]) {
    b[0][2] = a.b[0][2];
    expanded = true;
  }
  if (a.b[1][0] > b[1][0]) {
    b[1][0] = a.b[1][0];
    expanded = true;
  }
  if (a.b[1][1] > b[1][1]) {
    b[1][1] = a.b[1][1];
    expanded = true;
  }
  if (a.b[1][2] > b[1][2]) {
    b[1][2] = a.b[1][2];
    expanded = true;
  }
  return expanded;
}

ARC_INLINE arcBounds arcBounds::Intersect(const arcBounds &a) const {
  arcBounds n;
  n.b[0][0] = (a.b[0][0] > b[0][0]) ? a.b[0][0] : b[0][0];
  n.b[0][1] = (a.b[0][1] > b[0][1]) ? a.b[0][1] : b[0][1];
  n.b[0][2] = (a.b[0][2] > b[0][2]) ? a.b[0][2] : b[0][2];
  n.b[1][0] = (a.b[1][0] < b[1][0]) ? a.b[1][0] : b[1][0];
  n.b[1][1] = (a.b[1][1] < b[1][1]) ? a.b[1][1] : b[1][1];
  n.b[1][2] = (a.b[1][2] < b[1][2]) ? a.b[1][2] : b[1][2];
  return n;
}

ARC_INLINE arcBounds &arcBounds::IntersectSelf(const arcBounds &a) {
  if (a.b[0][0] > b[0][0]) {
    b[0][0] = a.b[0][0];
  }
  if (a.b[0][1] > b[0][1]) {
    b[0][1] = a.b[0][1];
  }
  if (a.b[0][2] > b[0][2]) {
    b[0][2] = a.b[0][2];
  }
  if (a.b[1][0] < b[1][0]) {
    b[1][0] = a.b[1][0];
  }
  if (a.b[1][1] < b[1][1]) {
    b[1][1] = a.b[1][1];
  }
  if (a.b[1][2] < b[1][2]) {
    b[1][2] = a.b[1][2];
  }
  return *this;
}

ARC_INLINE arcBounds arcBounds::Expand(const float d) const {
  return arcBounds(arcVec3(b[0][0] - d, b[0][1] - d, b[0][2] - d),
                   arcVec3(b[1][0] + d, b[1][1] + d, b[1][2] + d));
}

ARC_INLINE arcBounds &arcBounds::ExpandSelf(const float d) {
  b[0][0] -= d;
  b[0][1] -= d;
  b[0][2] -= d;
  b[1][0] += d;
  b[1][1] += d;
  b[1][2] += d;
  return *this;
}

ARC_INLINE arcBounds arcBounds::Translate(const arcVec3 &translation) const {
  return arcBounds(b[0] + translation, b[1] + translation);
}

ARC_INLINE arcBounds &arcBounds::TranslateSelf(const arcVec3 &translation) {
  b[0] += translation;
  b[1] += translation;
  return *this;
}

ARC_INLINE arcBounds arcBounds::Rotate(const arcMat3 &rotation) const {
  arcBounds bounds.FromTransformedBounds(*this, vec3_origin, rotation);
  return bounds;
}

ARC_INLINE arcBounds &arcBounds::RotateSelf(const arcMat3 &rotation) {
  FromTransformedBounds(*this, vec3_origin, rotation);
  return *this;
}

ARC_INLINE bool arcBounds::ContainsPoint(const arcVec3 &p) const {
  if (p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2] || p[0] > b[1][0] ||
      p[1] > b[1][1] || p[2] > b[1][2]) {
    return false;
  }
  return true;
}

ARC_INLINE bool arcBounds::IntersectsBounds(const arcBounds &a) const {
  if (a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2] ||
      a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2]) {
    return false;
  }
  return true;
}

ARC_INLINE ARCSphere arcBounds::ToSphere(void) const {
  ARCSphere sphere.SetOrigin((b[0] + b[1]) * 0.5f);
  sphere.SetRadius((b[1] - sphere.GetOrigin()).Length());
  return sphere;
}

ARC_INLINE void arcBounds::AxisProjection(const arcVec3 &dir, float &min,
                                          float &max) const {
  arcVec3 center, extents;

  center = (b[0] + b[1]) * 0.5f;
  extents = b[1] - center;

  float d1 = dir * center;
  float d2 = arcMath::Fabs(extents[0] * dir[0]) +
             arcMath::Fabs(extents[1] * dir[1]) +
             arcMath::Fabs(extents[2] * dir[2]);

  min = d1 - d2;
  max = d1 + d2;
}

ARC_INLINE void arcBounds::AxisProjection(const arcVec3 &origin,
                                          const arcMat3 &axis,
                                          const arcVec3 &dir, float &min,
                                          float &max) const {
  arcVec3 center, extents;

  center = (b[0] + b[1]) * 0.5f;
  extents = b[1] - center;
  center = origin + center * axis;

  float d1 = dir * center;
  float d2 = arcMath::Fabs(extents[0] * (dir * axis[0])) +
             arcMath::Fabs(extents[1] * (dir * axis[1])) +
             arcMath::Fabs(extents[2] * (dir * axis[2]));

  min = d1 - d2;
  max = d1 + d2;
}

ARC_INLINE int arcBounds::GetDimension() const { return 6; }

ARC_INLINE const float *arcBounds::ToFloatPtr() const { return &b[0].x; }

ARC_INLINE float *arcBounds::ToFloatPtr() { return &b[0].x; }

#endif