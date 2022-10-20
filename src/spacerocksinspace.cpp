#include "picovectorscope.h"

//
// Constants
//

typedef FixedPoint<4,20,int32_t,int32_t> GameScalar;
typedef Vector2<GameScalar> GameVector2;
typedef FixedPoint<1,20,int32_t,int32_t> Velocity;
typedef Vector2<Velocity> VelocityVector2;
typedef FixedPoint<1,20,int32_t,int32_t> Acceleration;
typedef Vector2<Acceleration> AccelerationVector2;

static constexpr float kGlobalScaleFloat = 0.015f;
static constexpr int kTargetRefreshRate = 240; // FPS
static constexpr float kGlobalSpeedFloat = (float) 60.f / (float) kTargetRefreshRate;

static constexpr GameScalar kGlobalScale = kGlobalScaleFloat;
static constexpr GameScalar kGlobalSpeed = kGlobalSpeedFloat;
static constexpr GameScalar kRecipGlobalSpeed = 1.f / kGlobalSpeedFloat;
static constexpr GameScalar kBulletSpeed = kGlobalSpeedFloat * 0.01f;
static constexpr uint32_t kBulletLife = (uint) (kRecipGlobalSpeed * 60);
static constexpr Velocity kParticleSpeed = kGlobalSpeedFloat * 0.004f;
static constexpr Velocity kFragmentSpeed = kGlobalSpeedFloat * 0.002f;
static constexpr GameScalar kFragmentRotationSpeed = kGlobalSpeedFloat * 0.05f;
static constexpr GameScalar kTextFragmentRotationSpeed = kGlobalSpeedFloat * 0.02f;
static constexpr GameScalar kPlayerShipRotationSpeed = kGlobalSpeedFloat * 0.1f;
static constexpr Velocity kThrust = kGlobalSpeedFloat * 0.0001f;
static constexpr Velocity kDrag = kGlobalSpeedFloat * 0.01f;
static constexpr float kAsteroidSpeedScaleFloat = kGlobalSpeedFloat * 2.f;
static constexpr GameScalar kAsteroidMaxRotationSpeed = kGlobalSpeedFloat * 0.03f;
static constexpr float kAsteroidCollisionRadiusFloat = kGlobalScaleFloat * 2.7f;
static constexpr float kBulletMass = 0.1f;
static constexpr uint kMaxBullets = 4;
static constexpr uint kMaxParticles = 128;
static constexpr uint kParticleLife = (uint) (10.f / kGlobalSpeedFloat);
static constexpr uint kMaxAsteroids = 32;
static constexpr uint kNumLives = 3;
static constexpr GameScalar kPlayerShipScale = kGlobalScale;
static constexpr GameScalar kBulletLaunchOffset = kPlayerShipScale * 3.3f;
static constexpr GameScalar kSafeRadius = kGlobalScaleFloat * 10.f;
static constexpr Intensity kRecipParticleLife = 1.f / kParticleLife;

static const char* s_title = "SPACE ROCKS\nIN\nSPACE";
static const BurnLength s_titleTotalBurnLength = CalcBurnLength(s_title);
static const char* s_gameOver = "GAME\nOVER";
static const BurnLength s_gameOverTotalBurnLength = CalcBurnLength(s_gameOver);
static FixedTransform2D s_titleTextTransform;

static uint32_t s_frameCounter = 0;

static constexpr SinTableValue kSin1 = -1.f;
static constexpr GameScalar kGame1 = kSin1;
static constexpr int32_t kShiftTest = SignedShift(2, 1);

static constexpr float kCastTest = (float) GameScalar(kSin1);
static constexpr Velocity kThrustTest = Mul<1,-4>(Velocity(0.1f), kThrust);
static constexpr Velocity kThrustTest2 = Mul<1,-4>(Velocity(-1.f), kThrust);

static LogChannel SpaceRocks(true);

//
// Some helper functions
//

template<typename T>
static void normalize(T& vec)
{
    typedef FixedPoint<14,14,int32_t,int32_t> NormScalar;
    NormScalar x = vec.x;
    NormScalar y = vec.y;
    typename T::ScalarType::IntermediateType length = ((x * x) + (y * y)).sqrt();
    //GameScalar::IntermediateType length = ((vec.x * vec.x) + (vec.y * vec.y)).sqrt();
    typename T::ScalarType::IntermediateType recipLength = length.recip();

    vec.x *= recipLength;
    vec.y *= recipLength;
}

