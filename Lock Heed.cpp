#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

using namespace std;

////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 700;
const int GROUND_Y = 600;

////////////////////////////////////////////////////////////
// ENUMS
////////////////////////////////////////////////////////////

enum GameState
{
    MENU,
    WW2_SELECT,
    GULF_SELECT,
    PLAYING,
    PAUSED
};

enum GameMode
{
    NONE,
    WW2,
    GULF
};

////////////////////////////////////////////////////////////
// REMOVE BG
////////////////////////////////////////////////////////////

void removeBG(sf::Image& img)
{
    for (unsigned int x = 0; x < img.getSize().x; x++)
    {
        for (unsigned int y = 0; y < img.getSize().y; y++)
        {
            sf::Color p = img.getPixel(x, y);

            if (p.g > 150 && p.g > p.r + 40 && p.g > p.b + 40)
                p.a = 0;

            if (p.r > 220 && p.g > 220 && p.b > 220)
                p.a = 0;

            img.setPixel(x, y, p);
        }
    }
}

////////////////////////////////////////////////////////////
// ABSTRACT ENTITY
////////////////////////////////////////////////////////////

class Entity
{
protected:
    sf::Sprite sprite;

public:

    virtual void update(float dt) = 0;

    virtual void draw(sf::RenderWindow& window)
    {
        window.draw(sprite);
    }

    virtual sf::FloatRect bounds()
    {
        return sprite.getGlobalBounds();
    }

    virtual sf::Vector2f pos()
    {
        return sprite.getPosition();
    }

    virtual ~Entity() {}
};

////////////////////////////////////////////////////////////
// ABSTRACT EFFECT
////////////////////////////////////////////////////////////

class Effect
{
protected:
    sf::Sprite sprite;

public:

    virtual bool update(float dt) = 0;

    virtual void draw(sf::RenderWindow& window)
    {
        window.draw(sprite);
    }

    virtual ~Effect() {}
};

////////////////////////////////////////////////////////////
// BLAST
////////////////////////////////////////////////////////////

class Blast : public Effect
{
private:

    float timer = 0;

public:

    Blast(sf::Texture& tex, sf::Vector2f position)
    {
        sprite.setTexture(tex);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setScale(0.25f, 0.25f);

        sprite.setPosition(position);
    }

    bool update(float dt) override
    {
        timer += dt;

        return timer > 0.15f;
    }
};

////////////////////////////////////////////////////////////
// EXPLOSION
////////////////////////////////////////////////////////////

class Explosion : public Effect
{
private:

    vector<sf::Texture>* frames;

    int frame = 0;

    float timer = 0;

public:

    Explosion(
        vector<sf::Texture>* tex,
        sf::Vector2f position,
        float scale)
    {
        frames = tex;

        sprite.setTexture((*frames)[0]);

        sprite.setOrigin(
            (*frames)[0].getSize().x / 2,
            (*frames)[0].getSize().y / 2
        );

        sprite.setScale(
            scale * 1.4f,
            scale * 1.4f
        );

        sprite.setPosition(position);
    }

    bool update(float dt) override
    {
        timer += dt;

        if (timer > 0.04f)
        {
            timer = 0;

            frame++;

            if (frame >= frames->size())
                return true;

            sprite.setTexture((*frames)[frame]);
        }

        return false;
    }
};

////////////////////////////////////////////////////////////
// SMOKE PARTICLE
////////////////////////////////////////////////////////////

class SmokeParticle : public Effect
{
private:

    float life = 0.8f;

    float timer = 0;

    float fadeSpeed;

public:

    SmokeParticle(
        sf::Texture& tex,
        sf::Vector2f pos,
        float scale)
    {
        sprite.setTexture(tex);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setPosition(pos);

        sprite.setScale(scale, scale);

        sprite.setRotation(rand() % 360);

        int alpha = 180 + rand() % 50;

        sprite.setColor(
            sf::Color(255, 255, 255, alpha)
        );

        fadeSpeed = 220 + rand() % 80;
    }

    bool update(float dt) override
    {
        timer += dt;

        sprite.move(0, -10 * dt);

        sprite.rotate(10 * dt);

        sf::Color c = sprite.getColor();

        if (c.a > 3)
            c.a -= fadeSpeed * dt;
        else
            c.a = 0;

        sprite.setColor(c);

        return timer >= life;
    }
};

////////////////////////////////////////////////////////////
// MISSILE
////////////////////////////////////////////////////////////

class Missile : public Entity
{
private:

    sf::Vector2f velocity;

    float ignitionDelay = 0.2f;

    float timer = 0;

    float speed = 220;

public:

