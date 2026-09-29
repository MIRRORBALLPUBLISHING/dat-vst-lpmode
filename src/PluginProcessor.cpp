#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

namespace ldat {

namespace {

constexpr int kParamVersion = 1;

juce::ParameterID Id(const char* name) { return juce::ParameterID(name, kParamVersion); }

}  // namespace

LDatProcessor::LDatProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "state", CreateLayout()) {
  bypass_ = dynamic_cast<juce::AudioParameterBool*>(apvts_.getParameter(kParamBypass));
  in_gain_db_ = apvts_.getRawParameterValue(kParamInGain);
  lp32k_on_ = apvts_.getRawParameterValue(kParamLp32k);
  emphasis_on_ = apvts_.getRawParameterValue(kParamEmphasis);
  threshold_ = apvts_.getRawParameterValue(kParamThreshold);
  out_gain_db_ = apvts_.getRawParameterValue(kParamOutGain);
}

juce::AudioProcessorValueTreeState::ParameterLayout LDatProcessor::CreateLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  const juce::NormalisableRange<float> gain_range(kGainMinDb, kGainMaxDb, 0.1f);
  const auto db = juce::AudioParameterFloatAttributes().withLabel("dB");

  layout.add(std::make_unique<juce::AudioParameterBool>(Id(kParamBypass), "Bypass", false));
  layout.add(std::make_unique<juce::AudioParameterFloat>(Id(kParamInGain), "Input Gain", gain_range, 0.0f, db));
  layout.add(std::make_unique<juce::AudioParameterBool>(Id(kParamLp32k), "32kHz Emulation", true));
  layout.add(std::make_unique<juce::AudioParameterBool>(Id(kParamEmphasis), "Emphasis", false));
  // continuous range, shown with one decimal so the box reads like the gains
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      Id(kParamThreshold), "Threshold",
      juce::NormalisableRange<float>(kThresholdMinDb, kThresholdMaxDb), kThresholdDefaultDb,
      db.withStringFromValueFunction([](float v, int) { return juce::String(v, 1); })));
  layout.add(std::make_unique<juce::AudioParameterFloat>(Id(kParamOutGain), "Output Gain", gain_range, 0.0f, db));
  return layout;
}

void LDatProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
  const int nch = juce::jmax(1, getTotalNumOutputChannels());
  emphasis_.Prepare(sample_rate, nch);
  lp32k_.Prepare(sample_rate, samples_per_block, nch);
  setLatencySamples(lp32k_.GetLatency());
  last_in_gain_ = juce::Decibels::decibelsToGain(in_gain_db_->load());
  last_out_gain_ = juce::Decibels::decibelsToGain(out_gain_db_->load());
  was_emphasis_ = emphasis_on_->load() > 0.5f;
  was_lp32k_ = lp32k_on_->load() > 0.5f;
}

bool LDatProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto in = layouts.getMainInputChannelSet();
  const auto out = layouts.getMainOutputChannelSet();
  if (in != out) return false;
  return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void LDatProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals no_denormals;
  const int n = buffer.getNumSamples();
  const int nin = getTotalNumInputChannels();
  const int nout = getTotalNumOutputChannels();
  for (int ch = nin; ch < nout; ++ch) buffer.clear(ch, 0, n);

  if (bypass_->get()) {
    in_meter_.Push(buffer);
    out_meter_.Push(buffer);
    return;
  }

  // input gain
  const float in_gain = juce::Decibels::decibelsToGain(in_gain_db_->load());
  buffer.applyGainRamp(0, n, last_in_gain_, in_gain);
  last_in_gain_ = in_gain;
  in_meter_.Push(buffer);

  // emphasis and lpf reset their state when switched on so no stale tail leaks
  const bool emphasis_on = emphasis_on_->load() > 0.5f;
  if (emphasis_on && !was_emphasis_) emphasis_.Reset();
  was_emphasis_ = emphasis_on;
  const bool lp_on = lp32k_on_->load() > 0.5f;
  if (lp_on && !was_lp32k_) lp32k_.Reset();
  was_lp32k_ = lp_on;

  if (emphasis_on) emphasis_.ProcessPre(buffer);
  if (lp_on) lp32k_.Process(buffer);

  // 16bit -> nonlinear -> 16bit
  dat_.SetThreshold(ThresholdFromDb(threshold_->load()));
  const int nch = juce::jmin(nout, PeakDetector::kMaxChannels);
  for (int ch = 0; ch < nch; ++ch) {
    float* p = buffer.getWritePointer(ch);
    for (int i = 0; i < n; ++i) p[i] = dat_.ProcessSample(p[i]);
  }

  if (emphasis_on) {
    emphasis_.ProcessDe(buffer);
  }

  // output gain
  const float out_gain = juce::Decibels::decibelsToGain(out_gain_db_->load());
  buffer.applyGainRamp(0, n, last_out_gain_, out_gain);
  last_out_gain_ = out_gain;
  out_meter_.Push(buffer);
}

juce::AudioProcessorEditor* LDatProcessor::createEditor() { return new LDatEditor(*this); }

void LDatProcessor::getStateInformation(juce::MemoryBlock& dest) {
  if (auto xml = apvts_.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void LDatProcessor::setStateInformation(const void* data, int size) {
  if (auto xml = getXmlFromBinary(data, size))
    if (xml->hasTagName(apvts_.state.getType()))
      apvts_.replaceState(juce::ValueTree::fromXml(*xml));
}

}  // namespace ldat

// JUCE entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ldat::LDatProcessor(); }
