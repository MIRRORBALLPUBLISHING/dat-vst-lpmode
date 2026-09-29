#include "ui/Fonts.h"

#include <BinaryData.h>

namespace ldat::fonts {

namespace {

/**
 * a Font built from a name resolves its typeface through a process wide cache
 * and the *default* LookAndFeel (juce_Font.cpp, juce_getTypefaceForFont), so a
 * per component LookAndFeel can't swap the face, and setDefaultSansSerifTypeface
 * would hit every other plugin in the host. carrying the typeface in the Font
 * skips that path entirely
 **/
juce::Typeface::Ptr Load(const char* data, int size) {
  return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
}

juce::Font Make(juce::Typeface::Ptr face, float px) {
  return juce::Font(juce::FontOptions(px).withTypeface(face));
}

}  // namespace

juce::Font Regular(float px) {
  static const juce::Typeface::Ptr face = Load(BinaryData::HostGroteskSemiBold_ttf, BinaryData::HostGroteskSemiBold_ttfSize);
  return Make(face, px);
}

juce::Font Bold(float px) {
  static const juce::Typeface::Ptr face = Load(BinaryData::HostGroteskExtraBold_ttf, BinaryData::HostGroteskExtraBold_ttfSize);
  return Make(face, px);
}

}  // namespace ldat::fonts
