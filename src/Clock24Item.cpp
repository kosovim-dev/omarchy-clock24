#include "Clock24Item.h"
#include <QPainter>
#include <QPixmap>
#include <cmath>
#include "SunRise.h"

Clock24Item::Clock24Item(QQuickItem* parent)
    : QQuickPaintedItem(parent), m_sunrise_minutes(0), m_sunset_minutes(0) {

    setOpaquePainting(true);

    int tileSize = 32;
    QPixmap texture(tileSize, tileSize);
    QPainter pixPainter(&texture);
    pixPainter.setRenderHint(QPainter::Antialiasing, false);

    QColor darkBg(20, 20, 20);
    QColor lightBg(35, 35, 35);
    QColor lineDef(45, 45, 45);

    texture.fill(darkBg);
    pixPainter.setPen(Qt::NoPen);
    pixPainter.setBrush(lightBg);
    pixPainter.drawRect(0, 0, 16, 16);
    pixPainter.drawRect(16, 16, 16, 16);

    pixPainter.setPen(lineDef);
    for (int i = 0; i < tileSize; i += 4) {
        pixPainter.drawLine(i, 0, 0, i);
        pixPainter.drawLine(tileSize, i, i, tileSize);
        pixPainter.drawLine(i, 16, 16, i + 16);
        pixPainter.drawLine(16, i, i + 16, 16);
    }
    pixPainter.end();

    m_carbonBrush.setTexture(texture);
    m_carbonBrush.setStyle(Qt::TexturePattern);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Clock24Item::updateTime);
    m_timer->setInterval(m_updateIntervalMs);

    updateTime();
    CalculateSunTimes();
}

int Clock24Item::updateIntervalMs() const {
    return m_updateIntervalMs;
}

void Clock24Item::setUpdateIntervalMs(int ms) {
    if (ms == m_updateIntervalMs) return;
    m_updateIntervalMs = ms;
    if (m_timer) m_timer->setInterval(ms);
    emit updateIntervalMsChanged();
}

bool Clock24Item::running() const {
    return m_running;
}

void Clock24Item::setRunning(bool running) {
    if (running == m_running) return;
    m_running = running;
    if (m_timer) {
        if (m_running) {
            m_timer->start();
            updateTime();
        } else {
            m_timer->stop();
        }
    }
    emit runningChanged();
}

double Clock24Item::latitude() const { return m_latitude; }

void Clock24Item::setLatitude(double latitude) {
    if (qFuzzyCompare(latitude, m_latitude)) return;
    m_latitude = latitude;
    CalculateSunTimes();
    update();
    emit latitudeChanged();
}

double Clock24Item::longitude() const { return m_longitude; }

void Clock24Item::setLongitude(double longitude) {
    if (qFuzzyCompare(longitude, m_longitude)) return;
    m_longitude = longitude;
    CalculateSunTimes();
    update();
    emit longitudeChanged();
}

double Clock24Item::timeZoneOffset() const { return m_timeZoneOffset; }

void Clock24Item::setTimeZoneOffset(double hours) {
    if (qFuzzyCompare(hours, m_timeZoneOffset)) return;
    m_timeZoneOffset = hours;
    CalculateSunTimes();
    update();
    emit timeZoneOffsetChanged();
}

bool Clock24Item::opaqueBackground() const { return m_opaqueBackground; }

void Clock24Item::setOpaqueBackground(bool opaque) {
    if (opaque == m_opaqueBackground) return;
    m_opaqueBackground = opaque;
    update();
    emit opaqueBackgroundChanged();
}

void Clock24Item::componentComplete() {
    QQuickPaintedItem::componentComplete();
    m_timer->setInterval(m_updateIntervalMs);
    if (m_running) m_timer->start();
}

void Clock24Item::updateTime() {
    m_currentDateTime = QDateTime::currentDateTime();

    static QDate lastDate;
    if (m_currentDateTime.date() != lastDate) {
        CalculateSunTimes();
        lastDate = m_currentDateTime.date();
    }

    update();
}

