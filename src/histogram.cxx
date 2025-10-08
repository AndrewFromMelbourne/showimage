//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2025 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#include "histogram.h"

#include <array>

// ========================================================================

namespace
{
    struct RGBCount
    {
        int r{0};
        int g{0};
        int b{0};
    };

    static constexpr int HistogramAlpha{191};
    static constexpr int HistogramHeight{128};
    static constexpr int ColourValues{256};
    static constexpr int BackgroundBrightness{63};
    static constexpr int HistogramBrightness{255};
};

// ========================================================================

QImage
histogramRGB(
    const QImage& input)
{
    const auto width = input.width();
    const auto height = input.height();
    std::array<RGBCount, ColourValues> counts{};

    QImage output{ColourValues, HistogramHeight, QImage::Format_ARGB32};

    for (auto j = 0 ; j < height ; ++j)
    {
        for (auto i = 0 ; i < width ; ++i)
        {
            const auto c = input.pixelColor(i, j);
            ++(counts[(c.red() * c.alpha()) / 255].r);
            ++(counts[(c.green() * c.alpha()) / 255].g);
            ++(counts[(c.blue() * c.alpha()) / 255].b);
        }
    }

    int max{0};

    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        max = std::max({max, counts[i].r, counts[i].g, counts[i].b});
    }

    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        const auto r = (counts[i].r * HistogramHeight) / max;
        const auto g = (counts[i].g * HistogramHeight) / max;
        const auto b = (counts[i].b * HistogramHeight) / max;

        for (auto j = 0 ; j < HistogramHeight ; ++j)
        {
            const QColor color(
                r >= j ? HistogramBrightness : BackgroundBrightness,
                g >= j ? HistogramBrightness : BackgroundBrightness,
                b >= j ? HistogramBrightness : BackgroundBrightness,
                HistogramAlpha);
            output.setPixelColor(i, HistogramHeight - 1 - j, color);

            }
    }

    return output;
}

//-------------------------------------------------------------------------

QImage
histogramIntensity(
    const QImage& input)
{
    const auto width = input.width();
    const auto height = input.height();
    std::array<int, 256> counts{};

    QImage output{ColourValues, HistogramHeight, QImage::Format_ARGB32};

    for (auto j = 0 ; j < height ; ++j)
    {
        for (auto i = 0 ; i < width ; ++i)
        {
            const auto intensity = qGray(input.pixelColor(i, j).rgb());
            ++(counts[intensity]);
        }
    }

    int max{0};

    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        max = std::max(max, counts[i]);
    }

    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        const auto rgb = (counts[i] * HistogramHeight) / max;

        for (auto j = 0 ; j < HistogramHeight ; ++j)
        {
            const auto c = (rgb >= j) ? HistogramBrightness : BackgroundBrightness;
            const QColor color(c, c, c, HistogramAlpha);
            output.setPixelColor(i, HistogramHeight - 1 - j, color);
        }
    }

    return output;
}

