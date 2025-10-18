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

#include "enlighten.h"

// ========================================================================

namespace
{

// ------------------------------------------------------------------------

double
flerp(
    double value1,
    double value2,
    double alpha)
{
    return (value1 * (1.0 - alpha)) + (value2 * alpha);
}

// ------------------------------------------------------------------------

double
diameter(const QImage& image)
{
    return sqrt((image.width() * image.width()) +
                 (image.height() * image.height()));
}

// ------------------------------------------------------------------------

QImage
blur(
    const QImage& input,
    int radius)
{
    const auto width = input.width();
    const auto height = input.height();
    const auto diameter = 2 * radius + 1;

    // row blurred image
    QImage rb{width, height, QImage::Format_Grayscale8};

    for (auto j = 0 ; j < height ; ++j)
    {
        const auto* row = input.constScanLine(j);
        auto* outputRow = rb.scanLine(j);

        int sum{0};

        for (auto k = -radius - 1 ; k < radius ; ++k)
        {
            sum += *(row + std::clamp(k, 0, width - 1));
        }

        for (auto i = 0 ; i < width ; ++i)
        {
            sum += *(row + std::clamp(i + radius, 0, width - 1));
            sum -= *(row + std::clamp(i - radius - 1, 0, width - 1));

            outputRow[i] = sum / diameter;
        }
    }

    QImage output{width, height, QImage::Format_Grayscale8};

    for (auto i = 0 ; i < width ; ++i)
    {
        int sum{0};

        for (auto k = -radius - 1 ; k < radius ; ++k)
        {
            sum += *(rb.constScanLine(std::clamp(k, 0, height - 1)) + i);
        }

        for (auto j = 0 ; j < height ; ++j)
        {
            sum += *(rb.constScanLine(std::clamp(j + radius, 0, height - 1)) + i);
            sum -= *(rb.constScanLine(std::clamp(j - radius - 1, 0, height - 1)) + i);

            *(output.scanLine(j) + i) = sum / diameter;
        }
    }

    return output;
}

// -------------------------------------------------------------------------

void
maximumRow(
    int j,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    auto* outputRow = output.scanLine(j);

    for (auto i = 0 ; i < width ; ++i)
    {
        const auto c = input.pixelColor(i, j);
        *(outputRow++) = std::max({c.red(), c.green(), c.blue()});
    }
}

// -------------------------------------------------------------------------

void
maximumRowARGB32(
    int row,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    auto* outputRow = output.scanLine(row);
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        *(outputRow++) = std::max({qRed(rgb), qGreen(rgb), qBlue(rgb)}) * qAlpha(rgb) / 255;
    }
}

// -------------------------------------------------------------------------

void
maximumRowRGB32(
    int row,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    auto* outputRow = output.scanLine(row);
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(row));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto rgb = *(pixel++);
        *(outputRow++) = std::max({qRed(rgb), qGreen(rgb), qBlue(rgb)});
    }
}

// -------------------------------------------------------------------------

void
maximumRowGrey8(
    int row,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    auto* outputRow = output.scanLine(row);
    const auto* pixel = input.constScanLine(row);
    std::copy(pixel, pixel + width, output.scanLine(row));
}

// -------------------------------------------------------------------------

auto
maximumRowFunction(
    const QImage& input)
{
    switch (input.format())
    {
        case QImage::Format_ARGB32:

            return maximumRowARGB32;

        case QImage::Format_RGB32:

            return maximumRowRGB32;

        case QImage::Format_Grayscale8:

            return maximumRowGrey8;

        default:

            return maximumRow;
    }
}

// ------------------------------------------------------------------------

void
maximumRowRange(
    int jStart,
    int jEnd,
    const QImage& input,
    QImage& output)
{
    auto rowFunction = maximumRowFunction(input);

    for (auto j = jStart ; j < jEnd ; ++j)
    {
        rowFunction(j, input, output);
    }
}

// ------------------------------------------------------------------------

QImage
maximum(const QImage& input)
{
    const auto height = input.height();
    const auto width = input.width();

    QImage output{width, height, QImage::Format_Grayscale8};
    const auto cores = QThread::idealThreadCount();
    const auto rowsPerCore = height / cores;

    if ((cores == 1) or (rowsPerCore < 100))
    {
        maximumRowRange(0, height, input, output);
    }
    else
    {
        QFutureSynchronizer<void> synchronizer;
        auto runner = [=, &input, &output](int jStart, int jEnd)
        {
            maximumRowRange(jStart, jEnd, input, output);
        };

        for (auto core = 0 ; core < cores ; ++core)
        {
            const auto jStart = core * rowsPerCore;
            const auto jEnd = (core == cores - 1) ? height : (jStart + rowsPerCore);

            synchronizer.addFuture(QtConcurrent::run(runner, jStart, jEnd));
        }
    }

    return output;
}

// ------------------------------------------------------------------------

}

// ========================================================================

void
enlighterRow(
    int j,
    double minI,
    double maxI,
    const QImage& mb,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    const auto* mbRow = mb.constScanLine(j);
    auto* outputRow = reinterpret_cast<QRgb*>(output.scanLine(j));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto c = input.pixel(i, j);
        const auto max = *(mbRow++);
        const auto illumination = std::clamp(max / 255.0, minI, maxI);

        if (illumination < maxI)
        {
            const auto p = illumination / maxI;
            const auto scale = (0.4 + (p * 0.6)) / p;

            const auto red = static_cast<int>(std::clamp(qRed(c) * scale, 0.0, 255.0));
            const auto green = static_cast<int>(std::clamp(qGreen(c) * scale, 0.0, 255.0));
            const auto blue = static_cast<int>(std::clamp(qBlue(c) * scale, 0.0, 255.0));

            c = qRgb(red, green, blue);
        }

        *(outputRow++) = c;
    }
}