void Clock24Item::CalculateSunTimes() {
    time_t now = time(nullptr);
    SunRise sr;
    sr.calculate(m_latitude, m_longitude, now);

    if (sr.hasRise && sr.hasSet) {
        struct tm local_rise = *localtime(&sr.riseTime);
        struct tm local_set = *localtime(&sr.setTime);
        m_sunrise_minutes = local_rise.tm_hour * 60 + local_rise.tm_min;
        m_sunset_minutes = local_set.tm_hour * 60 + local_set.tm_min;
    } else {
        m_sunrise_minutes = 6 * 60 + 0;
        m_sunset_minutes = 18 * 60 + 0;
    }

    int dayOfYear = m_currentDateTime.date().dayOfYear();
    double declination = 23.45 * sin((2.0 * M_PI * (284.0 + dayOfYear)) / 365.0) * M_PI / 180.0;
    double hourAngleSet = calculateHourAngleForElevation(-0.833, declination);
    double hourAngleCivil = calculateHourAngleForElevation(-6.0, declination);

    double solarNoon = 12.0 - (m_longitude - (m_timeZoneOffset * 15.0)) / 15.0;

    m_sunriseHour = solarNoon - (hourAngleSet * 180.0 / M_PI) / 15.0;
    m_sunsetHour = solarNoon + (hourAngleSet * 180.0 / M_PI) / 15.0;
    m_civilDawnHour = solarNoon - (hourAngleCivil * 180.0 / M_PI) / 15.0;
    m_civilDuskHour = solarNoon + (hourAngleCivil * 180.0 / M_PI) / 15.0;

    m_solarNoonAngle = ((solarNoon - 12.0) * 15.0 - 90.0) * M_PI / 180.0;
    m_solarMidnightAngle = m_solarNoonAngle + M_PI;
    m_hasSolarAxis = true;
}

double Clock24Item::calculateHourAngleForElevation(double targetElevation, double declination) {
    double latRad = m_latitude * M_PI / 180.0;
    double elRad = targetElevation * M_PI / 180.0;
    double numerator = sin(elRad) - (sin(latRad) * sin(declination));
    double denominator = cos(latRad) * cos(declination);
    double cosH = numerator / denominator;
    if (cosH >= 1.0) return 0.0;
    if (cosH <= -1.0) return M_PI;
    return acos(cosH);
}

double Clock24Item::GetAngleForMinutes(int minutes) const {
    if (minutes < 0) return -999;
    return ((double(minutes) - 720.0) / 1440.0) * 360.0 - 90.0;
}

double Clock24Item::CalculateMoonPhase() const {
    QDate date = m_currentDateTime.date();
    int year = date.year(), month = date.month(), day = date.day();
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    double jd = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

    double known_new_moon = 2451549.5;
    double moon_cycle = 29.53058867;
    double days_since = jd - known_new_moon;
    double phase = fmod(days_since, moon_cycle) / moon_cycle;
    if (phase < 0.0) phase += 1.0;
    return phase;
}

void Clock24Item::paint(QPainter* painter) {
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);

    double width = this->width();
    double height = this->height();
    if (m_opaqueBackground)
        painter->fillRect(QRectF(0, 0, width, height), QColor(32, 32, 32));
    double centerX = width / 2.0;
    double centerY = height / 2.0;
    double radiusDial = qMin(width, height) / 2.0 - 20.0;

    QRectF dialRect(centerX - radiusDial, centerY - radiusDial, radiusDial * 2.0, radiusDial * 2.0);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_carbonBrush);
    painter->drawEllipse(dialRect);

    DrawSolarTransitions(*painter, centerX, centerY, radiusDial);
    DrawClockFace(*painter, centerX, centerY, radiusDial);

    double cx = this->width() / 2.0;
    double cy = this->height() / 2.0;
    double radius = qMin(this->width(), this->height()) / 2.0 - 20.0;

    DrawSolarTransitions(*painter, cx, cy, radius);
    DrawClockFace(*painter, cx, cy, radius);

    DrawSolarAxis(*painter, cx, cy, radius);

    DrawSunriseHand(*painter, cx, cy, radius * 0.625);
    DrawSunsetHand(*painter, cx, cy, radius * 0.625);
    DrawHourHand(*painter, cx, cy, radius * 0.65);
    DrawMinuteHand(*painter, cx, cy, radius * 0.8);
    DrawSecondHand(*painter, cx, cy, radius * 0.9);

    painter->setBrush(QColor(255, 140, 0));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(cx, cy), 6.0, 6.0);

    DrawDigitalTime(*painter, cx, cy);
    DrawMoonPhase(*painter, cx, cy - 100.0);
    DrawDate(*painter, cx, cy);
}

