#include "BpmDetect.h"
#include <cmath>
#include <vector>

namespace ts
{

double bpmFromFileName (const juce::String& fileName)
{
    const auto lower = fileName.toLowerCase();
    int idx = lower.indexOf ("bpm");
    while (idx >= 0)
    {
        int end = idx;
        while (end > 0 && (lower[end - 1] == ' ' || lower[end - 1] == '_' || lower[end - 1] == '-'))
            --end;
        int start = end;
        while (start > 0 && (juce::CharacterFunctions::isDigit (lower[start - 1]) || lower[start - 1] == '.'))
            --start;
        if (start < end)
        {
            const double v = lower.substring (start, end).getDoubleValue();
            if (v >= 40.0 && v <= 250.0)
                return v;
        }
        idx = lower.indexOf (idx + 3, "bpm");
    }
    return 0.0;
}

double detectBpm (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    const int total = audio.getNumSamples();
    if (total < (int) (sampleRate * 1.0) || sampleRate <= 0)
        return 120.0;

    const int hop = 512;
    const int n = juce::jmin (total, (int) (sampleRate * 90.0));
    const int frames = n / hop;
    if (frames < 16)
        return 120.0;

    const int chans = juce::jmin (2, audio.getNumChannels());
    std::vector<double> energy ((size_t) frames, 0.0);
    for (int f = 0; f < frames; ++f)
    {
        double e = 0.0;
        for (int c = 0; c < chans; ++c)
        {
            const float* d = audio.getReadPointer (c) + f * hop;
            for (int i = 0; i < hop; ++i)
                e += (double) d[i] * d[i];
        }
        energy[(size_t) f] = std::log (1e-9 + e);
    }

    std::vector<double> onset ((size_t) frames, 0.0);
    double mean = 0.0;
    for (int f = 1; f < frames; ++f)
    {
        onset[(size_t) f] = juce::jmax (0.0, energy[(size_t) f] - energy[(size_t) f - 1]);
        mean += onset[(size_t) f];
    }
    mean /= frames;
    for (auto& v : onset)
        v -= mean;

    const double hopRate = sampleRate / hop;
    auto acf = [&] (double lag)
    {
        const int l0 = (int) lag;
        const double t = lag - l0;
        double sum = 0.0;
        for (int f = 0; f + l0 + 1 < frames; ++f)
            sum += onset[(size_t) f] * (onset[(size_t) (f + l0)] * (1.0 - t) + onset[(size_t) (f + l0 + 1)] * t);
        return sum / juce::jmax (1, frames - l0);
    };

    double best = 120.0, bestScore = -1e30;
    for (double bpm = 70.0; bpm <= 180.0; bpm += 0.25)
    {
        const double lag = hopRate * 60.0 / bpm;
        if (lag * 4 >= frames)
            continue;
        const double score = acf (lag) + 0.5 * acf (lag * 2.0) + 0.25 * acf (lag * 4.0);
        if (score > bestScore)
        {
            bestScore = score;
            best = bpm;
        }
    }

    // If the sample is a loop, its length is a whole number of beats.
    const double seconds = total / sampleRate;
    for (int beats : { 4, 8, 16, 32, 64 })
    {
        const double candidate = 60.0 * beats / seconds;
        if (std::abs (candidate - best) / best < 0.03)
            return std::round (candidate * 100.0) / 100.0;
    }

    return std::round (best * 10.0) / 10.0;
}

} // namespace ts
