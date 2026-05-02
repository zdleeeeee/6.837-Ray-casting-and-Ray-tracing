#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>


Renderer::Renderer(const ArgParser &args) :
    _args(args),
    _scene(args.input_file)
{
}

void
Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and callse traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera* cam = _scene.getCamera();
    for (int y = 0; y < h; ++y) {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x) {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            // Use PerspectiveCamera to generate a ray.
            // You should understand what generateRay() does.
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
            float range = (_args.depth_max - _args.depth_min);
            if (range) {
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
        }
    }
    // END SOLN

    // save the files 
    if (_args.output_file.size()) {
        image.savePNG(_args.output_file);
    }
    if (_args.depth_file.size()) {
        dimage.savePNG(_args.depth_file);
    }
    if (_args.normals_file.size()) {
        nimage.savePNG(_args.normals_file);
    }
}



Vector3f
Renderer::traceRay(const Ray &r,
    float tmin,
    int bounces,
    Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.

    // TODO: IMPLEMENT 
    if (_scene.getGroup()->intersect(r, tmin, h)) {
        // 获取材质
        Material* material = h.getMaterial();

        // 获取交点坐标
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        
        // 遍历所有光源，累加漫反射和镜面反射
        Vector3f directLight = Vector3f::ZERO;
        int numLights = _scene.getNumLights();
        for (int i = 0; i < numLights; i++) {
            // 获取从交点到光源的方向、光照强度、距离
            Light* light = _scene.getLight(i);
            Vector3f toLight;
            Vector3f lightIntensity;
            float distToLight;
            
            light->getIllumination(hitPoint, toLight, lightIntensity, distToLight);
            
            // 调用 Material::shade() 计算该光源的贡献
            Vector3f lightContribution = material->shade(r, h, toLight, lightIntensity);
            directLight += lightContribution;
        }
        
        // 获取环境光
        Vector3f ambient = _scene.getAmbientLight();

        // 最终颜色 = 环境光 + 直接光照
        return ambient + directLight;
    } else {
        // 返回背景色
        return _scene.getBackgroundColor(r.getDirection());
    };
}

