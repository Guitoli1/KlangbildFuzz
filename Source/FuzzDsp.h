#pragma once
#include <cmath>
#include <array>
#include <juce_core/juce_core.h>

// Klangbild Fuzz – Fuzz und Booster.
// Läuft vierfach überabgetastet; alle Filter hier sind einfache Ein-Pol-Filter in Abtastrate des Oversamplings.
namespace fuzz
{
struct OnePole
{
    float a = 0, z = 0;
    void set (double sr, double f) { a = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * f / sr)); }
    inline float lp (float x) { z += a * (x - z); return z; }
    inline float hp (float x) { return x - lp (x); }
    void reset() { z = 0; }
};

// Resonanter Buckel (für Mitten-Booster und Tone Bender), RBJ-Peak als Biquad
struct Peak
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (double sr, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = juce::MathConstants<double>::twoPi * f / sr;
        const double al = std::sin (w) / (2 * q), c = std::cos (w), a0 = 1 + al / A;
        b0 = (1 + al * A) / a0; b1 = -2 * c / a0; b2 = (1 - al * A) / a0; a1 = -2 * c / a0; a2 = (1 - al / A) / a0;
    }
    inline float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void reset() { z1 = z2 = 0; }
};

enum FuzzType { germanium = 0, silicon, bender, muff };
enum BoostType { clean = 0, treble, mid };

// Germanium-Transistor: weich, unsymmetrisch; Bias verschiebt den Arbeitspunkt.
// Niedriger Bias = "ausgehungert": Totzone → spuckender, abgerissener Ton.
inline float germaniumStage (float x, float g, float bias)
{
    const float starve = juce::jmax (0.0f, 0.5f - bias) * 0.10f;                    // Totzone
    const float ax = std::abs (x);
    x = (ax > starve ? (ax - starve) : 0.0f) * (x < 0 ? -1.0f : 1.0f);
    const float off = 0.35f * (1.0f - bias) + 0.08f;                                // Asymmetrie
    return std::tanh (g * x + off) - std::tanh (off);
}

// Silizium: mehr Verstärkung, härtere Knie
inline float siliconStage (float x, float g, float bias)
{
    const float off = 0.18f * (1.0f - bias);
    const float v = g * x + off;
    const float hard = v / std::pow (1.0f + std::pow (std::abs (v), 2.6f), 1.0f / 2.6f);
    return hard - off / std::pow (1.0f + std::pow (off, 2.6f), 1.0f / 2.6f);
}

// Dioden-Clipping (Muff): symmetrisch, weich
inline float diode (float x) { return x / (1.0f + std::abs (x)); }

struct Channel
{
    OnePole inHp, preLp, postLp1, postLp2, octHp, toneLp, toneHp, boostHp, boostLp, dcOut, guitarLp;
    Peak boostPeak, benderPeak;
    void reset()
    {
        for (auto* f : { &inHp, &preLp, &postLp1, &postLp2, &octHp, &toneLp, &toneHp, &boostHp, &boostLp, &dcOut, &guitarLp }) f->reset();
        boostPeak.reset(); benderPeak.reset();
    }
};
} // namespace fuzz
