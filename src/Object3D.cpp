#include "Object3D.h"

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center;      //Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0) {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f*a);
    float tminus = (-b - d) / (2.0f*a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin)) {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin) {
        t = tminus;
    }

    // one intersection at the front. one at the back 
    if ((tplus > tmin) && (tminus < tmin)) {
        t = tplus;
    }

    if (t < h.getT()) {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj) {
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const {
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;
    for (Object3D* o : m_members) {
        if (o->intersect(r, tmin, h)) {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}


Plane::Plane(const Vector3f &normal, float d, Material *m) : Object3D(m) {
    _normal = normal.normalized();
    _d = d;
}

bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    // Plane equation: P·n = d
    // Ray equation: P = o + t*d
    // Intersection: (o + t*d)·n = d
    // t = (d - o·n) / d·n
    
    const Vector3f& rayOrigin = r.getOrigin();
    const Vector3f& dir = r.getDirection();
    
    float denom = Vector3f::dot(dir, _normal);
    
    // Ray is parallel to plane
    if (std::abs(denom) < 1e-6f) {
        return false;
    }
    
    float t = (_d - Vector3f::dot(rayOrigin, _normal)) / denom;
    
    // Check if intersection is in valid range
    if (t > tmin && t < h.getT()) {
        Vector3f hitPoint = r.pointAtParameter(t);
        Vector3f normal = _normal;
        
        // Flip normal if ray is hitting from behind
        if (Vector3f::dot(dir, normal) > 0) {
            normal = -normal;
        }
        
        h.set(t, this->material, normal);
        return true;
    }
    
    return false;
}
bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
{
    // Möller-Trumbore algorithm for ray-triangle intersection
    // Ray: R(t) = O + t*D
    // Triangle: P = A + u*(B-A) + v*(C-A), where u>=0, v>=0, u+v<=1

    const Vector3f& O = r.getOrigin();
    const Vector3f& D = r.getDirection();

    Vector3f edge1 = _v[1] - _v[0];
    Vector3f edge2 = _v[2] - _v[0];

    // Calculate determinant
    Vector3f crossH = Vector3f::cross(D, edge2);
    float det = Vector3f::dot(edge1, crossH);

    // If determinant is near zero, ray lies in plane of triangle
    if (std::abs(det) < 1e-6f) {
        return false;
    }

    float invDet = 1.0f / det;

    // Calculate u parameter
    Vector3f s = O - _v[0];
    float u = invDet * Vector3f::dot(s, crossH);

    // Check if intersection is outside triangle
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    // Calculate v parameter
    Vector3f q = Vector3f::cross(s, edge1);
    float v = invDet * Vector3f::dot(D, q);

    // Check if intersection is outside triangle
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // Calculate t value
    float t = invDet * Vector3f::dot(edge2, q);

    // Check if intersection is in valid range
    if (t > tmin && t < h.getT()) {
        Vector3f hitPoint = r.pointAtParameter(t);

        // Interpolate normal using barycentric coordinates
        float w = 1.0f - u - v;
        Vector3f normal = w * _normals[0] + u * _normals[1] + v * _normals[2];
        normal = normal.normalized();

        h.set(t, this->material, normal);
        return true;
    }

    return false;
}


Transform::Transform(const Matrix4f &m,
    Object3D *obj) : Object3D(obj->getMaterial()), _object(obj) {
    _matrix = m;
    _inverseMatrix = m.inverse();
    _normalMatrix = m.inverse().transposed();
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    // Transform ray from world space to object space
    const Vector3f& rayOrigin = r.getOrigin();
    const Vector3f& rayDir = r.getDirection();
    
    // Transform ray origin to object space
    Vector4f origin4(rayOrigin.x(), rayOrigin.y(), rayOrigin.z(), 1.0f);
    Vector4f transformedOrigin4 = _inverseMatrix * origin4;
    Vector3f objectSpaceOrigin(transformedOrigin4.x(), transformedOrigin4.y(), transformedOrigin4.z());
    
    // Transform ray direction to object space (w=0 for vectors)
    Vector4f dir4(rayDir.x(), rayDir.y(), rayDir.z(), 0.0f);
    Vector4f transformedDir4 = _inverseMatrix * dir4;
    Vector3f objectSpaceDir(transformedDir4.x(), transformedDir4.y(), transformedDir4.z());
    
    // Create transformed ray in object space
    Ray objectSpaceRay(objectSpaceOrigin, objectSpaceDir);
    
    // Intersect with the object in object space
    Hit objectSpaceHit;
    objectSpaceHit.set(std::numeric_limits<float>::max(), nullptr, Vector3f(0, 0, 0));
    
    if (!_object->intersect(objectSpaceRay, tmin, objectSpaceHit)) {
        return false;
    }
    
    // Transform hit point back to world space
    float t = objectSpaceHit.getT();
    Vector3f objectSpaceNormal = objectSpaceHit.getNormal();
    
    // Transform normal from object space to world space using inverse transpose
    Vector4f normal4(objectSpaceNormal.x(), objectSpaceNormal.y(), objectSpaceNormal.z(), 0.0f);
    Vector4f worldNormal4 = _normalMatrix * normal4;
    Vector3f worldNormal(worldNormal4.x(), worldNormal4.y(), worldNormal4.z());
    worldNormal = worldNormal.normalized();
    
    // Update the original hit with world space values
    h.set(t, _object->getMaterial(), worldNormal);
    
    return true;
}