    Missile(
        sf::Texture& tex,
        sf::Vector2f startPos,
        sf::Vector2f target)
    {
        sprite.setTexture(tex);

        sprite.setScale(0.07f, 0.07f);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setPosition(startPos);

        sf::Vector2f dir = target - startPos;

        float len = sqrt(dir.x * dir.x + dir.y * dir.y);

        if (len != 0)
            dir /= len;

        velocity = dir * speed;

        float angle =
            atan2(dir.y, dir.x) * 180 / 3.14159f;

        sprite.setRotation(angle + 180);
    }

    bool smokeReady()
    {
        return timer > 0.5f;
    }

    void update(float dt) override
    {
        timer += dt;

        if (timer < ignitionDelay)
            return;

        sprite.move(velocity * dt);
    }

    bool out()
    {
        auto p = sprite.getPosition();

        return p.x < -50 ||
            p.x > WINDOW_WIDTH + 50 ||
            p.y < -50 ||
            p.y > WINDOW_HEIGHT + 50;
    }

    sf::FloatRect bounds() override
    {
        sf::FloatRect b = sprite.getGlobalBounds();

        float shrink = 0.8f;

        b.left += b.width * shrink / 2;

        b.top += b.height * shrink / 2;

        b.width *= (1 - shrink);

        b.height *= (1 - shrink);

        return b;
    }
};

////////////////////////////////////////////////////////////
// JET BASE CLASS
////////////////////////////////////////////////////////////

class Jet : public Entity
{
protected:

    float speed;

    float wobbleTime = 0;

    float vx = 0;

    bool stealth;

    int bombCount;

public:

    bool alive = true;

    Jet(
        sf::Texture& tex,
        float spd,
        int bombs,
        bool stealthMode)
    {
        sprite.setTexture(tex);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setScale(0.2f, 0.2f);

        sprite.setPosition(600, 200);

        speed = spd;

        bombCount = bombs;

        stealth = stealthMode;
    }

    virtual void specialAbility()
    {
    }

    void update(float dt) override
    {
        if (!alive)
        {
            sprite.move(0, 200 * dt);

            sprite.rotate(100 * dt);

            return;
        }

        vx = 0;

        sprite.setRotation(0);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            sprite.move(-speed * dt, 0);

            vx = -speed;

            sprite.setRotation(-8);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            sprite.move(speed * dt, 0);

            vx = speed;

            sprite.setRotation(8);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            sprite.move(0, -speed * dt);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            sprite.move(0, speed * dt);

        wobbleTime += 3 * dt;

        sprite.move(
            0,
            sin(wobbleTime) * 15 * dt
        );

        sf::Vector2f p = sprite.getPosition();

        if (p.x < 60)
            p.x = 60;

        if (p.x > WINDOW_WIDTH - 60)
            p.x = WINDOW_WIDTH - 60;

        if (p.y < 60)
            p.y = 60;

        if (p.y > 450)
            p.y = 450;

        sprite.setPosition(p);
    }

    float getVX()
    {
        return vx;
    }

    bool isStealth()
    {
        return stealth;
    }

    int getBombCount()
    {
        return bombCount;
    }

    void setTexture(sf::Texture& tex)
    {
        sprite.setTexture(tex);
    }

    sf::FloatRect bounds() override
    {
        sf::FloatRect b = sprite.getGlobalBounds();

        float shrink = 0.7f;

        b.left += b.width * shrink / 2;

        b.top += b.height * shrink / 2;

        b.width *= (1 - shrink);

        b.height *= (1 - shrink);

        return b;
    }
};

////////////////////////////////////////////////////////////
// WW2 JET
////////////////////////////////////////////////////////////

class WW2Jet : public Jet
{
public:

    WW2Jet(
        sf::Texture& tex,
        float spd,
        int bombs)
        :
        Jet(tex, spd, bombs, false)
    {
    }

    void specialAbility() override
    {
    }
};

////////////////////////////////////////////////////////////
// GULF JET
////////////////////////////////////////////////////////////

class GulfJet : public Jet
{
public:

    GulfJet(
        sf::Texture& tex,
        float spd,
        int bombs,
        bool stealthMode)
        :
        Jet(tex, spd, bombs, stealthMode)
    {
    }

    void specialAbility() override
    {
    }
};

////////////////////////////////////////////////////////////
// BOMB
////////////////////////////////////////////////////////////

class Bomb : public Entity
{
private:

    float vx;

    float vy = 0;

public:

    Bomb(
        sf::Texture& tex,
        sf::Vector2f startPos,
        float jetVX)
    {
        sprite.setTexture(tex);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setScale(0.07f, 0.07f);

        sprite.setPosition(startPos);

        vx = jetVX;

        sprite.setRotation(90);
    }

    void update(float dt) override
    {
        vy += 600 * dt;

        sprite.move(vx * dt, vy * dt);

        sprite.rotate(-150 * dt);
    }

    bool hitGround()
    {
        return sprite.getPosition().y >= GROUND_Y;
    }
};

////////////////////////////////////////////////////////////
// TANK
////////////////////////////////////////////////////////////

class Tank : public Entity
{
private:

