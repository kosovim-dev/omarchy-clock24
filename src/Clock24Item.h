#ifndef CLOCK24ITEM_H
#define CLOCK24ITEM_H

#include <QQuickPaintedItem>
#include <QTimer>
#include <QDateTime>
#include <QBrush>
#include <QPixmap>
#include <QtQml/qqmlregistration.h>

class Clock24Item : public QQuickPaintedItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(Clock24)

    Q_PROPERTY(int updateIntervalMs READ updateIntervalMs WRITE setUpdateIntervalMs NOTIFY updateIntervalMsChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double timeZoneOffset READ timeZoneOffset WRITE setTimeZoneOffset NOTIFY timeZoneOffsetChanged)
    Q_PROPERTY(bool opaqueBackground READ opaqueBackground WRITE setOpaqueBackground NOTIFY opaqueBackgroundChanged)

public:
    explicit Clock24Item(QQuickItem* parent = nullptr);

    int updateIntervalMs() const;
    void setUpdateIntervalMs(int ms);

    bool running() const;
    void setRunning(bool running);

    double latitude() const;
    void setLatitude(double latitude);

    double longitude() const;
    void setLongitude(double longitude);

    double timeZoneOffset() const;
    void setTimeZoneOffset(double hours);

    bool opaqueBackground() const;
    void setOpaqueBackground(bool opaque);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void paint(QPainter* painter) override;

Q_SIGNALS:
    void updateIntervalMsChanged();
    void runningChanged();
    void latitudeChanged();
    void longitudeChanged();
    void timeZoneOffsetChanged();
    void opaqueBackgroundChanged();

private slots:
    void updateTime();

private:
    void RenderStaticLayer(qreal devicePixelRatio);
    void CalculateSunTimes();
    double calculateHourAngleForElevation(double targetElevation, double declination);
    double GetAngleForMinutes(int minutes) const;
    double CalculateMoonPhase() const;

    void DrawSolarTransitions(QPainter& painter, double cx, double cy, double radius);
    void DrawSolarAxis(QPainter& painter, double cx, double cy, double radius);
    void DrawClockFace(QPainter& painter, double cx, double cy, double radius);
    void DrawHourLabels(QPainter& painter, double cx, double cy, double radius);
    void DrawSunriseHand(QPainter& painter, double cx, double cy, double length);
    void DrawSunsetHand(QPainter& painter, double cx, double cy, double length);
    void DrawHourHand(QPainter& painter, double cx, double cy, double length);
    void DrawMinuteHand(QPainter& painter, double cx, double cy, double length);
    void DrawSecondHand(QPainter& painter, double cx, double cy, double length);

    void DrawDigitalTime(QPainter& painter, double cx, double cy);
    void DrawMoonPhase(QPainter& painter, double cx, double cy);
    void DrawDate(QPainter& painter, double cx, double cy);
    void DrawInfo(QPainter& painter, double cx, double cy);

    QTimer* m_timer;
    QDateTime m_currentDateTime;
    QBrush m_carbonBrush;
    QPixmap m_staticLayer;
    bool m_staticDirty = true;

    int m_updateIntervalMs = 250;
    bool m_running = true;
    bool m_opaqueBackground = false;

    int m_sunrise_minutes;
    int m_sunset_minutes;

    double m_sunriseHour = 6.0;
    double m_sunsetHour = 18.0;
    double m_civilDawnHour = 5.5;
    double m_civilDuskHour = 18.5;

    // Location used for the solar arcs, sunrise/sunset hands and solar noon
    // axis. Configurable from QML / shell.json ("latitude", "longitude",
    // "timeZoneOffset"). Defaults are Melbourne, Australia.
    double m_latitude = -37.8136;
    double m_longitude = 144.9631;
    double m_timeZoneOffset = 10.0;

    double m_solarNoonAngle = 0.0;
    double m_solarMidnightAngle = 0.0;
    bool m_hasSolarAxis = false;
};

#endif // CLOCK24ITEM_H