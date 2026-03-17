#include <limits>
#include <cmath>
#include <optional>
#include <thread>
#include <vector>
#include "raylib.h"
#include "raymath.h"

class Sphere {
public:
    mutable Vector3 center{};
    float radius;
    Vector3 color{};
    bool lit;

    Sphere(const Vector3 center, const float radius, const Vector3 color, const bool lit) {
        this->center = center;
        this->radius = radius;
        this->color = color;
        this->lit = lit;
    }
};

constexpr auto O = Vector3(0.0f, 0.0f, 0.0f);
constexpr int screenWidth = 600;
constexpr int screenHeight = 600;
constexpr float viewportWidth = 1.0f;
constexpr float viewportHeight = 1.0f;
constexpr float viewportDistance = 1.0f;
constexpr auto fScreenWidth = static_cast<float>(screenWidth);
constexpr auto fScreenHeight = static_cast<float>(screenHeight);
constexpr auto backgroundColor = Vector3(30, 30, 30);
const auto ambientLight = Vector3(0.6, 0.5, 0.6);

const std::vector<Sphere> spheres = {
    Sphere(
        Vector3(0, 0, 10),
        0.7,
        Vector3(255, 150, 0),
        false
    ),
    Sphere(
        Vector3(1, 0, 10),
        0.3,
        Vector3(255, 0, 90),
        true
    ),
    Sphere(
        Vector3(1, 1, 10),
        0.5,
        Vector3(30, 255, 150),
        true
    ),
    Sphere(
        Vector3(1, 1, 10),
        0.45,
        Vector3(150, 0, 255),
        true
    )
};

Vector2 ScreenToCanvas(const Vector2 pos) {
    return Vector2(
        pos.x - fScreenWidth / 2.0f,
        pos.y - fScreenHeight / 2.0f
    );
}

Vector2 CanvasToScreen(const Vector2 pos) {
    return Vector2(
        pos.x + fScreenWidth / 2.0f,
        pos.y + fScreenHeight / 2.0f
    );
}

Vector3 CanvasToViewPortDirection(const int x, const int y) {
    return Vector3(
        static_cast<float>(x) * viewportWidth / fScreenWidth,
        static_cast<float>(y) * viewportHeight / fScreenHeight,
        viewportDistance
    );
}

Vector2 IntersectRaySphere(const Vector3 O, const Vector3 D, const Sphere sphere) {
    const float r = sphere.radius;
    const Vector3 CO = Vector3Subtract(O, sphere.center);

    const float a = Vector3DotProduct(D, D);
    const float b = 2*Vector3DotProduct(CO, D);
    const float c = Vector3DotProduct(CO, CO) - r*r;

    const float discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        return Vector2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    const float t1 = (-b + sqrt(discriminant)) / (2*a);
    const float t2 = (-b - sqrt(discriminant)) / (2*a);

    return Vector2(t1, t2);
}

Vector3 DiffuseLight(const Vector3 O, const Vector3 D, const float t, const std::optional<Sphere> &sphere) {
    const Vector3 intersection_point = Vector3Add(O, Vector3Scale(D, t));
    const Vector3 normal = Vector3Normalize(Vector3Subtract(intersection_point, sphere->center));
    const Vector3 dirToSun = Vector3Normalize(Vector3Subtract(spheres[0].center, intersection_point));
    const float light_intensity = Clamp(Vector3DotProduct(normal, dirToSun), 0.0f, 1.0f);
    return Vector3(light_intensity, light_intensity, light_intensity);
}

float FresnelSchlick(const float cos_theta, const float F0) {
    const float one_minus_cos = 1.0f - Clamp(cos_theta, 0.0f, 1.0f);
    return F0 + (1.0f - F0) * std::pow(one_minus_cos, 5.0f);
}

Vector3 ApplyFresnel(const Vector3 base_color, const Vector3 normal, const Vector3 view_dir) {
    const float cos_theta = Vector3DotProduct(normal, view_dir);
    const float fresnel = FresnelSchlick(cos_theta, 0.08f);
    const float fresnel_strength = 0.75f;
    return Vector3Lerp(base_color, Vector3(255.0f, 255.0f, 255.0f), fresnel * fresnel_strength);
}

