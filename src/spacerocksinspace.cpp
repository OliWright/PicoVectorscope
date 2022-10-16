#include "picovectorscope.h"

//
// Constants
//

static constexpr DisplayListScalar kBulletSpeed(0.01f);
static constexpr uint32_t kBulletLife = 60;
static constexpr DisplayListScalar kGlobalScale(0.015f);
static constexpr float kGlobalScaleFloat = (float) kGlobalScale;
static constexpr DisplayListScalar kParticleSpeed(0.01f);
static constexpr DisplayListScalar kFragmentSpeed(0.002f);
static constexpr float kPlayerShipRotationSpeed = 0.1f;
static constexpr DisplayListScalar kThrust = 0.0003f;
static constexpr DisplayListScalar kDrag = 0.99f;
static constexpr float kAsteroidMaxRotationSpeed = 0.03f;
static constexpr float kAsteroidCollisionRadius = 2.7f;
static constexpr float kBulletMass = 0.1f;
static constexpr uint kMaxBullets = 4;
static constexpr uint kMaxParticles = 128;
static constexpr uint kParticleLife = 30;
static constexpr uint kMaxAsteroids = 32;
static constexpr uint kNumLives = 3;
static constexpr DisplayListScalar kPlayerShipScale(kGlobalScale);
static constexpr DisplayListScalar kBulletLaunchOffset(kPlayerShipScale * 3.3f);
static constexpr DisplayListScalar kSafeRadius(kGlobalScale * 10.f);
static constexpr Intensity kRecipParticleLife = Intensity(1.f) / Intensity(kParticleLife);

static const char* s_title = "SPACE ROCKS\nIN\nSPACE";
static const BurnLength s_titleTotalBurnLength = CalcBurnLength(s_title);
static const char* s_gameOver = "GAME\nOVER";
static const BurnLength s_gameOverTotalBurnLength = CalcBurnLength(s_gameOver);
static FloatTransform2D s_titleTextTransform;

static uint32_t s_frameCounter = 0;

//
// Some helper functions
//

static void normalize(DisplayListVector2& vec)
{
    DisplayListScalar::MathsIntermediateType length = ((vec.x * vec.x) + (vec.y * vec.y)).sqrt();
    DisplayListScalar::MathsIntermediateType recipLength = DisplayListScalar(1.f) / length;

    vec.x *= recipLength;
    vec.y *= recipLength;
}

static void initVecRand0to1(DisplayListVector2& vec)
{
    vec.x = DisplayListScalar::randZeroToOne();
    vec.y = DisplayListScalar::randZeroToOne();
}

static void applyDrag(DisplayListScalar& val, const DisplayListScalar& drag)
{
    // If we just multiply by drag, then -ve numbers will never get to zero
    // because of fixed point things.
    if(val < DisplayListScalar(0))
    {
        val = DisplayListScalar(0) - ((DisplayListScalar(0) - val) * drag); // TODO: unary - operator
    }
    else
    {
        val *= drag;
    }
}

static void applyDrag(DisplayListVector2& vec, const DisplayListScalar& drag)
{
    applyDrag(vec.x, drag);
    applyDrag(vec.y, drag);
}

static bool testCollisionOctagon(const DisplayListVector2& p0, const DisplayListVector2& p1, DisplayListScalar distance)
{
    // Octagon collision
    DisplayListScalar dx = (p1.x - p0.x).abs();
    DisplayListScalar dy = (p1.y - p0.y).abs();
    return (dx < distance) && (dy < distance) && ((dx + dy) < distance);
}

//
// Classes and Structs
//

struct Fragments
{
    static void Reset()
    {
        for(uint i = 0; i < kMaxFragments; ++i)
        {
            s_fragments[i].m_intensity = 0;
        }
        s_numFragments = 0;
    }

    static void UpdateAndDraw(DisplayList& displayList)
    {
        for(uint i = 0; i < s_numFragments; ++i)
        {
            Fragment& fragment = s_fragments[i];
            fragment.m_intensity -= 0.01f;
            if(fragment.m_intensity < 0)
            {
                // Fragment is dead
                // Swap this slot with the end
                fragment = s_fragments[--s_numFragments];
                --i;
                continue;
            }
            
            fragment.Move();
        }

        PushFragmentsToDisplayList(displayList, s_fragments, s_numFragments);
    }

