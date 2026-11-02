#pragma once

#include "GeocodingService.h"
#include "StationData.h"

#include <QDialog>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;

// ----------------------------------------------------------------------------
// In-app roster editor, opened from the header "SITE ADMIN" button or Ctrl+E.
// Works on a copy of the watched-site list: roster table on the left, entry
// form on the right (add / update / remove), move up/down for grid order, and
// reset-to-defaults. Accepting returns the edited list; the caller persists
// it through StationRegistry. Entries are normalized (trimmed, upper-cased)
// and validated row-by-row; the table therefore only ever holds valid rows.
// ----------------------------------------------------------------------------
class AdminDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AdminDialog(const QVector<Station> &stations, bool offline = false,
                         QWidget *parent = nullptr);

    QVector<Station> stations() const { return m_stations; }

private slots:
    void onAdd();
    void onUpdate();
    void onRemove();
    void onMoveUp();
    void onMoveDown();
    void onResetDefaults();
    void onFormReturn();
    void onCitySearch();
    void onCityResults(const QVector<GeoResult> &results);
    void onCityError(const QString &msg);
    void onCityPicked(int row);
    void onSave();

private:
    void rebuildTable(int selectRow = -1);
    void loadForm(int row);
    bool formToStation(Station *out, QString *err) const;
    bool codeTaken(const QString &code, int exceptRow) const;
    void flashError(const QString &msg);
    void updateButtons();
    void applyStyleSheet();
    void setCityStatus(const QString &msg, bool isError);

    QVector<Station> m_stations;
    QTableWidget *m_table = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_code = nullptr;
    QLineEdit *m_region = nullptr;
    QLineEdit *m_lat = nullptr;
    QLineEdit *m_lon = nullptr;
    QLabel *m_error = nullptr;
    GeocodingService *m_geo = nullptr;
    QVector<GeoResult> m_cityResults;
    QLineEdit *m_citySearch = nullptr;
    QPushButton *m_cityBtn = nullptr;
    QLabel *m_cityStatus = nullptr;
    QListWidget *m_cityList = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_updateBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QPushButton *m_upBtn = nullptr;
    QPushButton *m_downBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
};
