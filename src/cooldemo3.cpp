#include "log.h"
#include "displaylist.h"
#include "transform2d.h"

static constexpr uint32_t kNumStars = 512;
typedef FixedPoint<7, int16_t, int32_t, false> StarCoordScalar;
struct StarCoord
{
    StarCoordScalar x, y, z;
};
typedef FixedPoint<13, int32_t, int32_t, false> StarCoordIntermediate;
static StarCoord s_stars[kNumStars];

//static constexpr StarCoordScalar kStarSpeed(3.f);
static constexpr StarCoordScalar kStarSpeed(2.f);

void coolDemo3(DisplayList& displayList, float dt)
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

            LOG_INFO("x: %f, y: %f, z; %f\n", (float) star.x, (float) star.y, (float) star.z);
        }
    }

    DisplayListPoint star2D = {0.f, 0.f, 0.2f};
    LOG_INFO("Stars update\n");
    constexpr StarCoordIntermediate proj = StarCoordIntermediate(0.25f);
    constexpr StarCoordIntermediate zOffset = StarCoordIntermediate(32.f);
    constexpr StarCoordIntermediate farZ = StarCoordScalar((StarCoordScalar::StorageType) 0x7fff);
    for(uint32_t i = 0; i < kNumStars; ++i)
    {
        StarCoord& star = s_stars[i];
        star.z -= kStarSpeed;
        if(star.z < StarCoordScalar(0.f))
        {
            star.z += StarCoordScalar(StarCoordScalar::kMaxStorageType);
        }

        StarCoordIntermediate x = (StarCoordScalar::MathsIntermediateType)star.x;
        StarCoordIntermediate y = (StarCoordScalar::MathsIntermediateType)star.y;
        StarCoordIntermediate z = (StarCoordScalar::MathsIntermediateType)star.z;
        LOG_INFO("x: %f, y: %f, z; %f\n", (float) x, (float) y, (float) z);

        StarCoordIntermediate recipZ = StarCoordIntermediate(1.f) / (z + zOffset);
        StarCoordIntermediate screenX = (proj * x * recipZ) + StarCoordIntermediate(0.5f);
        if((screenX < StarCoordIntermediate(1.f)) && (screenX > StarCoordIntermediate(0.f)))
        {
            StarCoordIntermediate screenY = (proj * y * recipZ) + StarCoordIntermediate(0.5f);
            if((screenY < StarCoordIntermediate(1.f)) && (screenY > StarCoordIntermediate(0.f)))
            {
                star2D.x = screenX;
                star2D.y = screenY;
#if 0
                constexpr StarCoordIntermediate recipFarZ = StarCoordIntermediate(1.f) / (farZ);
                StarCoordIntermediate brightness = StarCoordIntermediate(1.f) - (StarCoordIntermediate(z) * recipFarZ);
#else
                constexpr StarCoordIntermediate recipFarZ = StarCoordIntermediate(1.f) / (farZ + zOffset);
                constexpr StarCoordIntermediate zeroBrightness = zOffset * recipFarZ;
                StarCoordIntermediate brightness = /*StarCoordIntermediate(200.f) * */zOffset * recipZ;
                brightness -= zeroBrightness;
#endif
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
        LOG_INFO("x: %f, y: %f\n", (float) star2D.x, (float) star2D.y);
    }

#if 0
    for (uint32_t i = 0; i < kNumPoints; ++i)
    {
        displayList.PushPoint(points[i]);
    }
#endif
}
