#include "picovectorscope.h"

static constexpr uint32_t kNumStars = 1000;
typedef FixedPoint<8, 7, int16_t, int32_t, false> StarCoordScalar;
struct StarCoord
{
    StarCoordScalar x, y, z;
};
typedef FixedPoint<8, 18, int32_t, int32_t, false> StarCoordIntermediate;

constexpr float kProjFloat = 0.25f;
constexpr float kNearZFloat = 32.f;

constexpr StarCoordIntermediate kProj = StarCoordIntermediate(kProjFloat);
constexpr StarCoordIntermediate kZOffset = StarCoordIntermediate(kNearZFloat);
constexpr StarCoordIntermediate kNearZ = StarCoordIntermediate(kNearZFloat);
constexpr StarCoordIntermediate kFarZ = kNearZ + StarCoordScalar::kMax;
constexpr StarCoordIntermediate kRecipNearZ = kNearZ.recip();
constexpr StarCoordIntermediate kRecipFarZ = kFarZ.recip();

#if 0
constexpr float recipNearZFloat = (float) kRecipNearZ;
constexpr float idealrecipNearZFloat = 1.f / kNearZFloat;
constexpr float projOverNearZFloat = (float) Mul<0,-4>(kProj, kRecipNearZ);
constexpr float idealProjOverNearZFloat = kProjFloat * idealrecipNearZFloat;

constexpr float recipFarZFloat = (float) kRecipFarZ;
constexpr float idealrecipFarZFloat = 1.f / (float) kFarZ;
constexpr float projOverFarZFloat = (float) Mul<0,-6>(kProj, kRecipFarZ);
constexpr float idealProjOverFarZFloat = kProjFloat * idealrecipFarZFloat;

constexpr StarCoord kTestStarCoord = {-107.f, 37.f, 231.2f};
constexpr float kTestStarCoordXFloat = (float) kTestStarCoord.x;
constexpr float kTestStarCoordYFloat = (float) kTestStarCoord.y;
constexpr float kTestStarCoordZFloat = (float) kTestStarCoord.z;

constexpr StarCoordIntermediate kTestStarRecipZ = ((StarCoordIntermediate)kTestStarCoord.z + kZOffset).recip();
constexpr StarCoordIntermediate kTestStarRecipZ2 = Div(StarCoordIntermediate(1.f), ((StarCoordIntermediate)kTestStarCoord.z + kZOffset), 12, 4);
constexpr float kTestStarRecipZFloat = (float) kTestStarRecipZ;
constexpr float kTestStarRecipZFloat2 = (float) kTestStarRecipZ2;
constexpr float kTestStarRecipZFloat3 = (float) 1.f / (kTestStarCoordZFloat + kNearZFloat);
constexpr StarCoordIntermediate kTestStarProjOverZ = Mul<0,-4>(kProj, kTestStarRecipZ);
constexpr float kTestStarProjOverZFloat = (float) kTestStarProjOverZ;
constexpr float kTestStarProjOverZFloat2 = kProjFloat * kTestStarRecipZFloat3;
constexpr StarCoordIntermediate kTestStarScreenX = Mul<StarCoordScalar::kNumWholeBits,-4>(kTestStarCoord.x, kTestStarProjOverZ) + 0.5f;
constexpr float kTestStarScreenXFloat = (float) kTestStarScreenX;
constexpr float kTestStarScreenXFloat2 = kTestStarCoordXFloat * kTestStarProjOverZFloat2 + 0.5f;
#endif

static StarCoord s_stars[kNumStars];

static StarCoordScalar s_starSpeed(2.f);
static LogChannel StarDetails(false);

class Starfield : public Demo
{
public:
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static Starfield s_starfield;

void Starfield::UpdateAndRender(DisplayList& displayList, float dt)
{
    static bool doneInit = false;
    if(!doneInit)
    {
        doneInit = true;
        for(uint32_t i = 0; i < kNumStars; ++i)
        {
            StarCoord& star = s_stars[i];
            star.x = StarCoordScalar::randFullRange();
            star.y = StarCoordScalar::randFullRange();
            star.z = StarCoordScalar((StarCoordScalar::StorageType) (((StarCoordScalar::StorageType) SimpleRand()) & 0x7fff));

            LOG_INFO(StarDetails, "x: %f, y: %f, z; %f\n", (float) star.x, (float) star.y, (float) star.z);
        }
    }

    if(Buttons::IsJustPressed(Buttons::Id::Left))
    {
        s_starSpeed *= 0.75f; //<--- Broken
        //s_starSpeed = s_starSpeed * 0.75f;
    }
    if(Buttons::IsJustPressed(Buttons::Id::Right))
    {
        s_starSpeed *= 1.25f;
    }


    DisplayListPoint star2D = {0.f, 0.f, 0.2f};
    LOG_INFO(StarDetails, "Stars update\n");
    for(uint32_t i = 0; i < kNumStars; ++i)
    {
        StarCoord& star = s_stars[i];
        star.z -= s_starSpeed;
        if(star.z < StarCoordScalar(0.f))
        {
            star.z += StarCoordScalar(StarCoordScalar::kMaxStorageType);
        }

        // StarCoordIntermediate x = (StarCoordScalar::IntermediateType)star.x;
        // StarCoordIntermediate y = (StarCoordScalar::IntermediateType)star.y;
        // StarCoordIntermediate z = (StarCoordScalar::IntermediateType)star.z;
        // LOG_INFO(StarDetails, "x: %f, y: %f, z; %f\n", (float) x, (float) y, (float) z);

        // Careful fixed-point math to maintain precision
        StarCoordIntermediate recipZ = (z + kZOffset).recip();
        StarCoordIntermediate projOverZ = Mul<0,0>(kProj, recipZ);
        StarCoordIntermediate screenX = Mul<-4,StarCoordScalar::kNumWholeBits>(projOverZ, star.x) + 0.5f;
        if((screenX < StarCoordIntermediate(1.f)) && (screenX > StarCoordIntermediate(0.f)))
        {
            StarCoordIntermediate screenY = Mul<-4,StarCoordScalar::kNumWholeBits>(projOverZ, star.y) + 0.5f;
            if((screenY < StarCoordIntermediate(1.f)) && (screenY > StarCoordIntermediate(0.f)))
            {
                star2D.x = screenX;
                star2D.y = screenY;
                constexpr StarCoordIntermediate zeroBrightness = kNearZ * kRecipFarZ;
                StarCoordIntermediate brightness = kNearZ * recipZ; //< 1.0 at kNearZ
                brightness -= zeroBrightness;
                if(brightness > 0.f)
                {
                    if(brightness > 1.f)
                    {
                        brightness = 1.f;
                    }
                    star2D.brightness = brightness;
                    displayList.PushPoint(star2D);
                }
            }
        }
        LOG_INFO(StarDetails, "x: %f, y: %f\n", (float) star2D.x, (float) star2D.y);
    }
}
