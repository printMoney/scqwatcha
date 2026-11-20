#include "SampleLoader.h"
#include "engine/BpmDetect.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace ts
{

bool isSupportedAudioFile (const juce::File& file)
{
    return file.hasFileExtension ("wav;wave;aif;aiff;aifc;mp3;flac;m4a;caf;ogg");
}

std::unique_ptr<SampleData> decodeSample (const juce::File& file, juce::String& error)
{
    if (! file.existsAsFile())
    {
        error = "File not found: " + file.getFullPathName();
        return nullptr;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Cannot read this file format: " + file.getFileName();
        return nullptr;
    }

    if (reader->sampleRate <= 0 || reader->lengthInSamples <= 0)
    {
        error = "The file has no audio: " + file.getFileName();
        return nullptr;
    }

    constexpr double maxSeconds = 20.0 * 60.0;
    const auto maxFrames = (juce::int64) (maxSeconds * reader->sampleRate);
    const int frames = (int) juce::jmin (reader->lengthInSamples, maxFrames);

    auto s = std::make_unique<SampleData>();
    s->sampleRate = reader->sampleRate;
    s->path = file.getFullPathName();
    s->name = file.getFileNameWithoutExtension();
    s->audio.setSize (2, frames);
    s->audio.clear();

    const bool stereo = reader->numChannels >= 2;
    reader->read (&s->audio, 0, frames, 0, true, stereo);
    if (! stereo)
        s->audio.copyFrom (1, 0, s->audio, 0, 0, frames);

    s->computePeaks (4096);

    const double fromName = bpmFromFileName (file.getFileName());
    s->detectedBpm = fromName > 0 ? fromName : detectBpm (s->audio, s->sampleRate);
    return s;
}

} // namespace ts