    static void Add(GameShape shape, const DisplayListVector2& baseSpeed, const FloatTransform2D& transform, Intensity intensity)
    {
        uint numNewFragments = FragmentGameShape(shape, transform, s_fragments + s_numFragments, kMaxFragments - s_numFragments);
        for(uint i = 0; i < numNewFragments; ++i)
        {
            Fragment& fragment = s_fragments[s_numFragments + i];
            fragment.m_velocity.x = baseSpeed.x + (kFragmentSpeed * DisplayListScalar::randMinusOneToOne());
            fragment.m_velocity.y = baseSpeed.y + (kFragmentSpeed * DisplayListScalar::randMinusOneToOne());
            fragment.m_intensity = intensity;
            fragment.m_rotationSpeed = DisplayListScalar::randMinusOneToOne() * 0.05f;
        }
        s_numFragments += numNewFragments;
    }

    static void Add(const char* message, const FloatTransform2D& transform)
    {
        uint numNewFragments = FragmentText(message, transform, s_fragments + s_numFragments, kMaxFragments - s_numFragments, true);
        for(uint i = 0; i < numNewFragments; ++i)
        {
            Fragment& fragment = s_fragments[s_numFragments + i];
            fragment.m_velocity.x = (kFragmentSpeed * DisplayListScalar::randMinusOneToOne()) * 0.5f;
            fragment.m_velocity.y = (kFragmentSpeed * DisplayListScalar::randMinusOneToOne()) * 0.5f;
            fragment.m_intensity = 2.f;
            fragment.m_rotationSpeed = DisplayListScalar::randMinusOneToOne() * 0.02f;
        }
        s_numFragments += numNewFragments;
    }

    static constexpr uint kMaxFragments = 256;
    static Fragment s_fragments[kMaxFragments];
    static uint s_numFragments;
};
Fragment Fragments::s_fragments[Fragments::kMaxFragments] = {};
uint Fragments::s_numFragments = 0;

// Base class for all objects in the game.
// This is classic OOP.
struct BaseObject
{
    DisplayListVector2 m_position;
    DisplayListVector2 m_velocity;
    Intensity m_brightness;
    bool m_active;

    BaseObject() : m_active(false) {}

    void Move()
    {
        m_position.x = (m_position.x + m_velocity.x).frac();
        m_position.y = (m_position.y + m_velocity.y).frac();
    }
};

// An intermediate class for objects using a GameShape with scale and rotation
struct ShapeObject : public BaseObject
{
    GameShape m_shape;
    float m_rotation;
    float m_scale;

    ShapeObject() : BaseObject() {}

    void Rotate(float angle)
    {
        m_rotation += angle;

        if (m_rotation > (kPi * 2.f))
        {
            m_rotation -= kPi * 2.f;
        }
        if (m_rotation < 0.f)
        {
            m_rotation += kPi * 2.f;
        }
    }

    void CalcTransform(FloatTransform2D& outTransform)
    {
        float s, c;
        sincosf(m_rotation, &s, &c);

        outTransform.setAsRotation(s, c, FloatVector2(0.f, 0.f));
        outTransform *= m_scale;
        outTransform.setTranslation(FloatVector2((float) m_position.x, (float) m_position.y));
    }

    void UpdateAndDraw(DisplayList& displayList)
    {
        if(!m_active)
        {
            return;
        }

        Move();

        FloatTransform2D transform;
        CalcTransform(transform);
        PushGameShape(displayList, transform, m_shape, m_brightness);
    }
};

// Bullets kill themselves after a while, and draw as a point
struct Bullet : public BaseObject
{
    uint32_t m_launchFrame;

    static Bullet s_bullets[kMaxBullets];