Vector3 TraceRay(const Vector3 O, const Vector3 D, const float t_min, const float t_max) {
    float closest_t = std::numeric_limits<float>::max();
    std::optional<Sphere> closest_sphere;

    for (const auto& sphere : spheres) {
        const Vector2 intersection = IntersectRaySphere(O, D, sphere);
        const float t1 = intersection.x;
        const float t2 = intersection.y;

        if (t1 > t_min && t1 < t_max && t1 < closest_t) {
            closest_t = t1;
            closest_sphere = sphere;
        }

        if (t2 > t_min && t2 < t_max && t2 < closest_t) {
            closest_t = t2;
            closest_sphere = sphere;
        }
    }

    if (closest_sphere.has_value()) {
        const Vector3 hit_point = Vector3Add(O, Vector3Scale(D, closest_t));
        const Vector3 normal = Vector3Normalize(Vector3Subtract(hit_point, closest_sphere->center));
        const Vector3 view_dir = Vector3Normalize(Vector3Negate(D));

        Vector3 base_color = closest_sphere->color;
        if (closest_sphere->lit) {
            const Vector3 diffuse = DiffuseLight(O, D, closest_t, closest_sphere);
            const Vector3 light = Vector3Clamp(Vector3Add(ambientLight, diffuse), Vector3(0, 0, 0), Vector3(1, 1, 1));
            base_color = Vector3Multiply(base_color, light);
        }

        return ApplyFresnel(base_color, normal, view_dir);
    }

    return Vector3(backgroundColor.x, backgroundColor.y, backgroundColor.z);
}

Color ToColor(const Vector3 color) {
    return Color(
        static_cast<unsigned char>(Clamp(color.x, 0.0f, 255.0f)),
        static_cast<unsigned char>(Clamp(color.y, 0.0f, 255.0f)),
        static_cast<unsigned char>(Clamp(color.z, 0.0f, 255.0f)),
        255
    );
}

void RenderFrameParallel(std::vector<Color>& framebuffer) {
    const unsigned int hw_threads = std::thread::hardware_concurrency();
    const unsigned int thread_count = hw_threads == 0 ? 4 : hw_threads;
    const int rows_per_thread = screenHeight / static_cast<int>(thread_count);
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (unsigned int i = 0; i < thread_count; ++i) {
        const int y_start = static_cast<int>(i) * rows_per_thread;
        const int y_end = (i == thread_count - 1) ? screenHeight : y_start + rows_per_thread;

        workers.emplace_back([&framebuffer, y_start, y_end]() {
            for (int py = y_start; py < y_end; ++py) {
                const int y_canvas = py - screenHeight / 2;
                for (int px = 0; px < screenWidth; ++px) {
                    const int x_canvas = px - screenWidth / 2;
                    const Vector3 direction = CanvasToViewPortDirection(x_canvas, y_canvas);
                    const Vector3 color = TraceRay(O, direction, 1, std::numeric_limits<float>::max());
                    framebuffer[py * screenWidth + px] = ToColor(color);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Software Renderer");

    SetTargetFPS(999);
    std::vector<Color> framebuffer(static_cast<size_t>(screenWidth) * static_cast<size_t>(screenHeight));
    const std::vector<Vector3> og_pos = {};

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // do orbit
        for (size_t i = 1; i < spheres.size(); i++) {
            const float angle = GetTime() / static_cast<double>(i); // Adjust speed by changing multiplier
            spheres[i].center = Vector3(cosf(angle) * (static_cast<float>(i) * 1.65f), sinf(angle) * (static_cast<float>(i) * 1.75f), 10);
        }

        RenderFrameParallel(framebuffer);

        BeginDrawing();
        ClearBackground(Color(backgroundColor.x, backgroundColor.y, backgroundColor.z, 255));
        for (int y = 0; y < screenHeight; ++y) {
            for (int x = 0; x < screenWidth; ++x) {
                const Color color = framebuffer[y * screenWidth + x];
                DrawPixel(x, y, color);
            }
        }
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}