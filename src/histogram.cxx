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

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QThread>

#include "histogram.h"
#include "slice.h"

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>

// ========================================================================

void
Histogram::process(const QImage& image)
{
    if (m_isValid)
    {
        return;
    }

    m_isValid = true;

    switch (m_style)
    {
        case HISTOGRAM_RGB:

            if (m_image.format() == QImage::Format_Grayscale8)
            {
                m_image = ::histogramIntensity(image);
            }
            else
            {
                m_image = ::histogramRGB(image);
            }

            break;

        case HISTOGRAM_INTENSITY:

            m_image = ::histogramIntensity(image);
            break;

        case HISTOGRAM_OFF:

            m_image = QImage{};
            break;
    }
}

// ------------------------------------------------------------------------

void
Histogram::toggle() noexcept
{
    switch (m_style)
    {
    case HISTOGRAM_OFF:

        m_style = HISTOGRAM_RGB;
        break;

    case HISTOGRAM_RGB:

        m_style = HISTOGRAM_INTENSITY;
        break;

    case HISTOGRAM_INTENSITY:

        m_style = HISTOGRAM_OFF;
        break;
    }

    m_isValid = false;
}

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

// -------------------------------------------------------------------------

};

// ========================================================================

using RGBCountArray = std::array<RGBCount, ColourValues>;

// -------------------------------------------------------------------------

void
add(
    RGBCountArray& target,
    const RGBCountArray& source)
{
    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        target[i].r += source[i].r;
        target[i].g += source[i].g;
        target[i].b += source[i].b;
    }
}

// -------------------------------------------------------------------------

void
histogramColourRow(
    int j,
    const QImage& input,
    RGBCountArray& count)
{
    const auto width = input.width();
    for (auto i = 0 ; i < width ; ++i)
    {
        auto colour = input.pixelColor(i, j);
        const auto r = colour.red() * colour.alpha() / 255;
        const auto g = colour.green() * colour.alpha() / 255;
        const auto b = colour.blue() * colour.alpha() / 255;
        ++(count[r].r);
        ++(count[g].g);
        ++(count[b].b);
    }
}

// -------------------------------------------------------------------------

void
histogramColourRowARGB32(
    int row,
    const QImage& input,
    RGBCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        const auto r = (qRed(rgb) * qAlpha(rgb)) / 255;
        const auto g = (qGreen(rgb) * qAlpha(rgb)) / 255;
        const auto b = (qBlue(rgb) * qAlpha(rgb)) / 255;
        ++(count[r].r);
        ++(count[g].g);
        ++(count[b].b);
    }
}

// -------------------------------------------------------------------------

void
histogramColourRowRGB32(
    int row,
    const QImage& input,
    RGBCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        const auto r = qRed(rgb);
        const auto g = qGreen(rgb);
        const auto b = qBlue(rgb);
        ++(count[r].r);
        ++(count[g].g);
        ++(count[b].b);
    }
}

// -------------------------------------------------------------------------

void
histogramColourRowGrey8(
    int row,
    const QImage& input,
    RGBCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = input.constScanLine(row);

    for (auto i = 0 ; i < width ; ++i)
    {
        const auto intensity = *(pixel++);
        ++(count[intensity].r);
        ++(count[intensity].g);
        ++(count[intensity].b);
    }
}

// -------------------------------------------------------------------------

auto
histogramColourRowFunction(
    const QImage& input)
{
    switch (input.format())
    {
        case QImage::Format_ARGB32:

            return histogramColourRowARGB32;

        case QImage::Format_RGB32:

            return histogramColourRowRGB32;

        case QImage::Format_Grayscale8:

            return histogramColourRowGrey8;

        default:

            return histogramColourRow;
    }
}

// -------------------------------------------------------------------------

RGBCountArray
histogramColourCount(
    int jStart,
    int jEnd,
    const QImage& input)
{
    RGBCountArray counts{};
    auto rowFunction = histogramColourRowFunction(input);

    for (auto j = jStart ; j < jEnd ; ++j)
    {
        rowFunction(j, input, counts);
    }

    return counts;
}

// -------------------------------------------------------------------------