    float fireTimer = 0;

    float fireDelay;

    float scale;

public:

    Tank(
        sf::Texture& tex,
        float x,
        float difficulty)
    {
        sprite.setTexture(tex);

        float y = 450 + rand() % 150;

        scale =
            0.12f +
            (y - 450) / 300.f * 0.15f;

        sprite.setScale(scale, scale);

        sprite.setPosition(x, y);

        fireDelay =
            max(0.7f,
                2.0f - difficulty * 0.05f);
    }

    void update(float dt) override
    {
        fireTimer += dt;
    }

    bool ready()
    {
        if (fireTimer >= fireDelay)
        {
            fireTimer = 0;

            return true;
        }

        return false;
    }

    float getScale()
    {
        return scale;
    }

    sf::Vector2f gun()
    {
        return {
            sprite.getPosition().x + 25,
            sprite.getPosition().y
        };
    }

    sf::Vector2f explosionPos()
    {
        return {
            sprite.getPosition().x + 45,
            sprite.getPosition().y + 10
        };
    }
};

////////////////////////////////////////////////////////////
// HEAL PACK
////////////////////////////////////////////////////////////

class HealPack : public Entity
{
private:

    float lifeTimer = 0;

public:

    bool active = true;

    HealPack(sf::Texture& tex, sf::Vector2f position)
    {
        sprite.setTexture(tex);

        sprite.setOrigin(
            tex.getSize().x / 2,
            tex.getSize().y / 2
        );

        sprite.setScale(0.12f, 0.12f);

        sprite.setPosition(position);
    }

    void update(float dt) override
    {
        lifeTimer += dt;

        sprite.rotate(40 * dt);

        if (lifeTimer >= 5.0f)
            active = false;
    }
};

////////////////////////////////////////////////////////////
// GAME
////////////////////////////////////////////////////////////

class Game
{
private:

    sf::RenderWindow window;

    GameState state = MENU;

    GameMode mode = NONE;

    string base = "Assets/";

    ////////////////////////////////////////////////////////
    // MUSIC
    ////////////////////////////////////////////////////////

    sf::Music ww2Music;
    sf::Music gulfMusic;

    ////////////////////////////////////////////////////////
    // TEXTURES
    ////////////////////////////////////////////////////////

    sf::Texture bgTex;
    sf::Texture jetTex;
    sf::Texture bombTex;
    sf::Texture missileTex;
    sf::Texture blastTex;
    sf::Texture smokeTex;

    sf::Texture menuTex;
    sf::Texture crashTex;
    sf::Texture goTex;

    sf::Texture ww2MenuTex;
    sf::Texture gulfMenuTex;
    sf::Texture pausedTex;
    sf::Texture healTex;
    sf::Texture scoreBoardTex;
    sf::Texture scoreDigits[10];

    sf::Texture hpTex[6];

    vector<sf::Texture> tankTextures;
    vector<sf::Texture> explosionFrames;

    ////////////////////////////////////////////////////////
    // SPRITES
    ////////////////////////////////////////////////////////

    sf::Sprite background;
    sf::Sprite menuSprite;
    sf::Sprite goSprite;
    sf::Sprite ww2MenuSprite;
    sf::Sprite gulfMenuSprite;
    sf::Sprite pausedSprite;
    sf::Sprite hpSprite;
    sf::Sprite scoreBoardSprite;
    sf::Sprite digitSprites[6];
    HealPack* healPack = nullptr;

    ////////////////////////////////////////////////////////
    // OBJECTS
    ////////////////////////////////////////////////////////

    Jet* jet = nullptr;

    vector<Tank> tanks;
    vector<Missile> missiles;
    vector<Bomb> bombs;
    vector<Explosion> explosions;
    vector<Blast> blasts;
    vector<SmokeParticle> smokeParticles;

    ////////////////////////////////////////////////////////
    // DOUBLE BOMB
    ////////////////////////////////////////////////////////

    bool secondBombPending = false;

    float secondBombTimer = 0;

    sf::Vector2f pendingBombPos;

    float pendingBombVX = 0;

    ////////////////////////////////////////////////////////
    // GAMEPLAY
    ////////////////////////////////////////////////////////

    int health = 5;

    int score = 0;

    float spawnTimer = 0;

    float difficulty = 0;

    float healSpawnTimer = 0;

    float nextHealSpawn = 7;

    bool gameOver = false;

    ////////////////////////////////////////////////////////
    // FADE
    ////////////////////////////////////////////////////////

    float fadeAlpha = 0;

    float shakeTimer = 0;

    float shakeStrength = 0;

    float hitFlashTimer = 0;

    sf::RectangleShape redFlash;

    float goTimer = 0;

    sf::RectangleShape fadeRect;

    ////////////////////////////////////////////////////////
    // CLOCK
    ////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////
// VIDEO CUTSCENE
////////////////////////////////////////////////////////

