#pragma once

#include <juce_graphics/juce_graphics.h>

// embedded font faces. the typeface is carried in the Font itself, so the
// name lookup is never used -- see Fonts.cpp for why that matters
namespace ldat::fonts {

// Regular face at the given height in px.
juce::Font Regular(float px);

// Bold face at the given height in px.
juce::Font Bold(float px);

}  // namespace ldat::fonts
