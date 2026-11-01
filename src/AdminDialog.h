#pragma once

#include "StationData.h"

#include <QDialog>
#include <QVector>

class QLabel;
class QLineEdit;
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
    explicit AdminDialog(const QVector<Station> &stations, QWidget *parent = nullptr);

    QVector<Station> stations() const { return m_stations; }

private slots:
    void onAdd();
    void onUpdate();
    void onRemove();
    void onMoveUp();
    void onMoveDown();
    void onResetDefaults();
    void onFormReturn();
    void onSave();

private:
    void rebuildTable(int selectRow = -1);
    void loadForm(int row);
    bool formToStation(Station *out, QString *err) const;
    bool codeTaken(const QString &code, int exceptRow) const;
    void flashError(const QString &msg);
    void updateButtons();
    void applyStyleSheet();

    QVector<Station> m_stations;
    QTableWidget *m_table = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_code = nullptr;
    QLineEdit *m_region = nullptr;
    QLineEdit *m_lat = nullptr;
    QLineEdit *m_lon = nullptr;
    QLabel *m_error = nullptr;
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
