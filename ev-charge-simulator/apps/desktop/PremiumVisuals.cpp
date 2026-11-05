#include "PremiumVisuals.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QSettings>
#include <QMouseEvent>
#include <initializer_list>
#include <QPolygonF>
#include <QTimer>
#include <QtMath>
#include <QDebug>
#include <QStringList>
#include <QHash>

#include <algorithm>
#include <utility>
#include <cmath>

using ev::domain::ChargeRequest;
using ev::domain::ChargeResult;

namespace {
constexpr auto navy = "#E6F1FC";
constexpr auto cyan = "#00C9BA";
constexpr auto soft = "#93A9BE";

void text(QPainter& p, QRectF r, const QString& s, int size, const QColor& color, bool bold = false,
          Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
    QFont f(QStringLiteral("Segoe UI"));
    f.setPixelSize(size);
    f.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    p.setFont(f);
    p.setPen(color);
    p.drawText(r, align, s);
}

void roundedRect(QPainter& p, const QRectF& rect, qreal radius, const QColor& fill) {
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawRoundedRect(rect, radius, radius);
}

void placeholder(QPainter& p, const QRectF& area, const QString& message) {
    text(p, area, message, 13, QColor("#8B9CAF"), false, Qt::AlignCenter);
}

} // namespace

SocDial::SocDial(QWidget* parent) : QWidget(parent) {
    setMinimumSize(180, 180);
    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, [this] {
        frame_ = std::min(36, frame_ + 1);
        const double t = static_cast<double>(frame_) / 36.0;
        const double ease = 1.0 - std::pow(1.0 - t, 3.0);
        shown_ = from_ + (target_ - from_) * ease;
        update();
        if (frame_ >= 36) timer_->stop();
    });
}

void SocDial::setAnimationsEnabled(bool enabled) {
    animations_ = enabled;
    if (!enabled) { timer_->stop(); shown_ = target_; update(); }
}

void SocDial::setSoc(double initial, double target) {
    current_ = initial;
    if (std::abs(target_ - target) < 0.001 && shown_ > 0.001) { update(); return; }
    target_ = target;
    if (!animations_) { shown_ = target_; update(); return; }
    from_ = shown_;
    frame_ = 0;
    timer_->start();
}

void SocDial::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const double side = std::min(width(), height());
    const QRectF box((width() - side)/2 + 17, (height() - side)/2 + 17, side-34, side-34);
    QPen ring(QColor("#243B4F"), 13, Qt::SolidLine, Qt::RoundCap);
    p.setPen(ring); p.setBrush(Qt::NoBrush);
    p.drawArc(box, 225 * 16, -270 * 16);
    ring.setColor(QColor("#06DDC5")); p.setPen(ring);
    p.drawArc(box, 225 * 16, -qBound(0.0, shown_, 100.0)*2.7*16);
    ring.setColor(QColor("#E1F3F3")); ring.setWidth(4); p.setPen(ring);
    p.drawArc(box.adjusted(13,13,-13,-13), 225 * 16, -qBound(0.0, current_,100.0)*2.7*16);
    text(p, box.adjusted(0,20,0,-7), QString::number(shown_, 'f', 0)+"%", 34, Qt::white, true, Qt::AlignCenter);
    text(p, QRectF(box.left(),box.center().y()+22,box.width(),22), QStringLiteral("META DE CARGA"), 10, QColor("#94B9C5"),true,Qt::AlignCenter);
    text(p, QRectF(box.left(),box.bottom()-14,box.width(),23), QString("Início: %1%").arg(current_,0,'f',0), 10, QColor("#9EB4C5"),false,Qt::AlignCenter);
}

