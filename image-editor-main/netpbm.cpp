#include "netpbm.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>

static void skipWS(std::istream &s)
{
    for (;;) {
        int c = s.peek();
        if (c == EOF) return;
        if (c == '#') { std::string line; std::getline(s, line); }
        else if (std::isspace((unsigned char)c)) s.get();
        else return;
    }
}

static int readInt(std::istream &s)
{
    skipWS(s);
    int v = 0;
    s >> v;
    if (!s) throw std::runtime_error("Blad odczytu liczby z naglowka PNM.");
    return v;
}

static int toPbmVal(const QImage &img, int x, int y)
{
    int idx = img.pixelIndex(x, y);
    QRgb c = (img.colorCount() > 0) ? img.color(idx) : (idx ? 0xFF000000u : 0xFFFFFFFFu);
    return (qGray(c) < 128) ? 1 : 0;
}

static QImage makeMono(int w, int h)
{
    QImage img(w, h, QImage::Format_Mono);
    img.setColorCount(2);
    img.setColor(0, qRgb(255, 255, 255));
    img.setColor(1, qRgb(0, 0, 0));
    for (int y = 0; y < h; ++y)
        memset(img.scanLine(y), 0, img.bytesPerLine());
    return img;
}

QImage loadNetpbm(const QString &path)
{
    std::ifstream f(path.toStdString(), std::ios::binary);
    if (!f) throw std::runtime_error("Nie mozna otworzyc pliku.");

    skipWS(f);
    std::string magic;
    f >> magic;
    if (!f || magic.size() != 2 || magic[0] != 'P')
        throw std::runtime_error("Nieznany format pliku.");

    int w = readInt(f);
    int h = readInt(f);
    int maxval = 1;

    bool isBitmap = (magic == "P1" || magic == "P4");
    bool isBinary = (magic == "P4" || magic == "P5" || magic == "P6");

    if (!isBitmap)
        maxval = readInt(f);

    if (w <= 0 || h <= 0 || maxval <= 0 || maxval > 65535)
        throw std::runtime_error("Nieprawidlowe wymiary lub maxval.");

    // separator po nagłówku
    if (isBinary)
        f.get();

    QImage img;

    // P1 - ASCII PBM
    if (magic == "P1") {
        img = makeMono(w, h);
        int total = w * h;
        for (int i = 0; i < total; ++i) {
            int x = i % w;
            int y = i / w;
            skipWS(f);
            int c = f.get();
            if (c == EOF)
                throw std::runtime_error(
                    "Niekompletny plik P1: brakuje danych od piksela " +
                    std::to_string(i) + " (oczekiwano " + std::to_string(total) +
                    " pikseli dla " + std::to_string(w) + "x" + std::to_string(h) + ").");
            if (c != '0' && c != '1')
                throw std::runtime_error(
                    "Nieprawidlowa wartosc P1 ('" + std::string(1, (char)c) +
                    "') przy pikselu " + std::to_string(i) + ".");
            img.setPixel(x, y, (c == '1') ? 1 : 0);
        }
    }

    // P2 - ASCII PGM
    else if (magic == "P2") {
        img = QImage(w, h, QImage::Format_Grayscale8);
        for (int y = 0; y < h; ++y) {
            uchar *row = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int v = readInt(f);
                row[x] = (uchar)(v * 255 / maxval);
            }
        }
    }

    // P3 - ASCII PPM
    else if (magic == "P3") {
        img = QImage(w, h, QImage::Format_RGB888);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int r = readInt(f);
                int g = readInt(f);
                int b = readInt(f);
                if (maxval != 255) {
                    r = r * 255 / maxval;
                    g = g * 255 / maxval;
                    b = b * 255 / maxval;
                }
                img.setPixel(x, y, qRgb(r, g, b));
            }
        }
    }

    // P4 - Raw PBM
    else if (magic == "P4") {
        img = makeMono(w, h);
        int fileRowBytes = (w + 7) / 8;
        std::vector<unsigned char> buf(fileRowBytes);
        for (int y = 0; y < h; ++y) {
            f.read(reinterpret_cast<char*>(buf.data()), fileRowBytes);
            if (f.gcount() != (std::streamsize)fileRowBytes)
                throw std::runtime_error("Blad odczytu P4 w wierszu " + std::to_string(y) + ".");
            for (int x = 0; x < w; ++x) {
                int bit = (buf[x / 8] >> (7 - (x % 8))) & 1;
                img.setPixel(x, y, bit);
            }
        }
    }

    // P5 - Raw PGM
    else if (magic == "P5") {
        img = QImage(w, h, QImage::Format_Grayscale8);
        for (int y = 0; y < h; ++y) {
            uchar *row = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                if (maxval <= 255) {
                    unsigned char v = 0;
                    f.read(reinterpret_cast<char*>(&v), 1);
                    if (!f) throw std::runtime_error("Blad odczytu P5.");
                    row[x] = (maxval == 255) ? v : (uchar)(v * 255 / maxval);
                } else {
                    unsigned char hi = 0, lo = 0;
                    f.read(reinterpret_cast<char*>(&hi), 1);
                    f.read(reinterpret_cast<char*>(&lo), 1);
                    if (!f) throw std::runtime_error("Blad odczytu P5 16-bit.");
                    row[x] = (uchar)(((hi << 8) | lo) * 255 / maxval);
                }
            }
        }
    }

    // P6 - Raw PPM
    else if (magic == "P6") {
        img = QImage(w, h, QImage::Format_RGB888);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0;
                if (maxval <= 255) {
                    unsigned char rgb[3] = {0, 0, 0};
                    f.read(reinterpret_cast<char*>(rgb), 3);
                    if (!f) throw std::runtime_error("Blad odczytu P6.");
                    r = (maxval == 255) ? rgb[0] : rgb[0] * 255 / maxval;
                    g = (maxval == 255) ? rgb[1] : rgb[1] * 255 / maxval;
                    b = (maxval == 255) ? rgb[2] : rgb[2] * 255 / maxval;
                } else {
                    auto read16 = [&]() -> int {
                        unsigned char hi = 0, lo = 0;
                        f.read(reinterpret_cast<char*>(&hi), 1);
                        f.read(reinterpret_cast<char*>(&lo), 1);
                        return (hi << 8) | lo;
                    };
                    r = read16() * 255 / maxval;
                    g = read16() * 255 / maxval;
                    b = read16() * 255 / maxval;
                    if (!f) throw std::runtime_error("Blad odczytu P6 16-bit.");
                }
                img.setPixel(x, y, qRgb(r, g, b));
            }
        }
    }

    else {
        throw std::runtime_error("Nieobslugiwany format: " + magic);
    }

    if (img.isNull())
        throw std::runtime_error("Nie udalo sie utworzyc obrazu.");

    return img;
}