void Clock24Item::DrawSolarTransitions(QPainter& painter, double cx, double cy, double radius) {
    QRectF solarRect(cx - radius + 4.0, cy - radius + 4.0, (radius - 4.0) * 2.0, (radius - 4.0) * 2.0);

    auto toQtAngle16 = [](double decimalHour) {
        double dialAngleDeg = ((decimalHour - 12.0) * 15.0) - 90.0;
        return qRound(-dialAngleDeg * 16.0);
    };

    painter.setBrush(QBrush(QColor(12, 16, 28, 140)));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(solarRect);

    painter.setBrush(QBrush(QColor(235, 242, 252, 45)));
    int sunrise16 = toQtAngle16(m_sunriseHour);
    double daySpanHours = m_sunsetHour - m_sunriseHour;
    if (daySpanHours < 0.0) daySpanHours += 24.0;
    int daySpanAngle16 = qRound(-daySpanHours * 15.0 * 16.0);
    painter.drawPie(solarRect, sunrise16, daySpanAngle16);

    painter.setBrush(QBrush(QColor(38, 48, 74, 140)));

    double dawnSpanHours = m_sunriseHour - m_civilDawnHour;
    if (dawnSpanHours < 0.0) dawnSpanHours += 24.0;
    int dawnSpanAngle16 = qRound(dawnSpanHours * 15.0 * 16.0);
    painter.drawPie(solarRect, sunrise16, dawnSpanAngle16);

    double duskSpanHours = m_civilDuskHour - m_sunsetHour;
    if (duskSpanHours < 0.0) duskSpanHours += 24.0;
    int duskSpanAngle16 = qRound(-duskSpanHours * 15.0 * 16.0);
    painter.drawPie(solarRect, toQtAngle16(m_sunsetHour), duskSpanAngle16);
}

void Clock24Item::DrawSolarAxis(QPainter& painter, double cx, double cy, double radius) {
    if (!m_hasSolarAxis) return;

    QPen axisPen(QColor(255, 140, 0, 90), qMax(1.0, radius * 0.003), Qt::DashLine, Qt::RoundCap);
    painter.setPen(axisPen);

    double nx = cx + (radius * 0.72) * cos(m_solarNoonAngle);
    double ny = cy + (radius * 0.72) * sin(m_solarNoonAngle);
    painter.drawLine(QPointF(cx, cy), QPointF(nx, ny));

    double mx = cx + (radius * 0.72) * cos(m_solarMidnightAngle);
    double my = cy + (radius * 0.72) * sin(m_solarMidnightAngle);
    painter.drawLine(QPointF(cx, cy), QPointF(mx, my));

    int fontSize = qMax(7, qRound(radius * 0.023));
    painter.setFont(QFont("Arial", fontSize, QFont::Bold));
    painter.setPen(QColor(255, 140, 0, 140));

    QFontMetricsF fm(painter.font());
    int textFlags = Qt::AlignCenter | Qt::TextWordWrap;

    QString noonTxt = "SOLAR\nNOON";
    QRectF noonRect = fm.boundingRect(QRectF(0, 0, radius * 0.5, radius * 0.3), textFlags, noonTxt);

    double nBoxW = noonRect.width() + (fontSize * 1.5);
    double nBoxH = noonRect.height() + (fontSize * 0.8);

    double nLabelRadius = radius * 0.65;
    double nlx = cx + nLabelRadius * cos(m_solarNoonAngle);
    double nly = cy + nLabelRadius * sin(m_solarNoonAngle);

    QRectF noonTargetRect(nlx - nBoxW / 2.0, nly - nBoxH / 2.0, nBoxW, nBoxH);
    painter.setPen(QPen(QColor(255, 140, 0, 80), qMax(1.0, radius * 0.002)));
    painter.setBrush(QColor(20, 20, 20, 180));
    painter.drawRoundedRect(noonTargetRect, radius * 0.015, radius * 0.015);
    painter.drawText(noonTargetRect, textFlags, noonTxt);

    QString midnightTxt = "SOLAR\nMIDNIGHT";
    QRectF midnightRect = fm.boundingRect(QRectF(0, 0, radius * 0.5, radius * 0.3), textFlags, midnightTxt);

    double mBoxW = midnightRect.width() + (fontSize * 1.5);
    double mBoxH = midnightRect.height() + (fontSize * 0.8);

    double mLabelRadius = radius * 0.65;
    double mlx = cx + mLabelRadius * cos(m_solarMidnightAngle);
    double mly = cy + mLabelRadius * sin(m_solarMidnightAngle);

    QRectF midnightTargetRect(mlx - mBoxW / 2.0, mly - mBoxH / 2.0, mBoxW, mBoxH);
    painter.setPen(QPen(QColor(255, 140, 0, 80), qMax(1.0, radius * 0.002)));
    painter.setBrush(QColor(20, 20, 20, 180));
    painter.drawRoundedRect(midnightTargetRect, radius * 0.015, radius * 0.015);
    painter.drawText(midnightTargetRect, textFlags, midnightTxt);
}

