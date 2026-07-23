#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr float kArenaWidth = 46.0f;
constexpr float kArenaLength = 68.0f;
constexpr float kWallHeight = 16.0f;
constexpr float kGoalHalfWidth = 13.0f;
constexpr float kGoalHeight = 7.0f;
constexpr float kGoalDepth = 8.0f;
constexpr float kPi = 3.14159265358979323846f;

constexpr Color kBackground{5, 10, 22, 255};
constexpr Color kPanel{12, 20, 38, 235};
constexpr Color kPanelLight{22, 34, 58, 245};
constexpr Color kBlue{35, 165, 255, 255};
constexpr Color kOrange{255, 126, 40, 255};
constexpr Color kGold{255, 206, 72, 255};
constexpr Color kText{238, 244, 255, 255};
constexpr Color kMuted{142, 157, 184, 255};

float Clamp(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(value, maximum));
}

Vector3 Add(Vector3 a, Vector3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 Subtract(Vector3 a, Vector3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3 Scale(Vector3 value, float amount) {
    return {value.x * amount, value.y * amount, value.z * amount};
}

float Length(Vector3 value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vector3 Normalize(Vector3 value) {
    const float length = Length(value);
    return length > 0.0001f ? Scale(value, 1.0f / length) : Vector3{0.0f, 0.0f, 0.0f};
}

Font gUiFont{};
bool gUiFontLoaded = false;

Font UiFont() {
    return gUiFontLoaded ? gUiFont : GetFontDefault();
}

void LoadUiFont() {
#ifdef _WIN32
    const char* candidates[] = {
        "C:/Windows/Fonts/seguisb.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        Font font = LoadFontEx(path, 48, nullptr, 0);
        if (font.texture.id != 0) {
            gUiFont = font;
            gUiFontLoaded = true;
            SetTextureFilter(gUiFont.texture, TEXTURE_FILTER_BILINEAR);
            break;
        }
    }
#endif
}

void UnloadUiFont() {
    if (gUiFontLoaded) UnloadFont(gUiFont);
}

void DrawUiText(const std::string& text, float x, float y, float size, Color color = kText) {
    DrawTextEx(UiFont(), text.c_str(), {x, y}, size, 1.0f, color);
}

void DrawCenteredText(const std::string& text, Rectangle area, float size, Color color = kText) {
    const Vector2 dimensions = MeasureTextEx(UiFont(), text.c_str(), size, 1.0f);
    DrawUiText(text,
               area.x + (area.width - dimensions.x) * 0.5f,
               area.y + (area.height - dimensions.y) * 0.5f,
               size,
               color);
}

bool DrawButton(Rectangle area, const std::string& label, bool primary = false) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), area);
    Color fill = primary ? kBlue : kPanelLight;
    if (hovered) fill = primary ? Color{55, 185, 255, 255} : Color{34, 49, 78, 255};
    DrawRectangleRounded(area, 0.08f, 8, fill);
    DrawRectangleLinesEx(area, 1.0f, hovered ? kText : Color{70, 88, 120, 255});
    DrawCenteredText(label, area, 20.0f);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

struct Car {
    Vector3 position{0.0f, 1.0f, 18.0f};
    Vector3 velocity{};
    float yaw = kPi;
    float pitch = 0.0f;
    float roll = 0.0f;
    float boost = 100.0f;
    bool grounded = true;

    Vector3 Forward() const {
        const float cosine = std::cos(pitch);
        return {std::sin(yaw) * cosine, -std::sin(pitch), std::cos(yaw) * cosine};
    }
};

struct Ball {
    Vector3 position{0.0f, 3.0f, 0.0f};
    Vector3 velocity{};
};

enum class Screen {
    Menu,
    Settings,
    Playing,
    Paused
};

class Game {
public:
    Game() {
        ResetMatch();
    }

    void Update(float deltaTime) {
        if (screen_ == Screen::Playing) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                screen_ = Screen::Menu;
                return;
            }
            if (IsKeyPressed(KEY_P)) {
                screen_ = Screen::Paused;
                return;
            }

            ReadInput(deltaTime);
            UpdateCar(player_, deltaTime);
            for (Car& bot : bots_) UpdateBot(bot, deltaTime);
            HitBall(player_);
            for (Car& bot : bots_) HitBall(bot);
            UpdateBall(deltaTime);

            matchTime_ = std::max(0.0f, matchTime_ - deltaTime);
            if (matchTime_ <= 0.0f) screen_ = Screen::Menu;
        } else if (screen_ == Screen::Paused) {
            if (IsKeyPressed(KEY_P)) screen_ = Screen::Playing;
            if (IsKeyPressed(KEY_ESCAPE)) screen_ = Screen::Menu;
        }
    }

    void Draw() {
        BeginDrawing();
        if (screen_ == Screen::Menu) {
            DrawMenu();
        } else if (screen_ == Screen::Settings) {
            DrawSettings();
        } else {
            DrawWorld();
            DrawHud();
            if (screen_ == Screen::Paused) DrawPauseMenu();
        }
        EndDrawing();
    }

