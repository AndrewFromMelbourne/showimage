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

#pragma once

#include <QImage>

// ------------------------------------------------------------------------

class Scale
{
public:

    [[nodiscard]] bool fitToScreen() const noexcept { return m_fitToScreen; }
    [[nodiscard]] const char* fitToScreenLabel() const noexcept { return (m_fitToScreen) ? " [ FTS ]" : " [ FOS ]"; }
    [[nodiscard]] bool notScaled() const noexcept { return scaleOversized() and not oversize() and not m_fitToScreen; }
    [[nodiscard]] bool originalSize() const noexcept { return m_percent == 100; }
    [[nodiscard]] int percent() const noexcept { return m_percent; }
    [[nodiscard]] bool scaleActualSize() const noexcept { return m_zoom == 1; }
    [[nodiscard]] bool scaleOversized() const noexcept { return m_zoom == SCALE_OVERSIZED; }
    [[nodiscard]] bool scaleZoomed() const noexcept { return m_zoom > 1; }
    void toggleFitToScreen() noexcept { m_fitToScreen = !m_fitToScreen; }
    void toggleSmoothScale() noexcept { m_smoothScale = !m_smoothScale; }
    [[nodiscard]] const char* transformationLabel() const noexcept { return (m_smoothScale) ? " [ smooth ]" : " [ fast ]"; }
    [[nodiscard]] int zoomedHeight() const { return m_imageSize.height() * zoomValue(); }
    [[nodiscard]] int zoomedWidth() const { return m_imageSize.width() * zoomValue(); }
    [[nodiscard]] int zoomValue() const noexcept { return (m_zoom == 0) ? 1 : m_zoom; }

    [[nodiscard]] bool fitsWithinScreen() const noexcept;
    [[nodiscard]] bool oversize() const noexcept;
    [[nodiscard]] QImage scale(const QImage& image);
    void screenResize(const QSize& size) noexcept { m_screenSize = size; }
    [[nodiscard]] bool zoomIn() noexcept;
    [[nodiscard]] bool zoomOut() noexcept;

private:

    enum Zoom
    {
        SCALE_OVERSIZED = 0,
        SCALE_MAXIMUM = 5
    };

    [[nodiscard]] Qt::TransformationMode transformationMode() const noexcept;

    bool m_fitToScreen{true};
    QSize m_imageSize{};
    int m_percent{0};
    QSize m_processedSize{};
    QSize m_screenSize{};
    bool m_smoothScale{true};
    int m_zoom{0};
};
