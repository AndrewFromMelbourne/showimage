//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2024 Andrew Duncan
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
#include "ShowImage.h"
#include "splash.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QFontMetrics>
#include <QImageReader>
#include <QKeyEvent>

#include <algorithm>
#include <iostream>

//-------------------------------------------------------------------------

ShowImage::ShowImage(QWidget* parent)
:
    QMainWindow(parent),
    m_annotate{FONT_REGULAR},
    m_current{-1},
    m_directory{},
    m_enlighten{0},
    m_files{},
    m_fitToScreen{false},
    m_frame{0},
    m_frameCount{0},
    m_greyscale{false},
    m_image{
        splash,
        ShowImage::DEFAULT_WIDTH,
        ShowImage::DEFAULT_HEIGHT,
        QImage::Format_Grayscale8
    },
    m_imageProcessed{},
    m_isBlank{false},
    m_isSplash{true},
    m_percent{100},
    m_smoothScale{true},
    m_xOffset{0},
    m_yOffset{0},
    m_zoom{0}
{
}

// ------------------------------------------------------------------------

ShowImage::~ShowImage()
{
}

// ------------------------------------------------------------------------

void
ShowImage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (windowState() == Qt::WindowFullScreen)
        {
            qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
        }
        else
        {
            qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
        }
    }
}

// ------------------------------------------------------------------------

void
ShowImage::keyPressEvent(QKeyEvent* event)
{
    const auto key{event->key()};
    const bool isShift{(event->modifiers() & Qt::ShiftModifier) != 0};

    handleGeneralKeys(key, isShift);

    if (viewingImage())
    {
        handleImageViewingKeys(key, isShift);
    }
}

// ------------------------------------------------------------------------

void
ShowImage::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        imageNext();
    }
    if (event->buttons() & Qt::RightButton)
    {
        imagePrevious();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    paint(painter);
}

// ------------------------------------------------------------------------

void
ShowImage::resizeEvent(QResizeEvent *event)
{
    processImage();
}

// ------------------------------------------------------------------------

void
ShowImage::wheelEvent(QWheelEvent* event)
{
     auto delta = event->angleDelta();

     if (delta.y() > 0)
     {
        imageNext();
     }
     else if (delta.y() < 0)
     {
        imagePrevious();
     }
}

// ------------------------------------------------------------------------

void
ShowImage::annotate(QPainter& painter)
{
    if (not m_annotate or not haveImages())
    {
        return;
    }

    const auto text = annotation();
    static constexpr int padding{4};
    const QFont font("Mulish", m_annotate);

    QFontMetrics metrics(font);
    auto bound{metrics.boundingRect(text)};
    QRect rect
    {
        0,
        0,
        bound.width() + 2 * padding,
        bound.height() + 2 * padding
    };
    painter.fillRect(rect, QBrush(QColor(0, 0, 0, 128)));

    painter.setPen(QPen(Qt::green));
    painter.setFont(font);
    painter.drawText(padding, m_annotate + padding, text);
}

// ------------------------------------------------------------------------

QString
ShowImage::annotation() const
{
    auto name = m_files[m_current].absoluteFilePath();
    auto nameLength = name.length() - m_directory.length() - 1;
    auto text = QString("%1").arg(name.right(nameLength));

    text += QString(" ( %1 x %2 )").arg(QString::number(m_image.width()),
                                        QString::number(m_image.height()));

    text += QString(" [ %1 / %2 ]").arg(QString::number(m_current + 1),
                                        QString::number(m_files.size()));

    text += QString(" %1%").arg(QString::number(m_percent));

    if (not originalSize())
    {
        text += transformationLabel();
    }

    text += colourLabel();
    text += fitToScreenLabel();
    text += QString(" [ enlighten %1% ]").arg(QString::number(m_enlighten * 10));

    if (m_frameCount > 1)
    {
        text += QString(" [ frame %1/%2 ]").arg(QString::number(m_frame + 1),
                                                QString::number(m_frameCount));
    }

    return text;
}

// ------------------------------------------------------------------------

void
ShowImage::center()
{
    m_xOffset = 0;
    m_yOffset = 0;
    repaint();
}

// ------------------------------------------------------------------------

const char*
ShowImage::colourLabel() const noexcept
{
    if (m_greyscale)
    {
        return " [ grey ]";
    }
    else
    {
        return " [ colour ]";
    }
}

// ------------------------------------------------------------------------

void
ShowImage::enlighten(bool decrease)
{
    if (decrease)
    {
        if (m_enlighten > 0)
        {
            --m_enlighten;
        }
        else
        {
            m_enlighten = 10;
        }
    }
    else
    {
        if (m_enlighten < 10)
        {
            ++m_enlighten;
        }
        else
        {
            m_enlighten = 0;
        }
    }

    processImageAndRepaint();
}

