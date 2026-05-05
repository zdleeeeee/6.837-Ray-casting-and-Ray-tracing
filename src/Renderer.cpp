#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "Light.h"

#include <limits>
#include <cstdlib>
#include <ctime>
#include <cmath>


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

    // Supersampling factor for anti-aliasing
    int supersampleFactor = (_args.filter || _args.jitter) ? 3 : 1;
    int samplesPerPixel = _args.jitter ? 16 : 1;

    // Render at higher resolution for anti-aliasing
    int renderW = w * supersampleFactor;
    int renderH = h * supersampleFactor;

    Image image(renderW, renderH);
    Image nimage(renderW, renderH);
    Image dimage(renderW, renderH);

    // Initialize random seed for jitter sampling
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    Camera* cam = _scene.getCamera();

    // Render at supersampled resolution
    for (int y = 0; y < renderH; ++y) {
        for (int x = 0; x < renderW; ++x) {
            Vector3f accumulatedColor(0, 0, 0);
            Vector3f accumulatedNormal(0, 0, 0);
            float accumulatedDepth = 0;
            int validSamples = 0;

            // Jitter sampling: take multiple samples per pixel with random offsets
            for (int s = 0; s < samplesPerPixel; ++s) {
                // Calculate normalized device coordinates with jitter
                float jitterX = _args.jitter ? static_cast<float>(std::rand()) / RAND_MAX : 0.5f;
                float jitterY = _args.jitter ? static_cast<float>(std::rand()) / RAND_MAX : 0.5f;

                float ndcx = 2.0f * ((x + jitterX) / renderW) - 1.0f;
                float ndcy = 2.0f * ((y + jitterY) / renderH) - 1.0f;

                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                Hit hit;
                Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, hit);

                accumulatedColor += color;
                if (hit.getMaterial() != nullptr) {
                    accumulatedNormal += (hit.getNormal() + 1.0f) / 2.0f;
                    accumulatedDepth += hit.t;
                    validSamples++;
                }
            }

            // Average the samples
            Vector3f avgColor = accumulatedColor / static_cast<float>(samplesPerPixel);
            image.setPixel(x, y, avgColor);

            if (validSamples > 0) {
                nimage.setPixel(x, y, accumulatedNormal / static_cast<float>(validSamples));
                dimage.setPixel(x, y, Vector3f(accumulatedDepth / static_cast<float>(validSamples)));
            }
        }
    }

    // Apply Gaussian filter and downsample if filter is enabled
    if (_args.filter && supersampleFactor > 1) {
        Image filteredImage(w, h);
        Image filteredNImage(w, h);
        Image filteredDImage(w, h);

        // 3x3 Gaussian kernel (sigma = 1)
        float gaussianKernel[3][3] = {
            {1.0f, 2.0f, 1.0f},
            {2.0f, 4.0f, 2.0f},
            {1.0f, 2.0f, 1.0f}
        };
        float kernelSum = 16.0f;

        // Apply Gaussian filter and downsample
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Vector3f filteredColor(0, 0, 0);
                Vector3f filteredNormal(0, 0, 0);
                Vector3f filteredDepth(0, 0, 0);

                for (int ky = 0; ky < 3; ++ky) {
                    for (int kx = 0; kx < 3; ++kx) {
                        int sampleX = x * supersampleFactor + (kx - 1);
                        int sampleY = y * supersampleFactor + (ky - 1);

                        sampleX = std::max(0, std::min(sampleX, renderW - 1));
                        sampleY = std::max(0, std::min(sampleY, renderH - 1));

                        filteredColor += image.getPixel(sampleX, sampleY) * gaussianKernel[ky][kx];
                        filteredNormal += nimage.getPixel(sampleX, sampleY) * gaussianKernel[ky][kx];
                        filteredDepth += dimage.getPixel(sampleX, sampleY) * gaussianKernel[ky][kx];
                    }
                }

                filteredImage.setPixel(x, y, filteredColor / kernelSum);
                filteredNImage.setPixel(x, y, filteredNormal / kernelSum);
                filteredDImage.setPixel(x, y, filteredDepth / kernelSum);
            }
        }

        image = filteredImage;
        nimage = filteredNImage;
        dimage = filteredDImage;
    } else if (supersampleFactor > 1) {
        // Just downsample without filter
        Image downsampledImage(w, h);
        Image downsampledNImage(w, h);
        Image downsampledDImage(w, h);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int sampleX = x * supersampleFactor + supersampleFactor / 2;
                int sampleY = y * supersampleFactor + supersampleFactor / 2;
                sampleX = std::max(0, std::min(sampleX, renderW - 1));
                sampleY = std::max(0, std::min(sampleY, renderH - 1));

                downsampledImage.setPixel(x, y, image.getPixel(sampleX, sampleY));
                downsampledNImage.setPixel(x, y, nimage.getPixel(sampleX, sampleY));
                downsampledDImage.setPixel(x, y, dimage.getPixel(sampleX, sampleY));
            }
        }

        image = downsampledImage;
        nimage = downsampledNImage;
        dimage = downsampledDImage;
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
    if (!_scene.getGroup()->intersect(r, tmin, h)) {
        // No intersection, return background color
        return _scene.getBackgroundColor(r.getDirection());
    }

    // Intersection found, compute Phong shading
    Material* mat = h.getMaterial();
    Vector3f hitPoint = r.pointAtParameter(h.t);
    Vector3f N = h.getNormal().normalized();

    // Ambient light
    Vector3f ambient = _scene.getAmbientLight() * mat->getAmbientColor();

    // Direct lighting from all lights
    Vector3f directLight = Vector3f(0, 0, 0);
    int numLights = _scene.getNumLights();

    for (int i = 0; i < numLights; ++i) {
        Light* light = _scene.getLight(i);

        // Get illumination info from light
        Vector3f dirToLight;
        Vector3f lightIntensity;
        float distToLight;
        light->getIllumination(hitPoint, dirToLight, lightIntensity, distToLight);

        // Shadow check
        bool inShadow = false;
        if (_args.shadows) {
            // Cast a shadow ray from hit point to light
            // Offset the origin slightly to avoid self-intersection
            float shadowTmin = 0.001f;
            float shadowTmax = distToLight;
            Ray shadowRay(hitPoint + dirToLight * shadowTmin, dirToLight);
            Hit shadowHit;

            // Check if there's any intersection between hit point and light
            if (_scene.getGroup()->intersect(shadowRay, shadowTmin, shadowHit)) {
                if (shadowHit.getT() < shadowTmax) {
                    inShadow = true;
                }
            }
        }

        // If not in shadow, compute Phong shading
        if (!inShadow) {
            directLight += mat->shade(r, h, dirToLight, lightIntensity);
        }
    }

    // Total direct illumination
    Vector3f totalDirect = ambient + directLight;

    // Recursive ray tracing for reflections
    Vector3f indirectLight = Vector3f(0, 0, 0);
    Vector3f specColor = mat->getSpecularColor();
    float len = sqrt(specColor.x() * specColor.x() + specColor.y() * specColor.y() + specColor.z() * specColor.z());
    if (bounces > 0 && len > 0.001f){
        // Compute perfect reflection direction
        Vector3f V = -r.getDirection().normalized();
        Vector3f R = 2.0f * Vector3f::dot(N, V) * N - V;
        R = R.normalized();

        // Create reflected ray, offset origin to avoid self-intersection
        Ray reflectedRay(hitPoint + N * 0.001f, R);
        Hit reflectedHit;

        // Recursively trace the reflected ray
        Vector3f reflectedColor = traceRay(reflectedRay, tmin, bounces - 1, reflectedHit);

        // Weight by specular reflectivity
        indirectLight = reflectedColor * mat->getSpecularColor();
    }

    // Total illumination = direct + indirect
    return totalDirect + indirectLight;
}

