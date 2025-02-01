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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    static const int DEFAULT_WIDTH{640};
    static const int DEFAULT_HEIGHT{480};

    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

private:

    bool haveImages() const { return m_current != INVALID_INDEX; }
    bool viewingImage() const { return not m_isBlank and not m_isSplash; }
    bool originalSize() const { return m_percent == 100; }

    void annotate(QPainter& painter);
    const char* colourLabel() const;
    void handleGeneralKeys(int key);
    void handleImageViewingKeys(int key);
    void imageNext();
    void imagePrevious();
    void openDirectory();
    void openImage();
    bool oversize() const;
    void paint(QPainter& painter);
    void pan(int x, int y);
    QPoint placeImage(const QImage& image) const;
    void processImage();
    void readDirectory();
    const char* transformationLabel() const;
    Qt::TransformationMode transformationMode() const;
    int zoomedHeight() const;
    int zoomedWidth() const;

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
