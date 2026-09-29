#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/DatNonlinear.h"
#include "dsp/Emphasis.h"
#include "dsp/Lowpass32k.h"
#include "dsp/PeakDetector.h"

namespace ldat {

class LDatProcessor : public juce::AudioProcessor {
 public:
  LDatProcessor();
  ~LDatProcessor() override = default;

  void prepareToPlay(double sample_rate, int samples_per_block) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& dest) override;
  void setStateInformation(const void* data, int size) override;

  juce::AudioProcessorParameter* getBypassParameter() const override { return bypass_; }
  juce::AudioProcessorValueTreeState& GetState() { return apvts_; }
  PeakDetector& GetInputMeter() { return in_meter_; }
  PeakDetector& GetOutputMeter() { return out_meter_; }

 private:
  static juce::AudioProcessorValueTreeState::ParameterLayout CreateLayout();

  juce::AudioProcessorValueTreeState apvts_;
  juce::AudioParameterBool* bypass_ = nullptr;
  std::atomic<float>* in_gain_db_ = nullptr;
  std::atomic<float>* lp32k_on_ = nullptr;
  std::atomic<float>* emphasis_on_ = nullptr;
  std::atomic<float>* threshold_ = nullptr;
  std::atomic<float>* out_gain_db_ = nullptr;

  DatNonlinear dat_;
  Emphasis emphasis_;
  Lowpass32k lp32k_;
  PeakDetector in_meter_;
  PeakDetector out_meter_;

  float last_in_gain_ = 1.0f;
  float last_out_gain_ = 1.0f;
  bool was_emphasis_ = false;
  bool was_lp32k_ = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LDatProcessor)
};

}  // namespace ldat
