#pragma once
#include <QImage>
#include <QString>

QImage loadNetpbm(const QString &path);
bool saveNetpbm(const QImage &img, const QString &path, bool ascii = false);