    void UpdateAndDraw(DisplayList& displayList)
    {
        if(!m_active)
        {
            return;
        }
        if((s_frameCounter - m_launchFrame) > kBulletLife)
        {
            m_active = false;
            return;
        }

        //m_brightness = Intensity::randZeroToOne();

        Move();
        displayList.PushPoint(m_position.x, m_position.y, m_brightness);
    }

    void Fire(const DisplayListVector2& position, const DisplayListVector2& direction)
    {
        m_position.x = position.x + (direction.x * kBulletLaunchOffset);
        m_position.y = position.y + (direction.y * kBulletLaunchOffset);
        m_brightness = Intensity(1.f);
        m_velocity.x = direction.x * kBulletSpeed;
        m_velocity.y = direction.y * kBulletSpeed;
        m_launchFrame = s_frameCounter;
        m_active = true;
    }

    void Kill()
    {
        m_active = false;
    }

    static Bullet& Get(int idx)
    {
        return s_bullets[idx];
    }

    static Bullet* FindInactive()
    {
        for(uint i = 0; i < kMaxBullets; ++i)
        {
            if(!s_bullets[i].m_active)
            {
                return s_bullets + i;
            }
        }
        return nullptr;
    }
};

Bullet Bullet::s_bullets[kMaxBullets] = {};

// Particles are similar to bullets, but fade out
struct Particle : public BaseObject
{
    static void Emit(const DisplayListVector2& position, const DisplayListVector2& baseSpeed, Intensity brightness, uint count)
    {
        Intensity brightnessStep = brightness * kRecipParticleLife;
        for(uint i = 0; i < count; ++i)
        {
            if(s_numActive == kMaxParticles)
            {
                return;
            }
            Particle& particle = s_particles[s_numActive++];
            particle.m_position.x = position.x;
            particle.m_position.y = position.y;
            particle.m_brightness = brightness;
            particle.m_brightnessStep = brightnessStep;
            particle.m_velocity.x = baseSpeed.x + (kParticleSpeed * DisplayListScalar::randMinusOneToOne());
            particle.m_velocity.y = baseSpeed.y + (kParticleSpeed * DisplayListScalar::randMinusOneToOne());
            particle.m_active = true;
        }
    }

    static void UpdateAndDrawAll(DisplayList& displayList)
    {
        for(uint i = 0; i < s_numActive; ++i)
        {
            if(s_particles[i].UpdateAndDraw(displayList) == false)
            {
                // Swap with the last active
                if(i < (s_numActive - 1))
                {
                    s_particles[i] = s_particles[--s_numActive];
                    --i;
                }
            }
        }
    }

private:
    bool UpdateAndDraw(DisplayList& displayList)
    {
        if(!m_active)
        {
            return false;
        }
        m_brightness -= m_brightnessStep;
        if(m_brightness < 0)
        {
            m_active = false;
            return false;
        }

        Move();
        displayList.PushPoint(m_position.x, m_position.y, m_brightness);
        return true;
    }

    Intensity m_brightnessStep;

    static uint s_numActive;
    static Particle s_particles[kMaxParticles];
};

uint Particle::s_numActive = 0;
Particle Particle::s_particles[kMaxParticles] = {};

// An Asteroid encapsulates all the logic to destroy itself if it's sufficiently shot at
struct Asteroid : public ShapeObject
{

    enum class Size
    {
        eSmall,
        eMedium,
        eLarge,

        Count
    };

    struct SizeInfo
    {
        DisplayListScalar m_speedScale;
        DisplayListScalar m_speedBias;
        DisplayListScalar m_collisionRadius;
        DisplayListScalar m_bulletMassOverAsteroidMass;
        int16_t           m_numHitsToDestroy;
        float m_scale;
    };

    Size    m_size;
    float   m_rotationSpeed;
    int16_t m_numHitsToDestroy;

    static uint16_t s_numActive;
    static Asteroid s_asteroids[kMaxAsteroids];
    static const SizeInfo s_sizeInfo[];


    Asteroid() : ShapeObject()
    {
        m_brightness = 0.8f;
        m_rotation = 0.f;
        uint32_t rand = SimpleRand();
        m_shape = (GameShape) ((int) GameShape::eAsteroid0 + (rand & 0x3));
    }


