#include <SDL.h>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <random>

std::shared_ptr<SDL_Texture> load_image(SDL_Renderer *renderer, const std::string &file_name) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    surface = SDL_LoadBMP(file_name.c_str());
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        throw std::invalid_argument(SDL_GetError());
    }
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format,0, 255, 255));
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        throw std::invalid_argument(SDL_GetError());
    }
    SDL_FreeSurface(surface);
    return {texture, [](SDL_Texture *t) {
        SDL_DestroyTexture(t);
    }};
}

union vect_t {
    struct { double x; double y;} v;
    double p[2];
};

vect_t operator+(const vect_t a, const vect_t b) {
    vect_t ret = a;
    ret.v.x += b.v.x;
    ret.v.y += b.v.y;
    return ret;
}
vect_t operator*(const vect_t a, const double b) {
    vect_t ret = a;
    ret.v.x *= b;
    ret.v.y *= b;
    return ret;
}

struct wall{
    double x, y;
};

struct platform{
    double x, y, width;
};

struct player_t {
    vect_t p; ///< position
    vect_t v; ///< velocity
    vect_t a; ///< acceleration
};

bool is_on_the_ground(player_t player, platform platform) {
    if (player.p.v.y > platform.y-1 && player.p.v.y < platform.y+10 && player.v.v.y >=0 && player.p.v.x >= platform.x && player.p.v.x <= platform.x + platform.width ||
        player.p.v.y > platform.y-1 && player.p.v.y < platform.y+10 && player.v.v.y >=0 && player.p.v.x+64 >= platform.x && player.p.v.x+64 <= platform.x + platform.width) return true;
    else return false;
}

bool did_hit_the_wall(player_t player, wall walls) {
    if (player.p.v.x < walls.x + 129 && player.p.v.x > walls.x ||
        player.p.v.x + 64 < walls.x + 129 && player.p.v.x + 64 > walls.x)
        return true;
    else return false;
}

player_t update_player(player_t player_old, double dt, bool hit_wall, bool is_ground) {
    player_t player = player_old;


    if(is_ground){
        player_old.v.v.y = 0;
        player_old.a.v.y = 0;
    }

    if (!is_ground) { // w powietrzu
        player_old.a.v.y = 500;
    }

    if (std::abs(player_old.v.v.x) > 5000) {
        player_old.a.v.x = 0;
    }

    if (hit_wall) {
        player_old.v.v.x = player_old.v.v.x*(-1.01);
        if (player_old.v.v.y < 0) player_old.v.v.y = player_old.v.v.y*1.1;
        player.p = player_old.p;
    }

    player.p = player_old.p + (player_old.v * dt) + (player_old.a*dt*dt)*0.5;
    player.v = player_old.v + (player_old.a * dt);
    player.v =  player.v * 0.99;

//    if (player.p.v.y > 480) {
//        player.p.v.y = 480;
//        player.v.v.y = 0;
//    }

    return player;
}