QImage
histogramRGB(
    const QImage& input)
{
    const auto height = input.height();
    RGBCountArray counts{};

    QImage output{ColourValues, HistogramHeight, QImage::Format_ARGB32};
    const auto threads = concurrentRowSlice(input);

    if (threads)
    {
        const auto rowsPerCore = height / *threads;
        QFutureSynchronizer<RGBCountArray> synchronizer;
        auto runner = [=, &input](int jStart, int jEnd) -> auto
        {
            return histogramColourCount(jStart, jEnd, input);
        };

        for (auto thread = 0 ; thread < *threads ; ++thread)
        {
            const auto jStart = thread * rowsPerCore;
            const auto jEnd = (thread == *threads - 1) ? height : (jStart + rowsPerCore);

            synchronizer.addFuture(QtConcurrent::run(runner, jStart, jEnd));
        }

        synchronizer.waitForFinished();

        for (const auto& future : synchronizer.futures())
        {
            add(counts, future.result());
        }
    }
    else
    {
        add(counts, histogramColourCount(0, height, input));
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

// ========================================================================

using IntensityCountArray = std::array<int, ColourValues>;

// -------------------------------------------------------------------------

void
add(
    IntensityCountArray& target,
    const IntensityCountArray& source)
{
    for (auto i = 0 ; i < ColourValues ; ++i)
    {
        target[i] += source[i];
    }
}

// -------------------------------------------------------------------------

void
histogramGreyRow(
    int j,
    const QImage& input,
    IntensityCountArray& count)
{
    const auto width = input.width();
    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = input.pixelColor(i, j).rgb();
        const auto intensity = qGray(rgb) * qAlpha(rgb) / 255;
        ++(count[intensity]);
    }
}

// -------------------------------------------------------------------------

void
histogramGreyRowARGB32(
    int row,
    const QImage& input,
    IntensityCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        const auto intensity = (qGray(rgb) * qAlpha(rgb)) / 255;
        ++(count[intensity]);
    }

}

// -------------------------------------------------------------------------

void
histogramGreyRowRGB32(
    int row,
    const QImage& input,
    IntensityCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        const auto intensity = qGray(rgb);
        ++(count[intensity]);
    }
}

// -------------------------------------------------------------------------

void
histogramGreyRowGrey8(
    int row,
    const QImage& input,
    IntensityCountArray& count)
{
    const auto width = input.width();
    const auto* pixel = input.constScanLine(row);

    for (auto i = 0 ; i < width ; ++i)
    {
        const auto intensity = *(pixel++);
        ++(count[intensity]);
    }
}

// -------------------------------------------------------------------------

auto
histogramGreyRowFunction(
    const QImage& input)
{
    switch (input.format())
    {
        case QImage::Format_ARGB32:

            return histogramGreyRowARGB32;

        case QImage::Format_RGB32:

            return histogramGreyRowRGB32;

        case QImage::Format_Grayscale8:

            return histogramGreyRowGrey8;

        default:

            return histogramGreyRow;
    }
}

// -------------------------------------------------------------------------

IntensityCountArray
histogramGreyCount(
    int jStart,
    int jEnd,
    const QImage& input)
{
    IntensityCountArray counts{};
    auto rowFunction = histogramGreyRowFunction(input);

    for (auto j = jStart ; j < jEnd ; ++j)
    {
        rowFunction(j, input, counts);
    }

    return counts;
}

// -------------------------------------------------------------------------

QImage
histogramIntensity(
    const QImage& input)
{
    const auto height = input.height();
    IntensityCountArray counts{};

    QImage output{ColourValues, HistogramHeight, QImage::Format_ARGB32};

    const auto threads = concurrentRowSlice(input);

    if (threads)
    {
        const auto rowsPerCore = height / *threads;
        QFutureSynchronizer<IntensityCountArray> synchronizer;
        auto runner = [=, &input](int jStart, int jEnd) -> auto
        {
            return histogramGreyCount(jStart, jEnd, input);
        };

        for (auto thread = 0 ; thread < *threads ; ++thread)
        {
            const auto jStart = thread * rowsPerCore;
            const auto jEnd = (thread == *threads - 1) ? height : (jStart + rowsPerCore);

            synchronizer.addFuture(QtConcurrent::run(runner, jStart, jEnd));
        }

        synchronizer.waitForFinished();

        for (const auto& future : synchronizer.futures())
        {
            add(counts, future.result());
        }
    }
    else
    {
        add(counts, histogramGreyCount(0, height, input));
    }

    const auto max = std::ranges::max(counts);

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