    void Activate()
    {
        if(!m_active)
        {
            ++s_numActive;
            m_active = true;
        }
    }
    void Destroy()
    {
        if(m_active)
        {
            --s_numActive;
            m_active = false;
        }
    }

    void InitRandomExceptPosition(Size size)
    {
        m_size = size;
        const SizeInfo& sizeInfo = s_sizeInfo[(int) size];
        initVecRand0to1(m_velocity);
        m_velocity.x = (m_velocity.x * DisplayListScalar(2.f)) - DisplayListScalar(1.f);
        m_velocity.y = (m_velocity.y * DisplayListScalar(2.f)) - DisplayListScalar(1.f);
        normalize(m_velocity);
        DisplayListScalar::MathsIntermediateType speed = (DisplayListScalar::randZeroToOne() * sizeInfo.m_speedScale) + sizeInfo.m_speedBias;
        speed = sizeInfo.m_speedBias;
        m_velocity.x *= speed;
        m_velocity.y *= speed;
        m_scale = sizeInfo.m_scale;
        m_numHitsToDestroy = sizeInfo.m_numHitsToDestroy;
        Activate();
    }

    void InitRandom(Size size)
    {
        initVecRand0to1(m_position);
        InitRandomExceptPosition(size);
    }

    void Reset()
    {
        Destroy();

        uint32_t rand = SimpleRand();
        m_rotationSpeed = (float) (rand & 0xffff) * (kAsteroidMaxRotationSpeed * 2.f / 65535.f) - kAsteroidMaxRotationSpeed;
    }

    DisplayListScalar GetCollisionRadius() const
    {
        const SizeInfo& sizeInfo = s_sizeInfo[(int)m_size];
        return sizeInfo.m_collisionRadius;
    }

    void UpdateAndDraw(DisplayList& displayList)
    {
        if(!m_active)
        {
            return;
        }

        // Check for collision with a bullet
        const SizeInfo& sizeInfo = s_sizeInfo[(int)m_size];
        const DisplayListScalar collisionRadius = sizeInfo.m_collisionRadius;
        DisplayListVector velocityContribution;
        for(uint i = 0; i < kMaxBullets; ++i)
        {
            Bullet& bullet = Bullet::Get(i);
            if(bullet.m_active)
            {
                if(testCollisionOctagon(m_position, bullet.m_position, collisionRadius))
                {
                    velocityContribution.x = bullet.m_velocity.x * sizeInfo.m_bulletMassOverAsteroidMass;
                    velocityContribution.y = bullet.m_velocity.y * sizeInfo.m_bulletMassOverAsteroidMass;
                    m_velocity.x += velocityContribution.x;
                    m_velocity.y += velocityContribution.y;
                    velocityContribution.x = m_velocity.x;
                    velocityContribution.y = m_velocity.y;
                    bullet.Kill();
                    --m_numHitsToDestroy;

                    DisplayListVector2 particleVelocity;
                    particleVelocity.x = (bullet.m_velocity.x + m_velocity.x) * 0.5f;
                    particleVelocity.y = (bullet.m_velocity.y + m_velocity.y) * 0.5f;

                    Particle::Emit(bullet.m_position, particleVelocity, 0.2f, 5);
                    break;
                }
            }
        }

        if(m_numHitsToDestroy <= 0)
        {
            // Break the asteroid apart
            FloatTransform2D transform;
            CalcTransform(transform);
            Fragments::Add(m_shape, m_velocity, transform, 1.f);
            Destroy();
            if(m_size != Size::eSmall)
            {
                Size newSize = (Size) ((int) m_size - 1);
                for(int i = 0; i < 2; ++i)
                {
                    Asteroid* newAsteroid = FindInactive();
                    if(newAsteroid)
                    {
                        newAsteroid->m_position = m_position;
                        newAsteroid->InitRandomExceptPosition(newSize);
                        newAsteroid->m_velocity.x += velocityContribution.x;
                        newAsteroid->m_velocity.y += velocityContribution.y;
                    }
                }
            }
        }
        else
        {
            Rotate(m_rotationSpeed);
            ShapeObject::UpdateAndDraw(displayList);
        }
    }