int main(int argc, char *argv[])
{
    using namespace std::chrono_literals;
    using namespace std::chrono;
    using namespace  std;
    SDL_Window *window;
    SDL_Renderer *renderer;

    double dt = 1./60.;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(960, 720, false, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    auto player_texture = load_image(renderer, "player.bmp");
    auto wall_texture = load_image(renderer, "wall.bmp");
    auto background_texture = load_image(renderer, "background.bmp");
    auto platform_texture = load_image(renderer, "platform.bmp");
    auto longplatform_texture = load_image(renderer, "longplatform.bmp");
    auto clock_texture = load_image(renderer, "clock.bmp");
    auto carrot_texture = load_image(renderer, "carrot.bmp");
    auto gameover_texture = load_image(renderer, "gameover.bmp");

    auto one_texture = load_image(renderer, "one.bmp");
    auto two_texture = load_image(renderer, "two.bmp");
    auto three_texture = load_image(renderer, "three.bmp");
    auto four_texture = load_image(renderer, "four.bmp");
    auto five_texture = load_image(renderer, "five.bmp");
    auto six_texture = load_image(renderer, "six.bmp");
    auto seven_texture = load_image(renderer, "seven.bmp");
    auto eight_texture = load_image(renderer, "eight.bmp");
    auto nine_texture = load_image(renderer, "nine.bmp");
    auto zero_texture = load_image(renderer, "zero.bmp");

    int difficulty = 1;
    int overclock = 1;
    bool did_hit;
    bool is_ground = false;
    bool mirror = false;
    bool still_playing = true;
    bool game_started = false;
    std::string score;

    player_t player;
    player.p.v.x = 320;
    player.p.v.y = 700;
    player.a.v.x = 0;
    player.a.v.y = 0;
    player.v.v.x = 0;
    player.v.v.y = 0;

    wall walls[12];
    for (int i = 0; i < 12; i++){
        if (i < 6) {
            walls[i].x = 0;
            walls[i].y = 128*i;
        }else{
            walls[i].x = 960-128;
            walls[i].y = 128*(i-6);
        }
    }

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(130, 640); // define the range

    platform platforms[5];
    for (int i = 0; i < 5; i++){
        platforms[i].y = 600-i*160;
        platforms[i].x = distr(gen);
        platforms[i].width = 192;
    }

    platform longplatform;
    longplatform.x = 130;
    longplatform.y = 700;
    longplatform.width = 700;

    double game_time = 0.;
    steady_clock::time_point current_time = steady_clock::now();
    while (still_playing) {
        // events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    still_playing = false;
                break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP && !game_started) {
                        game_started = true;
                        game_time = 0;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP && is_ground) {
                        if (player.v.v.x > 0) player.v.v.y = -500 - player.v.v.x*0.95;
                        else player.v.v.y = -500 + player.v.v.x*0.95;
                        player.p.v.y -= 5;
                    }
                    // if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) player.a.v.y = 50;
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                        player.a.v.x = -600;
                        mirror = true;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        player.a.v.x = 600;
                        mirror = false;
                    }
                    break;
                case SDL_KEYUP:
                    if (event.key.keysym.scancode == SDL_SCANCODE_Q) still_playing = false;
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP) player.a.v.y = 0;
                    //if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) player.a.v.y = 0;
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) player.a.v.x = 0;
                    if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)player.a.v.x = 0;
                    break;
            }
        }

        // physics

        game_time += dt;

        did_hit = false;
        is_ground = false;
        for (auto wall : walls){
            if (did_hit_the_wall(player, wall)){
                did_hit = true;
                break;
            }
        }
        for (auto platform : platforms){
            if (is_on_the_ground(player, platform)){
                is_ground = true;
                break;
            }
        }
        if (is_on_the_ground(player, longplatform)) is_ground = true;

        if (game_started){
            for (int i = 0; i < 5; i++){
                platforms[i].y += difficulty;
                if (platforms[i].y > 760) {
                    platforms[i].y = -32;
                    platforms[i].x = distr(gen);
                }
            }

            player.p.v.y += difficulty;
            if (longplatform.y < 1000) longplatform.y += 1;

            if (game_time > 20 && difficulty == 1) difficulty = 2;
            if (game_time > 40 && difficulty == 2) difficulty = 3;
            if (game_time > 60 && difficulty == 3) {
                difficulty = 4;
                overclock = 12;
            }

        }

        player = update_player(player, dt, did_hit, is_ground);

        // graphics
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background_texture.get(), NULL, NULL);

        SDL_Rect longplatform_rect = {(int)longplatform.x,(int)longplatform.y,700, 32};
        SDL_RenderCopy(renderer, longplatform_texture.get(), NULL, &longplatform_rect);

        for (auto & platform : platforms){
            SDL_Rect platform_rect = {(int)platform.x,(int)platform.y,192, 32};
            SDL_RenderCopy(renderer, platform_texture.get(), NULL, &platform_rect);
        }

        SDL_Rect player_rect = {(int)player.p.v.x,(int)player.p.v.y-128,64, 128};
        if (mirror) SDL_RenderCopyEx(renderer, player_texture.get(), NULL, &player_rect,0, NULL, SDL_FLIP_HORIZONTAL);
        else SDL_RenderCopyEx(renderer, player_texture.get(), NULL, &player_rect,0, NULL, SDL_FLIP_NONE);

        for (auto & wall : walls){
            SDL_Rect wall_rect = {(int)wall.x,(int)wall.y,128, 128};
            SDL_RenderCopy(renderer, wall_texture.get(), NULL, &wall_rect);
        }

        SDL_Rect clock_rect = {16,64,96, 96};
        SDL_RenderCopy(renderer, clock_texture.get(), NULL, &clock_rect);

        SDL_Rect carrot_rect = {16,64,96, 96};
        SDL_RenderCopyEx(renderer, carrot_texture.get(), NULL, &carrot_rect,game_time*360*game_started*overclock*0.05, NULL, SDL_FLIP_NONE);


        if (player.p.v.y > 790){
            if (player.p.v.y < 900) {
                int tmp = (int)game_time;
                score = std::to_string(tmp);
            }
            player.p.v.y = 900;
            SDL_Rect gameover_rect = {0,0,920, 780};
            SDL_RenderCopy(renderer, gameover_texture.get(), NULL, &gameover_rect);

            int i = 0;
            for (auto & number : score){
                SDL_Rect number_rect = {128+i,300,64, 128};
                switch(number){
                    case '1':
                        SDL_RenderCopy(renderer, one_texture.get(), NULL, &number_rect);
                        break;
                    case '2':
                        SDL_RenderCopy(renderer, two_texture.get(), NULL, &number_rect);
                        break;
                    case '3':
                        SDL_RenderCopy(renderer, three_texture.get(), NULL, &number_rect);
                        break;
                    case '4':
                        SDL_RenderCopy(renderer, four_texture.get(), NULL, &number_rect);
                        break;
                    case '5':
                        SDL_RenderCopy(renderer, five_texture.get(), NULL, &number_rect);
                        break;
                    case '6':
                        SDL_RenderCopy(renderer, six_texture.get(), NULL, &number_rect);
                        break;
                    case '7':
                        SDL_RenderCopy(renderer, seven_texture.get(), NULL, &number_rect);
                        break;
                    case '8':
                        SDL_RenderCopy(renderer, eight_texture.get(), NULL, &number_rect);
                        break;
                    case '9':
                        SDL_RenderCopy(renderer, nine_texture.get(), NULL, &number_rect);
                        break;
                    case '0':
                        SDL_RenderCopy(renderer, zero_texture.get(), NULL, &number_rect);
                        break;
                }
                i += 64;
            }
        }


        SDL_RenderPresent(renderer);
        // delays

        current_time = current_time + microseconds((long long int)(dt*1000000.0));
        std::this_thread::sleep_until(current_time);

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}