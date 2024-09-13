#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "SDL2/SDL.h"

#undef main

const char* proj_name = "My ALX-se mvp project";
typedef struct
{
    float x;
    float y;
}
Cord;

typedef struct
{
    int tile;
    Cord where;
}
Obstruct;

typedef struct
{
    Cord a;
    Cord b;
}
Line;

typedef struct
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int xaxis_resolution;
    int yaxis_resolution;
}
Gpu;

typedef struct
{
    uint32_t* pixels;
    int width;
}
Display;

typedef struct
{
    int top;
    int bot;
    float size;
}
Wall;

typedef struct
{
    Line fov;
    Cord where;
    Cord velocity;
    float speed;
    float acceleration;
    float teeta;
}
Hero;

typedef struct
{
    const char** roofing;
    const char** fencing;
    const char** stepping;
}
Map;

//This function Rotates the player by some radian value.
static Cord turn(const Cord a, const float t)
{
    const Cord b = { a.x * cosf(t) - a.y * sinf(t), a.x * sinf(t) + a.y * cosf(t) };
    return b;
}

// This function Subtracts two Cords.
static Cord sub(const Cord a, const Cord b)
{
    const Cord c = { a.x - b.x, a.y - b.y };
    return c;
}

//This function Adds two Cords.
static Cord add(const Cord a, const Cord b)
{
    const Cord c = { a.x + b.x, a.y + b.y };
    return c;
}

//This function Multiplies a Cord by a scalar value.
static Cord mul(const Cord a, const float n)
{
    const Cord b = { a.x * n, a.y * n };
    return b;
}

//This function Returns the magnitude of a Cord.
static float mag(const Cord a)
{
    return sqrtf(a.x * a.x + a.y * a.y);
}

//This function Returns the unit vector of a Cord.
static Cord unit(const Cord a)
{
    return mul(a, 1.0f / mag(a));
}

//This function Returns the slope of a Cord.
static float slope(const Cord a)
{
    return a.y / a.x;
}

//This function return Fast floor (math.h is too slow).
static int fl(const float x)
{
    return (int) x - (x < (int) x);
}

//This function return Fast ceil (math.h is too slow).
static int cl(const float x)
{
    return (int) x + (x > (int) x);
}

//This function create Steps horizontally along a square grid.
static Cord sh(const Cord a, const Cord b)
{
    const float x = b.x > 0.0f ? fl(a.x + 1.0f) : cl(a.x - 1.0f);
    const float y = slope(b) * (x - a.x) + a.y;
    const Cord c = { x, y };
    return c;
}

//This function create Steps vertically along a square grid.
static Cord sv(const Cord a, const Cord b)
{
    const float y = b.y > 0.0f ? fl(a.y + 1.0f) : cl(a.y - 1.0f);
    const float x = (y - a.y) / slope(b) + a.x;
    const Cord c = { x, y };
    return c;
}

//This function Returns a decimal value of the ascii tile value on the map.
static int tile(const Cord a, const char** const tiles)
{
    const int x = a.x;
    const int y = a.y;
    return tiles[y][x] - '0';
}

//This function return Floating Cord decimal.
static float dec(const float x)
{
    return x - (int) x;
}

// This function Casts a ray from <where> in unit <direction> until a <fencing> tile is Obstruct.
static Obstruct cast(const Cord where, const Cord direction, const char** const fencing)
{
    // Determine whether to step horizontally or vertically on the grid.
    const Cord hor = sh(where, direction);
    const Cord ver = sv(where, direction);
    const Cord ray = mag(sub(hor, where)) < mag(sub(ver, where)) ? hor : ver;
    // Due to floating Cord error, the step may not make it to the next grid square.
    // Three directions (dy, dx, dc) of a tiny step will be added to the ray
    // depending on if the ray Obstruct a horizontal wall, a vertical wall, or the corner
    // of two walls, respectively.
    const Cord dc = mul(direction, 0.01f);
    const Cord dx = { dc.x, 0.0f };
    const Cord dy = { 0.0f, dc.y };
    const Cord test = add(ray,
        // Tiny step for corner of two grid squares.
        mag(sub(hor, ver)) < 1e-3f ? dc :
        // Tiny step for vertical grid square.
        dec(ray.x) == 0.0f ? dx :
        // Tiny step for a horizontal grid square.
        dy);
    const Obstruct Obstruct = { tile(test, fencing), ray };
    // If a wall was not Obstruct, then continue advancing the ray.
    return Obstruct.tile ? Obstruct : cast(ray, direction, fencing);
}

