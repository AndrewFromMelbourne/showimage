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

#include "scale.h"

//-------------------------------------------------------------------------

bool
Scale::fitsWithinScreen() const noexcept
{
    if ((m_processedSize.width() <= m_screenSize.width()) and
        (m_processedSize.height() <= m_screenSize.height()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//-------------------------------------------------------------------------

bool
Scale::oversize() const noexcept
{
    if ((zoomedWidth() > m_screenSize.width()) or
        (zoomedHeight() > m_screenSize.height()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

// ------------------------------------------------------------------------

QImage
Scale::scale(const QImage& image)
{
    QImage result;

    m_imageSize = image.size();

    if (notScaled() or scaleActualSize())
    {
        result = image;
        m_percent = 100;
    }
    else if (scaleZoomed())
    {
        result = image.scaled(image.width() * m_zoom,
                              image.height() * m_zoom,
                              Qt::KeepAspectRatio,
                              transformationMode());
        m_percent = m_zoom * 100;
    }
    else
    {
        result = image.scaled(m_screenSize,
                              Qt::KeepAspectRatio,
                              transformationMode());

        const double percent = (image.width() > 0)
                                ? std::round((100.0 * result.width()) / image.width())
                                : 0.0;
        m_percent = static_cast<int>(percent);
    }

    m_processedSize = result.size();

    return result;
}

// ------------------------------------------------------------------------

bool
Scale::zoomIn() noexcept
{
    if (m_zoom == SCALE_MAXIMUM)
    {
        return false;
    }

    ++m_zoom;

    return true;
}

// ------------------------------------------------------------------------

bool
Scale::zoomOut() noexcept
{
    if (m_zoom == 0)
    {
        return false;
    }

    --m_zoom;

    return true;
}

// ------------------------------------------------------------------------

Qt::TransformationMode
Scale::transformationMode() const noexcept
{
    return (m_smoothScale) ? Qt::SmoothTransformation : Qt::FastTransformation;
}
