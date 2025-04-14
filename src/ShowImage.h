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
    ~ShowImage();

    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

private:

    [[nodiscard]] bool fitToScreen() const noexcept { return m_fitToScreen; }
    [[nodiscard]] bool haveAnnotation() const noexcept { return m_annotate > FONT_OFF; }
    [[nodiscard]] bool haveBlankScreen() const noexcept { return m_isBlank; }
    [[nodiscard]] bool haveFrames() const noexcept { return m_frameCount > 1; }
    [[nodiscard]] bool haveImages() const noexcept { return m_current != INVALID_INDEX; }
    [[nodiscard]] bool notScaled() const noexcept { return scaleOversized() and not oversize() and not m_fitToScreen; }
    [[nodiscard]] bool originalSize() const noexcept { return m_percent == 100; }
    [[nodiscard]] bool scaleActualSize() const noexcept { return m_zoom == 1; }
    [[nodiscard]] bool scaleOversized() const noexcept { return m_zoom == SCALE_OVERSIZED; }
    [[nodiscard]] bool scaleZoomed() const noexcept { return m_zoom > 1; }
    [[nodiscard]] bool viewingImage() const noexcept { return not m_isBlank and not m_isSplash; }

    void annotate(QPainter& painter);
    [[nodiscard]] QString annotation() const;
    void center();
    [[nodiscard]] const char* colourLabel() const noexcept;
    void enlighten(bool decrease);
    [[nodiscard]] const char* fitToScreenLabel() const noexcept;
    void frameNext();
    void framePrevious();
    void handleGeneralKeys(int key, bool isShift);
    void handleImageViewingKeys(int key, bool isShift);
    void imageNext();
    void imagePrevious();
    void openDirectory();
    void openFrame();
    void openImage();
    [[nodiscard]] bool oversize() const noexcept;
    void paint(QPainter& painter);
    void pan(int x, int y);
    [[nodiscard]] QPoint placeImage(const QImage& image) const noexcept;
    void processImage();
    void processImageAndRepaint();
    void readDirectory();
    void toggleAnnotation();
    void toggleBlankScreen();
    void toggleFitToScreen();
    void toggleFullScreen();
    void toggleGreyScale();
    void toggleSmoothScale();
    [[nodiscard]] const char* transformationLabel() const noexcept;
    [[nodiscard]] Qt::TransformationMode transformationMode() const noexcept;
    void zoomIn();
    void zoomOut();
    [[nodiscard]] int zoomedHeight() const;
    [[nodiscard]] int zoomedWidth() const;

    static const int INVALID_INDEX{-1};
    static const int MAX_ZOOM{5};
    static const int SCALE_OVERSIZED{0};

    static const int FONT_OFF{0};
    static const int FONT_REGULAR{12};
    static const int FONT_LARGE{24};

    int m_annotate;
    int m_current;
    QString m_directory;
    int m_enlighten;
    QFileInfoList m_files;
    bool m_fitToScreen;
    int m_frame;
    int m_frameCount;
    bool m_greyscale;
    QImage m_image;
    QImage m_imageProcessed;
    bool m_isBlank;
    bool m_isSplash;
    int m_percent;
    bool m_smoothScale;
    int m_xOffset;
    int m_yOffset;
    int m_zoom;
};