// Party casting. Returns a percentage of <y> related to <yaxis_resolution> for roofing and
//This function return floor casting when lerping the floor or roofing.
static float pcast(const float size, const int yaxis_resolution, const int y)
{
    return size / (2 * (y + 1) - yaxis_resolution);
}

//This function Rotates a line by some radian amount.
static Line rotate(const Line l, const float t)
{
    const Line line = { turn(l.a, t), turn(l.b, t) };
    return line;
}

//This function return Linear interpolation.
static Cord lerp(const Line l, const float n)
{
    return add(l.a, mul(sub(l.b, l.a), n));
}

// This function return Setups the software gpu.
static Gpu setup(const int xaxis_resolution, const int yaxis_resolution)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        puts(SDL_GetError());
        exit(1);
    }
    SDL_Window* const window = SDL_CreateWindow(
        proj_name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        xaxis_resolution, yaxis_resolution,
        SDL_WINDOW_SHOWN);
    SDL_Renderer* const renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    // Notice the flip between xaxis_resolution and yaxis_resolution.
    // The texture is 90 degrees flipped on its side for fast cache access.
    SDL_Texture* const texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        yaxis_resolution, xaxis_resolution);
    if(window == NULL || renderer == NULL || texture == NULL)
    {
        puts(SDL_GetError());
        exit(1);
    }
    const Gpu gpu = { window, renderer, texture, xaxis_resolution, yaxis_resolution };
    return gpu;
}

// Presents the software gpu to the window.
//This function Calls the real GPU to rotate texture back 90 degrees before presenting.
static void present(const Gpu gpu)
{
    const SDL_Rect distance = {
        (gpu.xaxis_resolution - gpu.yaxis_resolution) / 2,
        (gpu.yaxis_resolution - gpu.xaxis_resolution) / 2,
        gpu.yaxis_resolution, gpu.xaxis_resolution,
    };
    SDL_RenderCopyEx(gpu.renderer, gpu.texture, NULL, &distance, -90, NULL, SDL_FLIP_NONE);
    SDL_RenderPresent(gpu.renderer);
}

//This function Locks the gpu, returning a Corder to video memory.
static Display lock(const Gpu gpu)
{
    void* screen;
    int pitch;
    SDL_LockTexture(gpu.texture, NULL, &screen, &pitch);
    const Display display = { (uint32_t*) screen, pitch / (int) sizeof(uint32_t) };
    return display;
}

//This function Places a pixels in gpu video memory.
static void put(const Display display, const int x, const int y, const uint32_t pixel)
{
    display.pixels[y + x * display.width] = pixel;
}

//This function Unlocks the gpu, making the Corder to video memory ready for presentation
static void unlock(const Gpu gpu)
{
    SDL_UnlockTexture(gpu.texture);
}

// This function Spins the hero when keys A,D are held down.
static Hero spin(Hero hero, const uint8_t* key)
{
    if(key[SDL_SCANCODE_A]) hero.teeta -= 0.1f;
    if(key[SDL_SCANCODE_D]) hero.teeta += 0.1f;
    return hero;
}

// This function Moves the hero when w,d are held down. Handles collision detection for the walls.
static Hero move(Hero hero, const char** const fencing, const uint8_t* key)
{
    const Cord last = hero.where, zero = { 0.0f, 0.0f };
    //this if statement Accelerates the motion with key held down.
    if(key[SDL_SCANCODE_W] || key[SDL_SCANCODE_S] )
    {
        const Cord reference = { 1.0f, 0.0f };
        const Cord direction = turn(reference, hero.teeta);
        const Cord acceleration = mul(direction, hero.acceleration);
        if(key[SDL_SCANCODE_W]) hero.velocity = add(hero.velocity, acceleration);
        if(key[SDL_SCANCODE_S]) hero.velocity = sub(hero.velocity, acceleration);
    }
    // Otherwise, its decelerates the motion (exponential decay).
    else hero.velocity = mul(hero.velocity, 1.0f - hero.acceleration / hero.speed);
    //this if statement Caps velocity if top speed is exceeded.
    if(mag(hero.velocity) > hero.speed) hero.velocity = mul(unit(hero.velocity), hero.speed);
    // Moves.
    hero.where = add(hero.where, hero.velocity);
    //this if statement Sets velocity to zero if there is a collision and puts hero back in bounds.
    if(tile(hero.where, fencing))
    {
        hero.velocity = zero;
        hero.where = last;
    }
    return hero;
}

//This function Returns a color value (RGB) from a decimal tile value.
static uint32_t color(const int tile)
{
    switch(tile)
    {
    default:
    case 1: return 0x00AAAA00; // Red.
    case 2: return 0x00A0000A; // Green.
    case 3: return 0x00000000; // Blue.
    case 4: return 0xA0A0A0A0;//grey
    }
}


