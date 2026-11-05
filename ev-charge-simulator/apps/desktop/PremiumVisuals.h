
#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QPoint>
#include <QPixmap>

#include "ev/domain/Model.h"

class QLabel;
class QPushButton;
class QTimer;
class QMouseEvent;
class QPainter;

class SocDial : public QWidget {
    Q_OBJECT

public:
    explicit SocDial(QWidget* parent = nullptr);

    void setSoc(double initial, double target);
    void setAnimationsEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    double current_ = 20.0;
    double target_ = 80.0;
    double shown_ = 0.0;
    double from_ = 0.0;

    bool animations_ = true;

    QTimer* timer_ = nullptr;
    int frame_ = 0;
};

class VehicleArtwork : public QWidget {
    Q_OBJECT

public:
    enum class RenderMode {
        Model3D,
        Image2D
    };

    explicit VehicleArtwork(QWidget* parent = nullptr);

    void setVehicle(QString name, QString id);
    void resetCamera();
    void setAnimationsEnabled(bool enabled);

    // Modo PNG estático
    void setVehicleImage(const QString& imagePath);

    // Modo sprite 360°
    void setSpriteFrames(
        const QString& directory,
        int frameCount = 36
    );

    void setRenderMode(RenderMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void paintImage2D(QPainter& painter);

    // Identificação do veículo
    QString name_ = QStringLiteral("Seu próximo destino");
    QString id_ = QStringLiteral("manual");

    // PNG estático
    QPixmap vehicleImage_;

    // Sequência de imagens para rotação
    QVector<QPixmap> spriteFrames_;

    // Controle de renderização
    RenderMode renderMode_ = RenderMode::Model3D;

    // Controle de rotação
    int currentFrame_ = 0;
    int spriteAnimationCounter_ = 0;
    double dragSensitivity_ = 0.35;

    // Animação
    QTimer* timer_ = nullptr;

    bool animations_ = true;
    bool dragging_ = false;

    QPoint lastMouse_;

    double angle_ = -0.72;
    double tick_ = 0.0;
};

class EnergyFlow : public QWidget {
    Q_OBJECT

public:
    explicit EnergyFlow(QWidget* parent = nullptr);

    void setData(
        double sourcePower,
        double batteryEnergy,
        double gridEnergy
    );

    void setAnimationsEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    double power_ = 0.0;
    double battery_ = 0.0;
    double grid_ = 0.0;

    int tick_ = 0;

    bool animations_ = true;

    QTimer* timer_ = nullptr;
};

class ProfileChart : public QWidget {
    Q_OBJECT

public:
    explicit ProfileChart(QWidget* parent = nullptr);

    void setData(
        const ev::domain::ChargeRequest& r,
        const ev::domain::ChargeResult& result
    );

    void reset();
    void setAnimationsEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    bool hasData_ = false;
    bool animations_ = true;

    int frame_ = 100;

    QTimer* timer_ = nullptr;

    ev::domain::ChargeRequest request_;
    ev::domain::ChargeResult result_;
};

class CostChart : public QWidget {
    Q_OBJECT

public:
    explicit CostChart(QWidget* parent = nullptr);

    void setData(const ev::domain::ChargeResult& result);
    void reset();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    bool hasData_ = false;

    ev::domain::ChargeResult result_;
};

class ComparisonChart : public QWidget {
    Q_OBJECT

public:
    explicit ComparisonChart(QWidget* parent = nullptr);

    void setData(
        const QVector<QString>& labels,
        const QVector<double>& hours,
        const QVector<double>& costs
    );

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<QString> labels_;
    QVector<double> hours_;
    QVector<double> costs_;
};

class PremiumVisuals : public QWidget {
    Q_OBJECT

public:
    explicit PremiumVisuals(QWidget* parent = nullptr);

    void setVehicle(const QString& name, const QString& id);

    void setSimulation(
        const ev::domain::ChargeRequest& r,
        const ev::domain::ChargeResult& result
    );

    void reset();

private:
    void setAnimationsEnabled(bool enabled);
    QString getVehicleAssetsPath(const QString& folder, int expectedFrames) const;

    QString vehicleId_ = QStringLiteral("manual");
    QString vehicleName_ = QStringLiteral("Veículo elétrico");

    bool animations_ = true;

    VehicleArtwork* artwork_ = nullptr;
    SocDial* socDial_ = nullptr;
    EnergyFlow* energyFlow_ = nullptr;
    ProfileChart* powerChart_ = nullptr;
    CostChart* costChart_ = nullptr;

    QLabel* vehicleLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* batteryEnergy_ = nullptr;
    QLabel* gridEnergy_ = nullptr;
    QLabel* effectivePower_ = nullptr;

};