namespace {
struct V3 { double x,y,z; };
V3 operator+(V3 a,V3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V3 operator-(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V3 operator*(V3 a,double b){return {a.x*b,a.y*b,a.z*b};}
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 cross(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V3 norm(V3 v){double n=std::sqrt(dot(v,v));return n>1e-9?v*(1/n):V3{0,0,1};}
struct Face { QVector<V3> v; QColor color; bool glass=false; double alpha=1.0; };
struct BodySpec {double nose,tail,shoulder,roof,hatch,roofRear,roofFront,roofWidth; QColor paint;};

BodySpec specFor(const QString& id) {
    if(id=="byd-dolphin")return {2.13,-2.00,1.17,1.82,-1.69,-1.18,0.60,0.70,QColor("#3FE4D8")};
    if(id=="renault-kwid-etech")return {1.95,-1.86,1.21,1.93,-1.45,-1.00,0.51,0.68,QColor("#F3B65D")};
    if(id=="nissan-leaf-40")return {2.28,-2.10,1.12,1.72,-1.85,-1.32,0.69,0.70,QColor("#A4B7FF")};
    if(id=="chevrolet-bolt-ev")return {2.02,-1.98,1.23,1.91,-1.57,-1.16,0.58,0.70,QColor("#D0E2EC")};
    return {2.13,-2.03,1.18,1.78,-1.61,-1.13,0.65,0.71,QColor("#63D9F6")};
}

QColor shaded(const Face& f) {
    if(f.v.size()<3)return f.color;
    const V3 n=norm(cross(f.v[1]-f.v[0],f.v[2]-f.v[0]));
    const double light=std::clamp(0.67+0.30*dot(n,norm({-0.7,-0.65,1.1})),0.40,1.05);
    QColor c=f.color; c.setRed(std::clamp(int(c.red()*light),0,255));
    c.setGreen(std::clamp(int(c.green()*light),0,255));c.setBlue(std::clamp(int(c.blue()*light),0,255));
    return c;
}

struct Camera {
    V3 right,up,forward,eye;
    QPointF origin;double scale;
    Camera(double yaw,int w,int h) {
        eye={6.9*std::cos(yaw),6.9*std::sin(yaw),4.3};
        forward=norm(V3{0,0,0.8}-eye);
        right=norm(cross(forward,{0,0,1}));up=norm(cross(right,forward));
        origin={w*.52,h*.64};
        scale=std::min(w/5.65,h/3.20);
    }
    QPointF project(V3 v)const{return {origin.x()+dot(v,right)*scale,origin.y()-dot(v,up)*scale};}
    double depth(V3 v)const{return dot(v-eye,forward);}
};

void face(QVector<Face>& faces,std::initializer_list<V3> pts,QColor col,bool glass=false){
    Face f; for(auto a:pts)f.v.push_back(a);f.color=col;f.glass=glass;faces.push_back(f);
}

void disk(QPainter& p,const Camera& cam,double x,double y,double z,double r,const QColor& c){
    QPolygonF poly;
    for(int i=0;i<=36;i++){double a=i*2*3.14159265358979323846/36.;poly<<cam.project({x+r*std::cos(a),y,z+r*std::sin(a)});}
    p.setPen(Qt::NoPen);p.setBrush(c);p.drawPolygon(poly);
}

void wheel(QPainter& p,const Camera& c,double x,double y,double z,double phase) {
    disk(p,c,x,y,z,.455,QColor("#061018"));
    disk(p,c,x,y+(y<0?-.018:.018),z,.325,QColor("#73899A"));
    disk(p,c,x,y+(y<0?-.025:.025),z,.265,QColor("#162538"));
    p.setPen(QPen(QColor("#C4DAE6"),2.5,Qt::SolidLine,Qt::RoundCap));
    for(int i=0;i<7;i++){double a=i*2*3.14159265358979323846/7.+phase;
       p.drawLine(c.project({x+.083*std::cos(a),y,z+.083*std::sin(a)}),
                  c.project({x+.235*std::cos(a),y,z+.235*std::sin(a)}));}
    disk(p,c,x,y+(y<0?-.03:.03),z,.075,QColor("#DDEBF3"));
}
}

namespace {

    struct VehicleVisualConfig {
        QString assetFolder;
        int frameCount = 0;
        VehicleArtwork::RenderMode renderMode =
            VehicleArtwork::RenderMode::Model3D;
    };

    const QHash<QString, VehicleVisualConfig>& vehicleVisualConfigs(){
        static const QHash<QString, VehicleVisualConfig> configs = {
            {
                QStringLiteral("byd-dolphin"), {
                    QStringLiteral("BYD_Dolphin"),26,
                    VehicleArtwork::RenderMode::Image2D
                }
            },

            {
                QStringLiteral("renault-kwid-etech"), {
                    QStringLiteral("Renault_KwidEtech"),7,
                    VehicleArtwork::RenderMode::Image2D
                }
            },

            {
                QStringLiteral("nissan-leaf-40"), {
                    QStringLiteral("Nissan_Leaf"), 29,
                    VehicleArtwork::RenderMode::Image2D
                }
            },

            {
                QStringLiteral("chevrolet-bolt-ev"), {
                    QStringLiteral("Chevrolet_BoltEV"), 31,
                    VehicleArtwork::RenderMode::Image2D
                }
            }
        
        };

        return configs;
    }

}

VehicleArtwork::VehicleArtwork(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(260, 205);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setCursor(Qt::OpenHandCursor);

    setToolTip(QStringLiteral(
        "Rotação automática. Arraste para controlar a visualização."
    ));

    timer_ = new QTimer(this);
    timer_->setInterval(33);

    connect(timer_, &QTimer::timeout, this, [this]() {
        tick_ += 0.033;

        if (!dragging_) {

            if (renderMode_ == RenderMode::Image2D &&
                !spriteFrames_.isEmpty()) {

                static constexpr int frameInterval = 7;

                spriteAnimationCounter_++;

                if (spriteAnimationCounter_ >= frameInterval) {
                    currentFrame_ =
                        (currentFrame_ + 1) % spriteFrames_.size();

                    spriteAnimationCounter_ = 0;
                }

            } else if (renderMode_ == RenderMode::Model3D) {
                angle_ += 0.0017;
            }
        }

        update();
    });

    timer_->start();
}

void VehicleArtwork::setAnimationsEnabled(bool enabled) {
    animations_=enabled;
    if(enabled)timer_->start(); else timer_->stop();
    update();
}

void VehicleArtwork::setVehicle(QString name,QString id){name_=std::move(name);id_=std::move(id);update();}
void VehicleArtwork::resetCamera(){angle_=-0.72;update();}

void VehicleArtwork::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        dragging_ = true;
        lastMouse_ = e->pos();

        setCursor(Qt::ClosedHandCursor);

        e->accept();
        return;
    }

    QWidget::mousePressEvent(e);
}

void VehicleArtwork::mouseMoveEvent(QMouseEvent* e)
{
    if (dragging_) {

        const double deltaX =
            e->position().x() - lastMouse_.x();

        if (!spriteFrames_.isEmpty() &&
            renderMode_ == RenderMode::Image2D) {

            // Rotação do sprite 360°
            const int frameDelta =
                qRound(deltaX * dragSensitivity_);

            const int count = spriteFrames_.size();

            currentFrame_ =
                (currentFrame_ - frameDelta % count + count) % count;

        } else if (renderMode_ == RenderMode::Model3D) {

            // Rotação do modelo 3D original
            angle_ += deltaX * 0.009;
        }

        lastMouse_ = e->pos();

        update();

        e->accept();
        return;
    }

    QWidget::mouseMoveEvent(e);
}

void VehicleArtwork::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        dragging_ = false;

        setCursor(Qt::OpenHandCursor);

        e->accept();
        return;
    }

    QWidget::mouseReleaseEvent(e);
}

void VehicleArtwork::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {

        if (!spriteFrames_.isEmpty() &&
            renderMode_ == RenderMode::Image2D) {
            currentFrame_ = 0;
        } else {
            resetCamera();
        }

        update();

        e->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(e);
}


