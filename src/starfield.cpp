#include "picovectorscope.h"

static constexpr uint32_t kNumStars = 1000;
typedef FixedPoint<7, int16_t, int32_t, false> StarCoordScalar;
struct StarCoord
{
    StarCoordScalar x, y, z;
};
typedef FixedPoint<18, int32_t, int32_t, false> StarCoordIntermediate;
static StarCoord s_stars[kNumStars];

//static constexpr StarCoordScalar s_starSpeed(0.1f);
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
        //s_starSpeed *= 0.75f; <--- Broken
        s_starSpeed = s_starSpeed * 0.75f;
    }
    if(Buttons::IsJustPressed(Buttons::Id::Right))
    {
        s_starSpeed = s_starSpeed * 1.25f;
    }


    DisplayListPoint star2D = {0.f, 0.f, 0.2f};
    LOG_INFO(StarDetails, "Stars update\n");
    constexpr StarCoordIntermediate proj = StarCoordIntermediate(0.25f);
    constexpr StarCoordIntermediate zOffset = StarCoordIntermediate(32.f);
    constexpr StarCoordIntermediate farZ = StarCoordScalar((StarCoordScalar::StorageType) 0x7fff);
    constexpr float farZFloat = (float) farZ;
    for(uint32_t i = 0; i < kNumStars; ++i)
    {
        StarCoord& star = s_stars[i];
        star.z -= s_starSpeed;
        if(star.z < StarCoordScalar(0.f))
        {
            star.z += StarCoordScalar(StarCoordScalar::kMaxStorageType);
        }

        StarCoordIntermediate x = (StarCoordScalar::IntermediateType)star.x;
        StarCoordIntermediate y = (StarCoordScalar::IntermediateType)star.y;
        StarCoordIntermediate z = (StarCoordScalar::IntermediateType)star.z;
        LOG_INFO(StarDetails, "x: %f, y: %f, z; %f\n", (float) x, (float) y, (float) z);

        //StarCoordIntermediate recipZ = StarCoordIntermediate(1.f) / (z + zOffset);
        StarCoordIntermediate recipZ = Div(StarCoordIntermediate(1.f), (z + zOffset), 12, 4);
        StarCoordIntermediate projRecipZ = Mul(proj, recipZ, 8);
        StarCoordIntermediate screenX = Mul(x, projRecipZ, 10) + StarCoordIntermediate(0.5f);
        if((screenX < StarCoordIntermediate(1.f)) && (screenX > StarCoordIntermediate(0.f)))
        {
            StarCoordIntermediate screenY = Mul(y, projRecipZ, 10) + StarCoordIntermediate(0.5f);
            if((screenY < StarCoordIntermediate(1.f)) && (screenY > StarCoordIntermediate(0.f)))
            {
                star2D.x = screenX;
                star2D.y = screenY;
                //constexpr StarCoordIntermediate recipFarZ = Div(StarCoordIntermediate(1.f), (farZ + zOffset), 12, 4);
                constexpr StarCoordIntermediate recipFarZ = (farZ + zOffset).recip();
                constexpr float recipFarZFloat = (float) recipFarZ;
                constexpr StarCoordIntermediate zeroBrightness = Mul(zOffset, recipFarZ, 16);
                StarCoordIntermediate brightness = Mul(zOffset, recipZ, 16);
                brightness -= zeroBrightness;
                if(brightness > StarCoordIntermediate(0.f))
                {
                    if(brightness > StarCoordIntermediate(1.f))
                    {
                        brightness = StarCoordIntermediate(1.f);
                    }
                    star2D.brightness = brightness;
                    displayList.PushPoint(star2D);
                }
            }
        }
        LOG_INFO(StarDetails, "x: %f, y: %f\n", (float) star2D.x, (float) star2D.y);
    }

#if 0
    for (uint32_t i = 0; i < kNumPoints; ++i)
    {
        displayList.PushPoint(points[i]);
    }
#endif
}