private:
    Screen screen_ = Screen::Menu;
    Car player_{};
    std::vector<Car> bots_{};
    Ball ball_{};
    int blueScore_ = 0;
    int orangeScore_ = 0;
    float matchTime_ = 180.0f;
    bool ballCam_ = true;
    float cameraFov_ = 75.0f;
    float aerialSensitivity_ = 1.0f;
    bool shadows_ = true;
    bool arenaEffects_ = true;

    void ResetMatch() {
        player_ = {};
        player_.position = {0.0f, 1.0f, 22.0f};
        player_.yaw = kPi;
        player_.boost = 100.0f;

        bots_.clear();
        for (int index = 0; index < 3; ++index) {
            Car bot;
            bot.position = {(static_cast<float>(index) - 1.0f) * 12.0f,
                            1.0f,
                            -22.0f - static_cast<float>(index) * 3.0f};
            bot.yaw = 0.0f;
            bots_.push_back(bot);
        }

        ball_ = {};
        blueScore_ = 0;
        orangeScore_ = 0;
        matchTime_ = 180.0f;
    }

    void ResetKickoff() {
        player_.position = {0.0f, 1.0f, 22.0f};
        player_.velocity = {};
        player_.yaw = kPi;
        player_.pitch = 0.0f;
        player_.roll = 0.0f;
        player_.boost = 100.0f;
        ball_ = {};
    }

    void ReadInput(float deltaTime) {
        float throttle = (IsKeyDown(KEY_W) ? 1.0f : 0.0f) - (IsKeyDown(KEY_S) ? 1.0f : 0.0f);
        float steering = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);

        if (IsGamepadAvailable(0)) {
            const float axisY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            const float axisX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            if (std::abs(axisY) > 0.15f) throttle = -axisY;
            if (std::abs(axisX) > 0.15f) steering = axisX;
        }

        const bool jumpPressed = IsKeyPressed(KEY_SPACE) ||
            (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        const bool boostHeld = IsKeyDown(KEY_LEFT_SHIFT) ||
            (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2));

        if (player_.grounded) {
            player_.yaw += steering * 2.35f * deltaTime *
                (0.35f + Clamp(Length(player_.velocity) / 28.0f, 0.0f, 1.0f));
            Vector3 forward = player_.Forward();
            forward.y = 0.0f;
            forward = Normalize(forward);
            player_.velocity = Add(player_.velocity, Scale(forward, throttle * 34.0f * deltaTime));
            if (jumpPressed) {
                player_.velocity.y = 12.5f;
                player_.grounded = false;
            }
        } else {
            player_.pitch -= throttle * 1.9f * aerialSensitivity_ * deltaTime;
            player_.yaw += steering * 1.8f * aerialSensitivity_ * deltaTime;
            const float airRoll = (IsKeyDown(KEY_E) ? 1.0f : 0.0f) - (IsKeyDown(KEY_Q) ? 1.0f : 0.0f);
            player_.roll += airRoll * 2.3f * aerialSensitivity_ * deltaTime;
            if (jumpPressed) player_.velocity = Add(player_.velocity, Scale(player_.Forward(), 9.0f));
        }

        if (boostHeld && player_.boost > 0.0f) {
            player_.velocity = Add(player_.velocity, Scale(player_.Forward(), 42.0f * deltaTime));
            player_.boost = std::max(0.0f, player_.boost - 28.0f * deltaTime);
        }

        if (IsKeyPressed(KEY_C)) ballCam_ = !ballCam_;
    }

    void UpdateCar(Car& car, float deltaTime) {
        car.velocity.y -= 24.0f * deltaTime;
        const float damping = car.grounded ? 0.987f : 0.997f;
        car.velocity = Scale(car.velocity, std::pow(damping, deltaTime * 60.0f));
        car.position = Add(car.position, Scale(car.velocity, deltaTime));

        if (car.position.y < 1.0f) {
            car.position.y = 1.0f;
            car.velocity.y = std::max(0.0f, -car.velocity.y * 0.18f);
            car.grounded = true;
            car.pitch *= 0.88f;
            car.roll *= 0.88f;
        } else {
            car.grounded = false;
        }

        if (car.position.y > kWallHeight - 1.0f) {
            car.position.y = kWallHeight - 1.0f;
            car.velocity.y *= -0.4f;
        }

        if (std::abs(car.position.x) > kArenaWidth - 1.0f) {
            car.position.x = Clamp(car.position.x, -kArenaWidth + 1.0f, kArenaWidth - 1.0f);
            car.velocity.x *= -0.55f;
        }

        const bool insideGoalWidth = std::abs(car.position.x) < kGoalHalfWidth;
        if (std::abs(car.position.z) > kArenaLength - 1.0f && !insideGoalWidth) {
            car.position.z = Clamp(car.position.z, -kArenaLength + 1.0f, kArenaLength - 1.0f);
            car.velocity.z *= -0.55f;
        }

        car.boost = std::min(100.0f, car.boost + 5.0f * deltaTime);
    }

    void UpdateBot(Car& bot, float deltaTime) {
        const Vector3 difference = Subtract(ball_.position, bot.position);
        bot.yaw = std::atan2(difference.x, difference.z);
        const Vector3 direction = Normalize({difference.x, 0.0f, difference.z});
        bot.velocity = Add(bot.velocity, Scale(direction, 24.0f * deltaTime));
        if (difference.y > 5.0f && Length(difference) < 14.0f && bot.grounded) {
            bot.velocity.y = 10.0f;
            bot.grounded = false;
        }
        UpdateCar(bot, deltaTime);
    }

    void HitBall(const Car& car) {
        const Vector3 difference = Subtract(ball_.position, car.position);
        const float distance = Length(difference);
        if (distance >= 3.3f) return;

        const Vector3 normal = Normalize(difference);
        ball_.velocity = Add(ball_.velocity, Scale(normal, 13.0f + Length(car.velocity) * 0.72f));
        ball_.velocity = Add(ball_.velocity, Scale(car.velocity, 0.32f));
        ball_.position = Add(car.position, Scale(normal, 3.35f));
    }

    void UpdateBall(float deltaTime) {
        ball_.velocity.y -= 18.0f * deltaTime;
        ball_.velocity = Scale(ball_.velocity, std::pow(0.998f, deltaTime * 60.0f));
        ball_.position = Add(ball_.position, Scale(ball_.velocity, deltaTime));

        if (ball_.position.y < 1.6f) {
            ball_.position.y = 1.6f;
            ball_.velocity.y = std::abs(ball_.velocity.y) * 0.72f;
        }
        if (ball_.position.y > kWallHeight - 1.6f) {
            ball_.position.y = kWallHeight - 1.6f;
            ball_.velocity.y *= -0.72f;
        }
        if (std::abs(ball_.position.x) > kArenaWidth - 1.6f) {
            ball_.position.x = Clamp(ball_.position.x, -kArenaWidth + 1.6f, kArenaWidth - 1.6f);
            ball_.velocity.x *= -0.78f;
        }

        const bool insideGoal = std::abs(ball_.position.x) < kGoalHalfWidth && ball_.position.y < kGoalHeight;
        if (std::abs(ball_.position.z) > kArenaLength - 1.6f && !insideGoal) {
            ball_.position.z = Clamp(ball_.position.z, -kArenaLength + 1.6f, kArenaLength - 1.6f);
            ball_.velocity.z *= -0.78f;
        }

        if (ball_.position.z > kArenaLength + kGoalDepth) {
            ++blueScore_;
            ResetKickoff();
        } else if (ball_.position.z < -kArenaLength - kGoalDepth) {
            ++orangeScore_;
            ResetKickoff();
        }
    }

    Camera3D CreateCamera() const {
        const Vector3 forward = player_.Forward();
        const Vector3 focus = ballCam_ ? ball_.position : Add(player_.position, Scale(forward, 14.0f));
        Vector3 direction = Normalize({focus.x - player_.position.x, 0.0f, focus.z - player_.position.z});
        if (!ballCam_) direction = Normalize({forward.x, 0.0f, forward.z});
        const Vector3 position = Add(player_.position, {-direction.x * 13.0f, 7.0f, -direction.z * 13.0f});
        return {position, focus, {0.0f, 1.0f, 0.0f}, cameraFov_, CAMERA_PERSPECTIVE};
    }

    void DrawArena() const {
        DrawCube({0.0f, -0.3f, 0.0f}, kArenaWidth * 2.0f, 0.5f, kArenaLength * 2.0f,
                 Color{14, 70, 68, 255});

        for (int index = -8; index <= 8; ++index) {
            const unsigned char green = static_cast<unsigned char>(78 + ((index & 1) != 0 ? 8 : 0));
            DrawCube({static_cast<float>(index) * 5.4f, 0.02f, 0.0f},
                     0.08f, 0.03f, kArenaLength * 2.0f, Color{16, green, 77, 255});
        }

        DrawCube({0.0f, 0.06f, 0.0f}, 0.20f, 0.03f, kArenaLength * 2.0f, Fade(kText, 0.65f));
        DrawCircle3D({0.0f, 0.08f, 0.0f}, 10.0f, {1.0f, 0.0f, 0.0f}, 90.0f, Fade(kText, 0.7f));

        const Color glass{80, 170, 230, 55};
        BeginBlendMode(BLEND_ALPHA);
        DrawCube({-kArenaWidth, kWallHeight * 0.5f, 0.0f}, 0.35f, kWallHeight, kArenaLength * 2.0f, glass);
        DrawCube({kArenaWidth, kWallHeight * 0.5f, 0.0f}, 0.35f, kWallHeight, kArenaLength * 2.0f, glass);
        DrawCube({0.0f, kWallHeight * 0.5f, -kArenaLength}, kArenaWidth * 2.0f, kWallHeight, 0.35f, glass);
        DrawCube({0.0f, kWallHeight * 0.5f, kArenaLength}, kArenaWidth * 2.0f, kWallHeight, 0.35f, glass);
        EndBlendMode();

        const float goalPositions[2] = {-kArenaLength - kGoalDepth * 0.5f,
                                         kArenaLength + kGoalDepth * 0.5f};
        for (float z : goalPositions) {
            const Color color = z < 0.0f ? kBlue : kOrange;
            DrawCube({-kGoalHalfWidth, kGoalHeight * 0.5f, z}, 0.5f, kGoalHeight, kGoalDepth, color);
            DrawCube({kGoalHalfWidth, kGoalHeight * 0.5f, z}, 0.5f, kGoalHeight, kGoalDepth, color);
            DrawCube({0.0f, kGoalHeight, z}, kGoalHalfWidth * 2.0f, 0.5f, kGoalDepth, color);
        }

        if (arenaEffects_) {
            for (int index = 0; index < 8; ++index) {
                const float x = (static_cast<float>(index % 4) - 1.5f) * 18.0f;
                const float z = (index < 4 ? -1.0f : 1.0f) * 30.0f;
                DrawCylinder({x, 0.15f, z}, 2.1f, 2.1f, 0.25f, 24, Fade(kGold, 0.75f));
            }
        }
    }

    void DrawCar(const Car& car, Color color) const {
        DrawCube(car.position, 2.8f, 1.1f, 4.8f, color);
        DrawCube({car.position.x, car.position.y + 0.75f, car.position.z - 0.2f},
                 2.2f, 0.8f, 2.4f, Color{25, 35, 55, 255});

        const float wheelX[2] = {-1.45f, 1.45f};
        const float wheelZ[2] = {-1.5f, 1.5f};
        for (float x : wheelX) {
            for (float z : wheelZ) {
                DrawCylinder({car.position.x + x, car.position.y - 0.45f, car.position.z + z},
                             0.46f, 0.46f, 0.35f, 14, Color{18, 18, 22, 255});
            }
        }
    }

    void DrawWorld() const {
        ClearBackground(kBackground);
        const Camera3D camera = CreateCamera();
        BeginMode3D(camera);
        DrawArena();
        if (shadows_) {
            DrawCircle3D({player_.position.x, 0.03f, player_.position.z}, 2.2f,
                         {1.0f, 0.0f, 0.0f}, 90.0f, Fade(BLACK, 0.4f));
        }
        DrawCar(player_, kBlue);
        for (const Car& bot : bots_) DrawCar(bot, kOrange);
        DrawSphere(ball_.position, 1.6f, Color{235, 238, 245, 255});
        DrawSphereWires(ball_.position, 1.62f, 12, 18, Color{40, 50, 70, 255});
        EndMode3D();
    }

    void DrawHud() const {
        const float width = static_cast<float>(GetScreenWidth());
        const float height = static_cast<float>(GetScreenHeight());

        Rectangle scoreBox{width * 0.5f - 185.0f, 18.0f, 370.0f, 68.0f};
        DrawRectangleRounded(scoreBox, 0.08f, 8, kPanel);
        DrawRectangleRounded({scoreBox.x, scoreBox.y, 90.0f, scoreBox.height},
                             0.08f, 8, Color{12, 74, 132, 255});
        DrawRectangleRounded({scoreBox.x + 280.0f, scoreBox.y, 90.0f, scoreBox.height},
                             0.08f, 8, Color{145, 60, 10, 255});
        DrawCenteredText(std::to_string(blueScore_), {scoreBox.x, scoreBox.y, 90.0f, 68.0f}, 36.0f);
        DrawCenteredText(std::to_string(orangeScore_), {scoreBox.x + 280.0f, scoreBox.y, 90.0f, 68.0f}, 36.0f);

        const int seconds = static_cast<int>(std::ceil(matchTime_));
        char clockText[16];
        std::snprintf(clockText, sizeof(clockText), "%d:%02d", seconds / 60, seconds % 60);
        DrawCenteredText(clockText, {scoreBox.x + 90.0f, scoreBox.y, 190.0f, 68.0f}, 28.0f);

        Rectangle boostBox{width - 155.0f, height - 125.0f, 120.0f, 82.0f};
        DrawRectangleRounded(boostBox, 0.08f, 8, kPanel);
        DrawUiText(std::to_string(static_cast<int>(player_.boost)), boostBox.x + 19.0f, boostBox.y + 8.0f, 34.0f, kGold);
        DrawUiText("BOOST", boostBox.x + 27.0f, boostBox.y + 52.0f, 14.0f, kMuted);
        DrawUiText(ballCam_ ? "BALL CAM" : "CAR CAM", 28.0f, height - 48.0f, 17.0f, ballCam_ ? kGold : kMuted);
        DrawUiText("ESC MENU   P PAUSE", 28.0f, 24.0f, 14.0f, kMuted);
    }

    void DrawMenu() {
        ClearBackground(kBackground);
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), Color{8, 18, 38, 255}, kBackground);
        const float x = 70.0f;
        const float y = 70.0f;
        DrawUiText("TURBO BALL", x, y, 58.0f);
        DrawUiText("ARENA", x, y + 58.0f, 58.0f, kBlue);
        DrawUiText("DIRECT RENDER EDITION", x, y + 128.0f, 16.0f, kMuted);

        Rectangle panel{x, y + 190.0f, 430.0f, 330.0f};
        DrawRectangleRounded(panel, 0.03f, 8, kPanel);
        if (DrawButton({panel.x + 35.0f, panel.y + 34.0f, panel.width - 70.0f, 56.0f}, "QUICK PLAY", true)) {
            ResetMatch();
            screen_ = Screen::Playing;
        }
        if (DrawButton({panel.x + 35.0f, panel.y + 106.0f, panel.width - 70.0f, 56.0f}, "FREE PLAY")) {
            ResetMatch();
            matchTime_ = 3600.0f;
            screen_ = Screen::Playing;
        }
        if (DrawButton({panel.x + 35.0f, panel.y + 178.0f, panel.width - 70.0f, 56.0f}, "SETTINGS")) {
            screen_ = Screen::Settings;
        }
        if (DrawButton({panel.x + 35.0f, panel.y + 250.0f, panel.width - 70.0f, 56.0f}, "QUIT")) {
            CloseWindow();
        }
        DrawUiText("WASD drive  |  SPACE jump/dodge  |  SHIFT boost", x, y + 545.0f, 15.0f, kMuted);
    }

    void DrawSettings() {
        ClearBackground(kBackground);
        DrawUiText("SETTINGS", 70.0f, 55.0f, 48.0f);
        Rectangle panel{70.0f, 130.0f, 620.0f, 450.0f};
        DrawRectangleRounded(panel, 0.03f, 8, kPanel);

        DrawUiText("CAMERA FOV", panel.x + 36.0f, panel.y + 38.0f, 19.0f);
        if (DrawButton({panel.x + 360.0f, panel.y + 25.0f, 70.0f, 44.0f}, "-")) cameraFov_ = std::max(55.0f, cameraFov_ - 5.0f);
        DrawCenteredText(std::to_string(static_cast<int>(cameraFov_)), {panel.x + 438.0f, panel.y + 25.0f, 92.0f, 44.0f}, 18.0f);
        if (DrawButton({panel.x + 538.0f, panel.y + 25.0f, 48.0f, 44.0f}, "+")) cameraFov_ = std::min(110.0f, cameraFov_ + 5.0f);

        DrawUiText("AERIAL SENSITIVITY", panel.x + 36.0f, panel.y + 112.0f, 19.0f);
        if (DrawButton({panel.x + 360.0f, panel.y + 99.0f, 70.0f, 44.0f}, "-")) aerialSensitivity_ = std::max(0.5f, aerialSensitivity_ - 0.1f);
        char sensitivityText[16];
        std::snprintf(sensitivityText, sizeof(sensitivityText), "%.1f", aerialSensitivity_);
        DrawCenteredText(sensitivityText, {panel.x + 438.0f, panel.y + 99.0f, 92.0f, 44.0f}, 17.0f);
        if (DrawButton({panel.x + 538.0f, panel.y + 99.0f, 48.0f, 44.0f}, "+")) aerialSensitivity_ = std::min(2.0f, aerialSensitivity_ + 0.1f);

        if (DrawButton({panel.x + 36.0f, panel.y + 180.0f, 250.0f, 50.0f}, shadows_ ? "SHADOWS: ON" : "SHADOWS: OFF")) shadows_ = !shadows_;
        if (DrawButton({panel.x + 304.0f, panel.y + 180.0f, 282.0f, 50.0f}, arenaEffects_ ? "ARENA FX: ON" : "ARENA FX: OFF")) arenaEffects_ = !arenaEffects_;

        DrawUiText("Direct rendering avoids the previous black-screen pipeline.", panel.x + 36.0f, panel.y + 270.0f, 15.0f, kMuted);
        DrawUiText("No render textures or post-processing shaders are used.", panel.x + 36.0f, panel.y + 295.0f, 15.0f, kMuted);
        if (DrawButton({panel.x + 36.0f, panel.y + 355.0f, 180.0f, 52.0f}, "BACK", true)) screen_ = Screen::Menu;
    }

    void DrawPauseMenu() {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));
        Rectangle panel{static_cast<float>(GetScreenWidth()) * 0.5f - 240.0f,
                        static_cast<float>(GetScreenHeight()) * 0.5f - 160.0f,
                        480.0f,
                        320.0f};
        DrawRectangleRounded(panel, 0.05f, 8, kPanel);
        DrawCenteredText("PAUSED", {panel.x, panel.y + 25.0f, panel.width, 60.0f}, 34.0f);
        if (DrawButton({panel.x + 70.0f, panel.y + 110.0f, panel.width - 140.0f, 52.0f}, "RESUME", true)) screen_ = Screen::Playing;
        if (DrawButton({panel.x + 70.0f, panel.y + 180.0f, panel.width - 140.0f, 52.0f}, "MAIN MENU")) screen_ = Screen::Menu;
    }
};

}  // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1600, 900, "Turbo Ball Arena 4.0 - Direct Render");
    SetWindowMinSize(960, 540);
    SetExitKey(KEY_NULL);
    SetTargetFPS(144);
    LoadUiFont();

    Game game;
    while (!WindowShouldClose()) {
        game.Update(std::min(GetFrameTime(), 0.033f));
        game.Draw();
    }

    UnloadUiFont();
    CloseWindow();
    return 0;
}