    sf::Texture currentVideoTexture;

    sf::Sprite videoSprite;

    sf::Music videoAudio;

    vector<sf::Texture> videoFrames;

    int nextFrameToLoad = 1;

    bool videoPreloaded = false;

    float backgroundLoadTimer = 0;

    bool playingVideo = false;

    bool waitingForVideo = false;

    float videoDelayTimer = 0;

    int currentVideoFrame = 0;

    float videoTimer = 0;

    float videoFrameDuration = 1.0f / 25.0f;

    sf::Clock clock;

    ////////////////////////////////////////////////////////
// LOAD VIDEO
////////////////////////////////////////////////////////

    void loadVideo()
    {
        videoFrames.clear();

        try
        {
            if (!videoAudio.openFromFile(
                base +
                "Videos/New Project 150/AUDIO.ogg"))
            {
                throw runtime_error(
                    "Video audio missing"
                );
            }
        }
        catch (exception& e)
        {
            cout << "Video Error: "
                << e.what()
                << endl;
        }

        videoAudio.setVolume(100);
    }

public:

    Game()
        :
        window(
            sf::VideoMode(
                WINDOW_WIDTH,
                WINDOW_HEIGHT
            ),
            "Lockheed"
        )
    {
        window.setFramerateLimit(60);

        loadAssets();

        loadMusic();
    }

    ////////////////////////////////////////////////////////
    // LOAD MUSIC
    ////////////////////////////////////////////////////////

    void loadMusic()
    {
        try
        {
            if (!ww2Music.openFromFile(
                base + "Sounds/WW2BGM.ogg"))
            {
                throw runtime_error(
                    "WW2BGM.ogg could not be loaded"
                );
            }

            if (!gulfMusic.openFromFile(
                base + "Sounds/GFBGM.ogg"))
            {
                throw runtime_error(
                    "GFBGM.ogg could not be loaded"
                );
            }

            ww2Music.setLoop(true);
            gulfMusic.setLoop(true);

            ww2Music.setVolume(50);
            gulfMusic.setVolume(50);
        }
        catch (exception& e)
        {
            cout << "Music Error: "
                << e.what()
                << endl;
        }
    }

    ////////////////////////////////////////////////////////
    // STOP MUSIC
    ////////////////////////////////////////////////////////

    void stopAllMusic()
    {
        ww2Music.stop();
        gulfMusic.stop();
    }

    ////////////////////////////////////////////////////////
    // PLAY MODE MUSIC
    ////////////////////////////////////////////////////////

    void playModeMusic()
    {
        stopAllMusic();

        if (mode == WW2)
            ww2Music.play();

        else if (mode == GULF)
            gulfMusic.play();
    }

    ////////////////////////////////////////////////////////
    // LOAD ASSETS
    ////////////////////////////////////////////////////////

    void loadAssets()
    {
        try
        {
            if (!menuTex.loadFromFile(
                base + "Backgrounds/Menu.jpeg"))
            {
                throw runtime_error(
                    "Menu.jpeg not found"
                );
            }
        }
        catch (exception& e)
        {
            cout << "Texture Error: "
                << e.what()
                << endl;
        }

        menuSprite.setTexture(menuTex);

        menuSprite.setScale(
            (float)WINDOW_WIDTH /
            menuTex.getSize().x,

            (float)WINDOW_HEIGHT /
            menuTex.getSize().y
        );

        ww2MenuTex.loadFromFile(
            base + "UI/ww2.png"
        );

        ww2MenuSprite.setTexture(ww2MenuTex);

        ww2MenuSprite.setScale(
            (float)WINDOW_WIDTH /
            ww2MenuTex.getSize().x,

            (float)WINDOW_HEIGHT /
            ww2MenuTex.getSize().y
        );

        gulfMenuTex.loadFromFile(
            base + "UI/gf.png"
        );

        gulfMenuSprite.setTexture(gulfMenuTex);

        gulfMenuSprite.setScale(
            (float)WINDOW_WIDTH /
            gulfMenuTex.getSize().x,

            (float)WINDOW_HEIGHT /
            gulfMenuTex.getSize().y
        );

        ////////////////////////////////////////////////////////
        // PAUSED SCREEN
        ////////////////////////////////////////////////////////

        pausedTex.loadFromFile(
            base + "Backgrounds/PAUSED.png"
        );

        pausedSprite.setTexture(pausedTex);

        pausedSprite.setScale(
            (float)WINDOW_WIDTH /
            pausedTex.getSize().x,

            (float)WINDOW_HEIGHT /
            pausedTex.getSize().y
        );

        goTex.loadFromFile(
            base + "Backgrounds/GO.jpeg"
        );

        goSprite.setTexture(goTex);

        goSprite.setScale(
            (float)WINDOW_WIDTH /
            goTex.getSize().x,

            (float)WINDOW_HEIGHT /
            goTex.getSize().y
        );

        fadeRect.setSize(
            sf::Vector2f(
                WINDOW_WIDTH,
                WINDOW_HEIGHT
            )
        );

        fadeRect.setFillColor(
            sf::Color(0, 0, 0, 0)
        );

        redFlash.setSize(
            sf::Vector2f(
                WINDOW_WIDTH,
                WINDOW_HEIGHT
            )
        );

        redFlash.setFillColor(
            sf::Color(255, 0, 0, 0)
        );

        sf::Image img;

        img.loadFromFile(
            base + "Effects/Blast.jpeg"
        );

        removeBG(img);

        blastTex.loadFromImage(img);

        smokeTex.loadFromFile(
            base + "Effects/Smoke.png"
        );

        crashTex.loadFromFile(
            base + "Jets/DF22.png"
        );

        healTex.loadFromFile(
            base + "UI/HP.png"
        );

        ////////////////////////////////////////////////////////
// SCOREBOARD
////////////////////////////////////////////////////////

        scoreBoardTex.loadFromFile(
            base + "UI/SB.png"
        );

        scoreBoardSprite.setTexture(scoreBoardTex);

        scoreBoardSprite.setScale(0.22f, 0.22f);

        scoreBoardSprite.setPosition(20, 20);

        for (int i = 0; i < 10; i++)
        {
            scoreDigits[i].loadFromFile(
                base + "UI/S" +
                to_string(i) +
                ".png"
            );
        }

        for (int i = 0; i < 6; i++)
        {
            digitSprites[i].setTexture(
                scoreDigits[0]
            );

            digitSprites[i].setScale(
                0.035f,
                0.035f
            );
        }

        for (int i = 0; i <= 5; i++)
        {
            hpTex[i].loadFromFile(
                base +
                "UI/" +
                to_string(i) +
                ".png"
            );
        }

        hpSprite.setTexture(hpTex[5]);

        hpSprite.setScale(0.22f, 0.22f);

        hpSprite.setPosition(470, -100);



    }

