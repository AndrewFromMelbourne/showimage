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
    void wheelEvent(QWheelEvent* event) override;

private:

    bool haveImages() const { return m_current != INVALID_INDEX; }
    bool originalSize() const { return m_percent == 100; }

    void annotate(QPainter& painter);
    void imageNext();
    void imagePrevious();
    void openDirectory();
    void openImage();
    bool oversize() const;
    void paint(QPainter& painter);
    void pan(int x, int y);
    QPoint placeImage(const QImage& image) const;
    const char* transformationLabel() const;
    Qt::TransformationMode transformationMode() const;
    int zoomedHeight() const;
    int zoomedWidth() const;

    static const int INVALID_INDEX{-1};
    static const int MAX_ZOOM{5};
    static const int SCALE_OVERSIZED{0};

    bool m_annotate;
    int m_current;
    QFileInfoList m_files;
    QImage m_image;
    int m_percent;
    bool m_smoothScale;
    int m_xOffset;
    int m_yOffset;
    int m_zoom;
};
