#include <limits>
#include <optional>
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
constexpr int screenWidth = 400;
constexpr int screenHeight = 400;
constexpr float viewportWidth = 1.0f;
constexpr float viewportHeight = 1.0f;
constexpr float viewportDistance = 1.0f;
constexpr auto fScreenWidth = static_cast<float>(screenWidth);
constexpr auto fScreenHeight = static_cast<float>(screenHeight);
constexpr auto backgroundColor = Vector3(30, 30, 30);
const auto directionLight = Vector3Normalize(Vector3(1.0, -0.5, -1.0));

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

Vector3 diffuse_light(const Vector3 O, const Vector3 D, const float t, const std::optional<Sphere> &sphere) {
    const Vector3 intersection_point = Vector3Add(O, Vector3Scale(D, t));
    const Vector3 normal = Vector3Normalize(Vector3Subtract(intersection_point, sphere->center));
    const Vector3 dirToSun = Vector3Normalize(Vector3Subtract(spheres[0].center, intersection_point));
    const float light_intensity = Vector3DotProduct(normal, dirToSun);
    return Vector3Lerp(Vector3(0.35, 0.35, 0.35), Vector3(1.0, 1.0, 1.0), light_intensity);
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
        if (!closest_sphere->lit) {
            return closest_sphere->color;
        }

        const Vector3 light = diffuse_light(O, D, closest_t, closest_sphere);
        return Vector3Multiply(closest_sphere->color, light);
    }

    return Vector3(backgroundColor.x, backgroundColor.y, backgroundColor.z);
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Software Renderer");

    SetTargetFPS(60);
    const std::vector<Vector3> og_pos = {};

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // do orbit
        for (size_t i = 1; i < spheres.size(); i++) {
            const float angle = GetTime() / static_cast<double>(i); // Adjust speed by changing multiplier
            spheres[i].center = Vector3(cosf(angle) * (static_cast<float>(i) * 1.65f), sinf(angle) * (static_cast<float>(i) * 1.75f), 10);
        }

        BeginDrawing();
        ClearBackground(Color(backgroundColor.x, backgroundColor.y, backgroundColor.z, 255));
        // draw each pixel
        for (int x = -screenWidth / 2; x <= screenWidth / 2; x++) {
            for (int y = -screenWidth / 2; y <= screenWidth / 2; y++) {
                const auto D = CanvasToViewPortDirection(x, y);
                const auto color = TraceRay(O, D, 1, std::numeric_limits<float>::max());
                const auto screen_pos = CanvasToScreen(Vector2(x, y));
                DrawPixel(screen_pos.x, screen_pos.y, Color(color.x, color.y, color.z, 255));
            }
        }
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}