void VehicleArtwork::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // =========================================================
    // MODO 2D - SPRITE 360° OU PNG ESTÁTICO
    // =========================================================
    if (renderMode_ == RenderMode::Image2D) {

    const QRectF viewport(0, 0, width(), height());

    // =====================================================
    // BACKGROUND
    // =====================================================

    QLinearGradient bg(0, 0, width(), height());
    bg.setColorAt(0.0, QColor("#10213C"));
    bg.setColorAt(0.6, QColor("#0C182B"));
    bg.setColorAt(1.0, QColor("#071425"));

    p.fillRect(viewport, bg);

    // =====================================================
    // GRID
    // =====================================================

    p.save();

    p.setPen(QPen(QColor(71, 107, 138, 36), 1));

    constexpr int gridSize = 30;

    for (int x = 0; x <= width(); x += gridSize)
        p.drawLine(x, 0, x, height());

    for (int y = 0; y <= height(); y += gridSize)
        p.drawLine(0, y, width(), y);

    p.restore();

    // =====================================================
    // ANIMAÇÃO VERTICAL
    // =====================================================

    const double hover = animations_
        ? 5.0 * std::sin(tick_ * 1.65)
        : 0.0;

    // =====================================================
    // SOMBRA
    // =====================================================

    const QPointF shadowCenter(
        width() * 0.50,
        height() * 0.76 + hover * 0.25
    );

    p.save();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 85));

    p.drawEllipse(
        QRectF(
            shadowCenter.x() - width() * 0.27,
            shadowCenter.y() - 10,
            width() * 0.54,
            20
        )
    );

    p.restore();

    // =====================================================
    // SELEÇÃO DO FRAME
    // =====================================================

    const QPixmap* currentImage = nullptr;

    if (!spriteFrames_.isEmpty()) {

        const int safeIndex =
            qBound(0, currentFrame_, spriteFrames_.size() - 1);

        currentImage = &spriteFrames_[safeIndex];

    } else if (!vehicleImage_.isNull()) {

        currentImage = &vehicleImage_;
    }

    // =====================================================
    // DESENHO DO VEÍCULO
    // =====================================================

    if (currentImage && !currentImage->isNull()) {

        QSize imageSize = currentImage->size();

        imageSize.scale(
            QSize(
                static_cast<int>(width() * 0.96),
                static_cast<int>(height() * 0.82)
            ),
            Qt::KeepAspectRatio
        );

        const QRectF imageRect(
            (width() - imageSize.width()) / 2.0,
            (height() - imageSize.height()) / 2.0 + hover,
            imageSize.width(),
            imageSize.height()
        );

        p.drawPixmap(
            imageRect,
            *currentImage,
            currentImage->rect()
        );

    } else {

        p.setPen(QColor("#FCA5A5"));
        p.setFont(QFont("Arial", 10, QFont::Bold));

        p.drawText(
            viewport,
            Qt::AlignCenter,
            QStringLiteral("Imagem do veículo não carregada")
        );
    }

    // =====================================================
    // BADGE
    // =====================================================

    roundedRect(
        p,
        QRectF(14, 14, 150, 28),
        14,
        QColor(20, 64, 78, 220)
    );

    const QString modeLabel = !spriteFrames_.isEmpty()
        ? QStringLiteral("● SPRITE 360°")
        : QStringLiteral("● MODELO 2D PNG");

    text(
        p,
        QRectF(24, 14, 135, 28),
        modeLabel,
        10,
        QColor("#6FF1DF"),
        true
    );

    // =====================================================
    // INDICADOR DE ROTAÇÃO
    // =====================================================

    if (!spriteFrames_.isEmpty()) {

        const int angle =
            currentFrame_ * 360 / spriteFrames_.size();

        roundedRect(
            p,
            QRectF(width() - 83, 14, 69, 28),
            14,
            QColor(20, 64, 78, 220)
        );

        text(
            p,
            QRectF(width() - 77, 14, 57, 28),
            QString("%1°").arg(angle),
            10,
            QColor("#6FF1DF"),
            true,
            Qt::AlignCenter
        );
    }

    // =====================================================
    // INSTRUÇÃO
    // =====================================================

    const QString instruction = !spriteFrames_.isEmpty()
        ? QStringLiteral("ROTAÇÃO AUTOMÁTICA  •  ARRASTE PARA CONTROLAR")
        : QStringLiteral("VISUALIZAÇÃO 2D");

    text(
        p,
        QRectF(12, height() - 32, width() - 24, 23),
        instruction,
        10,
        QColor("#8CB0C5"),
        false,
        Qt::AlignCenter
    );

    return;
}

    // =========================================================
    // MODO 3D - RENDERIZAÇÃO ORIGINAL
    // =========================================================

    const QRectF viewport(0, 0, width(), height());

    QLinearGradient bg(0, 0, width(), height());
    bg.setColorAt(0, QColor("#10213C"));
    bg.setColorAt(.6, QColor("#0C182B"));
    bg.setColorAt(1, QColor("#071425"));

    p.fillRect(viewport, bg);

    Camera c(angle_, width(), height());

    p.setPen(QPen(QColor(71, 107, 138, 36), 1));

    for (double x = -5; x <= 5; x += .5)
        p.drawLine(c.project({x, -4, 0}), c.project({x, 4, 0}));

    for (double y = -4; y <= 4; y += .5)
        p.drawLine(c.project({-5, y, 0}), c.project({5, y, 0}));

    for (int k = 0; k < 5; k++) {
        QPolygonF sh;

        for (int i = 0; i <= 90; i++) {
            double a = i * 2 * 3.14159265358979323846 / 90.0;

            sh << c.project({
                2.53 * std::cos(a),
                1.28 * std::sin(a),
                .025 + k * .001
            });
        }

        p.setPen(QPen(QColor(20, 214, 208, 30 - k * 4), k == 0 ? 2 : 1));

        if (k == 4)
            p.setBrush(QColor(0, 0, 0, 62));
        else
            p.setBrush(Qt::NoBrush);

        p.drawPolygon(sh);
    }

    const BodySpec b = specFor(id_);

    const double hover = animations_
        ? 0.018 * std::sin(tick_ * 1.65)
        : 0.0;

    const double y = 0.83;
    const double z = 0.53 + hover;
    const double belt = b.shoulder + hover;

    QVector<Face> faces;
    faces.reserve(22);

    const QColor side = b.paint.darker(122);
    const QColor top = b.paint.lighter(116);
    const QColor dark = b.paint.darker(160);

    face(faces, {
        {b.tail, -y, z}, {b.nose, -y, z}, {b.nose, -y, belt - .24},
        {b.nose - .22, -y, belt - .14}, {.84, -y, belt},
        {b.tail + .14, -y, belt - .03}, {b.tail, -y, belt - .20}
    }, side);

    face(faces, {
        {b.nose, y, z}, {b.tail, y, z}, {b.tail, y, belt - .20},
        {b.tail + .14, y, belt - .03}, {.84, y, belt},
        {b.nose - .22, y, belt - .14}, {b.nose, y, belt - .24}
    }, side.darker(112));

    face(faces, {
        {b.nose, -y, z}, {b.nose, y, z},
        {b.nose, y, belt - .24}, {b.nose, -y, belt - .24}
    }, dark);

    face(faces, {
        {b.tail, y, z}, {b.tail, -y, z},
        {b.tail, -y, belt - .20}, {b.tail, y, belt - .20}
    }, dark);

    face(faces, {
        {b.tail + .14, -y, belt - .03}, {.84, -y, belt},
        {.84, y, belt}, {b.tail + .14, y, belt - .03}
    }, top);

    const double cf = .83;
    const double cr = b.hatch;
    const double rf = b.roofFront;
    const double rr = b.roofRear;

    const double zroof = b.roof + hover;
    const double wy = b.roofWidth;

    face(faces, {
        {cf, -y, belt}, {b.nose - .22, -y, belt - .14},
        {b.nose, -y, belt - .24}, {b.nose, y, belt - .24},
        {b.nose - .22, y, belt - .14}, {cf, y, belt}
    }, top);

    face(faces, {
        {b.tail, -y, belt - .20}, {cr, -y, belt},
        {cr, y, belt}, {b.tail, y, belt - .20}
    }, top);

    face(faces, {
        {rr, -wy, zroof}, {rf, -wy, zroof},
        {rf, wy, zroof}, {rr, wy, zroof}
    }, top.lighter(115));

    const QColor glass("#132C45");

    face(faces, {
        {cf, -y, belt}, {rf, -wy, zroof},
        {rf, wy, zroof}, {cf, y, belt}
    }, glass, true);

    face(faces, {
        {cr, y, belt}, {rr, wy, zroof},
        {rr, -wy, zroof}, {cr, -y, belt}
    }, glass, true);

    face(faces, {
        {cr, -y, belt}, {cf, -y, belt},
        {rf, -wy, zroof}, {rr, -wy, zroof}
    }, glass, true);

    face(faces, {
        {cf, y, belt}, {cr, y, belt},
        {rr, wy, zroof}, {rf, wy, zroof}
    }, glass, true);

    std::sort(faces.begin(), faces.end(), [&](const Face& a, const Face& bb) {
        auto avg = [](const Face& f) {
            V3 v{0, 0, 0};

            for (auto pt : f.v)
                v = v + pt;

            return v * (1.0 / f.v.size());
        };

        return c.depth(avg(a)) > c.depth(avg(bb));
    });

    const double sideFar = c.eye.y < 0 ? y : -y;
    const double sideNear = -sideFar;

    wheel(p, c, -1.37, sideFar + .018 * (sideFar > 0 ? 1 : -1), .49 + hover, .0);
    wheel(p, c, 1.40, sideFar + .018 * (sideFar > 0 ? 1 : -1), .49 + hover, .0);

    for (const Face& f : faces) {
        QPolygonF poly;

        for (auto v : f.v)
            poly << c.project(v);

        QColor col = shaded(f);
        const QRectF bounds = poly.boundingRect();

        QLinearGradient metal(bounds.topLeft(), bounds.bottomLeft());

        if (f.glass) {
            metal.setColorAt(0, QColor("#345D79"));
            metal.setColorAt(.32, QColor("#1B3A55"));
            metal.setColorAt(1, QColor("#091F36"));
        } else {
            metal.setColorAt(0, col.lighter(127));
            metal.setColorAt(.40, col);
            metal.setColorAt(1, col.darker(129));
        }

        p.setBrush(QBrush(metal));

        p.setPen(QPen(
            f.glass ? QColor(154, 225, 247, 130) : QColor(255, 255, 255, 49),
            f.glass ? 1.2 : 0.85
        ));

        p.drawPolygon(poly);
    }

    for (double yy : {-y, y}) {
        const double wy2 = yy < 0 ? -wy : wy;

        p.setPen(QPen(QColor("#A3D7E5"), 1.8));

        p.drawLine(
            c.project({(cf + cr) * .5, yy, belt}),
            c.project({(rf + rr) * .5, wy2, zroof})
        );

        p.setPen(QPen(QColor(235, 255, 255, 58), 1.2));

        p.drawLine(c.project({-.21, yy, belt - .09}), c.project({-.21, yy, .72 + hover}));
        p.drawLine(c.project({.94, yy, belt - .11}), c.project({.94, yy, .72 + hover}));
        p.drawLine(c.project({b.tail + .13, yy, belt - .22}), c.project({b.nose - .10, yy, belt - .22}));

        p.setPen(QPen(QColor(8, 25, 37, 150), 3.4, Qt::SolidLine, Qt::RoundCap));

        for (double xx : {-.63, .56}) {
            p.drawLine(
                c.project({xx - .11, yy + (yy > 0 ? .008 : -.008), belt - .17}),
                c.project({xx + .08, yy + (yy > 0 ? .008 : -.008), belt - .17})
            );
        }

        QPolygonF mirror;

        mirror << c.project({.77, yy, belt + .04})
               << c.project({.96, yy * 1.12, belt + .09})
               << c.project({.92, yy * 1.13, belt - .04})
               << c.project({.78, yy, belt - .03});

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#0A1F2F"));
        p.drawPolygon(mirror);
    }

    for (double yy : {-.59, .59}) {
        QPolygonF head;

        head << c.project({b.nose + .012, yy - .18, belt - .17})
             << c.project({b.nose + .012, yy + .15, belt - .17})
             << c.project({b.nose + .012, yy + .13, belt - .28})
             << c.project({b.nose + .012, yy - .17, belt - .28});

        p.setBrush(QColor("#DDFBFF"));
        p.setPen(QPen(QColor(106, 239, 249, 190), 2));
        p.drawPolygon(head);

        p.setPen(QPen(QColor(255, 78, 100, 230), 5, Qt::SolidLine, Qt::RoundCap));

        p.drawLine(
            c.project({b.tail - .012, yy - .13, belt - .12}),
            c.project({b.tail - .012, yy + .13, belt - .12})
        );
    }

    wheel(p, c, -1.37, sideNear + .02 * (sideNear > 0 ? 1 : -1), .49 + hover, .0);
    wheel(p, c, 1.40, sideNear + .02 * (sideNear > 0 ? 1 : -1), .49 + hover, .0);

    roundedRect(p, QRectF(12, 12, 150, 27), 13, QColor(20, 64, 78, 210));

    text(
        p,
        QRectF(23, 12, 135, 27),
        QStringLiteral("● MODELO 3D ORIGINAL"),
        10,
        QColor("#6FF1DF"),
        true
    );

    text(
        p,
        QRectF(12, height() - 31, width() - 24, 23),
        QStringLiteral("ARRASTE PARA GIRAR   /   DUPLO CLIQUE PARA CENTRALIZAR"),
        10,
        QColor("#8CB0C5"),
        false,
        Qt::AlignCenter
    );
}


