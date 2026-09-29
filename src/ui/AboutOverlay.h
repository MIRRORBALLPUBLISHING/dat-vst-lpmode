#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace ldat {

// covers the whole editor, dims it and shows the about text in a centred
// panel. while visible it swallows every mouse event, so the controls under
// it cannot be touched. no modal loop, that is not safe inside a plugin.
class AboutOverlay : public juce::Component {
 public:
  AboutOverlay();

  // called when the user clicks outside the panel or hits escape
  std::function<void()> on_close;

  void paint(juce::Graphics& g) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent& e) override;
  bool keyPressed(const juce::KeyPress& k) override;

 private:
  // panel bounds, shared by paint and resized
  juce::Rectangle<int> PanelRect() const;
  juce::TextEditor text_;
};

}  // namespace ldat
