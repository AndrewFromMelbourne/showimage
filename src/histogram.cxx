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

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>

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

//-------------------------------------------------------------------------

using Channel = int (QColor::*)() const;

std::function<int(const QColor&, Channel)>
scaleChannelFunction(
    const QImage& input)
{
    if (input.hasAlphaChannel())
    {
        return []( const QColor& c, Channel channel ) -> int
        {
            return ( (c.*channel)() * c.alpha() ) / 255;
        };
    }

    return []( const QColor& c, Channel channel ) -> int
    {
        return (c.*channel)();
    };
}

//-------------------------------------------------------------------------

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
    auto scaleChannel = scaleChannelFunction(input);

    for (auto j = 0 ; j < height ; ++j)
    {
        for (auto i = 0 ; i < width ; ++i)
        {
            const auto c = input.pixelColor(i, j);
            ++(counts[scaleChannel(c, &QColor::red)].r);
            ++(counts[scaleChannel(c, &QColor::green)].g);
            ++(counts[scaleChannel(c, &QColor::blue)].b);
        }
    }

    int max{};

    for (const auto& count : counts)
    {
        max = std::max({max, count.r, count.g, count.b});
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
    std::array<int, ColourValues> counts{};

    QImage output{ColourValues, HistogramHeight, QImage::Format_ARGB32};

    for (auto j = 0 ; j < height ; ++j)
    {
        for (auto i = 0 ; i < width ; ++i)
        {
            const auto intensity = qGray(input.pixelColor(i, j).rgb());
            ++(counts[intensity]);
        }
    }

    auto max = std::ranges::max(counts);

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

