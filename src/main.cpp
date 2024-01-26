#include "color.h"
#include "image.h"
#include "input.h"
#include "mathUtil.h"
#include "rays.h"
#include <iostream>
#include <cmath>
#include <limits>

#include <mutex>
#include <thread>

float refraction_index;
constexpr float kEpsilon = 1e-4f;
constexpr int MAX_DEPTH = 10;
Color bkg_color(0.0f, 0.0f, 0.0f);

#if 0
// Use this as example when doing TraceRay and TraceRayRecursive

#endif

double SchlickFresnel(double n1, double n2, double cosI, double cosT)
{
    double R0 = (n1 - n2) / (n1 + n2);
    R0 *= R0;
    double x = 1.0 - (n1 > n2 ? cosI : cosT);
    double x2 = x * x;
    double x5 = x2 * x2 * x;
    return R0 + (1.0 - R0) * x5;
}

RayResult IntersectScene(Ray &ray, const std::vector<std::shared_ptr<Object>> &objects)
{
    RayResult res;
    for (const auto &obj : objects)
    {
        auto temp_res = ray.Intersect(obj.get());
        if (temp_res.t > 0 && (res.t < 0 || temp_res.t < res.t))
        {
            res = temp_res;
            res.object = obj.get();
        }
    }
    return res;
}

Color TraceRay(Ray ray, int depth, InputFileData &input);

