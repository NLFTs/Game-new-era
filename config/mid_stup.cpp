#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>

// =================================================================
// 1. MATH CORE: LINEAR ALGEBRA ENGINE
// =================================================================

namespace Engine::Math {
    struct Vector3 {
        double x, y, z;

        Vector3(double _x = 0, double _y = 0, double _z = 0) : x(_x), y(_y), z(_z) {}

        // Operator Overloading untuk kalkulasi vektor
        Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
        Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
        Vector3 operator*(double scalar) const { return {x * scalar, y * scalar, z * scalar}; }

        double dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
        
        Vector3 cross(const Vector3& v) const {
            return {
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            };
        }

        double length() const { return std::sqrt(x*x + y*y + z*z); }
        
        Vector3 normalize() const {
            double len = length();
            return (len > 0) ? (*this * (1.0 / len)) : Vector3(0,0,0);
        }
    };

    struct Matrix4x4 {
        double m[4][4] = {0};
        static Matrix4x4 Identity() {
            Matrix4x4 mat;
            for(int i=0; i<4; ++i) mat.m[i][i] = 1.0;
            return mat;
        }
    };
}

// =================================================================
// 2. SHADER & RENDERING PIPELINE (CRTP Pattern)
// =================================================================

template <typename DerivedShader>
class ShaderBase {
public:
    void execute() {
        static_cast<DerivedShader*>(this)->apply_impl();
    }
};

class PhongShader : public ShaderBase<PhongShader> {
public:
    void apply_impl() {
        // Simulasi perhitungan pencahayaan Ambient, Diffuse, Specular
        std::cout << "[Shader] Applying Phong Lighting Model..." << std::endl;
    }
};

// =================================================================
// 3. ENTITY COMPONENT SYSTEM (ECS) CORE
// =================================================================

class Component {
public:
    virtual ~Component() = default;
    virtual void update(double dt) = 0;
};

class TransformComponent : public Component {
public:
    Engine::Math::Vector3 position;
    Engine::Math::Vector3 rotation;
    
    TransformComponent(double x, double y, double z) : position(x, y, z) {}
    
    void update(double dt) override {
        position.x += 0.01 * dt; // Simulasi pergerakan konstan
    }
};

class MeshComponent : public Component {
public:
    std::string modelPath;
    int vertexCount;

    MeshComponent(std::string path) : modelPath(path), vertexCount(1000) {}

    void update(double dt) override {
        // Render logic biasanya di sini
    }
};

class Entity {
    size_t id;
    std::vector<std::unique_ptr<Component>> components;

public:
    Entity(size_t _id) : id(_id) {}

    template <typename T, typename... Args>
    void addComponent(Args&&... args) {
        components.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void update(double dt) {
        for (auto& comp : components) comp->update(dt);
    }
    
    size_t getId() const { return id; }
};

// =================================================================
// 4. SCENE GRAPH & EVENT SYSTEM
// =================================================================

class SceneManager {
private:
    std::vector<std::shared_ptr<Entity>> entities;
    std::map<std::string, std::function<void()>> eventCallbacks;

public:
    void addEntity(std::shared_ptr<Entity> e) {
        entities.push_back(e);
    }

    void onEvent(const std::string& eventName, std::function<void()> callback) {
        eventCallbacks[eventName] = callback;
    }

    void triggerEvent(const std::string& eventName) {
        if (eventCallbacks.count(eventName)) {
            eventCallbacks[eventName]();
        }
    }

    void update(double dt) {
        for (auto& entity : entities) {
            entity->update(dt);
        }
    }
};

// =================================================================
// 5. RENDER ENGINE MAIN LOOP
// =================================================================

class RenderEngine {
private:
    bool isRunning;
    SceneManager scene;
    PhongShader currentShader;

public:
    RenderEngine() : isRunning(false) {}

    void initialize() {
        std::cout << "--- Initializing Procedural Render Engine ---" << std::endl;
        isRunning = true;

        // Setup Scene
        auto player = std::make_shared<Entity>(1);
        player->addComponent<TransformComponent>(0.0, 5.0, -10.0);
        player->addComponent<MeshComponent>("assets/hero.obj");
        
        scene.addEntity(player);

        scene.onEvent("OnCrash", [](){
            std::cout << "!!! ALERT: Engine detected a collision event !!!" << std::endl;
        });
    }

    void processInput() {
        // Simulasi input async
    }

    void update(double deltaTime) {
        scene.update(deltaTime);
        
        // Simulasi logika kompleks
        if (std::rand() % 100 > 95) {
            scene.triggerEvent("OnCrash");
        }
    }

    void render() {
        currentShader.execute();
        std::cout << "[Render] Flushing buffers to GPU... Frame Rendered." << std::endl;
    }

    void start() {
        initialize();
        double deltaTime = 0.016; // Simulasi 60 FPS

        for (int frame = 0; frame < 10; ++frame) {
            std::cout << "\n--- Processing Frame: " << frame << " ---" << std::endl;
            processInput();
            update(deltaTime);
            render();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

// =================================================================
// 6. ENTRY POINT
// =================================================================

int main() {
    try {
        RenderEngine engine;
        engine.start();
    } catch (const std::exception& e) {
        std::cerr << "Engine Runtime Error: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "\nEngine Cleanly Shutdown." << std::endl;
    return 0;
}