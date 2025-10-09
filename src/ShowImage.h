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

#include <QFileInfo>
#include <QMainWindow>
#include <QPainter>

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
    virtual ~ShowImage();

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

    struct Offset
    {
        Offset(int xx, int yy) noexcept
        :
            x(xx),
            y(yy)
        {}

        void add(int dx, int dy) noexcept
        {
            x += dx;
            y += dy;
        }

        int x;
        int y;
    };

    [[nodiscard]] const char* colourLabel() const noexcept { return (m_greyscale) ? " [ grey ]" : " [ colour ]"; }
    [[nodiscard]] bool fitToScreen() const noexcept { return m_fitToScreen; }
    [[nodiscard]] const char* fitToScreenLabel() const noexcept { return (m_fitToScreen) ? " [ FTS ]" : " [ FOS ]"; }
    [[nodiscard]] bool haveAnnotation() const noexcept { return m_annotate > FONT_OFF; }
    [[nodiscard]] bool haveBlankScreen() const noexcept { return m_isBlank; }
    [[nodiscard]] bool haveFrames() const noexcept { return m_frameIndexMax > 0; }
    [[nodiscard]] bool haveImages() const noexcept { return m_current != INVALID_INDEX; }
    [[nodiscard]] bool notScaled() const noexcept { return scaleOversized() and not oversize() and not m_fitToScreen; }
    [[nodiscard]] bool originalSize() const noexcept { return m_percent == 100; }
    [[nodiscard]] bool scaleActualSize() const noexcept { return m_zoom == 1; }
    [[nodiscard]] bool scaleOversized() const noexcept { return m_zoom == SCALE_OVERSIZED; }
    [[nodiscard]] bool scaleZoomed() const noexcept { return m_zoom > 1; }
    [[nodiscard]] const char* transformationLabel() const noexcept { return (m_smoothScale) ? " [ smooth ]" : " [ fast ]"; }
    [[nodiscard]] bool viewingImage() const noexcept { return not m_isBlank and not m_isSplash; }
    [[nodiscard]] int zoomedHeight() const { return m_image.height() * zoomValue(); }
    [[nodiscard]] int zoomedWidth() const { return m_image.width() * zoomValue(); }
    [[nodiscard]] int zoomValue() const noexcept { return (m_zoom == 0) ? 1 : m_zoom; }

    // --------------------------------------------------------------------

    void center() noexcept
    {
        m_offset = Offset(0, 0);
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
    void imageNext();
    void imageNext(bool step);
    void imagePrevious();
    void imagePrevious(bool step);
    void openDirectory();
    void openFrame();
    void openImage();
    [[nodiscard]] bool oversize() const noexcept;
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
    [[nodiscard]] Qt::TransformationMode transformationMode() const noexcept;
    void zoomIn();
    void zoomOut();

    static const int INVALID_INDEX{-1};

    enum Histogram
    {
        HISTOGRAM_OFF = 0,
        HISTOGRAM_RGB = 1,
        HISTOGRAM_INTENSITY = 2
    };

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

    static const int PAN_STEP{10};

    AnnotationFont m_annotate;
    int m_current;
    QString m_directory;
    int m_enlighten;
    std::vector<QFileInfo> m_files;
    bool m_fitToScreen;
    int m_frame;
    int m_frameIndexMax;
    bool m_greyscale;
    Histogram m_histogram;
    QImage m_histogramImage;
    QImage m_image;
    QImage m_imageProcessed;
    bool m_isBlank;
    bool m_isSplash;
    int m_percent;
    bool m_smoothScale;
    Offset m_offset;
    int m_zoom;
};