// ------------------------------------------------------------------------

const char*
ShowImage::fitToScreenLabel() const noexcept
{
    if (m_fitToScreen)
    {
        return " [ FTS ]";
    }
    else
    {
        return " [ FOS ]";
    }
}

// ------------------------------------------------------------------------

void
ShowImage::frameNext()
{
    if (haveFrames())
    {
        ++m_frame;

        if (m_frame == m_frameCount)
        {
            m_frame = 0;
        }

        openFrame();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::framePrevious()
{
    if (haveFrames())
    {
        --m_frame;

        if (m_frame == -1)
        {
            m_frame = m_frameCount - 1;
        }

        openFrame();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::handleGeneralKeys(int key, bool isShift)
{
    switch (key)
    {
        case Qt::Key_Escape:

            QCoreApplication::quit();
            break;

        case Qt::Key_Space:

            toggleBlankScreen();
            break;

        case Qt::Key_O:

            openDirectory();
            break;

        case Qt::Key_R:

            readDirectory();
            break;

        default:

            break;
    }
}

// ------------------------------------------------------------------------

void
ShowImage::handleImageViewingKeys(int key, bool isShift)
{
    switch (key)
    {
        case Qt::Key_Left:

            imagePrevious();
            break;

        case Qt::Key_Right:

            imageNext();
            break;

        case Qt::Key_Up:

            zoomIn();
            break;

        case Qt::Key_Down:

            zoomOut();
            break;

        case Qt::Key_A:

            pan(10, 0);
            break;

        case Qt::Key_C:

            center();
            break;

        case Qt::Key_D:

            pan(-10, 0);
            break;

        case Qt::Key_E:

            enlighten(isShift);
            break;

        case Qt::Key_F:

            toggleFitToScreen();
            break;

        case Qt::Key_G:

            toggleGreyScale();
            break;

        case Qt::Key_S:

            pan(0, -10);
            break;

        case Qt::Key_W:

            pan(0, 10);
            break;

        case Qt::Key_X:

            toggleSmoothScale();
            break;

        case Qt::Key_Z:

            toggleAnnotation();
            break;

        case Qt::Key_Comma:
        case Qt::Key_Less:

            framePrevious();
            break;

        case Qt::Key_Period:
        case Qt::Key_Greater:

            frameNext();
            break;

        case Qt::Key_F11:

            toggleFullScreen();
            break;

        default:

            break;
    }
}

// ------------------------------------------------------------------------

void
ShowImage::imageNext()
{
    if (haveImages())
    {
        ++m_current;

        if (m_current == m_files.size())
        {
            m_current = 0;
        }

        m_frame = 0;

        openImage();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::imagePrevious()
{
    if (haveImages())
    {
        --m_current;

        if (m_current == -1)
        {
            m_current = m_files.size() - 1;
        }

        m_frame = 0;

        openImage();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::openDirectory()
{
    m_directory = QFileDialog::getExistingDirectory(this, "Image folder");
    readDirectory();
}

// ------------------------------------------------------------------------

void
ShowImage::openFrame()
{
    QImageReader reader{m_files[m_current].filePath()};

    for (int i = 0 ; i < m_frame ; ++i)
    {
        reader.read();
    }

    m_image = reader.read();

    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::openImage()
{
    QImageReader reader{m_files[m_current].filePath()};

    m_frameCount = reader.imageCount();
    m_image = reader.read();

    m_xOffset = 0;
    m_yOffset = 0;

    m_enlighten = 0;

    processImageAndRepaint();
}

// ------------------------------------------------------------------------

bool
ShowImage::oversize() const noexcept
{
    if ((zoomedWidth() > width()) or (zoomedHeight() > height()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

// ------------------------------------------------------------------------

void
ShowImage::paint(QPainter& painter)
{
    if (m_isSplash)
    {
        const auto point = placeImage(m_image);
        painter.drawImage(point, m_image);
        return;
    }

    if (m_isBlank)
    {
        return;
    }

    if (not oversize())
    {
        m_xOffset = 0;
        m_yOffset = 0;
    }

    const auto point = placeImage(m_imageProcessed);
    painter.drawImage(point, m_imageProcessed);

    annotate(painter);
}

// ------------------------------------------------------------------------

void
ShowImage::pan(int x, int y)
{
    if (oversize() and (m_zoom != SCALE_OVERSIZED))
    {
        m_xOffset += (x * m_zoom);
        m_yOffset += (y * m_zoom);

        repaint();
    }
}

// ------------------------------------------------------------------------

QPoint
ShowImage::placeImage(const QImage& image) const noexcept
{
    auto x = (width() / 2) - (image.width() / 2) + m_xOffset;
    auto y = (height() / 2) - (image.height() / 2) + m_yOffset;

    return QPoint(x, y);
}

// ------------------------------------------------------------------------

void
ShowImage::processImage()
{
    m_imageProcessed = (m_greyscale)
                     ? m_image.convertToFormat(QImage::Format_Grayscale8)
                     : m_image;

    if (m_enlighten > 0)
    {
        m_imageProcessed = ::enlighten(m_imageProcessed, m_enlighten / 10.0);
    }

    if (notScaled() or scaleActualSize())
    {
        m_percent = 100;
        return;
    }

    if (scaleZoomed())
    {
        m_imageProcessed = m_imageProcessed.scaled(m_imageProcessed.width() * m_zoom,
                                                    m_imageProcessed.height() * m_zoom,
                                                    Qt::KeepAspectRatio,
                                                    transformationMode());

        m_percent = m_zoom * 100;
        return;
    }

    m_imageProcessed = m_imageProcessed.scaled(QSize(width(), height()),
                                                Qt::KeepAspectRatio,
                                                transformationMode());

    auto percent = (100.0 * m_imageProcessed.width()) / m_image.width();
    m_percent = static_cast<int>(0.5 + percent);
}

// ------------------------------------------------------------------------

void
ShowImage::processImageAndRepaint()
{
    processImage();
    repaint();
}

// ------------------------------------------------------------------------

void
ShowImage::readDirectory()
{
    m_files.clear();

    if (m_directory.length() > 0)
    {
        QDirIterator iter(m_directory,
                          {"*.bmp", "*.gif", "*.jpg", "*.jpeg", "*.png"},
                          QDir::Files,
                          QDirIterator::Subdirectories);
        while (iter.hasNext())
        {
            m_files.append(iter.nextFileInfo());
        }
    }

    if (m_files.size() > 0)
    {
        std::sort(
            m_files.begin(),
            m_files.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.absoluteFilePath() < rhs.absoluteFilePath();
            });

        m_current = 0;
        m_isSplash = false;
        openImage();

        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    else
    {
        m_current = INVALID_INDEX;
        m_image = QImage(splash,
                         ShowImage::DEFAULT_WIDTH,
                         ShowImage::DEFAULT_HEIGHT,
                         QImage::Format_Grayscale8);

        m_isSplash = true;

        m_xOffset = 0;
        m_yOffset = 0;

        if (isFullScreen())
        {
            showNormal();
        }
        resize(ShowImage::DEFAULT_WIDTH,
               ShowImage::DEFAULT_HEIGHT);
        setFixedSize(ShowImage::DEFAULT_WIDTH,
                     ShowImage::DEFAULT_HEIGHT);

        repaint();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::toggleAnnotation()
{
    switch (m_annotate)
    {
    case FONT_OFF:

        m_annotate = FONT_REGULAR;
        break;

    case FONT_REGULAR:

        m_annotate = FONT_LARGE;
        break;

    case FONT_LARGE:

        m_annotate = FONT_OFF;
        break;
    }
    repaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleBlankScreen()
{
    m_isBlank = not m_isBlank;
    repaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleFitToScreen()
{
    m_fitToScreen = !m_fitToScreen;
    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleFullScreen()
{
    if (windowState() == Qt::WindowFullScreen)
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::toggleGreyScale()
{
    m_greyscale = !m_greyscale;
    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleSmoothScale()
{
    m_smoothScale = !m_smoothScale;

    if (not originalSize())
    {
        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------

const char*
ShowImage::transformationLabel() const noexcept
{
    if (m_smoothScale)
    {
        return " [ smooth ]";
    }
    else
    {
        return " [ fast ]";
    }
}

// ------------------------------------------------------------------------

Qt::TransformationMode
ShowImage::transformationMode() const noexcept
{
    if (m_smoothScale)
    {
        return Qt::SmoothTransformation;
    }
    else
    {
        return Qt::FastTransformation;
    }
}

// ------------------------------------------------------------------------

void
ShowImage::zoomIn()
{
    if (m_zoom < MAX_ZOOM)
    {
        ++m_zoom;
        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::zoomOut()
{
    if (m_zoom > 0)
    {
        --m_zoom;

        if (m_zoom == 0)
        {
            m_xOffset = 0;
            m_yOffset = 0;
        }

        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------


int
ShowImage::zoomedHeight() const
{
    auto zoom = (m_zoom == 0) ? 1 : m_zoom;

    return m_image.height() * zoom;
}

// ------------------------------------------------------------------------

int
ShowImage::zoomedWidth() const
{
    auto zoom = (m_zoom == 0) ? 1 : m_zoom;

    return m_image.width() * zoom;
}