    static Asteroid& Get(int idx)
    {
        return s_asteroids[idx];
    }

    static Asteroid* FindInactive()
    {
        for(uint i = 0; i < kMaxAsteroids; ++i)
        {
            if(!s_asteroids[i].m_active)
            {
                return s_asteroids + i;
            }
        }
        return nullptr;
    }

    static void DestroyAll()
    {
        for(uint i = 0; i < kMaxAsteroids; ++i)
        {
            Get(i).Reset();
        }
    }

};

// Intialise the Asteroid::SizeInfo table
#define ASTEROID_SIZE_INFO(minSpeed, maxSpeed, scale, mass, numHitsToDestroy) { \
    DisplayListScalar(kGlobalScaleFloat * (maxSpeed - minSpeed)), \
    DisplayListScalar(kGlobalScaleFloat * minSpeed), \
    DisplayListScalar(kGlobalScale * kAsteroidCollisionRadius * scale), \
    DisplayListScalar(kBulletMass) / DisplayListScalar(mass), \
    numHitsToDestroy, \
    (float) kGlobalScale * scale } \

const Asteroid::SizeInfo Asteroid::s_sizeInfo[] = 
{
    ASTEROID_SIZE_INFO(0.4f, 0.6f,  0.75f, 0.25f, 1), // eSmall
    ASTEROID_SIZE_INFO(0.2f, 0.3f,  1.5f,  0.5f,  2), // eMedium
    ASTEROID_SIZE_INFO(0.1f, 0.15f, 3.f,   1.f,   4), // eLarge
};
static_assert(count_of(Asteroid::s_sizeInfo) == (int) Asteroid::Size::Count, "");

uint16_t Asteroid::s_numActive = 0;
Asteroid Asteroid::s_asteroids[kMaxAsteroids] = {};

// The player's ship needs to draw two shapes.  The ship itself, and the thrust flame.
struct PlayerShip : public ShapeObject
{
    PlayerShip()
    : ShapeObject()
    {
        m_brightness = 1.f;
        m_scale = (float) kPlayerShipScale;
        m_shape = GameShape::eShip;
        m_active = false;
    }

    void Reset()
    {
        m_position = DisplayListVector2(0.5f, 0.5f);
        m_velocity = DisplayListVector2(0.f, 0.f);
        m_rotation = 0.f;
        m_active = true;
    }

    void UpdateAndDraw(DisplayList& displayList, bool thrust)
    {
        if(!m_active)
        {
            return;
        }

        Move();

        // Check for collision with asteroids
        constexpr DisplayListScalar kShipCollisionRadius = kGlobalScale * 2.f;
        for(uint i = 0; i < kMaxAsteroids; ++i)
        {
            const Asteroid& asteroid = Asteroid::Get(i);
            DisplayListScalar asteroidCollisionRadius = asteroid.GetCollisionRadius();
            if(asteroid.m_active && testCollisionOctagon(asteroid.m_position, m_position, asteroidCollisionRadius + kShipCollisionRadius))
            {
                // Boom
                m_active = false;
                DisplayListVector2 particleVelocity;
                particleVelocity.x = (asteroid.m_velocity.x + m_velocity.x) * 0.5f;
                particleVelocity.y = (asteroid.m_velocity.y + m_velocity.y) * 0.5f;
                Particle::Emit(m_position, particleVelocity, 1.f, 8);

                FloatTransform2D transform;
                CalcTransform(transform);
                Fragments::Add(m_shape, m_velocity, transform, 4.f);

                return;
            }
        }

        FloatTransform2D transform;
        CalcTransform(transform);
        PushGameShape(displayList, transform, m_shape, m_brightness);
        if(thrust)
        {
            PushGameShape(displayList, transform, GameShape::eThrust, Intensity(4.f));//m_brightness);
        }
    }
};
static PlayerShip s_playerShip;

