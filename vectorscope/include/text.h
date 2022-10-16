#pragma once

#include "displaylist.h"
#include "transform2d.h"
#include "shapes.h"

void TextPrint(DisplayList& displayList, const FloatTransform2D& transform, const char* message, Intensity intensity, BurnLength burnLength = BurnLength::kMaxFloat, bool centre = false);

void CalcTextTransform(const DisplayListVector2& pos, const DisplayListScalar& scale, FloatTransform2D& outTranform);

BurnLength CalcBurnLength(const char* message);

uint32_t FragmentText(const char* message,
                      const FloatTransform2D& transform,
                      Fragment* outFragments,
                      uint32_t outFragmentsCapacity,
                      bool centre);