EnergyFlow::EnergyFlow(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(88);
    timer_ = new QTimer(this);
    timer_->setInterval(30);
    connect(timer_, &QTimer::timeout, this, [this] { tick_ = (tick_ + 1) % 240; update(); });
    timer_->start();
}

void EnergyFlow::setData(double sourcePower, double batteryEnergy, double gridEnergy) {
    power_ = sourcePower; battery_ = batteryEnergy; grid_ = gridEnergy;
    update();
}

void EnergyFlow::setAnimationsEnabled(bool enabled) {
    animations_ = enabled;
    if (enabled) timer_->start(); else { timer_->stop(); tick_ = 0; }
    update();
}

void EnergyFlow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF left(18, 14, 56, 43);
    const QRectF right(width()-78, 14, 56, 43);
    p.setPen(QPen(QColor("#7CE8DD"), 2));
    p.setBrush(QColor("#1C4754"));
    p.drawRoundedRect(left, 9, 9);
    p.drawRoundedRect(right, 7, 7);
    p.setPen(QPen(QColor("#6EE9D8"), 3));
    p.drawLine(QPointF(left.center().x()-8,25), QPointF(left.center().x()+8,25));
    p.drawLine(QPointF(left.center().x(),18), QPointF(left.center().x(),34));
    p.drawLine(QPointF(left.center().x(),42), QPointF(left.center().x(),45));
    p.drawLine(QPointF(right.right()+1,25), QPointF(right.right()+5,25));
    p.setBrush(QColor("#14D8BD"));p.setPen(Qt::NoPen);
    const double chargeFrac = grid_ > 0.0 ? std::clamp(battery_/grid_, 0.0, 1.0) : 0.0;
    p.drawRoundedRect(QRectF(right.left()+5,right.top()+5,(right.width()-10)*chargeFrac,right.height()-10),4,4);
    const double start=left.right()+18, end=right.left()-18;
    p.setPen(QPen(QColor("#2D5764"),3,Qt::SolidLine,Qt::RoundCap));
    p.drawLine(QPointF(start,35),QPointF(end,35));
    if (end>start) {
        for(int i=0;i<5;++i) {
            const double f=animations_ ? std::fmod((tick_/240.0)+i/5.0,1.0) : (i+0.5)/5.0;
            p.setPen(Qt::NoPen);p.setBrush(QColor(77,243,219,static_cast<int>(90+165*f)));
            p.drawEllipse(QPointF(start+(end-start)*f,35),3.0+2*f,3.0+2*f);
        }
    }
    text(p,QRectF(0,60,93,23),QStringLiteral("CARREGADOR"),10,QColor("#A8CAD3"),true,Qt::AlignCenter);
    text(p,QRectF(width()-100,60,100,23),QStringLiteral("BATERIA"),10,QColor("#A8CAD3"),true,Qt::AlignCenter);
    text(p,QRectF(95,61,std::max(50,width()-195),22),QStringLiteral("%1 kW  |  transferência ilustrativa").arg(power_,0,'f',1),10,QColor("#82AEBB"),false,Qt::AlignCenter);
}