// High level game state
struct GameState
{
    enum State
    {
        AttractModeTitle,
        AttractModeTitleDestroyed,
        AttractModeDemo,
        GameStartLevel,
        GameWaitForShipSafe,
        Game,
        GamePlayerDestroyed,
        GameInterLevel,
        GameOver,

        Count
    };

    typedef FixedPoint<13, int32_t, int32_t, false> Duration;

    struct StateInfo
    {
        Duration m_duration;
        State    m_autoNextState;
    };

    static const StateInfo s_stateInfo[];

    static State s_currentState;
    static Duration s_timeInCurrentState;
    static uint s_numLives;
    static uint s_score;
    static uint s_level;

    static void ConfigureAsteroidsForLevel(uint level)
    {
        Asteroid::DestroyAll();

        Asteroid::FindInactive()->InitRandom(Asteroid::Size::eLarge);
        Asteroid::FindInactive()->InitRandom(Asteroid::Size::eLarge);
        Asteroid::FindInactive()->InitRandom(Asteroid::Size::eLarge);
        Asteroid::FindInactive()->InitRandom(Asteroid::Size::eLarge);
    }

    static void ChangeState(State newState)
    {
        s_timeInCurrentState = 0;
        s_currentState = newState;

        // State initialisation
        switch(s_currentState)
        {
            case State::AttractModeTitle:
                // Nice clean title screen
                Asteroid::DestroyAll();
                Fragments::Reset();
                break;

            case State::AttractModeTitleDestroyed:
                Fragments::Add(s_title, s_titleTextTransform);
                break;

            case State::AttractModeDemo:
                Asteroid::DestroyAll();
                for(int i = 0; i < 3; ++i)
                {
                    Asteroid::FindInactive()->InitRandom(Asteroid::Size::eLarge);
                    Asteroid::FindInactive()->InitRandom(Asteroid::Size::eMedium);
                    Asteroid::FindInactive()->InitRandom(Asteroid::Size::eSmall);
                }
                break;

            case State::GameStartLevel:
                ConfigureAsteroidsForLevel(s_level);
                ChangeState(State::GameWaitForShipSafe);
                break;

            case State::GameWaitForShipSafe:
                if(s_numLives == 0)
                {
                    ChangeState(State::GameOver);
                }
                break;

            case State::Game:
                // Active the player ship
                s_playerShip.Reset();
                s_playerShip.m_active = true;
                break;

            default:
                break;
        }

    }

    static void Update(Duration dt)
    {
        const StateInfo& stateInfo = s_stateInfo[(int)s_currentState];
        s_timeInCurrentState += dt;
        if(stateInfo.m_duration != 0)
        {
            // Auto change-state after timeout
            if(s_timeInCurrentState > stateInfo.m_duration)
            {
                ChangeState(stateInfo.m_autoNextState);
            }
        }

        switch(s_currentState)
        {
            case State::AttractModeTitle:
            case State::AttractModeDemo:
                if(Buttons::IsJustPressed(Buttons::Id::Fire))
                {
                    // Start a new game
                    s_numLives = kNumLives;
                    s_score = 0;
                    s_level = 0;
                    ChangeState(State::GameStartLevel);
                }
                break;

            case State::GameWaitForShipSafe:
            {
                if(s_timeInCurrentState < Duration(2.f))
                {
                    break;
                }
                bool allClear = true;
                for(uint i = 0; i < kMaxAsteroids; ++i)
                {
                    const Asteroid& asteroid = Asteroid::Get(i);
                    if(asteroid.m_active && testCollisionOctagon(asteroid.m_position, DisplayListVector2(0.5f, 0.5f), kSafeRadius))
                    {
                        allClear = false;
                        break;
                    }
                }
                if(allClear)
                {
                    ChangeState(State::Game);
                }
                break;
            }

            case State::Game:
                if(!s_playerShip.m_active)
                {
                    // Player ship has been destroyed
                    --s_numLives;
                    ChangeState(State::GamePlayerDestroyed);
                }
                else if(Asteroid::s_numActive == 0)
                {
                    // Level clear
                    ++s_level;
                    ChangeState(State::GameInterLevel);
                }
                break;

            case State::GameOver:
                if((s_timeInCurrentState > Duration(3.f)) && Buttons::IsJustPressed(Buttons::Id::Fire))
                {
                    ChangeState(State::AttractModeTitle);
                }
                break;

            default:
                break;
        }

    }
};

