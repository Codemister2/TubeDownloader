#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr float PI_F = 3.14159265358979323846f;
constexpr float ARENA_W = 46.0f;
constexpr float ARENA_L = 68.0f;
constexpr float WALL_H = 16.0f;
constexpr float GOAL_W = 13.0f;
constexpr float GOAL_H = 7.0f;
constexpr float GOAL_D = 8.0f;
constexpr Color BG{5, 10, 22, 255};
constexpr Color PANEL{12, 20, 38, 235};
constexpr Color PANEL2{22, 34, 58, 245};
constexpr Color BLUE{35, 165, 255, 255};
constexpr Color ORANGE{255, 126, 40, 255};
constexpr Color GOLD{255, 206, 72, 255};
constexpr Color TEXT{238, 244, 255, 255};
constexpr Color MUTED{142, 157, 184, 255};

float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }
float len3(Vector3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
Vector3 add(Vector3 a, Vector3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
Vector3 sub(Vector3 a, Vector3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
Vector3 mul(Vector3 a,float s) { return {a.x*s,a.y*s,a.z*s}; }
Vector3 norm(Vector3 v) { float l=len3(v); return l>0.0001f?mul(v,1.0f/l):Vector3{0,0,0}; }

Font gFont{}; bool gFontLoaded=false;
Font UiFont(){ return gFontLoaded?gFont:GetFontDefault(); }
void LoadUiFont(){
#ifdef _WIN32
    const char* paths[]={"C:/Windows/Fonts/seguisb.ttf","C:/Windows/Fonts/segoeui.ttf","C:/Windows/Fonts/arial.ttf"};
    for(const char* p:paths){ if(FileExists(p)){ Font f=LoadFontEx(p,48,nullptr,0); if(f.texture.id){ gFont=f; gFontLoaded=true; SetTextureFilter(gFont.texture,TEXTURE_FILTER_BILINEAR); break; } } }
#endif
}
void UnloadUiFont(){ if(gFontLoaded) UnloadFont(gFont); }
void Text(const std::string& s,float x,float y,float size,Color c=TEXT){ DrawTextEx(UiFont(),s.c_str(),{x,y},size,1.0f,c); }
void CenterText(const std::string& s,Rectangle r,float size,Color c=TEXT){ Vector2 m=MeasureTextEx(UiFont(),s.c_str(),size,1.0f); Text(s,r.x+(r.width-m.x)/2,r.y+(r.height-m.y)/2,size,c); }

bool Button(Rectangle r,const std::string& label,bool primary=false){
    Vector2 m=GetMousePosition(); bool hover=CheckCollisionPointRec(m,r); Color fill=primary?BLUE:PANEL2; if(hover) fill=primary?Color{55,185,255,255}:Color{34,49,78,255};
    DrawRectangleRounded(r,0.08f,8,fill); DrawRectangleLinesEx(r,1.0f,hover?TEXT:Color{70,88,120,255}); CenterText(label,r,20,TEXT);
    return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

struct Car{
    Vector3 p{0,1,18},v{}; float yaw=PI_F; float pitch=0,roll=0; float boost=100; bool grounded=true; int score=0;
    Vector3 forward() const { float cp=std::cos(pitch); return {std::sin(yaw)*cp,-std::sin(pitch),std::cos(yaw)*cp}; }
};
struct Ball{ Vector3 p{0,3,0},v{}; };

enum class Screen{Menu,Settings,Game,Pause};

class Game{
public:
    Screen screen=Screen::Menu; Car player; std::vector<Car> bots; Ball ball; int blue=0,orange=0; float clock=180; bool ballCam=true; float camFov=75; float sensitivity=1.0f; bool shadows=true; bool arenaFx=true;

    Game(){ resetMatch(); }
    void resetMatch(){ player={}; player.p={0,1,22}; player.yaw=PI_F; player.boost=100; bots.clear(); for(int i=0;i<3;i++){ Car b; b.p={(i-1)*12.0f,1,-22.0f-i*3}; b.yaw=0; bots.push_back(b);} ball={{0,3,0},{}}; blue=orange=0; clock=180; }
    void kickoff(){ player.p={0,1,22}; player.v={}; player.yaw=PI_F; player.pitch=player.roll=0; player.boost=100; ball={{0,3,0},{}}; }

    void input(float dt){
        float throttle=(IsKeyDown(KEY_W)?1.f:0.f)-(IsKeyDown(KEY_S)?1.f:0.f);
        float steer=(IsKeyDown(KEY_D)?1.f:0.f)-(IsKeyDown(KEY_A)?1.f:0.f);
        if(IsGamepadAvailable(0)){ throttle=std::abs(GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_Y))>0.15f?-GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_Y):throttle; steer=std::abs(GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_X))>0.15f?GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_X):steer; }
        bool boost=IsKeyDown(KEY_LEFT_SHIFT)||(IsGamepadAvailable(0)&&IsGamepadButtonDown(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
        bool jump=IsKeyPressed(KEY_SPACE)||(IsGamepadAvailable(0)&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        if(player.grounded){
            player.yaw += steer*2.35f*dt*(0.35f+clampf(len3(player.v)/28.f,0,1));
            Vector3 f=player.forward(); f.y=0; f=norm(f); player.v=add(player.v,mul(f,throttle*34.f*dt));
            if(jump){ player.v.y=12.5f; player.grounded=false; }
        }else{
            player.pitch += -throttle*1.9f*sensitivity*dt; player.yaw += steer*1.8f*sensitivity*dt;
            float ar=(IsKeyDown(KEY_E)?1.f:0.f)-(IsKeyDown(KEY_Q)?1.f:0.f); player.roll+=ar*2.3f*sensitivity*dt;
            if(jump) player.v=add(player.v,mul(player.forward(),9.0f));
        }
        if(boost&&player.boost>0){ player.v=add(player.v,mul(player.forward(),42.f*dt)); player.boost=std::max(0.f,player.boost-28.f*dt); }
        if(IsKeyPressed(KEY_C)) ballCam=!ballCam;
    }

    void carPhysics(Car& c,float dt){
        c.v.y-=24.f*dt; c.v=mul(c.v,std::pow(c.grounded?0.987f:0.997f,dt*60)); c.p=add(c.p,mul(c.v,dt));
        if(c.p.y<1){c.p.y=1;c.v.y=std::max(0.f,-c.v.y*0.18f);c.grounded=true;c.pitch*=0.88f;c.roll*=0.88f;} else c.grounded=false;
        if(c.p.y>WALL_H-1){c.p.y=WALL_H-1;c.v.y*=-0.4f;}
        if(std::abs(c.p.x)>ARENA_W-1){c.p.x=clampf(c.p.x,-ARENA_W+1,ARENA_W-1);c.v.x*=-0.55f;}
        bool inGoal=std::abs(c.p.x)<GOAL_W;
        if(std::abs(c.p.z)>ARENA_L-1 && !inGoal){c.p.z=clampf(c.p.z,-ARENA_L+1,ARENA_L-1);c.v.z*=-0.55f;}
        c.boost=std::min(100.f,c.boost+5.f*dt);
    }

    void botAI(Car& b,float dt){ Vector3 d=sub(ball.p,b.p); b.yaw=std::atan2(d.x,d.z); Vector3 f=norm(Vector3{d.x,0,d.z}); b.v=add(b.v,mul(f,24.f*dt)); if(d.y>5&&len3(d)<14&&b.grounded){b.v.y=10;b.grounded=false;} carPhysics(b,dt); }

    void hitBall(Car& c){ Vector3 d=sub(ball.p,c.p); float dist=len3(d); if(dist<3.3f){Vector3 n=norm(d); float rel=len3(c.v); ball.v=add(ball.v,mul(n,13.f+rel*0.72f)); ball.v=add(ball.v,mul(c.v,0.32f)); ball.p=add(c.p,mul(n,3.35f));} }
    void ballPhysics(float dt){
        ball.v.y-=18.f*dt; ball.v=mul(ball.v,std::pow(0.998f,dt*60)); ball.p=add(ball.p,mul(ball.v,dt));
        if(ball.p.y<1.6f){ball.p.y=1.6f;ball.v.y=std::abs(ball.v.y)*0.72f;}
        if(ball.p.y>WALL_H-1.6f){ball.p.y=WALL_H-1.6f;ball.v.y*=-0.72f;}
        if(std::abs(ball.p.x)>ARENA_W-1.6f){ball.p.x=clampf(ball.p.x,-ARENA_W+1.6f,ARENA_W-1.6f);ball.v.x*=-0.78f;}
        bool goalMouth=std::abs(ball.p.x)<GOAL_W&&ball.p.y<GOAL_H;
        if(std::abs(ball.p.z)>ARENA_L-1.6f&&!goalMouth){ball.p.z=clampf(ball.p.z,-ARENA_L+1.6f,ARENA_L-1.6f);ball.v.z*=-0.78f;}
        if(ball.p.z>ARENA_L+GOAL_D){blue++;kickoff();} if(ball.p.z<-ARENA_L-GOAL_D){orange++;kickoff();}
    }

    void update(float dt){
        if(screen==Screen::Game){ if(IsKeyPressed(KEY_ESCAPE)){screen=Screen::Menu;return;} if(IsKeyPressed(KEY_P)){screen=Screen::Pause;return;} input(dt); carPhysics(player,dt); for(auto&b:bots)botAI(b,dt); hitBall(player); for(auto&b:bots)hitBall(b); ballPhysics(dt); clock=std::max(0.f,clock-dt); if(clock<=0)screen=Screen::Menu; }
        else if(screen==Screen::Pause){ if(IsKeyPressed(KEY_P))screen=Screen::Game; if(IsKeyPressed(KEY_ESCAPE))screen=Screen::Menu; }
    }

    Camera3D camera(){ Vector3 f=player.forward(); Vector3 focus=ballCam?ball.p:add(player.p,mul(f,14)); Vector3 back=norm(Vector3{focus.x-player.p.x,0,focus.z-player.p.z}); if(!ballCam) back=norm(Vector3{f.x,0,f.z}); Vector3 pos=add(player.p,Vector3{-back.x*13,7.0f,-back.z*13}); return {pos,focus,{0,1,0},camFov,CAMERA_PERSPECTIVE}; }

    void arena(){
        DrawCube({0,-0.3f,0},ARENA_W*2,0.5f,ARENA_L*2,Color{14,70,68,255});
        for(int i=-8;i<=8;i++) DrawCube({i*5.4f,0.02f,0},0.08f,0.03f,ARENA_L*2,Color{16,78+(i&1)*8,77,255});
        DrawCube({0,0.06f,0},0.20f,0.03f,ARENA_L*2,Fade(TEXT,0.65f)); DrawCircle3D({0,0.08f,0},10,{1,0,0},90,Fade(TEXT,0.7f));
        Color glass={80,170,230,55}; BeginBlendMode(BLEND_ALPHA); DrawCube({-ARENA_W, WALL_H/2,0},0.35f,WALL_H,ARENA_L*2,glass); DrawCube({ARENA_W,WALL_H/2,0},0.35f,WALL_H,ARENA_L*2,glass); DrawCube({0,WALL_H/2,-ARENA_L},ARENA_W*2,WALL_H,0.35f,glass); DrawCube({0,WALL_H/2,ARENA_L},ARENA_W*2,WALL_H,0.35f,glass); EndBlendMode();
        for(float z:{-ARENA_L-GOAL_D/2,ARENA_L+GOAL_D/2}){Color c=z<0?BLUE:ORANGE; DrawCube({-GOAL_W,GOAL_H/2,z},0.5f,GOAL_H,GOAL_D,c);DrawCube({GOAL_W,GOAL_H/2,z},0.5f,GOAL_H,GOAL_D,c);DrawCube({0,GOAL_H,z},GOAL_W*2,0.5f,GOAL_D,c);} 
        for(int i=0;i<8;i++){float x=((i%4)-1.5f)*18;float z=(i<4?-1:1)*30;DrawCylinder({x,0.15f,z},2.1f,2.1f,0.25f,24,Fade(GOLD,0.75f));}
    }

    void drawCar(const Car& c,Color col){ rlPushMatrix(); rlTranslatef(c.p.x,c.p.y,c.p.z); rlRotatef(c.yaw*180.f/PI_F,0,1,0); rlRotatef(c.pitch*180.f/PI_F,1,0,0); rlRotatef(c.roll*180.f/PI_F,0,0,1); DrawCube({0,0,0},2.8f,1.1f,4.8f,col); DrawCube({0,0.75f,-0.2f},2.2f,0.8f,2.4f,Color{25,35,55,255}); for(float x:{-1.45f,1.45f})for(float z:{-1.5f,1.5f})DrawCylinder({x,-0.45f,z},0.46f,0.46f,0.35f,14,Color{18,18,22,255}); rlPopMatrix(); }

    void world(){ ClearBackground(BG); Camera3D cam=camera(); BeginMode3D(cam); arena(); if(shadows){DrawCircle3D({player.p.x,0.03f,player.p.z},2.2f,{1,0,0},90,Fade(BLACK,0.4f));} drawCar(player,BLUE); for(auto&b:bots)drawCar(b,ORANGE); DrawSphere(ball.p,1.6f,Color{235,238,245,255}); DrawSphereWires(ball.p,1.62f,12,18,Color{40,50,70,255}); EndMode3D(); }

    void hud(){
        Rectangle score{GetScreenWidth()/2.f-185,18,370,68}; DrawRectangleRounded(score,0.08f,8,PANEL); DrawRectangleRounded({score.x,score.y,90,score.height},0.08f,8,Color{12,74,132,255}); DrawRectangleRounded({score.x+280,score.y,90,score.height},0.08f,8,Color{145,60,10,255}); CenterText(std::to_string(blue),{score.x,score.y,90,68},36); CenterText(std::to_string(orange),{score.x+280,score.y,90,68},36); int sec=(int)std::ceil(clock); char buf[16]; std::snprintf(buf,sizeof(buf),"%d:%02d",sec/60,sec%60); CenterText(buf,{score.x+90,score.y,190,68},28);
        Rectangle boost{GetScreenWidth()-155.f,GetScreenHeight()-125.f,120,82}; DrawRectangleRounded(boost,0.08f,8,PANEL); Text(std::to_string((int)player.boost),boost.x+19,boost.y+8,34,GOLD); Text("BOOST",boost.x+27,boost.y+52,14,MUTED);
        Text(ballCam?"BALL CAM":"CAR CAM",28,GetScreenHeight()-48.f,17,ballCam?GOLD:MUTED); Text("ESC MENU   P PAUSE",28,24,14,MUTED);
    }

    void menu(){ ClearBackground(BG); DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),Color{8,18,38,255},BG); float x=70,y=70; Text("TURBO BALL",x,y,58,TEXT); Text("ARENA",x,y+58,58,BLUE); Text("DIRECT RENDER EDITION",x,y+128,16,MUTED); Rectangle p{x,y+190,430,330}; DrawRectangleRounded(p,0.03f,8,PANEL); if(Button({p.x+35,p.y+34,p.width-70,56},"QUICK PLAY",true)){resetMatch();screen=Screen::Game;} if(Button({p.x+35,p.y+106,p.width-70,56},"FREE PLAY")){resetMatch();clock=3600;screen=Screen::Game;} if(Button({p.x+35,p.y+178,p.width-70,56},"SETTINGS"))screen=Screen::Settings; if(Button({p.x+35,p.y+250,p.width-70,56},"QUIT"))CloseWindow(); Text("WASD drive  |  SPACE jump/dodge  |  SHIFT boost",x,y+545,15,MUTED); }

    void settings(){ ClearBackground(BG); Text("SETTINGS",70,55,48); Rectangle p{70,130,620,450}; DrawRectangleRounded(p,0.03f,8,PANEL); Text("CAMERA FOV",p.x+36,p.y+38,19); if(Button({p.x+360,p.y+25,70,44},"-"))camFov=std::max(55.f,camFov-5); CenterText(std::to_string((int)camFov),{p.x+438,p.y+25,92,44},18); if(Button({p.x+538,p.y+25,48,44},"+"))camFov=std::min(110.f,camFov+5); Text("AERIAL SENSITIVITY",p.x+36,p.y+112,19); if(Button({p.x+360,p.y+99,70,44},"-"))sensitivity=std::max(.5f,sensitivity-.1f); CenterText(std::to_string((int)std::round(sensitivity*10))/10.0f==sensitivity?std::to_string(sensitivity):std::to_string(sensitivity),{p.x+438,p.y+99,92,44},17); if(Button({p.x+538,p.y+99,48,44},"+"))sensitivity=std::min(2.f,sensitivity+.1f); if(Button({p.x+36,p.y+180,250,50},shadows?"SHADOWS: ON":"SHADOWS: OFF"))shadows=!shadows; if(Button({p.x+304,p.y+180,282,50},arenaFx?"ARENA FX: ON":"ARENA FX: OFF"))arenaFx=!arenaFx; Text("This build intentionally avoids post-processing and render textures",p.x+36,p.y+270,15,MUTED); Text("so the arena remains visible on more Windows GPUs.",p.x+36,p.y+295,15,MUTED); if(Button({p.x+36,p.y+355,180,52},"BACK",true))screen=Screen::Menu; }

    void draw(){ BeginDrawing(); if(screen==Screen::Menu)menu(); else if(screen==Screen::Settings)settings(); else {world();hud(); if(screen==Screen::Pause){DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.65f)); Rectangle p{GetScreenWidth()/2.f-240,GetScreenHeight()/2.f-160,480,320};DrawRectangleRounded(p,0.05f,8,PANEL);CenterText("PAUSED",{p.x,p.y+25,p.width,60},34);if(Button({p.x+70,p.y+110,p.width-140,52},"RESUME",true))screen=Screen::Game;if(Button({p.x+70,p.y+180,p.width-140,52},"MAIN MENU"))screen=Screen::Menu;}} EndDrawing(); }
};
}

int main(){ SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT); InitWindow(1600,900,"Turbo Ball Arena 4.0 - Direct Render"); SetWindowMinSize(960,540); SetExitKey(KEY_NULL); SetTargetFPS(144); LoadUiFont(); Game game; while(!WindowShouldClose()){float dt=std::min(GetFrameTime(),0.033f);game.update(dt);game.draw();} UnloadUiFont(); CloseWindow(); return 0; }