ProfileChart::ProfileChart(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(210);
    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, [this] {
        frame_ = std::min(100, frame_+5);
        update();
        if (frame_ >= 100) timer_->stop();
    });
}

void ProfileChart::setAnimationsEnabled(bool enabled) {
    animations_ = enabled;
    if (!enabled) { timer_->stop();frame_=100;update(); }
}

void ProfileChart::setData(const ChargeRequest& r, const ChargeResult& result) {
    const double oldPower = hasData_ ? result_.effectivePower.value() : -1.0;
    const bool changed = !hasData_ || std::abs(oldPower-result.effectivePower.value()) > 0.01 ||
        std::abs(request_.battery.targetSoc.value()-r.battery.targetSoc.value()) > 0.01;
    request_=r;result_=result;hasData_=true;
    if (changed && animations_) { frame_=0;timer_->start(); }
    else if (!animations_) { frame_=100; }
    update();
}

void ProfileChart::reset() { hasData_=false;timer_->stop();update(); }

void ProfileChart::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    QRectF plot(45,30,std::max(50,width()-65),std::max(70,height()-78));
    p.setPen(QPen(QColor("#25384F"),1));
    for(int i=0;i<=4;++i){const auto y=plot.bottom()-i*plot.height()/4.;p.drawLine(QPointF(plot.left(),y),QPointF(plot.right(),y));}
    if(!hasData_){placeholder(p,plot,QStringLiteral("Calcule uma recarga para ver o perfil de potência"));return;}
    const double soc0=request_.battery.currentSoc.value();
    const double soc1=request_.battery.targetSoc.value();
    const double bound=std::max(0.1,std::min({request_.charger.ratedPower.value(),request_.vehicleMaxPower.value(),
      request_.charger.installationLimit ? request_.charger.installationLimit->value() : 1.0e9}));
    const double maxW=std::max(1.,bound*1.24);
    auto powerAt=[&](double soc) {
        double output=bound;
        for(const auto& s:request_.curve.segments)if(soc>=s.fromSoc.value() && soc<s.toSoc.value())output=std::min(output,s.maxPower.value());
        return output;
    };
    QPainterPath line;QPainterPath fill;
    const int n=120;
    for(int i=0;i<=n;++i){const double soc=soc0+(soc1-soc0)*i/n;
        const QPointF pt(plot.left()+plot.width()*i/n,plot.bottom()-plot.height()*powerAt(soc)/maxW);
        if(i==0){line.moveTo(pt);fill.moveTo(plot.left(),plot.bottom());fill.lineTo(pt);}else{line.lineTo(pt);fill.lineTo(pt);} }
    fill.lineTo(plot.right(),plot.bottom());fill.closeSubpath();
    QLinearGradient grad(0,plot.top(),0,plot.bottom());grad.setColorAt(0,QColor(0,209,189,86));grad.setColorAt(1,QColor(0,201,186,4));
    p.save();
    p.setClipRect(QRectF(plot.left()-3,plot.top()-4,plot.width()*frame_/100.0+6,plot.height()+9));
    p.fillPath(fill,grad);p.setPen(QPen(QColor("#18DBCA"),3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.drawPath(line);
    p.setPen(Qt::NoPen);p.setBrush(QColor("#18DBCA"));
    if (frame_ == 100) p.drawEllipse(line.currentPosition(),4,4);
    p.restore();
    for(int i=0;i<=4;++i){double kw=maxW*i/4.;double y=plot.bottom()-plot.height()*i/4.;text(p,QRectF(0,y-11,40,21),QString::number(kw,'f',maxW < 4.0 ? 2 : (maxW < 20.0 ? 1 : 0)),10,QColor(soft),false,Qt::AlignRight|Qt::AlignVCenter);}
    for(int i=0;i<=4;++i){double soc=soc0+(soc1-soc0)*i/4.;double x=plot.left()+plot.width()*i/4.;text(p,QRectF(x-24,plot.bottom()+9,48,20),QString::number(soc,'f',0)+"%",10,QColor(soft),false,Qt::AlignCenter);}
    text(p,QRectF(plot.left(),0,plot.width(),24), request_.curve.enabled() ? QStringLiteral("Potência por faixa de carga") : QStringLiteral("Modelo de potência constante"),12,QColor("#A6C3D2"));
    text(p,QRectF(0,0,34,25),QStringLiteral("kW"),10,QColor(soft),true);
}

CostChart::CostChart(QWidget* parent):QWidget(parent){setMinimumHeight(210);}
void CostChart::setData(const ChargeResult& result){result_=result;hasData_=true;update();}
void CostChart::reset(){hasData_=false;update();}
void CostChart::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    if(!hasData_){placeholder(p,rect(),QStringLiteral("A composição do custo aparecerá aqui"));return;}
    const double vals[3]={result_.cost.energy.value(),result_.cost.time.value(),result_.cost.fixed.value()};
    const char* labels[3]={"Energia","Tarifa por tempo","Taxa fixa"};
    const QColor colors[3]={QColor("#12D9C8"),QColor("#7C9BFA"),QColor("#F8B86B")};
    const double top=24,left=15,barWidth=std::max(100,width()-155);
    const double largest=std::max({0.01,vals[0],vals[1],vals[2]});
    for(int i=0;i<3;++i){double yy=top+i*61;
        text(p,QRectF(left,yy,barWidth,20),QString::fromUtf8(labels[i]),12,QColor(navy),true);
        text(p,QRectF(left+barWidth,yy,125,20),QStringLiteral("R$ %1").arg(vals[i],0,'f',2),12,QColor(soft),false,Qt::AlignRight|Qt::AlignVCenter);
        roundedRect(p,QRectF(left,yy+25,barWidth+125,9),5,QColor("#25384F"));
        if(vals[i]>0)roundedRect(p,QRectF(left,yy+25,(barWidth+125)*(vals[i]/largest),9),5,colors[i]);
    }
}

