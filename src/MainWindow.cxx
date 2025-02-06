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

#include "MainWindow.h"
#include "splash.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QFontMetrics>
#include <QImageReader>
#include <QKeyEvent>

#include <algorithm>
#include <iostream>

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
//
// Based on Enlighten by Paul Haeberli
//
// https://github.com/PaulHaeberli/Enlighten
//
// ------------------------------------------------------------------------

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

// ------------------------------------------------------------------------

}

// ========================================================================

MainWindow::MainWindow(QWidget* parent)
:
    QMainWindow(parent),
    m_annotate{FONT_REGULAR},
    m_current{-1},
    m_directory{},
    m_enlighten{0},
    m_files{},
    m_fitToScreen{false},
    m_greyscale{false},
    m_image{
        splash,
        MainWindow::DEFAULT_WIDTH,
        MainWindow::DEFAULT_HEIGHT,
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

MainWindow::~MainWindow()
{
}

// ------------------------------------------------------------------------

void
MainWindow::changeEvent(QEvent* event)
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
MainWindow::keyPressEvent(QKeyEvent* event)
{
    const auto key{event->key()};

    handleGeneralKeys(key);

    if (viewingImage())
    {
        handleImageViewingKeys(key);
    }
}

// ------------------------------------------------------------------------

void
MainWindow::mousePressEvent(QMouseEvent* event)
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
MainWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    paint(painter);
}

// ------------------------------------------------------------------------

void
MainWindow::resizeEvent(QResizeEvent *event)
{
    processImage();
}

// ------------------------------------------------------------------------

void
MainWindow::wheelEvent(QWheelEvent* event)
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
MainWindow::annotate(QPainter& painter)
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
MainWindow::annotation() const
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

    return text;
}

// ------------------------------------------------------------------------

const char*
MainWindow::colourLabel() const
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

const char*
MainWindow::fitToScreenLabel() const
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
MainWindow::handleGeneralKeys(int key)
{
    switch (key)
    {
        case Qt::Key_Escape:

            QCoreApplication::quit();

            break;

        case Qt::Key_Space:

            m_isBlank = not m_isBlank;
            repaint();

            break;

        case Qt::Key_O:

            openDirectory();
            readDirectory();

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
MainWindow::handleImageViewingKeys(int key)
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

            if (m_zoom < MAX_ZOOM)
            {
                ++m_zoom;
                processImage();
                repaint();
            }

            break;

        case Qt::Key_Down:

            if (m_zoom > 0)
            {
                --m_zoom;

                if (m_zoom == 0)
                {
                    m_xOffset = 0;
                    m_yOffset = 0;
                }

                processImage();
                repaint();
            }

            break;

        case Qt::Key_A:

            pan(10, 0);

            break;

        case Qt::Key_C:

            m_xOffset = 0;
            m_yOffset = 0;
            repaint();

            break;

        case Qt::Key_D:

            pan(-10, 0);

            break;

        case Qt::Key_E:

            if (m_enlighten < 10)
            {
                ++m_enlighten;
            }
            else
            {
                m_enlighten = 0;
            }
            processImage();
            repaint();

            break;

        case Qt::Key_F:

            m_fitToScreen = !m_fitToScreen;
            processImage();
            repaint();

            break;

        case Qt::Key_G:

            m_greyscale = !m_greyscale;
            processImage();
            repaint();

            break;

        case Qt::Key_S:

            pan(0, -10);

            break;

        case Qt::Key_W:

            pan(0, 10);

            break;

        case Qt::Key_X:

            m_smoothScale = !m_smoothScale;

            if (not originalSize())
            {
                processImage();
                repaint();
            }

            break;

        case Qt::Key_Z:

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

            break;

        case Qt::Key_F11:

            if (windowState() == Qt::WindowFullScreen)
            {
                showNormal();
            }
            else
            {
                showFullScreen();
            }

            break;

        default:

            break;
    }
}

// ------------------------------------------------------------------------