    ////////////////////////////////////////////////////////
    // LOAD MODE
    ////////////////////////////////////////////////////////

    void loadMode(
        GameMode m,
        float jetSpeed,
        int bombCount,
        bool stealthMode)
    {
        mode = m;

        if (mode == GULF)
            bgTex.loadFromFile(
                base + "Backgrounds/GulfWar.png"
            );
        else
            bgTex.loadFromFile(
                base + "Backgrounds/WW2.png"
            );

        background.setTexture(bgTex);

        missileTex.loadFromFile(
            base + "Enemies/Missile.png"
        );

        if (jet != nullptr)
            delete jet;

        if (mode == WW2)
        {
            jet = new WW2Jet(
                jetTex,
                jetSpeed,
                bombCount
            );
        }
        else
        {
            jet = new GulfJet(
                jetTex,
                jetSpeed,
                bombCount,
                stealthMode
            );
        }

        playModeMusic();

        explosionFrames.clear();

        for (int i = 1; i <= 10; i++)
        {
            sf::Image img;

            img.loadFromFile(
                base +
                "Effects/Explosion " +
                to_string(i) +
                ".png"
            );

            removeBG(img);

            sf::Texture t;

            t.loadFromImage(img);

            explosionFrames.push_back(t);
        }

        tankTextures.clear();

        vector<string> names =
        {
            "T72.png",
            "T62.png",
            "T55.png",
            "T69.png"
        };

        for (auto& n : names)
        {
            try
            {
                sf::Texture t;

                if (!t.loadFromFile(
                    base + "Enemies/" + n))
                {
                    throw runtime_error(
                        "Missing tank texture: " + n
                    );
                }

                tankTextures.push_back(t);
            }
            catch (exception& e)
            {
                cout << "Tank Error: "
                    << e.what()
                    << endl;
            }
        }

        tanks.clear();
        missiles.clear();
        bombs.clear();
        explosions.clear();
        blasts.clear();

        secondBombPending = false;

        secondBombTimer = 0;

        health = 5;

        score = 0;

        hpSprite.setTexture(hpTex[5]);

        spawnTimer = 0;

        difficulty = 0;

        gameOver = false;

        fadeAlpha = 0;

        goTimer = 0;

        healSpawnTimer = 0;

        nextHealSpawn = 5 + rand() % 6;

        if (healPack != nullptr)
        {
            delete healPack;
            healPack = nullptr;
        }

        state = PLAYING;

        clock.restart();
    }

    ////////////////////////////////////////////////////////
    // RUN
    ////////////////////////////////////////////////////////

    void run()
    {
        while (window.isOpen())
        {
            float dt =
                clock.restart().asSeconds();

            handle();

            if (state == PLAYING)
                update(dt);

            render();
        }
    }

    ////////////////////////////////////////////////////////
    // HANDLE
    ////////////////////////////////////////////////////////