ComparisonChart::ComparisonChart(QWidget* parent):QWidget(parent){setMinimumHeight(180);}
void ComparisonChart::setData(const QVector<QString>& labels,const QVector<double>& hours,const QVector<double>& costs){labels_=labels;hours_=hours;costs_=costs;update();}
void ComparisonChart::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    if(labels_.isEmpty()){placeholder(p,rect(),QStringLiteral("Adicione cenários para comparar tempo e custo"));return;}
    const auto n=std::min<int>(5, static_cast<int>(labels_.size()));
    const double maxT=std::max(0.001,*std::max_element(hours_.begin(),hours_.end()));
    const double maxC=std::max(0.001,*std::max_element(costs_.begin(),costs_.end()));
    double rowH=std::min(52.0,(height()-30.0)/n);
    for(int i=0;i<n;++i){double y=15+i*rowH;
        text(p,QRectF(8,y,128,16),labels_[i],11,QColor(navy),true);
        const double baseX=145, avail=std::max(60,width()-250);
        roundedRect(p,QRectF(baseX,y+2,avail,9),4,QColor("#27364C"));
        roundedRect(p,QRectF(baseX,y+2,avail*hours_[i]/maxT,9),4,QColor("#0DB8AD"));
        roundedRect(p,QRectF(baseX,y+16,avail,8),4,QColor("#27364C"));
        if(costs_[i]>0)roundedRect(p,QRectF(baseX,y+16,avail*costs_[i]/maxC,8),4,QColor("#5278DC"));
        text(p,QRectF(baseX+avail+8,y-1,95,16),QStringLiteral("%1 h").arg(hours_[i],0,'f',1),10,QColor(soft));
        text(p,QRectF(baseX+avail+8,y+14,95,16),QStringLiteral("R$ %1").arg(costs_[i],0,'f',2),10,QColor(soft));
    }
    text(p,QRectF(145,height()-19,width()-145,18),QStringLiteral("Verde: tempo    •    Azul: custo"),10,QColor(soft));
}


PremiumVisuals::PremiumVisuals(QWidget* parent)
    : QWidget(parent)
{
    // animations_ = QSettings(
    //     QStringLiteral("EVCharge"),
    //     QStringLiteral("Simulator")
    // ).value(
    //     QStringLiteral("animations"),
    //     true
    // ).toBool();

    animations_ = true;

    // =========================================================
    // Layout principal
    // =========================================================

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(14);

    // =========================================================
    // Painel principal
    // =========================================================

    auto* hero = new QFrame(this);
    hero->setObjectName("heroPanel");
    hero->setMinimumHeight(470);
    hero->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    auto* row = new QHBoxLayout(hero);
    row->setContentsMargins(24, 20, 22, 20);
    row->setSpacing(22);

    // =========================================================
    // Coluna esquerda: identificação + veículo
    // =========================================================

    auto* left = new QVBoxLayout;
    left->setSpacing(7);

    auto* eyebrow = new QLabel(
        QStringLiteral("EV CHARGE   /   ESTÚDIO AUTOMOTIVO"),
        hero
    );
    eyebrow->setObjectName("heroEyebrow");
    left->addWidget(eyebrow);

    vehicleLabel_ = new QLabel(vehicleName_, hero);
    vehicleLabel_->setObjectName("heroName");
    left->addWidget(vehicleLabel_);

    statusLabel_ = new QLabel(
        QStringLiteral(
            "Visualização 360° do veículo. Arraste para controlar a rotação."
        ),
        hero
    );
    statusLabel_->setObjectName("heroNote");
    statusLabel_->setWordWrap(true);
    left->addWidget(statusLabel_);

    // =========================================================
    // Visualização do veículo
    // =========================================================

    artwork_ = new VehicleArtwork(hero);

    artwork_->setMinimumSize(400, 300);
    artwork_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );


// Localização e validação dos sprites
// const QString applicationPath =
//     QCoreApplication::applicationDirPath();

// const QString workingPath =
//     QDir::currentPath();

// const QStringList candidates = {
//     QDir(applicationPath).filePath("assets/BYD_Dolphin"),
//     QDir(workingPath).filePath("apps/desktop/assets/BYD_Dolphin"),
//     QDir(workingPath).filePath("assets/BYD_Dolphin"),
//     QDir(workingPath).filePath("../apps/desktop/assets/BYD_Dolphin"),
//     QDir(workingPath).filePath("../../apps/desktop/assets/BYD_Dolphin")
// };

// QString assetsPath;

// qDebug() << "\n========== DIAGNÓSTICO DOS SPRITES ==========";
// qDebug() << "Executável:" << applicationPath;
// qDebug() << "Working directory:" << workingPath;

// for (const QString& candidate : candidates) {
//     QDir dir(candidate);

//     const QStringList frames =
//         dir.entryList(
//             QStringList() << "frame_*.png",
//             QDir::Files,
//             QDir::Name
//         );

//     qDebug() << "\nPasta:" << dir.absolutePath();
//     qDebug() << "Existe:" << dir.exists();
//     qDebug() << "Quantidade de PNGs:" << frames.size();

//     if (frames.size() == 26 &&
//         frames.contains("frame_000.png") &&
//         frames.contains("frame_025.png")) {

//         assetsPath = dir.absolutePath();
//         break;
//     }
// }

// if (!assetsPath.isEmpty()) {
//     qDebug() << "\nPasta válida encontrada:" << assetsPath;

//     artwork_->setSpriteFrames(assetsPath, 26);
// } else {
//     qWarning() << "\nERRO: Não foi encontrada uma pasta"
//                   " contendo os 20 sprites!";
// }

