#pragma once
#include <JuceHeader.h>

// Чистая математика ZDF без лишнего мусора
class ZDFFilter {
public:
    enum Type { LowPass, HighPass, Bell };

    void setParams(float freq, float res, float sampleRate, Type t, float gainDB = 0.0f) {
        float g = std::tan(juce::MathConstants<float>::pi * freq / sampleRate);
        float k = 1.0f / res;
        float gainLinear = std::pow(10.0f, gainDB / 20.0f);

        // Коэффициенты для топологии TPT (Topology Preserving Transform)
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;

        type = t;
        this->g = g;
        this->k = k;
        this->gain = gainLinear;
    }

    float process(float x) {
        float v3 = x - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;

        if (type == LowPass)  return v2;
        if (type == HighPass) return x - k * v1 - v2;
        if (type == Bell)     return x + k * (gain - 1.0f) * v1;

        return x;
    }

private:
    float s1 = 0.0f, s2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float g = 0.0f, k = 0.0f, gain = 1.0f;
    Type type = LowPass;
};