    void handle()
    {
        sf::Event e;

        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();

            if (e.type == sf::Event::KeyPressed)
            {
                if (playingVideo)
                {
                    if (e.key.code == sf::Keyboard::Enter)
                    {
                        playingVideo = false;

                        videoAudio.stop();

                        state = MENU;
                    }

                    continue;
                }

                if (state == MENU)
                {
                    if (e.key.code == sf::Keyboard::H)
                        state = WW2_SELECT;

                    if (e.key.code == sf::Keyboard::B)
                        state = GULF_SELECT;
                }

                else if (state == WW2_SELECT)
                {
                    if (e.key.code == sf::Keyboard::A)
                    {
                        jetTex.loadFromFile(
                            base + "Jets/ME262.png"
                        );

                        bombTex.loadFromFile(
                            base +
                            "Weapons/BombWW2.png"
                        );

                        loadMode(
                            WW2,
                            380,
                            1,
                            false
                        );
                    }

                    if (e.key.code == sf::Keyboard::B)
                    {
                        jetTex.loadFromFile(
                            base + "Jets/HE162.png"
                        );

                        bombTex.loadFromFile(
                            base +
                            "Weapons/BombWW2.png"
                        );

                        loadMode(
                            WW2,
                            280,
                            2,
                            false
                        );
                    }
                }

                else if (state == GULF_SELECT)
                {
                    if (e.key.code == sf::Keyboard::A)
                    {
                        jetTex.loadFromFile(
                            base + "Jets/F22.png"
                        );

                        bombTex.loadFromFile(
                            base +
                            "Weapons/BombGF.png"
                        );

                        loadMode(
                            GULF,
                            420,
                            1,
                            false
                        );
                    }

                    if (e.key.code == sf::Keyboard::B)
                    {
                        jetTex.loadFromFile(
                            base + "Jets/F35.png"
                        );

                        bombTex.loadFromFile(
                            base +
                            "Weapons/BombGF.png"
                        );

                        loadMode(
                            GULF,
                            320,
                            2,
                            true
                        );
                    }
                }

                else
                {
                    ////////////////////////////////////////////////////////
                    // PAUSE SYSTEM
                    ////////////////////////////////////////////////////////

                    if (e.key.code == sf::Keyboard::Escape)
                    {
                        if (state == PLAYING)
                        {
                            state = PAUSED;

                            ww2Music.pause();
                            gulfMusic.pause();
                        }
                        else if (state == PAUSED)
                        {
                            state = PLAYING;

                            if (mode == WW2)
                                ww2Music.play();

                            else if (mode == GULF)
                                gulfMusic.play();
                        }
                    }

                    if (state != PLAYING)
                        continue;

                    if (
                        e.key.code ==
                        sf::Keyboard::Space &&
                        jet->alive
                        )
                    {
                        bombs.emplace_back(
                            bombTex,
                            jet->pos(),
                            jet->getVX()
                        );

                        if (
                            jet->getBombCount() == 2
                            )
                        {
                            secondBombPending = true;

                            secondBombTimer = 0;

                            pendingBombPos =
                                jet->pos();

                            pendingBombVX =
                                jet->getVX();
                        }
                    }
                }
            }
        }
    }

    ////////////////////////////////////////////////////////
    // UPDATE
    ////////////////////////////////////////////////////////

    void update(float dt)
    {

        ////////////////////////////////////////////////////////
// VIDEO DELAY
////////////////////////////////////////////////////////

        if (waitingForVideo)
        {
            videoDelayTimer += dt;

            fadeAlpha += 80 * dt;

            if (fadeAlpha > 255)
                fadeAlpha = 255;

            fadeRect.setFillColor(
                sf::Color(
                    0,
                    0,
                    0,
                    (int)fadeAlpha
                )
            );

            if (videoDelayTimer >= 3.0f)
            {
                waitingForVideo = false;

                playingVideo = true;

                currentVideoFrame = 0;

                videoTimer = 0;

                videoAudio.play();

                shakeTimer = 0;
                shakeStrength = 0;

                redFlash.setFillColor(
                    sf::Color(255, 0, 0, 0)
                );
            }

            return;
        }

        ////////////////////////////////////////////////////////
// VIDEO PLAYBACK
////////////////////////////////////////////////////////

        if (playingVideo)
        {
            videoTimer += dt;

            while (videoTimer >= videoFrameDuration)
            {
                videoTimer -= videoFrameDuration;

                currentVideoFrame++;

                if (currentVideoFrame > 1147)
                {
                    playingVideo = false;

                    videoAudio.stop();

                    state = MENU;

                    return;
                }

                stringstream ss;

                ss << setw(4)
                    << setfill('0')
                    << currentVideoFrame;

                string path =
                    base +
                    "Videos/New Project 150/" +
                    ss.str() +
                    "_4.jpeg";

                currentVideoTexture.loadFromFile(path);

                videoSprite.setTexture(currentVideoTexture);
            }

            return;
        }

        jet->update(dt);

        ////////////////////////////////////////////////////////////
// HEAL PACK SPAWN
////////////////////////////////////////////////////////////

        healSpawnTimer += dt;

        if (healSpawnTimer >= nextHealSpawn &&
            healPack == nullptr)
        {
            healSpawnTimer = 0;

            nextHealSpawn = 5 + rand() % 6;

            sf::Vector2f healPos(
                100 + rand() % (WINDOW_WIDTH - 200),
                120 + rand() % 250
            );

            healPack = new HealPack(
                healTex,
                healPos
            );
        }

        if (healPack != nullptr)
        {
            healPack->update(dt);

            if (
                jet->alive &&
                healPack->bounds().intersects(
                    jet->bounds()
                )
                )
            {
                if (health < 5)
                {
                    health++;

                    hpSprite.setTexture(
                        hpTex[health]
                    );
                }

                delete healPack;

                healPack = nullptr;
            }
            else if (!healPack->active)
            {
                delete healPack;

                healPack = nullptr;
            }
        }

        if (secondBombPending)
        {
            secondBombTimer += dt;

            if (secondBombTimer >= 0.15f)
            {
                bombs.emplace_back(
                    bombTex,
                    pendingBombPos,
                    pendingBombVX
                );

                secondBombPending = false;
            }
        }

        spawnTimer += dt;

        if (spawnTimer > 2.0f &&
            tanks.size() < 6)
        {
            spawnTimer = 0;

            float x =
                100 +
                rand() %
                (WINDOW_WIDTH - 200);

            tanks.emplace_back(
                tankTextures[
                    rand() %
                        tankTextures.size()
                ],
                x,
                difficulty
            );
        }

        for (auto& t : tanks)
        {
            t.update(dt);

            if (t.ready())
            {
                if (jet->isStealth())
                {
                    int chance = rand() % 10;

                    if (chance == 0)
                        continue;
                }

                missiles.emplace_back(
                    missileTex,
                    t.gun(),
                    jet->pos()
                );
            }
        }

        for (int i = 0; i < missiles.size(); i++)
        {
            missiles[i].update(dt);

            if (missiles[i].smokeReady())
            {
                int smokeCount = 1 + rand() % 2;

                for (int s = 0; s < smokeCount; s++)
                {
                    sf::Vector2f smokePos =
                    {
                        missiles[i].pos().x + (rand() % 12 - 6),
                        missiles[i].pos().y + (rand() % 12 - 6)
                    };

                    float smokeScale =
                        0.04f +
                        (rand() % 5) * 0.01f;

                    smokeParticles.emplace_back(
                        smokeTex,
                        smokePos,
                        smokeScale
                    );
                }
            }

            if (
                jet->alive &&
                missiles[i].bounds().intersects(
                    jet->bounds()
                )
                )
            {
                blasts.emplace_back(
                    blastTex,
                    jet->pos()
                );

                health--;

                shakeTimer = 0.25f;

                shakeStrength = 12.f;

                hitFlashTimer = 0.18f;

                hpSprite.setTexture(
                    hpTex[max(0, health)]
                );

                missiles.erase(
                    missiles.begin() + i
                );

                i--;

                if (health <= 0)
                {
                    jet->alive = false;

                    jet->setTexture(crashTex);

                    stopAllMusic();

                    waitingForVideo = true;

                    videoDelayTimer = 0;

                    loadVideo();

                    shakeTimer = 0;
                    shakeStrength = 0;

                    redFlash.setFillColor(
                        sf::Color(255, 0, 0, 0)
                    );
                }

                continue;
            }

            if (missiles[i].out())
            {
                missiles.erase(
                    missiles.begin() + i
                );

                i--;
            }
        }

        for (int i = 0; i < bombs.size(); i++)
        {
            bombs[i].update(dt);

            for (int j = 0; j < tanks.size(); j++)
            {
                if (
                    bombs[i].bounds().intersects(
                        tanks[j].bounds()
                    )
                    )
                {
                    explosions.emplace_back(
                        &explosionFrames,
                        tanks[j].explosionPos(),
                        tanks[j].getScale()
                    );

                    score += 10;

                    tanks.erase(
                        tanks.begin() + j
                    );

                    bombs.erase(
                        bombs.begin() + i
                    );

                    i--;

                    break;
                }
            }

            if (
                i >= 0 &&
                bombs[i].hitGround()
                )
            {
                explosions.emplace_back(
                    &explosionFrames,
                    bombs[i].pos(),
                    0.25f
                );

                bombs.erase(
                    bombs.begin() + i
                );

                i--;
            }
        }

        for (int i = 0; i < explosions.size(); i++)
        {
            if (explosions[i].update(dt))
            {
                explosions.erase(
                    explosions.begin() + i
                );

                i--;
            }
        }

        for (int i = 0; i < blasts.size(); i++)
        {
            if (blasts[i].update(dt))
            {
                blasts.erase(
                    blasts.begin() + i
                );

                i--;
            }
        }

        for (int i = 0; i < smokeParticles.size(); i++)
        {
            if (smokeParticles[i].update(dt))
            {
                smokeParticles.erase(
                    smokeParticles.begin() + i
                );

                i--;
            }
        }

        updateScoreDisplay();

        if (shakeTimer > 0)
        {
            shakeTimer -= dt;

            shakeStrength *= 0.9f;
        }

        if (hitFlashTimer > 0)
        {
            hitFlashTimer -= dt;

            int alpha =
                (hitFlashTimer / 0.18f) * 120;

            redFlash.setFillColor(
                sf::Color(255, 0, 0, alpha)
            );
        }
        else
        {
            redFlash.setFillColor(
                sf::Color(255, 0, 0, 0)
            );
        }

        if (gameOver)
        {
            goTimer += dt;

            fadeAlpha += 120 * dt;

            if (fadeAlpha > 255)
                fadeAlpha = 255;

            fadeRect.setFillColor(
                sf::Color(
                    0,
                    0,
                    0,
                    (int)fadeAlpha
                )
            );

            if (goTimer > 3.0f)
            {
                stopAllMusic();

                state = MENU;
            }
        }
    }

    ////////////////////////////////////////////////////////
