// Camera class
//
// Copyright (C) 2023 Oli Wright
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// A copy of the GNU General Public License can be found in the file
// LICENSE.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.
//
// oli.wright.github@gmail.com

#pragma once
#include "transform3d.h"

class Camera
{
public:
    Camera()
    {
        m_cameraToWorld.setAsIdentity();
        Calculate();
    }

    // Pass a cameraToWorld transform to position and orient the camera in world space.
    void SetCameraToWorld(const FixedTransform3D& cameraToWorld) { m_cameraToWorld = cameraToWorld; }

    // Parameterise the projection.
    // `tanHalfVerticalFOV` isn't as scary as it sounds.  For a 90 degree vertical
    // field of view, you would pass 1, because tan(90/2) == 1
    // Smaller numbers will linearly scale the field of view, effectively zooming in.
    void SetTanHalfVerticalFOV(StandardFixedOrientationScalar tanHalfVerticalFOV) { m_recipTanHalfVerticalFOV = Recip(tanHalfVerticalFOV); }

    // After changing the cameraToWorld transform, or the field of view, you must call
    // Calculate to update the internal fields.
    void Calculate()
    {
        m_cameraToWorld.orthonormalInvert(m_worldToClip);
        StandardFixedOrientationScalar scale = m_aspectRatio * m_recipTanHalfVerticalFOV;
        m_worldToClip.m[0][0] *= scale;
        m_worldToClip.m[1][0] *= scale;
        m_worldToClip.m[2][0] *= scale;
        m_worldToClip.t.x *= scale;
        m_worldToClip.m[0][1] *= m_recipTanHalfVerticalFOV;
        m_worldToClip.m[1][1] *= m_recipTanHalfVerticalFOV;
        m_worldToClip.m[2][1] *= m_recipTanHalfVerticalFOV;
        m_worldToClip.t.y *= m_recipTanHalfVerticalFOV;
    }

    const StandardFixedTranslationVector& GetPosition() const { return m_cameraToWorld.t; }

    // The worldToClip transform is used to transform points from world space to
    // clip space.
    // Clip space is arranged so that the clip planes are at a nice 45 degrees
    // which greatly simplifies clipping operations.
    const FixedTransform3D& GetWorldToClip() const { return m_worldToClip; }

private:
    FixedTransform3D               m_cameraToWorld;
    FixedTransform3D               m_worldToClip;
    StandardFixedOrientationScalar m_aspectRatio = 3.f / 4.f;
    StandardFixedOrientationScalar m_recipTanHalfVerticalFOV = 1.f;
};