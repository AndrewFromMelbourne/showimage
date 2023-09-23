#include "MainWindow.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QKeyEvent>

#include <algorithm>
#include <iostream>

// ------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent)
:
    QMainWindow(parent),
    m_annotate{true},
    m_current{-1},
    m_files{},
    m_image{},
    m_percent{100},
    m_smoothScale{false},
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
    switch (event->key())
    {
        case Qt::Key_Escape:

            QCoreApplication::quit();

            break;

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
                repaint();
            }

            break;

        case Qt::Key_Down:

            if (m_zoom > 0)
            {
                --m_zoom;
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

        case Qt::Key_O:

            openDirectory();

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
                repaint();
            }

            break;

        case Qt::Key_Z:

            m_annotate = !m_annotate;
            repaint();

            break;

        default:

            break;
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
    if (m_annotate and haveImages())
    {
        painter.setPen(QPen(Qt::green));
        painter.setFont(QFont("Helvetica", 12));
        QString annotation = QString("%1").arg(m_files[m_current].fileName());

        annotation += QString(" ( %1 x %2 )").arg(QString::number(m_image.width()),
                                                  QString::number(m_image.height()));

        annotation += QString(" [ %1 / %2 ]").arg(QString::number(m_current + 1),
                                                  QString::number(m_files.size()));

        annotation += QString(" %1%").arg(QString::number(m_percent));

        if (not originalSize())
        {
            annotation += transformationLabel();
        }

        painter.drawText(4, 12, annotation);
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
    auto directory = QFileDialog::getExistingDirectory(this, "Image folder");

    if (directory.length() > 0)
    {
        QDir imageDir(directory);
        imageDir.setFilter(QDir::Files);
        imageDir.setNameFilters({"*.bmp", "*.gif", "*.jpg", "*.jpeg", "*.png"});
        imageDir.setSorting(QDir::Name);

        m_files = imageDir.entryInfoList();
    }
    else
    {
        m_files.clear();
    }

    if (m_files.size() > 0)
    {
        m_current = 0;
        openImage();

        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    else
    {
        m_current = INVALID_INDEX;
        m_image = QImage();

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
    m_image.load(m_files[m_current].filePath());

    m_xOffset = 0;
    m_yOffset = 0;

    repaint();
}

// ------------------------------------------------------------------------

void
MainWindow::paint(QPainter& painter)
{
    if (not oversize())
    {
        m_xOffset = 0;
        m_yOffset = 0;
    }

    if (((m_zoom == SCALE_OVERSIZED) and not oversize()) or (m_zoom == 1))
    {
        m_percent = 100;

        auto point = placeImage(m_image);
        painter.drawImage(point, m_image);
    }
    else
    {
        QImage resized;

        if (m_zoom == SCALE_OVERSIZED)
        {
            resized = m_image.scaled(QSize(width(), height()),
                                     Qt::KeepAspectRatio,
                                     transformationMode());

            auto percent = (100.0 * resized.width()) / m_image.width();
            m_percent = static_cast<int>(0.5 + percent);
        }
        else
        {
            resized = m_image.scaled(m_image.width() * m_zoom,
                                     m_image.height() * m_zoom,
                                     Qt::KeepAspectRatio,
                                     transformationMode());

            m_percent = m_zoom * 100;
        }

        auto point = placeImage(resized);
        painter.drawImage(point, resized);
    }

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
