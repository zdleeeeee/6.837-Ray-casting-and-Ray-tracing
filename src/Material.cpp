#include "Material.h"
Vector3f Material::shade(const Ray &ray,
    const Hit &hit,
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
    // TODO implement Diffuse and Specular phong terms
    // return Vector3f(0, 0, 0);

    // get normal vector of the hit point
    Vector3f N = hit.getNormal().normalized();
    // get direction to light (point to light)
    Vector3f L = dirToLight.normalized();
    
    // compute diffuse term
    // diffuse = k_diffuse * I_light * max(0, N·L)
    float LNdot = Vector3f::dot(L, N);
    Vector3f diffuse = Vector3f::ZERO;
    if (LNdot > 0) {
        diffuse = LNdot * lightIntensity * _diffuseColor;
    }
    
    // compute specular term
    // specular = k_specular * I_light * max(0, R·V)^shininess
    Vector3f specular = Vector3f::ZERO;
    if (LNdot > 0 && _shininess > 0) {
        // compute view vector (from hit point to camera = -ray.getDirection().normalized())
        Vector3f E = -ray.getDirection().normalized();
        
        // compute perfect reflection vector
        // R = 2(N·E)N - E
        Vector3f R = 2 * Vector3f::dot(N, E) * N - E;
        R = R.normalized();
        
        // compute reflection dot product
        float LRdot = Vector3f::dot(L, R);
        if (LRdot > 0) {
            float specularFactor = pow(LRdot, _shininess);
            specular = specularFactor * lightIntensity * _specularColor;
        }
    }
    
    // return final color
    return diffuse + specular;
}