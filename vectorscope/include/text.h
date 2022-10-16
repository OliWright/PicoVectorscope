#pragma once

#include "displaylist.h"
#include "transform2d.h"
#include "shapes.h"

void TextPrint(DisplayList& displayList, const FixedTransform2D& transform, const char* message, Intensity intensity, BurnLength burnLength = BurnLength::kMaxFloat, bool centre = false);

void CalcTextTransform(const DisplayListVector2& pos, const DisplayListScalar& scale, FixedTransform2D& outTranform);

BurnLength CalcBurnLength(const char* message);

uint32_t FragmentText(const char* message,
                      const FixedTransform2D& transform,
                      Fragment* outFragments,
                      uint32_t outFragmentsCapacity,
                      bool centre);