const GameState::StateInfo GameState::s_stateInfo[] = {
    {10.f, State::AttractModeTitleDestroyed}, // AttractModeTitle
    {2.f, State::AttractModeDemo}, // AttractModeTitleDestroyed
    {10.f, State::AttractModeTitle}, // AttractModeDemo
    {0}, // GameStartLevel
    {0}, // GameWaitForShipSafe
    {0}, // Game
    {1.f, State::GameWaitForShipSafe}, // GamePlayerDestroyed
    {3.f, State::GameStartLevel}, // GameInterLevel
    {10.f, State::AttractModeTitle}, // GameOver
};
static_assert(count_of(GameState::s_stateInfo) == (int) GameState::State::Count, "");
GameState::State GameState::s_currentState = GameState::State::AttractModeTitle;
GameState::Duration GameState::s_timeInCurrentState = 0.f;
uint GameState::s_numLives;
uint GameState::s_score;
uint GameState::s_level;

//
// Initialisation
//

class SpaceRocksInSpace : public Demo
{
public:
    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static SpaceRocksInSpace s_spaceRocksInSpace;

constexpr BurnLength test = Mul(BurnLength(20.f), BurnLength(10.f));

void SpaceRocksInSpace::Init()
{
    CalcTextTransform(DisplayListVector2(0.5f, 0.7f), 0.08f, s_titleTextTransform);
}

void SpaceRocksInSpace::UpdateAndRender(DisplayList& displayList, float dt)
{
    GameState::Update(GameState::Duration(dt));

    const char* message = nullptr;
    switch(GameState::s_currentState)
    {
        case GameState::State::AttractModeTitle:
            message = s_title;
            break;
        case GameState::State::GameOver:
            message = s_gameOver;
            break;
        default:
            break;
    }
    if(message != nullptr)
    {
        //BurnLength burnLength = Mul(BurnLength(GameState::s_timeInCurrentState), BurnLength(20.f));
        BurnLength burnLength = Mul(BurnLength(20.f), GameState::s_timeInCurrentState, 0, 4);
        TextPrint(displayList, s_titleTextTransform, message, Intensity(1.f), burnLength, true);
    }

    if(Buttons::IsHeld(Buttons::Id::Left))
    {
        s_playerShip.Rotate(kPlayerShipRotationSpeed);
    }
    if(Buttons::IsHeld(Buttons::Id::Right))
    {
        s_playerShip.Rotate(-kPlayerShipRotationSpeed);
    }
    
    float s, c;
    sincosf(s_playerShip.m_rotation, &s, &c);

    bool thrust = Buttons::IsHeld(Buttons::Id::Thrust);
    if(thrust)
    {
        DisplayListVector2 acceleration = DisplayListVector2(DisplayListScalar(c) * kThrust, DisplayListScalar(s) * kThrust);
        s_playerShip.m_velocity.x += acceleration.x;
        s_playerShip.m_velocity.y += acceleration.y;
    }
    applyDrag(s_playerShip.m_velocity, kDrag);

    if(s_playerShip.m_active && Buttons::IsJustPressed(Buttons::Id::Fire))
    {
        // Fire
        Bullet* pBullet = Bullet::FindInactive();
        if(pBullet)
        {
            DisplayListVector2 dir(c, s);
            pBullet->Fire(s_playerShip.m_position, dir);
        }
    }

    s_playerShip.UpdateAndDraw(displayList, thrust);

    for(uint i = 0; i < kMaxBullets; ++i)
    {
        Bullet::Get(i).UpdateAndDraw(displayList);
    }

    for(uint i = 0; i < kMaxAsteroids; ++i)
    {
        Asteroid::Get(i).UpdateAndDraw(displayList);
    }

    Particle::UpdateAndDrawAll(displayList);
    Fragments::UpdateAndDraw(displayList);

    ++s_frameCounter;
}