// UPDATE SCORE DISPLAY
////////////////////////////////////////////////////////

    void updateScoreDisplay()
    {
        int temp = score;

        int digits[6] = { 0 };

        for (int i = 5; i >= 0; i--)
        {
            digits[i] = temp % 10;

            temp /= 10;
        }

        float startX = 102;
        float y = 87;

        for (int i = 0; i < 6; i++)
        {
            digitSprites[i].setTexture(
                scoreDigits[digits[i]]
            );

            digitSprites[i].setPosition(
                startX + i * 21,
                y
            );
        }
    }

    ////////////////////////////////////////////////////////
    // RENDER
    ////////////////////////////////////////////////////////

    void render()
    {
        window.clear();

        sf::View view = window.getDefaultView();

        if (shakeTimer > 0)
        {
            float offsetX =
                (rand() % 21 - 10) *
                shakeStrength * 0.1f;

            float offsetY =
                (rand() % 21 - 10) *
                shakeStrength * 0.1f;

            view.move(offsetX, offsetY);
        }

        window.setView(view);

        if (state == MENU)
        {
            window.draw(menuSprite);
        }

        else if (state == WW2_SELECT)
        {
            window.draw(ww2MenuSprite);
        }

        else if (state == GULF_SELECT)
        {
            window.draw(gulfMenuSprite);
        }

        else
        {
            if (playingVideo)
            {
                window.draw(videoSprite);

                window.display();

                return;
            }

            window.draw(background);

            for (auto& t : tanks)
                t.draw(window);

            for (auto& s : smokeParticles)
                s.draw(window);

            for (auto& m : missiles)
                m.draw(window);

            for (auto& b : bombs)
                b.draw(window);

            for (auto& e : explosions)
                e.draw(window);

            for (auto& bl : blasts)
                bl.draw(window);

            if (healPack != nullptr)
                healPack->draw(window);

            jet->draw(window);

            window.draw(hpSprite);

            ////////////////////////////////////////////////////////
            // SCOREBOARD
            ////////////////////////////////////////////////////////

            window.draw(scoreBoardSprite);

            for (int i = 0; i < 6; i++)
            {
                window.draw(digitSprites[i]);
            }

            ////////////////////////////////////////////////////////
            // PAUSED SCREEN
            ////////////////////////////////////////////////////////

            if (state == PAUSED)
            {
                sf::RectangleShape overlay;

                overlay.setSize(
                    sf::Vector2f(
                        WINDOW_WIDTH,
                        WINDOW_HEIGHT
                    )
                );

                overlay.setFillColor(
                    sf::Color(0, 0, 0, 140)
                );

                window.draw(overlay);

                window.draw(pausedSprite);
            }

            if (gameOver)
            {
                window.draw(fadeRect);

                if (fadeAlpha >= 255)
                    window.draw(goSprite);
            }
        }

        if (waitingForVideo)
        {
            window.draw(fadeRect);
        }

        window.draw(redFlash);

        window.display();
    }

    ////////////////////////////////////////////////////////
    // DESTRUCTOR
    ////////////////////////////////////////////////////////

    ~Game()
    {
        stopAllMusic();
        if (healPack != nullptr)
            delete healPack;
        if (jet != nullptr)
            delete jet;
    }
};

////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////

int main()
{
    try
    {
        Game g;

        g.run();
    }
    catch (exception& e)
    {
        cout << "Fatal Error: "
            << e.what()
            << endl;
    }

    return 0;
}