void
MainWindow::imageNext()
{
    if (haveImages())
    {
        ++m_current;

        if (m_current == m_files.size())
        {
            m_current = 0;
        }

        openImage();
    }
}

// ------------------------------------------------------------------------

void
MainWindow::imagePrevious()
{
    if (haveImages())
    {
        --m_current;

        if (m_current == -1)
        {
            m_current = m_files.size() - 1;
        }

        openImage();
    }
}

// ------------------------------------------------------------------------

void
MainWindow::openDirectory()
{
    m_directory = QFileDialog::getExistingDirectory(this, "Image folder");
}

// ------------------------------------------------------------------------

bool
MainWindow::oversize() const
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
MainWindow::openImage()
{
    QImageReader reader{m_files[m_current].filePath()};
    m_image = reader.read();
    m_isSplash = false;

    m_xOffset = 0;
    m_yOffset = 0;

    m_enlighten = 0;

    processImage();
    repaint();
}

// ------------------------------------------------------------------------

void
MainWindow::paint(QPainter& painter)
{
    if (m_isSplash)
    {
        auto point = placeImage(m_image);
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

    auto point = placeImage(m_imageProcessed);
    painter.drawImage(point, m_imageProcessed);

    annotate(painter);
}

// ------------------------------------------------------------------------

void
MainWindow::pan(int x, int y)
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
MainWindow::placeImage(const QImage& image) const
{
    auto x = (width() / 2) - (image.width() / 2) + m_xOffset;
    auto y = (height() / 2) - (image.height() / 2) + m_yOffset;

    return QPoint(x, y);
}

// ------------------------------------------------------------------------

void
MainWindow::processImage()
{
    m_imageProcessed = (m_greyscale)
                     ? m_image.convertToFormat(QImage::Format_Grayscale8)
                     : m_image;

    if (m_enlighten > 0)
    {
        m_imageProcessed = enlighten(m_imageProcessed, m_enlighten / 10.0);
    }

    if (((m_zoom == SCALE_OVERSIZED) and
        not oversize() and
        not m_fitToScreen) or (m_zoom == 1))
    {
        m_percent = 100;
    }
    else
    {
        if (m_zoom == SCALE_OVERSIZED)
        {
            m_imageProcessed = m_imageProcessed.scaled(QSize(width(), height()),
                                                       Qt::KeepAspectRatio,
                                                       transformationMode());

            auto percent = (100.0 * m_imageProcessed.width()) / m_image.width();
            m_percent = static_cast<int>(0.5 + percent);
        }
        else
        {
            m_imageProcessed = m_imageProcessed.scaled(m_imageProcessed.width() * m_zoom,
                                                       m_imageProcessed.height() * m_zoom,
                                                       Qt::KeepAspectRatio,
                                                       transformationMode());

            m_percent = m_zoom * 100;
        }
    }
}

// ------------------------------------------------------------------------

void
MainWindow::readDirectory()
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
        openImage();

        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    else
    {
        m_current = INVALID_INDEX;
        m_image = QImage(splash,
                         MainWindow::DEFAULT_WIDTH,
                         MainWindow::DEFAULT_HEIGHT,
                         QImage::Format_Grayscale8);

        m_isSplash = true;

        m_xOffset = 0;
        m_yOffset = 0;

        if (isFullScreen())
        {
            showNormal();
        }
        resize(MainWindow::DEFAULT_WIDTH,
               MainWindow::DEFAULT_HEIGHT);
        setFixedSize(MainWindow::DEFAULT_WIDTH,
                     MainWindow::DEFAULT_HEIGHT);

        repaint();
    }
}

// ------------------------------------------------------------------------

const char*
MainWindow::transformationLabel() const
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
MainWindow::transformationMode() const
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

int
MainWindow::zoomedHeight() const
{
    auto zoom = (m_zoom == 0) ? 1 : m_zoom;

    return m_image.height() * zoom;
}

// ------------------------------------------------------------------------

int
MainWindow::zoomedWidth() const
{
    auto zoom = (m_zoom == 0) ? 1 : m_zoom;

    return m_image.width() * zoom;
}
