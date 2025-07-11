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
        const auto* row = input.scanLine(j);
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
            sum += *(rb.scanLine(std::clamp(k, 0, height - 1)) + i);
        }

        for (auto j = 0 ; j < height ; ++j)
        {
            sum += *(rb.scanLine(std::clamp(j + radius, 0, height - 1)) + i);
            sum -= *(rb.scanLine(std::clamp(j - radius - 1, 0, height - 1)) + i);

            *(output.scanLine(j) + i) = sum / diameter;
        }
    }

    return output;
}

// ------------------------------------------------------------------------

QImage
maximum(const QImage& input)
{
    const auto width = input.width();
    const auto height = input.height();

    QImage output{width, height, QImage::Format_Grayscale8};

    for (auto j = 0 ; j < height ; ++j)
    {
        auto outputRow = output.scanLine(j);

        for (auto i = 0 ; i < width ; ++i)
        {
            const auto c = input.pixelColor(i, j);
            *(outputRow++) = std::max({c.red(), c.green(), c.blue()});
        }
    }

    return output;
}

// ------------------------------------------------------------------------

}

// ========================================================================

QImage
enlighten(
    const QImage& input,
    double strength)
{
    const auto mb = blur(maximum(input), 12);
    const auto width = input.width();
    const auto height = input.height();

    QImage output{width, height, QImage::Format_RGB32};

    const auto strength2 = strength * strength;
    const auto minI = 1.0 / flerp(1.0, 10.0, strength2);
    const auto maxI = 1.0 / flerp(1.0, 1.111, strength2);

    for (auto j = 0 ; j < height ; ++j)
    {
        auto mbRow = mb.scanLine(j);
        auto outputRow = reinterpret_cast<QRgb*>(output.scanLine(j));

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

    return output;
}

