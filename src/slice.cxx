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

#include <QThread>

#include "slice.h"

// ========================================================================
namespace
{

// ------------------------------------------------------------------------

static constexpr int MinSliceSize{100};

// ------------------------------------------------------------------------

std::optional<int>
concurrentSlice(
    int dimensionSize)
{

    const auto cores = QThread::idealThreadCount();

    if ((cores == 1) or (dimensionSize < 2 * MinSliceSize))
    {
        return {};
    }

    for (auto threads = 2 ; threads < cores - 1 ; ++threads)
    {
        if (dimensionSize / (threads + 1) < MinSliceSize)
        {
            return threads;
        }
    }

    return cores;
}

// ------------------------------------------------------------------------

}

// ========================================================================

std::optional<int>
concurrentColumnSlice(
    const QImage& image)
{
    return concurrentSlice(image.width());
}

//-------------------------------------------------------------------------

std::optional<int>
concurrentRowSlice(
    const QImage& image)
{
    return concurrentSlice(image.height());
}