// qDebug() << "============================================\n";

    left->addWidget(artwork_, 1);

    auto* renderNote = new QLabel(
        QStringLiteral(
            "Visualização ilustrativa baseada em imagens 2D. "
            "Não representa um modelo CAD oficial do fabricante."
        ),
        hero
    );
    renderNote->setObjectName("renderNote");
    renderNote->setWordWrap(true);
    left->addWidget(renderNote);

    // =========================================================
    // Coluna direita: indicadores de carga
    // =========================================================

    auto* right = new QVBoxLayout;
    right->setSpacing(10);

    auto* socLabel = new QLabel(
        QStringLiteral("ESTADO DE CARGA  /  SoC"),
        hero
    );
    socLabel->setObjectName("heroEyebrow");
    socLabel->setAlignment(Qt::AlignCenter);

    right->addWidget(socLabel);

    socDial_ = new SocDial(hero);
    right->addWidget(socDial_, 0, Qt::AlignCenter);

    auto* flowLabel = new QLabel(
        QStringLiteral("CARREGADOR   →   BATERIA"),
        hero
    );
    flowLabel->setStyleSheet(
        QStringLiteral(
            "color:#83B1C3;"
            "font-size:10px;"
            "font-weight:700;"
        )
    );
    flowLabel->setAlignment(Qt::AlignCenter);

    right->addWidget(flowLabel);

    energyFlow_ = new EnergyFlow(hero);
    energyFlow_->setMinimumWidth(180);
    energyFlow_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    right->addWidget(energyFlow_);

    // =========================================================
    // Métricas
    // =========================================================

    auto addMetric = [&](const QString& title, QLabel*& value) {
        auto* item = new QHBoxLayout;
        item->setSpacing(8);

        auto* key = new QLabel(title, hero);
        key->setStyleSheet(
            QStringLiteral(
                "color:#9CBACB;"
                "font-size:11px;"
            )
        );
        key->setWordWrap(true);

        value = new QLabel(QStringLiteral("—"), hero);
        value->setStyleSheet(
            QStringLiteral(
                "color:#F2FAFF;"
                "font-size:13px;"
                "font-weight:700;"
            )
        );
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        item->addWidget(key, 1);
        item->addWidget(value, 0);

        right->addLayout(item);
    };

    addMetric(
        QStringLiteral("Energia para bateria"),
        batteryEnergy_
    );

    addMetric(
        QStringLiteral("Energia da rede"),
        gridEnergy_
    );

    addMetric(
        QStringLiteral("Potência efetiva"),
        effectivePower_
    );

    right->addStretch(1);

    // =========================================================
    // Distribuição das colunas
    // =========================================================

    row->addLayout(left, 7);
    row->addLayout(right, 3);

    outer->addWidget(hero);

    // =========================================================
    // Gráficos
    // =========================================================

    auto* charts = new QHBoxLayout;
    charts->setSpacing(14);

    auto createPanel = [&](const QString& heading, QWidget* plot) {
        auto* frame = new QFrame(this);
        frame->setObjectName("chartPanel");
        frame->setMinimumHeight(220);
        frame->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Expanding
        );

        auto* col = new QVBoxLayout(frame);
        col->setContentsMargins(18, 16, 18, 14);
        col->setSpacing(8);

        auto* title = new QLabel(heading, frame);
        title->setObjectName("chartTitle");

        col->addWidget(title);
        col->addWidget(plot, 1);

        return frame;
    };

    powerChart_ = new ProfileChart(this);
    costChart_ = new CostChart(this);

    charts->addWidget(
        createPanel(
            QStringLiteral("PERFIL DE RECARGA  /  POTÊNCIA × SoC"),
            powerChart_
        ),
        3
    );

    charts->addWidget(
        createPanel(
            QStringLiteral("DISTRIBUIÇÃO DO CUSTO"),
            costChart_
        ),
        2
    );

    outer->addLayout(charts, 1);

    // =========================================================
    // Estado das animações
    // =========================================================

    setAnimationsEnabled(animations_);
}

QString PremiumVisuals::getVehicleAssetsPath(
    const QString& folder,
    int expectedFrames) const
{
    const QString applicationPath =
        QCoreApplication::applicationDirPath();

    const QString workingPath =
        QDir::currentPath();

    const QString relativePath =
        QStringLiteral("assets/") + folder;

    const QStringList candidates = {
        QDir(applicationPath).filePath(relativePath),

        QDir(workingPath).filePath("apps/desktop/" + relativePath),
        QDir(workingPath).filePath(relativePath),
        QDir(workingPath).filePath("../apps/desktop/" + relativePath),
        QDir(workingPath).filePath("../../apps/desktop/" + relativePath)
    };

    for (const QString& candidate : candidates) {
        QDir dir(candidate);

        if (!dir.exists()) {
            continue;
        }

        const QStringList frames = dir.entryList(
            QStringList() << "frame_*.png",
            QDir::Files,
            QDir::Name
        );

        if (frames.size() != expectedFrames) {
            continue;
        }

        const QString firstFrame =
            QStringLiteral("frame_000.png");

        const QString lastFrame =
            QStringLiteral("frame_%1.png")
                .arg(expectedFrames - 1, 3, 10, QChar('0'));

        if (!frames.contains(firstFrame) ||
            !frames.contains(lastFrame)) {
            continue;
        }

        qDebug() << "[PremiumVisuals] Assets encontrados:"
                 << dir.absolutePath();

        return dir.absolutePath();
    }

    qWarning() << "[PremiumVisuals] Assets não encontrados:"
               << folder;

    return {};
}

void PremiumVisuals::setVehicle(
    const QString& name,
    const QString& id)
{
    if (!artwork_) {
        return;
    }

    qDebug() << "[PremiumVisuals] Selecionando veículo:"
             << name << id;

    vehicleName_ = name;
    vehicleId_ = id;

    vehicleLabel_->setText(
        name.isEmpty()
            ? QStringLiteral("Selecione um veículo")
            : name
    );

    // Atualiza os dados internos do veículo.
    artwork_->setVehicle(name, id);

    // Configuração visual padrão.
    artwork_->setRenderMode(
        VehicleArtwork::RenderMode::Model3D
    );

    if (id.isEmpty()) {
        return;
    }

    // Procura a configuração pelo ID.
    const auto& configs = vehicleVisualConfigs();

    const auto it = configs.constFind(id.toLower());

    if (it == configs.cend()) {
        qDebug() << "[PremiumVisuals] Utilizando modelo 3D para:" << id;
        return;
    }

    const VehicleVisualConfig& config = it.value();

    // Modelo 3D ou imagem estática não precisam carregar sprites.
    if (config.renderMode != VehicleArtwork::RenderMode::Image2D) {
        artwork_->setRenderMode(config.renderMode);
        return;
    }

    // Localiza os assets.
    const QString assetsPath = getVehicleAssetsPath(
        config.assetFolder,
        config.frameCount
    );

    if (assetsPath.isEmpty()) {
        qWarning() << "[PremiumVisuals] Assets não encontrados para:"
                   << id;
        return;
    }

    qDebug() << "[PremiumVisuals] Carregando sprites:"
             << id << assetsPath;

    artwork_->setSpriteFrames(
        assetsPath,
        config.frameCount
    );
}