Color ShadeRay(const Object &object, InputFileData &input, const RayResult &ray_result,
               Ray &ray, Image *texture = nullptr, int depth = 0, std::vector<double> ior_stack = {1.0})
{
    if (depth > MAX_DEPTH)
    {
        return bkg_color;
    }
    Material material;
    Color diffuse;
    Vec3 object_normal;
    Vec3 view_dir = -Vec3::Normalize(ray.direction);
    auto lights = input.lights;
    auto objects = input.objects;

    auto intersection_point = ray.at(ray_result.t);
    if (object.type == ObjectType::SPHERE)
    {
        auto sphere = dynamic_cast<const Sphere &>(object);
        material = sphere.material;
        object_normal = Vec3::Normalize((intersection_point - sphere.pos) / sphere.radius);
        if (sphere.texture_index != -1 && texture != nullptr)
        {
            auto phi = acos(object_normal.z);
            auto theta = atan2(object_normal.y, object_normal.x);
            auto u = theta / (2 * M_PI) + 0.5;
            auto v = phi / M_PI;
            int x = std::round(u * texture->width);
            int y = std::round(v * texture->height);
            //clamp
            x = std::max(0, std::min(texture->width - 1, x));
            y = std::max(0, std::min(texture->height - 1, y));

            diffuse = texture->getPixel(x, y);
        }
    }
    else
    {
        auto face = dynamic_cast<const Face &>(object);
        Vec3 v1v0 = face.v1.pos - face.v0.pos;
        Vec3 v2v0 = face.v2.pos - face.v0.pos;
        material = face.v0.material;
        diffuse = material.diffuse;
        if (!face.has_normals)
        {
            object_normal = Vec3::Normalize(Vec3::Cross(v1v0, v2v0));
        }
        else
        {
            Vec3 interpolated_normal = ray_result.interpolated_normal;
            object_normal = Vec3::Normalize(interpolated_normal);
        }
        if (face.texture_index != -1 && texture != nullptr)
        {
            auto u = ray_result.interpolated_uv.x;
            auto v = ray_result.interpolated_uv.y;
            int x = std::round(u * texture->width);
            int y = std::round(v * texture->height);
            
            x = std::max(0, std::min(texture->width - 1, x));
            y = std::max(0, std::min(texture->height - 1, y));
            diffuse = texture->getPixel(x, y);


        }
    }

    Color ambient = diffuse * material.k_ambient;
    Color diffuse_sum;
    Color specular_sum;
    double eta = material.eta;
    double alpha = material.alpha;
    double ior = refraction_index;
    for (const auto &light : lights)
    {
        // Compute the direction to the light source
        Vec3 light_dir;
        float distance_to_light;
        if (light.type == LightType::DIRECTIONAL)
        {
            light_dir = -Vec3::Normalize(light.pos);
            distance_to_light = std::numeric_limits<float>::infinity();
        }
        else
        {
            light_dir = Vec3::Normalize(light.pos - intersection_point);
            distance_to_light = Vec3::Distance(intersection_point, light.pos);
        }

        // Cast a shadow ray towards the light source to check for occlusion
        Ray shadow_ray(intersection_point + light_dir * kEpsilon, light_dir);
        double shadow_opacity = 1.0;
        for (const auto &obj : objects)
        {
            if (&object == obj.get())
            {
                continue; // Skip the current triangle to avoid self-intersection
            }
            float t = shadow_ray.Intersect(obj.get()).t;
            if (t > 0 && t < distance_to_light)
            {
                shadow_opacity *= (1.0 - obj->material.alpha);
                if (shadow_opacity < 0.01)
                {
                    break;
                }
            }
        }
        shadow_opacity = std::clamp(shadow_opacity, 0.0, 1.0);
        // Compute the diffuse and specular contribution from the light source
        float diffuse_factor = std::max(0.0f, Vec3::Dot(object_normal, light_dir));
        Color diffuse_contribution = diffuse * diffuse_factor * light.color * shadow_opacity;

        Vec3 half_vec = Vec3::Normalize(light_dir + view_dir);
        float specular_factor = std::pow(std::max(0.0f, Vec3::Dot(object_normal, half_vec)), material.specular_exponent);
        Color specular_contribution = material.specular * specular_factor * light.color * shadow_opacity;
        diffuse_sum += diffuse_contribution;
        specular_sum += specular_contribution;
    }
    Color local_illumination = ambient + diffuse_sum * material.k_diffuse + specular_sum * material.k_specular;

    // Reflection and refraction
    double reflection_factor = material.k_specular;
    Color reflection_color = Color(0, 0, 0);
    Color refraction_color = Color(0, 0, 0);
    if (reflection_factor > 0)
    {
        Vec3 reflection_dir = Vec3::Reflect(ray.direction, object_normal);
        Ray reflection_ray(intersection_point + kEpsilon * reflection_dir, reflection_dir);
        reflection_color = TraceRay(reflection_ray, depth + 1, input) * reflection_factor;
    }
    if (material.alpha < 1.f)
    {
        bool entering = Vec3::Dot(object_normal, view_dir) > 0;
        double n1 = entering ? ior_stack.back() : material.eta;
        double n2;
        if (entering)
        {
            n2 = material.eta;
        }
        else
        {
            if (!ior_stack.empty())
            {
                n2 = ior_stack.back();
            }
            else
            {
                // Handle the case when ior_stack is empty.
                // For example, you can set n2 to a default value, or log an error and return a default color.
                return Color(0, 0, 0);
            }
        }

        double n = n1 / n2;
        double cosI = -Vec3::Dot(object_normal, view_dir);
        double sinT2 = n * n * (1.0 - cosI * cosI);
        double cosT = std::sqrt(1.0 - sinT2);
        double fresnel = SchlickFresnel(n1, n2, cosI, cosT);

        if (sinT2 > 1.0)
        {
            if (material.k_specular != 0.0) // Check if ks != 0
            {
                return reflection_color * fresnel;
            }
        }
        else
        {
            // Compute the refraction direction and ray
            Vec3 refraction_dir = Vec3::Normalize(n * view_dir + (n * cosI - cosT) * object_normal);
            Ray refraction_ray(intersection_point + kEpsilon * refraction_dir, refraction_dir);

            // Update the ior_stack based on whether the ray is entering or exiting the object
            if (entering)
            {
                ior_stack.push_back(material.eta);
            }
            else
            {
                ior_stack.pop_back();
            }

            // Trace the refracted ray if the material is not fully opaque
            if (material.alpha != 1.0)
            {
                if (texture != nullptr)
                {
                    refraction_color = ShadeRay(object, input, ray_result, refraction_ray, texture, depth + 1, ior_stack) * fresnel;
                }
                else
                {
                    refraction_color = ShadeRay(object, input, ray_result, refraction_ray, nullptr, depth + 1, ior_stack) * fresnel;
                }
            }
        }
        reflection_color = reflection_color * fresnel;
        refraction_color = refraction_color * (1.0 - fresnel);
    }

    return local_illumination + reflection_color + refraction_color;
}