//This function Calculates wall size using the <corrected> ray to the wall.
static Wall project(const int xaxis_resolution, const int yaxis_resolution, const float focal, const Cord corrected)
{
    //checking Normal distance of corrected ray is clamped to some small value else wall size will shoot to infinity without wall collision.
    const float normal = corrected.x < 1e-2f ? 1e-2f : corrected.x;
    const float size = 0.5f * focal * xaxis_resolution / normal;
    const int top = (yaxis_resolution + size) / 2.0f;
    const int bot = (yaxis_resolution - size) / 2.0f;
    // Top and bottom values are clamped to screen size else renderer will waste cycles
    // (or segfault) when rasterizing pixels off screen.
    const Wall wall = { top > yaxis_resolution ? yaxis_resolution : top, bot < 0 ? 0 : bot, size };
    return wall;
}

//This function Renders the entire scene from the <hero> perspective given a <map> and a software <gpu>.
static void render(const Hero hero, const Map map, const Gpu gpu)
{
    const int t0 = SDL_GetTicks();
    const Line camera = rotate(hero.fov, hero.teeta);
    const Display display = lock(gpu);
    // Ray cast for all columns of the window.
    for(int x = 0; x < gpu.xaxis_resolution; x++)
    {
        const Cord direction = lerp(camera, x / (float) gpu.xaxis_resolution);
        const Obstruct Obstruct = cast(hero.where, direction, map.fencing);
        const Cord ray = sub(Obstruct.where, hero.where);
        const Line trace = { hero.where, Obstruct.where };
        const Cord corrected = turn(ray, -hero.teeta);
        const Wall wall = project(gpu.xaxis_resolution, gpu.yaxis_resolution, hero.fov.a.x, corrected);
        // Renders flooring.
        for(int y = 0; y < wall.bot; y++)
            put(display, x, y, color(tile(lerp(trace, -pcast(wall.size, gpu.yaxis_resolution, y)), map.stepping)));
        // Renders wall.
        for(int y = wall.bot; y < wall.top; y++)
            put(display, x, y, color(Obstruct.tile));
        // Renders roofing.
        for(int y = wall.top; y < gpu.yaxis_resolution; y++)
            put(display, x, y, color(tile(lerp(trace, +pcast(wall.size, gpu.yaxis_resolution, y)), map.roofing)));
    }
     
    unlock(gpu);
    present(gpu);
    // Caps frame rate to ~60 fps if the vertical sync (VSYNC) init failed.
    const int t1 = SDL_GetTicks();
    const int ms = 16 - (t1 - t0);
    SDL_Delay(ms < 0 ? 0 : ms);
}
//This function get event input keys to close the program
static bool done()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT
        || event.key.keysym.sym == SDLK_END
        || event.key.keysym.sym == SDLK_ESCAPE;
}

//This function Changes the field of view. A focal value of 1.0 is 90 degrees.
static Line viewport(const float focal)
{
    const Line fov = {
        { focal, -1.0f },
        { focal, +1.0f },
    };
    return fov;
}

static Hero born(const float focal)
{
    const Hero hero = {
        viewport(focal),
        // Where.
        { 3.5f, 3.5f },
        // Velocity.
        { 0.0f, 0.0f },
        // Speed.
        0.10f,
        // Acceleration.
        0.015f,
        // teeta radians.
        0.0f
    };
    return hero;
}

//This function Builds the map/world. Note the static prefix for the parties. Map lives in .bss in private.
static Map build()
{
   static const char* roofing[] = {
        "4444444444444444",
        "4444444444444444",
        "4444444444444444",
        "4444444444444444",
        "4444444444444444",
        "4444444444444444",
        "4444444444444444",
    };
    static const char* fencing[] = {
        "1111111111111111",
        "1000000000000001",
        "1011100011110001",
        "1010000000000001",
        "1011100011110001",
        "1000000000111101",
        "1111111111111111",
    };
   static const char* stepping[] = {
        "3333333333333333",
        "3333333333333333",
        "3333333333333333",
        "3333333333333333",
        "3333333333333333",
        "3333333333333333",
        "3333333333333333",
   };
    const Map map = { roofing, fencing, stepping };
    return map;
}

// This function start/initiate the execution of program 
int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;
    const Gpu gpu = setup(700, 600);
    const Map map = build();
    Hero hero = born(0.8f);
    while(!done())
    {
        const uint8_t* key = SDL_GetKeyboardState(NULL);
        hero = spin(hero, key);
        hero = move(hero, map.fencing, key);
        render(hero, map, gpu);
    }
    // No need to free anything - gives quick exit.
    return 0;
}