// -------------------------------------------------------------------------

void
enlighterRowARGB32(
    int j,
    double minI,
    double maxI,
    const QImage& mb,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(j));
    const auto* mbRow = mb.constScanLine(j);
    auto* outputRow = reinterpret_cast<QRgb*>(output.scanLine(j));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto c = *(pixel++);
        const auto max = *(mbRow++);
        const auto illumination = std::clamp(max / 255.0, minI, maxI);

        if (illumination < maxI)
        {
            const auto p = illumination / maxI;
            const auto scale = (0.4 + (p * 0.6)) / p;

            const auto red = static_cast<int>(std::clamp(qRed(c) * scale, 0.0, 255.0));
            const auto green = static_cast<int>(std::clamp(qGreen(c) * scale, 0.0, 255.0));
            const auto blue = static_cast<int>(std::clamp(qBlue(c) * scale, 0.0, 255.0));

            c = qRgb(red, green, blue);
        }

        *(outputRow++) = c;
    }
}

// -------------------------------------------------------------------------

void
enlighterRowRGB32(
    int j,
    double minI,
    double maxI,
    const QImage& mb,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    const auto* pixel = reinterpret_cast<const QRgb*>(input.constScanLine(j));
    const auto* mbRow = mb.constScanLine(j);
    auto* outputRow = reinterpret_cast<QRgb*>(output.scanLine(j));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto c = *(pixel++);
        const auto max = *(mbRow++);
        const auto illumination = std::clamp(max / 255.0, minI, maxI);

        if (illumination < maxI)
        {
            const auto p = illumination / maxI;
            const auto scale = (0.4 + (p * 0.6)) / p;

            const auto red = static_cast<int>(std::clamp(qRed(c) * scale, 0.0, 255.0));
            const auto green = static_cast<int>(std::clamp(qGreen(c) * scale, 0.0, 255.0));
            const auto blue = static_cast<int>(std::clamp(qBlue(c) * scale, 0.0, 255.0));

            c = qRgb(red, green, blue);
        }

        *(outputRow++) = c;
    }
}

// -------------------------------------------------------------------------

void
enlighterRowGrey8(
    int j,
    double minI,
    double maxI,
    const QImage& mb,
    const QImage& input,
    QImage& output)
{
    const auto width = input.width();
    const auto* pixel = input.constScanLine(j);
    const auto* mbRow = mb.constScanLine(j);
    auto* outputRow = reinterpret_cast<QRgb*>(output.scanLine(j));

    for (auto i = 0 ; i < width ; ++i)
    {
        auto grey = *(pixel++);
        const auto max = *(mbRow++);
        const auto illumination = std::clamp(max / 255.0, minI, maxI);

        if (illumination < maxI)
        {
            const auto p = illumination / maxI;
            const auto scale = (0.4 + (p * 0.6)) / p;

            grey = static_cast<int>(std::clamp(grey * scale, 0.0, 255.0));
        }

        *(outputRow++) = qRgb(grey, grey, grey);;
    }

}

// -------------------------------------------------------------------------

auto
enlightenRowFunction(
    const QImage& input)
{
    switch (input.format())
    {
        case QImage::Format_ARGB32:

            return enlighterRowARGB32;

        case QImage::Format_RGB32:

            return enlighterRowRGB32;

        case QImage::Format_Grayscale8:

            return enlighterRowGrey8;

        default:

            return enlighterRow;
    }
}

// ------------------------------------------------------------------------

void
enlightenRowRange(
    int jStart,
    int jEnd,
    double minI,
    double maxI,
    const QImage& mb,
    const QImage& input,
    QImage& output)
{
    auto rowFunction = enlightenRowFunction(input);

    for (auto j = jStart ; j < jEnd ; ++j)
    {
        rowFunction(j, minI, maxI, mb, input, output);
    }
}

// ------------------------------------------------------------------------

QImage
enlighten(
    const QImage& input,
    double strength)
{
    const auto mb = blur(maximum(input), 12);
    const auto height = input.height();
    const auto width = input.width();

    QImage output{width, height, QImage::Format_ARGB32};

    const auto strength2 = strength * strength;
    const auto minI = 1.0 / flerp(1.0, 10.0, strength2);
    const auto maxI = 1.0 / flerp(1.0, 1.111, strength2);
#if 1
    const auto cores = QThread::idealThreadCount();
    const auto rowsPerCore = height / cores;

    if ((cores == 1) or (rowsPerCore < 100))
    {
        enlightenRowRange(0, height, minI, maxI, mb, input, output);
    }
    else
    {
        QFutureSynchronizer<void> synchronizer;
        auto runner = [=, &mb, &input, &output](int jStart, int jEnd)
        {
            enlightenRowRange(jStart, jEnd, minI, maxI, mb, input, output);
        };

        for (auto core = 0 ; core < cores ; ++core)
        {
            const auto jStart = core * rowsPerCore;
            const auto jEnd = (core == cores - 1) ? height : (jStart + rowsPerCore);

            synchronizer.addFuture(QtConcurrent::run(runner, jStart, jEnd));
        }
    }
#else
    enlightenRowRange(0, height, minI, maxI, mb, input, output);
#endif

    return output;
}
