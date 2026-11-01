#include "MainWindow.h"
#include "AdminDialog.h"
#include "AppIcon.h"
#include "StationRegistry.h"
#include "Theme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QGuiApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setFont(Theme::sans(12));
    QApplication::setApplicationName(QStringLiteral("360WeatherDash"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setWindowIcon(AppIcon::icon());
    // matches 360weatherdash.desktop on Wayland docks (app id → launcher entry)
    QGuiApplication::setDesktopFileName(QStringLiteral("360weatherdash"));

    QCommandLineParser cli;
    cli.setApplicationDescription(QStringLiteral("360WeatherDash — world weather operations console"));
    cli.addHelpOption();
    cli.addVersionOption();
    QCommandLineOption grabOpt(QStringLiteral("grab"),
                               QStringLiteral("Render one frame to FILE after the first scan, then exit."),
                               QStringLiteral("FILE"));
    QCommandLineOption grabAdminOpt(QStringLiteral("grab-admin"),
                               QStringLiteral("Open the site-admin dialog over the console, drive an add + save "
                                              "through its widgets, render PREFIX-admin.png / -admin-added.png / "
                                              "-main.png, then exit."),
                               QStringLiteral("PREFIX"));
    QCommandLineOption offlineOpt(QStringLiteral("offline"),
                                  QStringLiteral("Start with feed polling disabled (link-down drill)."));
    QCommandLineOption iconOpt(QStringLiteral("icon"),
                               QStringLiteral("Render the application icon (256 px) to FILE, then exit."),
                               QStringLiteral("FILE"));
    cli.addOption(grabOpt);
    cli.addOption(grabAdminOpt);
    cli.addOption(offlineOpt);
    cli.addOption(iconOpt);
    cli.process(app);

    if (cli.isSet(iconOpt))
        return AppIcon::pixmap(256).save(cli.value(iconOpt)) ? 0 : 1;

    MainWindow win(cli.isSet(offlineOpt));
    win.show();

    const QString grabPath = cli.value(grabOpt);
    if (!grabPath.isEmpty()) {
        // Render pass: wait for the first full scan (or 30 s), let two blink
        // frames elapse, then dump the window to the requested file.
        QTimer::singleShot(30000, &win, [&win, grabPath] {
            const QPixmap px = win.grab();
            px.save(grabPath);
            QApplication::exit(px.isNull() ? 1 : 0);
        });
        QTimer *poll = new QTimer(&win);
        QObject::connect(poll, &QTimer::timeout, &win, [&win, poll, grabPath] {
            if (win.firstScanDone()) {
                QTimer::singleShot(1600, &win, [&win, grabPath] {
                    const QPixmap px = win.grab();
                    px.save(grabPath);
                    QApplication::exit(px.isNull() ? 1 : 0);
                });
                poll->stop();
            }
        });
        poll->start(200);
    }

    const QString adminPrefix = cli.value(grabAdminOpt);
    if (!adminPrefix.isEmpty()) {
        // Exercise the site-admin dialog end to end: render it as opened,
        // drive a real add + save through its widgets (same slots the
        // buttons use), then render the reconfigured console.
        auto *dlg = new AdminDialog(win.registry()->stations(), &win);
        dlg->show();
        int rc = 0;
        const auto grabTo = [&rc](QWidget *w, const QString &path) {
            const QPixmap px = w->grab();
            if (px.isNull() || !px.save(path))
                rc = 1;
        };
        QTimer::singleShot(1200, &win, [&win, dlg, adminPrefix, &grabTo, &rc] {
            grabTo(dlg, adminPrefix + QStringLiteral("-admin.png"));
            const auto set = [dlg](const char *name, const QString &v) {
                if (auto *e = dlg->findChild<QLineEdit *>(QLatin1String(name)))
                    e->setText(v);
            };
            set("nameEdit", QStringLiteral("BERGEN"));
            set("codeEdit", QStringLiteral("ENBR"));
            set("regionEdit", QStringLiteral("N. EUROPE"));
            set("latEdit", QStringLiteral("60.29"));
            set("lonEdit", QStringLiteral("5.22"));
            if (auto *b = dlg->findChild<QPushButton *>(QStringLiteral("addButton")))
                b->click();
        });
        QTimer::singleShot(2400, &win, [&win, dlg, adminPrefix, &grabTo, &rc] {
            grabTo(dlg, adminPrefix + QStringLiteral("-admin-added.png"));
            if (auto *b = dlg->findChild<QPushButton *>(QStringLiteral("saveButton")))
                b->click();
            win.applyAdminRoster(dlg->stations());
        });
        QTimer::singleShot(3600, &win, [&win, adminPrefix, &grabTo, &rc] {
            grabTo(&win, adminPrefix + QStringLiteral("-main.png"));
            QApplication::exit(rc);
        });
    }

    return app.exec();
}