void Clock24Item::DrawClockFace(QPainter& painter, double cx, double cy, double radius) {
    painter.setPen(QPen(QColor(255, 140, 0), qMax(1.5, radius * 0.005)));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(cx, cy), radius, radius);

    double tickOuter = radius - (radius * 0.013);
    double tickInner = radius - (radius * 0.047);

    for (int hour = 0; hour < 24; ++hour) {
        double angle = (double(hour) * 15.0 - 90.0) * M_PI / 180.0;
        double x1 = cx + tickOuter * cos(angle);
        double y1 = cy + tickOuter * sin(angle);
        double x2 = cx + tickInner * cos(angle);
        double y2 = cy + tickInner * sin(angle);

        if (hour % 6 == 0) {
            painter.setPen(QPen(QColor(255, 140, 0), qMax(4.0, radius * 0.015)));
        } else {
            painter.setPen(QPen(QColor(255, 140, 0), qMax(2.5, radius * 0.010)));
        }
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    painter.setPen(QPen(QColor(0, 200, 0), qMax(1.0, radius * 0.005)));
    double minOuter = radius - (radius * 0.008);
    double minInner = radius - (radius * 0.031);
    for (int minute = 0; minute < 60; ++minute) {
        if (minute % 5 == 0) continue;
        double angle = (double(minute) * 6.0 - 90.0) * M_PI / 180.0;
        double x1 = cx + minOuter * cos(angle);
        double y1 = cy + minOuter * sin(angle);
        double x2 = cx + minInner * cos(angle);
        double y2 = cy + minInner * sin(angle);
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }
    DrawHourLabels(painter, cx, cy, radius);
}

void Clock24Item::DrawHourLabels(QPainter& painter, double cx, double cy, double radius) {
    double labelRadius = radius - (radius * 0.092);

    int normalFontSize = qMax(9, qRound(radius * 0.031));
    int boldFontSize = qMax(11, qRound(radius * 0.040));

    QFont normalFont("Arial", normalFontSize);
    QFont boldFont("Arial", boldFontSize, QFont::Bold);

    for (int hour = 0; hour < 24; ++hour) {
        double angle = (hour * 15.0 - 90.0) * M_PI / 180.0;
        double x = cx + labelRadius * cos(angle);
        double y = cy + labelRadius * sin(angle);

        int displayHour = (hour + 12) % 24;
        QString text = QString::number(displayHour);

        if (displayHour == 0 || displayHour == 6 || displayHour == 12 || displayHour == 18) {
            painter.setFont(boldFont);
            painter.setPen(QColor(255, 140, 0));
        } else {
            painter.setFont(normalFont);
            painter.setPen(QColor(200, 140, 0));
        }

        QRectF rect = painter.fontMetrics().boundingRect(text);
        painter.drawText(QPointF(x - (rect.width() / 2.0), y + (painter.fontMetrics().capHeight() / 2.0)), text);
    }
}