void PremiumVisuals::setAnimationsEnabled(bool enabled)
{
    animations_ = enabled;

    if (artwork_) {
        artwork_->setAnimationsEnabled(enabled);
    }

    if (socDial_) {
        socDial_->setAnimationsEnabled(enabled);
    }

    if (energyFlow_) {
        energyFlow_->setAnimationsEnabled(enabled);
    }

    QSettings settings(
        QStringLiteral("EVCharge"),
        QStringLiteral("Simulator")
    );

    settings.setValue(
        QStringLiteral("animations"),
        enabled
    );
}

void PremiumVisuals::setSimulation(const ChargeRequest& r,const ChargeResult& result){
    setVehicle(QString::fromStdString(r.vehicle.name),QString::fromStdString(r.vehicle.id));
    socDial_->setSoc(r.battery.currentSoc.value(),r.battery.targetSoc.value());
    statusLabel_->setText(QStringLiteral("%1 kWh úteis   /   %2 kW efetivos   /   Recarga %3")
        .arg(r.battery.usableCapacity.value(),0,'f',1).arg(result.effectivePower.value(),0,'f',1)
        .arg(r.charger.type==ev::domain::ChargerType::AC?QStringLiteral("AC"):QStringLiteral("DC")));
    energyFlow_->setData(result.effectivePower.value(),result.batteryEnergy.value(),result.gridEnergy.value());
    batteryEnergy_->setText(QStringLiteral("%1 kWh").arg(result.batteryEnergy.value(),0,'f',2));
    gridEnergy_->setText(QStringLiteral("%1 kWh").arg(result.gridEnergy.value(),0,'f',2));
    effectivePower_->setText(QStringLiteral("%1 kW").arg(result.effectivePower.value(),0,'f',2));
    powerChart_->setData(r,result);costChart_->setData(result);
}

void PremiumVisuals::reset(){
    powerChart_->reset();costChart_->reset();
    batteryEnergy_->setText(QStringLiteral("—"));gridEnergy_->setText(QStringLiteral("—"));effectivePower_->setText(QStringLiteral("—"));
}

void VehicleArtwork::setVehicleImage(const QString& imagePath)
{
    QPixmap image;

    if (!image.load(imagePath)) {
        qWarning() << "Não foi possível carregar a imagem:" << imagePath;
        return;
    }

    vehicleImage_ = image;
    update();
}

void VehicleArtwork::setRenderMode(RenderMode mode)
{
    if (mode == RenderMode::Image2D && vehicleImage_.isNull()) {
        qWarning() << "Não é possível ativar o modo 2D: imagem não carregada.";
        return;
    }

    renderMode_ = mode;
    update();
}

void VehicleArtwork::paintImage2D(QPainter& p)
{
    const QRectF area = rect();

    // Fundo
    QLinearGradient background(area.topLeft(), area.bottomRight());
    background.setColorAt(0.0, QColor("#111C2D"));
    background.setColorAt(1.0, QColor("#0B1220"));

    p.fillRect(area, background);

    // Grid discreto
    p.save();
    p.setPen(QPen(QColor(255, 255, 255, 12), 1));

    constexpr int gridSize = 30;

    for (int x = 0; x < width(); x += gridSize)
        p.drawLine(x, 0, x, height());

    for (int y = 0; y < height(); y += gridSize)
        p.drawLine(0, y, width(), y);

    p.restore();

    // Movimento vertical
    const double hover = animations_
        ? 5.0 * std::sin(tick_ * 1.65)
        : 0.0;

    // Sombra
    const QPointF center(width() * 0.5, height() * 0.70);

    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 75));
    p.drawEllipse(
        QRectF(center.x() - width() * 0.25,
               center.y() - 9,
               width() * 0.50,
               18)
    );
    p.restore();

    // Imagem proporcional
    const QSizeF availableSize(
        width() * 0.82,
        height() * 0.68
    );

    QSize imageSize = vehicleImage_.size();
    imageSize.scale(
        availableSize.toSize(),
        Qt::KeepAspectRatio
    );

    const QRectF imageRect(
        (width() - imageSize.width()) / 2.0,
        height() * 0.08 + hover,
        imageSize.width(),
        imageSize.height()
    );  

    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(imageRect, vehicleImage_, vehicleImage_.rect());
    p.restore();

    // Identificação do modo
    p.setPen(QColor("#22C7B8"));
    p.setFont(QFont("Arial", 8, QFont::Bold));
    p.drawText(
        QRectF(12, 12, width() - 24, 20),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("MODELO 2D PNG")
    );
}


void VehicleArtwork::setSpriteFrames(
    const QString& directory,
    int frameCount)
{
    qDebug() << "\n========== CARREGAMENTO DOS SPRITES ==========";
    qDebug() << "[VehicleArtwork] Diretório:" << QDir(directory).absolutePath();
    qDebug() << "[VehicleArtwork] Frames esperados:" << frameCount;

    if (frameCount <= 0) {
        qWarning() << "[VehicleArtwork] Quantidade de frames inválida.";
        return;
    }

    QDir dir(directory);

    if (!dir.exists()) {
        qWarning() << "[VehicleArtwork] Diretório não encontrado:"
                   << dir.absolutePath();
        return;
    }

    QVector<QPixmap> loadedFrames;
    loadedFrames.reserve(frameCount);

    QSize largestSize;

    for (int i = 0; i < frameCount; ++i) {

        const QString filename = dir.filePath(
            QStringLiteral("frame_%1.png")
                .arg(i, 3, 10, QChar('0'))
        );

        QPixmap frame;

        if (!frame.load(filename) || frame.isNull()) {
            qWarning() << "[VehicleArtwork] Falha ao carregar:"
                       << filename;
            return;
        }

        largestSize.setWidth(
            qMax(largestSize.width(), frame.width())
        );

        largestSize.setHeight(
            qMax(largestSize.height(), frame.height())
        );

        qDebug() << "[VehicleArtwork] Frame:" << i
                 << "| Dimensão:" << frame.size()
                 << "| Bytes:" << QFileInfo(filename).size();

        loadedFrames.append(std::move(frame));
    }

    // Só substitui os frames atuais depois que todos forem carregados.
    spriteFrames_ = std::move(loadedFrames);

    currentFrame_ = 0;
    spriteAnimationCounter_ = 0;
    renderMode_ = RenderMode::Image2D;

    qDebug() << "[VehicleArtwork] Carregamento concluído.";
    qDebug() << "[VehicleArtwork] Total:" << spriteFrames_.size();
    qDebug() << "[VehicleArtwork] Maior dimensão:" << largestSize;
    qDebug() << "==============================================\n";

    update();
}