Color TraceRay(Ray ray, int depth, InputFileData &input)
{
    if (depth >= MAX_DEPTH)
    {
        return input.bkgcolor;
    }

    double tMin = std::numeric_limits<double>::infinity();
    int minObj = -1;
    Object *obj = nullptr;

    // For each object, look for an intersection with the input ray.
    for (int i = 0; i < input.objects.size(); i++)
    {
        auto res = ray.Intersect(input.objects[i].get());
        if (res.t > 0 && res.t < tMin)
        {
            tMin = res.t;
            minObj = i;
            obj = input.objects[i].get();
        }
    }
    if (minObj == -1)
    {
        return input.bkgcolor;
    }

    // Compute the intersection point
    Vec3 intersection_point = ray.at(tMin);
    Image *texture = nullptr;
    if (obj->texture_index != -1)
    {
        texture = &input.texture[obj->texture_index];
    }
    auto color = ShadeRay(*obj, input, ray.Intersect(input.objects[minObj].get()), ray, texture, depth, std::vector<double>(input.index_of_refraction));
    return color;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " inputfile" << std::endl;
        exit(1);
    }
    InputFileData input = get_input(argv[1]);
    refraction_index = input.index_of_refraction;
    bkg_color = input.bkgcolor;
#if 1
    Image image(input.imsize.first, input.imsize.second);
    // fill image with background color
    image.fill(input.bkgcolor);
    Point eye = input.eye;
    Vec3 u = Vec3::Normalize(input.updir.Cross(input.viewdir));
    Vec3 v = Vec3::Normalize(u.Cross(input.viewdir));
    float hfov = input.hfov;
    float aspect = (float)input.imsize.first / (float)input.imsize.second;
    float width = 2 * tan((hfov / 2 * (M_PI / 180)));
    float height = width / aspect;
    const float d = 1;
    Point ul = eye + (Vec3::Normalize(input.viewdir) * d) - (u * (width / 2)) + (v * (height / 2));
    Point ur = eye + (Vec3::Normalize(input.viewdir) * d) + (u * (width / 2)) + (v * (height / 2));
    Point lr = eye + (Vec3::Normalize(input.viewdir) * d) + (u * (width / 2)) - (v * (height / 2));

    const int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_threads);

    auto render = [&](int start, int end)
    {
        for (int i = start; i < end; i++)
        {
            for (int j = 0; j < input.imsize.second; j++)
            {
                auto delta_h = (ur - ul) / ((float)input.imsize.first);
                Point p = lr - (delta_h * (float)i) + (v * height * ((float)j / (float)input.imsize.second));
                Ray ray = Ray(eye, Vec3::Normalize(p - eye));

                for (int k = 0; k < input.objects.size(); k++)
                {
                    auto color = TraceRay(ray, 1, input);
                    image.setPixel(i, j, Color::Clamp(color));
                }
            }
        }
    };

    // Assign work to threads
    int pixels_per_thread = input.imsize.first / num_threads;
    for (int t = 0; t < num_threads; t++)
    {
        int start = t * pixels_per_thread;
        int end = (t == num_threads - 1) ? input.imsize.first : start + pixels_per_thread;

        threads[t] = std::thread(render, start, end);
    }

    // Wait for all threads to complete their work
    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    // parse the file name from argv[1] if it contains . after the name
    std::string filename = argv[1];
    size_t pos = filename.find(".");
    if (pos != std::string::npos)
    {
        filename = filename.substr(0, pos);
    }
    // write the image to a file
    image.save(filename + ".ppm");
    std::cout << "Image saved to " << filename << ".ppm" << std::endl;
#else
    input_print_helper(input);
#endif
    return 0;
}
