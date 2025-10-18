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
#include <QThread>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <ranges>

// ------------------------------------------------------------------------

ShowImage::ShowImage(QWidget* parent)
:
    QMainWindow(parent),
    m_annotate{FONT_REGULAR},
    m_enlighten{0},
    m_files{},
    m_frame{},
    m_greyscale{false},
    m_histogram{},
    m_image{
        splash,
        ShowImage::DEFAULT_WIDTH,
        ShowImage::DEFAULT_HEIGHT,
        QImage::Format_Grayscale8
    },
    m_imageProcessed{},
    m_isBlank{false},
    m_isSplash{true},
    m_offset{0, 0}
{
    QImageReader::setAllocationLimit(0);
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
    if (m_isSplash)
    {
        setExtents();
    }

    m_scale.screenResize(event->size());
    processImage();
}

// ------------------------------------------------------------------------

void
ShowImage::wheelEvent(QWheelEvent* event)
{
    const auto delta = event->angleDelta();
    const auto dy = event->isInverted() ? -delta.y() : delta.y();

    if (dy > 0)
    {
        imagePrevious();
    }
    else if (dy < 0)
    {
        imageNext();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::setExtents()
{
    if (m_isSplash)
    {
        resize(ShowImage::DEFAULT_WIDTH, ShowImage::DEFAULT_HEIGHT);
        setFixedSize(ShowImage::DEFAULT_WIDTH, ShowImage::DEFAULT_HEIGHT);
    }
    else
    {
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

#ifdef Q_OS_WIN32
    const auto hint = Qt::MSWindowsFixedSizeDialogHint;
    if (m_isSplash)
    {
        setWindowFlags(windowFlags() | hint);
    }
    else
    {
        setWindowFlags(windowFlags() & ~hint);
    }
    show();
#endif
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

    const QFontMetrics metrics(font);
    auto bound{metrics.boundingRect(text)};
    const QRect rect
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
    const auto name = m_files.absolutePath();
    const auto nameLength = name.length() - m_files.directory().length() - 1;
    auto text = QString("%1").arg(name.right(nameLength));

    text += QString(" ( %1 x %2 )").arg(QString::number(m_image.width()),
                                        QString::number(m_image.height()));

    text += QString(" [ %1 / %2 ]").arg(QString::number(m_files.index() + 1),
                                        QString::number(m_files.count()));

    text += QString(" %1%").arg(QString::number(m_scale.percent()));

    if (not m_scale.originalSize())
    {
        text += m_scale.transformationLabel();
    }

    text += colourLabel();
    text += m_scale.fitToScreenLabel();
    text += QString(" [ enlighten %1% ]").arg(QString::number(m_enlighten * 10));

    if (m_frame.max() > 0)
    {
        text += QString(" [ frame %1/%2 ]").arg(QString::number(m_frame.index() + 1),
                                                QString::number(m_frame.max() + 1));
    }

    return text;
}

// ------------------------------------------------------------------------

void
ShowImage::enlighten(bool decrease)
{
    bool repaint = false;

    if (decrease)
    {
        if (m_enlighten > ENLIGHTEN_MINIMUM)
        {
            --m_enlighten;
            repaint = true;
        }
    }
    else
    {
        if (m_enlighten < ENLIGHTEN_MAXIMUM)
        {
            ++m_enlighten;
            repaint = true;
        }
    }

    if (repaint)
    {
        m_histogram.invalidate();
        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::frameNext()
{
    if (m_frame.next())
    {
        openFrame();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::framePrevious()
{
    if (m_frame.previous())
    {
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
    const auto panStep = isShift ? PAN_STEP_LARGE : PAN_STEP_SMALL;

    switch (key)
    {
        case Qt::Key_Left:

            imagePrevious(isShift);
            break;

        case Qt::Key_Right:

            imageNext(isShift);
            break;

        case Qt::Key_Up:

            zoomIn();
            break;

        case Qt::Key_Down:

            zoomOut();
            break;

        case Qt::Key_A:

            pan(panStep, 0);
            break;

        case Qt::Key_C:

            centerAndRepaint();
            break;

        case Qt::Key_D:

            pan(-panStep, 0);
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

        case Qt::Key_H:

            toggleHistogram();
            break;

        case Qt::Key_S:

            pan(0, -panStep);
            break;

        case Qt::Key_W:

            pan(0, panStep);
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
ShowImage::histogram(QPainter& painter)
{
    const auto& histogramImage = m_histogram.image();
    if (histogramImage.isNull())
    {
        return;
    }

    const auto x = width() - histogramImage.width() - 2;
    const auto y = height() - histogramImage.height() - 2;
    painter.drawImage(QPoint(x, y), histogramImage);
}

// ------------------------------------------------------------------------

void
ShowImage::imageNext(bool step)
{
    if (haveImages())
    {
        m_files.next(step);
        openImage();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::imagePrevious(bool step)
{
    if (haveImages())
    {
        m_files.previous(step);
        openImage();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::openDirectory()
{
    m_files.setDirectory(QFileDialog::getExistingDirectory(this, "Image folder"));
    readDirectory();
}

// ------------------------------------------------------------------------

void
ShowImage::openFrame()
{
    QImageReader reader{m_files.path()};

    for (auto i = 0 ; i < m_frame.index() ; ++i)
    {
        reader.read();
    }

    m_image = reader.read();
    m_histogram.invalidate();

    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::openImage()
{
    QImageReader reader{m_files.path()};

    m_frame.set(reader.imageCount() - 1);
    m_image = reader.read();

    center();
    m_enlighten = 0;
    m_histogram.invalidate();

    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::paint(QPainter& painter)
{
    if (m_isSplash)
    {
        painter.drawImage(placeImage(m_image), m_image);
        return;
    }

    if (m_isBlank)
    {
        return;
    }

    if (not m_scale.oversize())
    {
        center();
    }

    if ((m_image.width() > 0) and (m_image.height() > 0))
    {
        painter.drawImage(placeImage(m_imageProcessed), m_imageProcessed);
    }

    histogram(painter);
    annotate(painter);
}

// ------------------------------------------------------------------------

void
ShowImage::pan(int x, int y)
{
    if (m_scale.oversize() and not m_scale.scaleOversized())
    {
        const auto zoom = m_scale.zoomValue();
        m_offset.add(x * zoom, y * zoom);
        repaint();
    }
}

// ------------------------------------------------------------------------

QPoint
ShowImage::placeImage(const QImage& image) const noexcept
{
    const auto x = (width() / 2) - (image.width() / 2) + m_offset.x;
    const auto y = (height() / 2) - (image.height() / 2) + m_offset.y;

    return QPoint(x, y);
}

// ------------------------------------------------------------------------

void
ShowImage::processImage()
{
    if ((m_image.width() == 0) or (m_image.height() == 0))
    {
        return;
    }

    processImageGreyscale();
    processImageEnlighten();
    processImageHistogram();
    processImageResize();
}

// ------------------------------------------------------------------------

void
ShowImage::processImageEnlighten()
{
    if (m_enlighten > 0)
    {
        const auto enlighten = m_enlighten / static_cast<double>(ENLIGHTEN_MAXIMUM);
        m_imageProcessed = ::enlighten(m_imageProcessed, enlighten);
    }
}

// ------------------------------------------------------------------------

void
ShowImage::processImageGreyscale()
{
    m_imageProcessed = (m_greyscale)
                     ? m_image.convertToFormat(QImage::Format_Grayscale8)
                     : m_image;
}

// ------------------------------------------------------------------------

void
ShowImage::processImageHistogram()
{
    m_histogram.process(m_imageProcessed);
}

// ------------------------------------------------------------------------

void
ShowImage::processImageResize()
{
    m_imageProcessed = m_scale.scale(m_imageProcessed);
}

// ------------------------------------------------------------------------

void
ShowImage::readDirectory()
{
    if (m_files.readDirectory())
    {
        splashScreenDisable();
        openImage();
    }
    else
    {
        splashScreenEnable();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::splashScreenDisable()
{
    if (m_isSplash)
    {
        m_isSplash = false;
        setExtents();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::splashScreenEnable()
{
    if (not m_isSplash)
    {
        m_isSplash = true;
        m_image = QImage(splash,
                         ShowImage::DEFAULT_WIDTH,
                         ShowImage::DEFAULT_HEIGHT,
                         QImage::Format_Grayscale8);

        center();

        if (isFullScreen())
        {
            showNormal();
        }

        setExtents();
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
    m_scale.toggleFitToScreen();
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
    m_histogram.invalidate();
    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleHistogram()
{
    m_histogram.toggle();
    processImageAndRepaint();
}

// ------------------------------------------------------------------------

void
ShowImage::toggleSmoothScale()
{
    m_scale.toggleSmoothScale();

    if (not m_scale.originalSize())
    {
        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::zoomIn()
{
    if (m_scale.zoomIn())
    {
        processImageAndRepaint();
    }
}

// ------------------------------------------------------------------------

void
ShowImage::zoomOut()
{
    if (m_scale.zoomOut())
    {
        processImageAndRepaint();
    }
}