bool saveNetpbm(const QImage &img, const QString &path, bool ascii)
{
    std::ofstream f(path.toStdString(),
                    ascii ? std::ios::out : (std::ios::out | std::ios::binary));
    if (!f) return false;

    bool isMono = (img.format() == QImage::Format_Mono ||
                   img.format() == QImage::Format_MonoLSB);
    bool isGray = (img.format() == QImage::Format_Grayscale8 ||
                   img.format() == QImage::Format_Grayscale16);

    // Bitmapa: P1 (ASCII) / P4 (raw)
    if (isMono) {
        f << (ascii ? "P1" : "P4") << "\n"
          << img.width() << " " << img.height() << "\n";

        if (ascii) {
            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    if (x > 0) f << ' ';
                    f << toPbmVal(img, x, y);
                }
                f << '\n';
            }
        } else {
            int fileRowBytes = (img.width() + 7) / 8;
            std::vector<unsigned char> buf(fileRowBytes);
            for (int y = 0; y < img.height(); ++y) {
                std::fill(buf.begin(), buf.end(), 0u);
                for (int x = 0; x < img.width(); ++x)
                    if (toPbmVal(img, x, y))
                        buf[x / 8] |= static_cast<unsigned char>(0x80 >> (x % 8));
                f.write(reinterpret_cast<const char*>(buf.data()), fileRowBytes);
            }
        }
    }

    // Skala szarosci
    else if (isGray) {
        f << (ascii ? "P2" : "P5") << "\n"
          << img.width() << " " << img.height() << "\n255\n";

        QImage tmp = img.convertToFormat(QImage::Format_Grayscale8);

        if (ascii) {
            for (int y = 0; y < tmp.height(); ++y) {
                const uchar *row = tmp.constScanLine(y);
                for (int x = 0; x < tmp.width(); ++x) {
                    if (x) f << ' ';
                    f << (int)row[x];
                }
                f << '\n';
            }
        } else {
            for (int y = 0; y < tmp.height(); ++y) {
                const uchar *row = tmp.constScanLine(y);
                f.write(reinterpret_cast<const char*>(row), tmp.width());
            }
        }
    }

    //RGB
    else {
        f << (ascii ? "P3" : "P6") << "\n"
          << img.width() << " " << img.height() << "\n255\n";

        QImage tmp = img.convertToFormat(QImage::Format_RGB888);

        if (ascii) {
            for (int y = 0; y < tmp.height(); ++y) {
                for (int x = 0; x < tmp.width(); ++x) {
                    QRgb px = tmp.pixel(x, y);
                    if (x) f << ' ';
                    f << qRed(px) << ' ' << qGreen(px) << ' ' << qBlue(px);
                }
                f << '\n';
            }
        } else {
            for (int y = 0; y < tmp.height(); ++y) {
                for (int x = 0; x < tmp.width(); ++x) {
                    QRgb px = tmp.pixel(x, y);
                    unsigned char rgb[3] = {
                        (unsigned char)qRed(px),
                        (unsigned char)qGreen(px),
                        (unsigned char)qBlue(px)
                    };
                    f.write(reinterpret_cast<const char*>(rgb), 3);
                }
            }
        }
    }

    return f.good();
}