void Clock24Item::DrawSunriseHand(QPainter& painter, double cx, double cy, double length) {
    double angle = GetAngleForMinutes(m_sunrise_minutes);
    if (angle < -900) return;

    double radian = angle * M_PI / 180.0;
    double x = cx + length * cos(radian);
    double y = cy + length * sin(radian);

    painter.setFont(QFont("Arial", qMax(16, qRound(length * 0.18))));
    QString sunIcon = "\u263C";
    QRectF rect = painter.fontMetrics().boundingRect(sunIcon);
    painter.setPen(QColor(255, 220, 50));
    painter.drawText(QPointF(x - rect.width() / 2.0, y + rect.height() / 4.0), sunIcon);

    int timeFontSize = qMax(8, qRound(length * 0.05));
    painter.setFont(QFont("Arial", timeFontSize, QFont::Bold));
    QString label = QString("%1:%2").arg(m_sunrise_minutes/60, 2, 10, QChar('0')).arg(m_sunrise_minutes%60, 2, 10, QChar('0'));

    double boxW = length * 0.25;
    double boxH = length * 0.1;
    painter.drawText(QRectF(x - boxW/2.0, y - boxH * 2.0, boxW, boxH), Qt::AlignCenter, label);
}

void Clock24Item::DrawSunsetHand(QPainter& painter, double cx, double cy, double length) {
    double angle = GetAngleForMinutes(m_sunset_minutes);
    if (angle < -900) return;

    double radian = angle * M_PI / 180.0;
    double x = cx + length * cos(radian);
    double y = cy + length * sin(radian);

    painter.setFont(QFont("Arial", qMax(16, qRound(length * 0.18))));
    QString sunIcon = "\u263C";
    QRectF rect = painter.fontMetrics().boundingRect(sunIcon);
    painter.setPen(QColor(255, 100, 50));
    painter.drawText(QPointF(x - rect.width() / 2.0, y + rect.height() / 4.0), sunIcon);

    int timeFontSize = qMax(8, qRound(length * 0.05));
    painter.setFont(QFont("Arial", timeFontSize, QFont::Bold));
    QString label = QString("%1:%2").arg(m_sunset_minutes/60, 2, 10, QChar('0')).arg(m_sunset_minutes%60, 2, 10, QChar('0'));

    double boxW = length * 0.25;
    double boxH = length * 0.1;
    painter.drawText(QRectF(x - boxW/2.0, y - boxH * 2.0, boxW, boxH), Qt::AlignCenter, label);
}