template<typename T>
static void randomNormal(T& vec)
{
    SinTableValue s, c;
    SinTable::SinCos(LookUpTableBase::Index::randZeroToOne() * (kPi * 2), s, c);
    vec.x = s;
    vec.y = c;
}

template<typename T>
static void initVecRand0to1(T& vec)
{
    vec.x = T::ScalarType::randZeroToOne();
    vec.y = T::ScalarType::randZeroToOne();
}

template<typename T>
static void initVecRandMinus1to1(T& vec)
{
    vec.x = T::ScalarType::randMinusOneToOne();
    vec.y = T::ScalarType::randMinusOneToOne();
}

static void applyDrag(Velocity& val, const Velocity& drag)
{
    if(val < 0)
    {
        val -= Mul<1,-4>(val, drag);
    }
    else
    {
        val += Mul<1,-4>(-val, drag);
    }
}

static void applyDrag(VelocityVector2& vec, const Velocity& drag)
{
    applyDrag(vec.x, drag);
    applyDrag(vec.y, drag);
}

static bool testCollisionOctagon(const GameVector2& p0, const GameVector2& p1, GameScalar distance)
{
    // Octagon collision
    GameScalar dx = (p1.x - p0.x).abs();
    GameScalar dy = (p1.y - p0.y).abs();
    return (dx < distance) && (dy < distance) && ((dx + dy) < distance);
}

