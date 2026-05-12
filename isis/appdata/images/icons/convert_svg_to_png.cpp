// Simple Qt utility to convert SVG icons to PNG
#include <QApplication>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QDebug>

void convertSvgToPng(const QString &svgPath, const QString &pngPath, int size = 24) {
    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        qWarning() << "Failed to load SVG:" << svgPath;
        return;
    }

    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    if (image.save(pngPath)) {
        qDebug() << "Converted:" << svgPath << "->" << pngPath;
    } else {
        qWarning() << "Failed to save PNG:" << pngPath;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString iconsDir = QDir::currentPath();

    // Convert modern SVG icons to PNG
    QStringList conversions = {
        "rgb_modern.svg:rgb.png",
        "viewmag+_modern.svg:viewmag+.png",
        "viewmag-_modern.svg:viewmag-.png",
        "viewmag1_modern.svg:viewmag1.png",
        "viewmagfit_modern.svg:viewmagfit.png",
        "move_modern.svg:move.png",
        "stretch_modern.svg:stretch.png",
        "fileopen_modern.svg:fileopen.png",
        "filesave_modern.svg:filesave.png",
        "measure_modern.svg:measure.png",
    };

    for (const QString &pair : conversions) {
        QStringList parts = pair.split(":");
        QString svgPath = iconsDir + "/" + parts[0];
        QString pngPath = iconsDir + "/" + parts[1];

        if (QFile::exists(svgPath)) {
            convertSvgToPng(svgPath, pngPath, 24);
        } else {
            qDebug() << "SVG not found:" << svgPath;
        }
    }

    qDebug() << "Conversion complete!";
    return 0;
}