void Clock24Item::DrawHourHand(QPainter& painter, double cx, double cy, double length) {
    QTime time = m_currentDateTime.time();
    double hour = double((time.hour() + 12) % 24) + double(time.minute()) / 60.0;
    double radian = (hour * 15.0 - 90.0) * M_PI / 180.0;

    painter.setPen(QPen(QColor(255, 140, 0), 10, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(cx, cy), QPointF(cx + length * cos(radian), cy + length * sin(radian)));
}

void Clock24Item::DrawMinuteHand(QPainter& painter, double cx, double cy, double length) {
    QTime time = m_currentDateTime.time();
    double radian = (((double(time.minute()) * 6.0) + (double(time.second()) * 0.1)) - 90.0) * M_PI / 180.0;

    painter.setPen(QPen(QColor(0, 200, 0), 6, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(cx, cy), QPointF(cx + length * cos(radian), cy + length * sin(radian)));
}

void Clock24Item::DrawSecondHand(QPainter& painter, double cx, double cy, double length) {
    QTime time = m_currentDateTime.time();
    double radian = (((double(time.second()) + double(time.msec()) / 1000.0) * 6.0) - 90.0) * M_PI / 180.0;

    painter.setPen(QPen(Qt::red, 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(cx, cy), QPointF(cx + length * cos(radian), cy + length * sin(radian)));
}

void Clock24Item::DrawDigitalTime(QPainter& painter, double cx, double cy) {
    QString timeStr = m_currentDateTime.time().toString("hh:mm:ss");
    double radius = qMin(width(), height()) / 2.0 - 20.0;

    int fontSize = qMax(10, qRound(radius * 0.037));
    QFont font("Arial", fontSize, QFont::Bold);
    font.setFeature("tnum", 1);
    painter.setFont(font);

    QFontMetricsF fm(painter.font());
    double textWidth = fm.horizontalAdvance(timeStr);
    double textHeight = fm.height();

    double paddingX = fontSize * 1.5;
    double paddingY = fontSize * 0.6;
    double boxW = textWidth + paddingX;
    double boxH = textHeight + paddingY;

    double digitalY = cy - (radius * 0.25);

    double borderThickness = qMax(1.5, radius * 0.004);
    painter.setPen(QPen(QColor(255, 140, 0), borderThickness));
    painter.setBrush(QColor(20, 20, 20, 220));

    QRectF textRect(cx - boxW/2.0, digitalY - boxH/2.0, boxW, boxH);
    painter.drawRoundedRect(textRect, radius * 0.021, radius * 0.021);

    painter.setPen(QColor(255, 140, 0));
    painter.drawText(textRect, Qt::AlignCenter, timeStr);
}

void Clock24Item::DrawMoonPhase(QPainter& painter, double cx, double cy) {
    double phase = CalculateMoonPhase();
    QString moonIcon = (phase < 0.03 || phase > 0.97) ? "🌑" : (phase < 0.22) ? "🌒" : (phase < 0.28) ? "🌓" : (phase < 0.47) ? "🌔" : (phase < 0.53) ? "🌕" : (phase < 0.72) ? "🌖" : (phase < 0.78) ? "🌗" : "🌘";
    int illumination = (int)((1.0 - fabs(phase - 0.5) * 2) * 100);
    QString phaseText = QString("%1 %2%").arg(moonIcon).arg(illumination);

    double radius = qMin(width(), height()) / 2.0 - 20.0;
    int fontSize = qMax(10, qRound(radius * 0.037));
    painter.setFont(QFont("Arial", fontSize, QFont::Bold));

    QFontMetricsF fm(painter.font());
    double textWidth = fm.horizontalAdvance(phaseText);
    double textHeight = fm.height();

    double paddingX = fontSize * 1.5;
    double paddingY = fontSize * 0.6;
    double boxW = textWidth + paddingX;
    double boxH = textHeight + paddingY;

    double moonY = cy - (radius * 0.25) + (radius * 0.105);

    double borderThickness = qMax(1.5, radius * 0.004);
    painter.setPen(QPen(QColor(255, 140, 0), borderThickness));
    painter.setBrush(QColor(20, 20, 20, 220));

    QRectF textRect(cx - boxW/2.0, moonY - boxH/2.0, boxW, boxH);
    painter.drawRoundedRect(textRect, radius * 0.021, radius * 0.021);

    painter.setPen(QColor(255, 220, 100));
    painter.drawText(textRect, Qt::AlignCenter, phaseText);
}

void Clock24Item::DrawDate(QPainter& painter, double cx, double cy) {
    QString fullDateStr = m_currentDateTime.date().toString("yyyy-MM-dd  dddd");
    double radius = qMin(width(), height()) / 2.0 - 20.0;

    int fontSize = qMax(12, qRound(radius * 0.026));
    painter.setFont(QFont("Arial", fontSize));

    QFontMetricsF fm(painter.font());
    double textWidth = fm.horizontalAdvance(fullDateStr);
    double textHeight = fm.height();

    double paddingX = fontSize * 1.5;
    double paddingY = fontSize * 0.6;
    double boxW = textWidth + paddingX;
    double boxH = textHeight + paddingY;

    double dateY = cy + radius - (radius * 0.524);

    double borderThickness = qMax(1.5, radius * 0.004);
    painter.setPen(QPen(QColor(255, 140, 0), borderThickness));
    painter.setBrush(QColor(20, 20, 20, 200));

    QRectF textRect(cx - boxW/2.0, dateY - boxH/2.0, boxW, boxH);
    painter.drawRoundedRect(textRect, radius * 0.015, radius * 0.015);

    painter.setPen(QColor(200, 200, 200));
    painter.drawText(textRect, Qt::AlignCenter, fullDateStr);
}

void Clock24Item::DrawInfo(QPainter& painter, double cx, double cy) {
    QString info = QString("Melbourne | Sunrise: %1 | Sunset: %2")
                      .arg(QTime(0, 0).addSecs(m_sunrise_minutes * 60).toString("hh:mm"))
                      .arg(QTime(0, 0).addSecs(m_sunset_minutes * 60).toString("hh:mm"));

    double radius = qMin(width(), height()) / 2.0 - 20.0;
    int fontSize = qMax(7, qRound(radius * 0.023));
    painter.setFont(QFont("Arial", fontSize));

    double infoY = cy + radius + (radius * 0.079);
    double boxW = radius * 0.789;
    double boxH = radius * 0.042;

    painter.setPen(QColor(180, 180, 180));
    painter.drawText(QRectF(cx - boxW/2.0, infoY - boxH/2.0, boxW, boxH), Qt::AlignCenter, info);
}