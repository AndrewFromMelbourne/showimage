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

#pragma once

#include <QMainWindow>
#include <QPainter>

#include "files.h"
#include "frame.h"
#include "histogram.h"
#include "scale.h"

#include <vector>

// ------------------------------------------------------------------------

class ShowImage
:
    public QMainWindow
{
    Q_OBJECT

public:

    static const int DEFAULT_WIDTH{640};
    static const int DEFAULT_HEIGHT{480};

    ShowImage(QWidget* parent = nullptr);
    virtual ~ShowImage() = default;

    ShowImage(const ShowImage&) = delete;
    ShowImage(ShowImage &&) = delete;
    ShowImage& operator=(const ShowImage&) = delete;
    ShowImage&& operator=(ShowImage &&) = delete;

    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

    void setExtents();

private:

    // --------------------------------------------------------------------

    class Offset
    {
    public:

        Offset(int x, int y) noexcept
        :
            m_x(x),
            m_y(y),
            m_zoom(1),
            m_zoomedX(x * m_zoom),
            m_zoomedY(y * m_zoom)
        {}

        void center() noexcept
        {
            m_x = 0;
            m_y = 0;
            m_zoomedX = 0;
            m_zoomedY = 0;
        }

        void pan(int dx, int dy, int zoom) noexcept
        {
            m_x += dx;
            m_y += dy;
            zoomed(zoom);
        }

        [[nodiscard]] int x() const noexcept { return m_zoomedX; }
        [[nodiscard]] int y() const noexcept { return m_zoomedY; }

        void zoomed(int zoom) noexcept
        {
            m_zoom = zoom;
            m_zoomedX = m_x * m_zoom;
            m_zoomedY = m_y * m_zoom;
        }

    private:

        int m_x{};
        int m_y{};
        int m_zoom{1};
        int m_zoomedX{};
        int m_zoomedY{};
    };

    // --------------------------------------------------------------------

    [[nodiscard]] const char* colourLabel() const noexcept { return (m_greyscale) ? " [ grey ]" : " [ colour ]"; }
    [[nodiscard]] bool haveAnnotation() const noexcept { return m_annotate > FONT_OFF; }
    [[nodiscard]] bool haveBlankScreen() const noexcept { return m_isBlank; }
    [[nodiscard]] bool haveImages() const noexcept { return m_files.haveImages(); }
    [[nodiscard]] bool haveSplashScreen() const noexcept { return m_isSplash; }
    [[nodiscard]] bool viewingImage() const noexcept { return not m_isBlank and not m_isSplash; }

    // --------------------------------------------------------------------

    void center() noexcept
    {
        m_offset.center();
    }

    // --------------------------------------------------------------------

    void centerAndRepaint()
    {
        center();
        repaint();
    }

    // --------------------------------------------------------------------

    void processImageAndRepaint()
    {
        processImage();
        repaint();
    }

    // --------------------------------------------------------------------

    void annotate(QPainter& painter);
    [[nodiscard]] QString annotation() const;
    void enlighten(bool decrease);
    void frameNext();
    void framePrevious();
    void handleGeneralKeys(int key, bool isShift);
    void handleImageViewingKeys(int key, bool isShift);
    void histogram(QPainter& painter);
    void imageNext(bool step = false);
    void imagePrevious(bool step = false);
    void openDirectory();
    void openFrame();
    void openImage();
    void paint(QPainter& painter);
    void pan(int x, int y);
    [[nodiscard]] QPoint placeImage(const QImage& image) const noexcept;
    void processImage();
    void processImageEnlighten();
    void processImageGreyscale();
    void processImageHistogram();
    void processImageResize();
    void readDirectory();
    void splashScreenDisable();
    void splashScreenEnable();
    void toggleAnnotation();
    void toggleBlankScreen();
    void toggleFitToScreen();
    void toggleFullScreen();
    void toggleGreyScale();
    void toggleHistogram();
    void toggleSmoothScale();
    void zoomIn();
    void zoomOut();

    enum Zoom
    {
        SCALE_OVERSIZED = 0,
        SCALE_MAXIMUM = 5
    };

    enum AnnotationFont
    {
        FONT_OFF = 0,
        FONT_REGULAR = 12,
        FONT_LARGE = 24
    };

    enum EnlightenLimits
    {
        ENLIGHTEN_MINIMUM = 0,
        ENLIGHTEN_MAXIMUM = 10
    };

    enum PanStep
    {
        PAN_STEP_SMALL = 10,
        PAN_STEP_LARGE = 100
    };

    AnnotationFont m_annotate;
    int m_enlighten;
    Files m_files;
    Frame m_frame;
    bool m_greyscale;
    Histogram m_histogram;
    QImage m_image;
    QImage m_imageProcessed;
    bool m_isBlank;
    bool m_isSplash;
    Scale m_scale;
    Offset m_offset;
};
