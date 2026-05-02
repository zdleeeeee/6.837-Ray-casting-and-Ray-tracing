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
    // TODO implement Plane constructor
    _normal = normal.normalized();
    _d = d;
}
bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    // TODO implement
    // d是ray的方向
    float dn = Vector3f::dot(r.getDirection(), _normal);
    
    // 光线与平面平行（方向与法线垂直），无交点
    if (fabs(dn) < 1e-6) {
        return false;
    }
    
    // (p' - o) dot N = p' dot N - o dot N = _d - o dot N
    // 所以t = (p' - o) dot N / dn = (_d - o dot N) / dn
    float t = (_d - Vector3f::dot(r.getOrigin(), _normal)) / dn;
    
    // 检查 t 是否在有效范围内
    if (t < tmin || t > h.getT()) {
        return false;
    }
    
    h.set(t, this->material, _normal);
    return true;
}
bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const 
{
    // TODO implement
    // 使用 Möller-Trumbore 算法
    
    // 计算两条边
    Vector3f e1 = _v[1] - _v[0];
    Vector3f e2 = _v[2] - _v[0];
    
    // 计算光线方向与 e2 的叉积
    Vector3f s1 = Vector3f::cross(r.getDirection(), e2);
    
    // 计算行列式
    float divisor = Vector3f::dot(s1, e1);
    
    // 如果行列式为0，光线与三角形平面平行
    if (fabs(divisor) < 1e-6) {
        return false;
    }
    
    float invDivisor = 1.0f / divisor;
    
    // 计算从 _v[0] 到光线起点的向量
    Vector3f d = r.getOrigin() - _v[0];
    
    // 计算重心坐标 u
    float u = invDivisor * Vector3f::dot(s1, d);
    if (u < 0 || u > 1) {
        return false;
    }
    
    // 计算 v
    Vector3f s2 = Vector3f::cross(d, e1);
    float v = invDivisor * Vector3f::dot(r.getDirection(), s2);
    if (v < 0 || u + v > 1) {
        return false;
    }
    
    // 计算 t
    float t = invDivisor * Vector3f::dot(e2, s2);
    if (t < tmin || t > h.getT()) {
        return false;
    }
    
    // 计算交点处的法线（使用重心坐标插值）
    Vector3f normal = (1 - u - v) * _normals[0] + u * _normals[1] + v * _normals[2];
    normal = normal.normalized();
    
    h.set(t, this->material, normal);
    return true;
}


Transform::Transform(const Matrix4f &m,
    Object3D *obj) : _object(obj) {
    // TODO implement Transform constructor
    _matrix = m;
    _invMatrix = m.inverse();
    _invTranspose = _invMatrix.transposed();
}
bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    // TODO implement
    // 步骤：
    // 1. 将光线从世界坐标变换到局部坐标
    // 2. 在局部坐标中与子对象求交
    // 3. 将交点信息变换回世界坐标
    
    // 将光线原点变换到局部坐标
    Vector4f newOrigin4 = _invMatrix * Vector4f(r.getOrigin(), 1.0f);
    Vector3f newOrigin(newOrigin4.x(), newOrigin4.y(), newOrigin4.z());
    
    // 将光线方向变换到局部坐标（方向向量，w=0）
    Vector4f newDir4 = _invMatrix * Vector4f(r.getDirection(), 0.0f);
    Vector3f newDir(newDir4.x(), newDir4.y(), newDir4.z());
    newDir = newDir.normalized();
    
    // 创建局部坐标系中的光线
    Ray localRay(newOrigin, newDir);
    
    // 创建局部坐标系中的 Hit 对象
    Hit localHit;
    
    // 在局部坐标系中与子对象求交
    if (!_object->intersect(localRay, tmin, localHit)) {
        return false;
    }
    
    // 将交点 t 值变换回世界坐标
    // t 值在局部坐标系和世界坐标系中相同（因为方向归一化后，距离不变）
    float worldT = localHit.getT();
    
    // 如果 worldT 比当前已存储的交点更远，则忽略
    if (worldT < tmin || worldT > h.getT()) {
        return false;
    }
    
    // 计算世界坐标系中的交点位置
    Vector3f worldPoint = r.pointAtParameter(worldT);
    
    // 将法线从局部坐标变换到世界坐标
    // 使用逆转置矩阵变换法线，然后归一化
    Vector4f localNormal(localHit.getNormal(), 0.0f);
    Vector4f worldNormal4 = _invTranspose * localNormal;
    Vector3f worldNormal(worldNormal4.x(), worldNormal4.y(), worldNormal4.z());
    worldNormal = worldNormal.normalized();
    
    // 更新 Hit 对象
    h.set(worldT, localHit.getMaterial(), worldNormal);
    return true;
}