template<typename T>
static inline void wrap(Vector2<T>& vec)
{
    vec.x = vec.x.frac();
    vec.y = vec.y.frac();
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
            fragment.m_intensity -= GameScalar(kGlobalSpeedFloat * 0.01f);
            if(fragment.m_intensity < 0)
            {
                // Fragment is dead
                // Swap this slot with the end
                fragment = s_fragments[--s_numFragments];
                --i;
                continue;
            }
            
            fragment.Move();
            wrap(fragment.m_position);
        }

        PushFragmentsToDisplayList(displayList, s_fragments, s_numFragments);
    }

    static void Add(GameShape shape, const VelocityVector2& baseSpeed, const FixedTransform2D& transform, Intensity intensity)
    {
        uint numNewFragments = FragmentGameShape(shape, transform, s_fragments + s_numFragments, kMaxFragments - s_numFragments);
        for(uint i = 0; i < numNewFragments; ++i)
        {
            Fragment& fragment = s_fragments[s_numFragments + i];
            fragment.m_velocity.x = baseSpeed.x + Mul<1,-4>(Velocity::randMinusOneToOne(), kFragmentSpeed);
            fragment.m_velocity.y = baseSpeed.y + Mul<1,-4>(Velocity::randMinusOneToOne(), kFragmentSpeed);
            fragment.m_intensity = intensity * (Intensity::randZeroToOne() + 0.5f);
            fragment.m_rotationSpeed = Mul<1,-4>(GameScalar::randMinusOneToOne(), kFragmentRotationSpeed);
        }
        s_numFragments += numNewFragments;
    }

    static void Add(const char* message, const FixedTransform2D& transform)
    {
        uint numNewFragments = FragmentText(message, transform, s_fragments + s_numFragments, kMaxFragments - s_numFragments, true);
        for(uint i = 0; i < numNewFragments; ++i)
        {
            Fragment& fragment = s_fragments[s_numFragments + i];
            fragment.m_velocity.x = Mul<1,-4>(Velocity::randMinusOneToOne(), kFragmentSpeed) * 0.5f;
            fragment.m_velocity.y = Mul<1,-4>(Velocity::randMinusOneToOne(), kFragmentSpeed) * 0.5f;
            fragment.m_intensity = Intensity::randZeroToOne() + 0.5f;
            fragment.m_rotationSpeed = Mul<1,-4>(GameScalar::randMinusOneToOne(), kTextFragmentRotationSpeed);
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
    GameVector2 m_position;
    VelocityVector2 m_velocity;
    Intensity m_brightness;
    bool m_active;

    BaseObject() : m_active(false) {}

    void Move()
    {
        m_position.x += m_velocity.x;
        m_position.y += m_velocity.y;
        wrap(m_position);
    }
};

// An intermediate class for objects using a GameShape with scale and rotation
struct ShapeObject : public BaseObject
{
    GameShape m_shape;
    GameScalar m_rotation;
    GameScalar m_scale;

    ShapeObject() : BaseObject() {}

    void Rotate(GameScalar angle)
    {
        m_rotation += angle;

        if (m_rotation > GameScalar(kPi * 2.f))
        {
            m_rotation -= GameScalar(kPi * 2.f);
        }
        if (m_rotation < 0)
        {
            m_rotation += GameScalar(kPi * 2.f);
        }
    }

    void CalcTransform(FixedTransform2D& outTransform)
    {
        SinTableValue s, c;
        SinTable::SinCos(m_rotation, s, c);
        outTransform.setAsRotation(s, c);

        outTransform *= FixedTransform2D::ScalarType(m_scale);
        outTransform.setTranslation(FixedTransform2D::Vector2Type(m_position.x, m_position.y));
    }

    void UpdateAndDraw(DisplayList& displayList)
    {
        if(!m_active)
        {
            return;
        }

        Move();

        FixedTransform2D transform;
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

    void Fire(const GameVector2& position, const GameVector2& direction)
    {
        m_position.x = position.x + Mul<1,1>(direction.x, kBulletLaunchOffset);
        m_position.y = position.y + Mul<1,1>(direction.y, kBulletLaunchOffset);
        m_brightness = Intensity(1.f);
        m_velocity.x = Mul<1,-4>(direction.x, kBulletSpeed);
        m_velocity.y = Mul<1,-4>(direction.y, kBulletSpeed);
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
    static void Emit(const GameVector2& position, const GameVector2& baseSpeed, Intensity brightness, uint count)
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
            particle.m_velocity.x = baseSpeed.x + Mul<1,-4>(GameScalar::randMinusOneToOne(), kParticleSpeed);
            particle.m_velocity.y = baseSpeed.y + Mul<1,-4>(GameScalar::randMinusOneToOne(), kParticleSpeed);
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
        GameScalar m_speedScale;
        GameScalar m_speedBias;
        GameScalar m_collisionRadius;
        GameScalar m_bulletMassOverAsteroidMass;
        GameScalar m_scale;
        int16_t    m_numHitsToDestroy;
    };

    Size       m_size;
    GameScalar m_rotationSpeed;
    int16_t    m_numHitsToDestroy;

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
        //initVecRandMinus1to1(m_velocity);
        //normalize(m_velocity);
        randomNormal(m_velocity);
        Velocity speed = Mul<1,-4>(Velocity::randZeroToOne(), sizeInfo.m_speedScale) + sizeInfo.m_speedBias;
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
        m_rotationSpeed = kAsteroidMaxRotationSpeed * GameScalar::randMinusOneToOne();
    }

    GameScalar GetCollisionRadius() const
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
        const GameScalar collisionRadius = sizeInfo.m_collisionRadius;
        for(uint i = 0; i < kMaxBullets; ++i)
        {
            Bullet& bullet = Bullet::Get(i);
            if(bullet.m_active)
            {
                if(testCollisionOctagon(m_position, bullet.m_position, collisionRadius))
                {
                    GameVector2 particleVelocity;
                    particleVelocity.x = (bullet.m_velocity.x + m_velocity.x) >> 1;
                    particleVelocity.y = (bullet.m_velocity.y + m_velocity.y) >> 1;

                    Particle::Emit(bullet.m_position, particleVelocity, 0.2f, 5);
                    --m_numHitsToDestroy;
                    
                    if(m_numHitsToDestroy > 0)
                    {
                        m_velocity.x += bullet.m_velocity.x * sizeInfo.m_bulletMassOverAsteroidMass;
                        m_velocity.y += bullet.m_velocity.y * sizeInfo.m_bulletMassOverAsteroidMass;
                    }
                    bullet.Kill();
                    break;
                }
            }
        }

        if(m_numHitsToDestroy <= 0)
        {
            // Break the asteroid apart
            FixedTransform2D transform;
            CalcTransform(transform);
            VelocityVector2 parentVelocity = m_velocity;
            Fragments::Add(m_shape, parentVelocity, transform, 1.f);
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
                        newAsteroid->m_velocity.x += parentVelocity.x;
                        newAsteroid->m_velocity.y += parentVelocity.y;
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
    kAsteroidSpeedScaleFloat * kGlobalScaleFloat * (maxSpeed - minSpeed), \
    kAsteroidSpeedScaleFloat * kGlobalScaleFloat * minSpeed, \
    kAsteroidCollisionRadiusFloat * scale, \
    kBulletMass / mass, \
    kGlobalScaleFloat * scale, \
    numHitsToDestroy }

constexpr Asteroid::SizeInfo Asteroid::s_sizeInfo[] = 
{
    ASTEROID_SIZE_INFO(0.2f,  0.3f,   0.75f, 0.25f, 1), // eSmall
    ASTEROID_SIZE_INFO(0.1f,  0.15f,  1.5f,  0.5f,  2), // eMedium
    ASTEROID_SIZE_INFO(0.05f, 0.075f, 3.f,   1.f,   4), // eLarge
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
        m_position = GameVector2(0.5f, 0.5f);
        m_velocity = VelocityVector2(0.f, 0.f);
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
        constexpr GameScalar kShipCollisionRadius = kGlobalScaleFloat * 2.f;
        for(uint i = 0; i < kMaxAsteroids; ++i)
        {
            const Asteroid& asteroid = Asteroid::Get(i);
            GameScalar asteroidCollisionRadius = asteroid.GetCollisionRadius();
            if(asteroid.m_active && testCollisionOctagon(asteroid.m_position, m_position, asteroidCollisionRadius + kShipCollisionRadius))
            {
                // Boom
                m_active = false;
                GameVector2 particleVelocity;
                particleVelocity.x = (asteroid.m_velocity.x + m_velocity.x) * 0.5f;
                particleVelocity.y = (asteroid.m_velocity.y + m_velocity.y) * 0.5f;
                Particle::Emit(m_position, particleVelocity, 1.f, 8);

                FixedTransform2D transform;
                CalcTransform(transform);
                Fragments::Add(m_shape, m_velocity, transform, 4.f);

                return;
            }
        }

        FixedTransform2D transform;
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

    typedef FixedPoint<10, 13, int32_t, int32_t, false> Duration;

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
                    if(asteroid.m_active && testCollisionOctagon(asteroid.m_position, GameVector2(0.5f, 0.5f), kSafeRadius))
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
    {8.f, State::AttractModeTitleDestroyed}, // AttractModeTitle
    {2.f, State::AttractModeDemo}, // AttractModeTitleDestroyed
    {7.f, State::AttractModeTitle}, // AttractModeDemo
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
    SpaceRocksInSpace() : Demo(0, kTargetRefreshRate) {}

    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static SpaceRocksInSpace s_spaceRocksInSpace;

constexpr BurnLength test = BurnLength(20.f) * BurnLength(10.f);

void SpaceRocksInSpace::Init()
{
    CalcTextTransform(DisplayListVector2(0.5f, 0.7f), 0.08f, s_titleTextTransform);

    for(uint i = 0; i < (int) Asteroid::Size::Count; ++i)
    {
        const Asteroid::SizeInfo& sizeInfo = Asteroid::s_sizeInfo[i];
        LOG_INFO(SpaceRocks, "Size %d: %f, %f\n", i, (float)sizeInfo.m_speedBias, (float)(sizeInfo.m_speedScale + sizeInfo.m_speedBias));

        LOG_INFO(SpaceRocks, "Rand: %f\n", (float) GameScalar::randMinusOneToOne());
        LOG_INFO(SpaceRocks, "Rand: %f\n", (float) GameScalar::randMinusOneToOne());
        LOG_INFO(SpaceRocks, "Rand: %f\n", (float) GameScalar::randMinusOneToOne());
        LOG_INFO(SpaceRocks, "Rand: %f\n", (float) GameScalar::randMinusOneToOne());
    }
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
        BurnLength burnLength = BurnLength(20.f) * GameState::s_timeInCurrentState;
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
    
    SinTableValue s, c;
    SinTable::SinCos(s_playerShip.m_rotation, s, c);

    bool thrust = Buttons::IsHeld(Buttons::Id::Thrust);
    if(thrust)
    {
        VelocityVector2 acceleration;
        acceleration.x = Mul<1,-4>(Velocity(c), kThrust);
        acceleration.y = Mul<1,-4>(Velocity(s), kThrust);
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
            GameVector2 dir(c, s);
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
