// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageManagerDialog.h — Dialog for managing pages and page properties.

#ifndef VT_PAGE_MANAGER_DIALOG_H
#define VT_PAGE_MANAGER_DIALOG_H

#include "../layout/LayoutEngine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace vt {

/// Modal dialog with a split layout: page list on the left, page properties
/// on the right.  Lets the user create, rename, delete, navigate, and
/// configure inheritance for pages.
class PageManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PageManagerDialog(LayoutEngine *engine, QWidget *parent = nullptr);

    /// The page the user selected to navigate to (empty if cancelled).
    QString selectedPage() const { return m_selectedPage; }

private slots:
    void onNewPage();
    void onRenamePage();
    void onDeletePage();
    void onGoToPage();
    void onPageSelectionChanged();
    void onApplyProperties();

private:
    void refreshList();
    void loadPageProperties(const QString &pageName);

    LayoutEngine *m_engine;

    // Left panel — page list
    QListWidget  *m_list          = nullptr;
    QPushButton  *m_btnNew        = nullptr;
    QPushButton  *m_btnRename     = nullptr;
    QPushButton  *m_btnDelete     = nullptr;
    QPushButton  *m_btnGoTo       = nullptr;

    // Right panel — page properties
    QGroupBox    *m_propsGroup    = nullptr;
    QLineEdit    *m_propNameEdit  = nullptr;
    QCheckBox    *m_propSystemChk = nullptr;
    QComboBox    *m_propInheritCombo = nullptr;
    QPushButton  *m_propApplyBtn  = nullptr;

    QString       m_selectedPage;
    QString       m_editingPage;   // page currently shown in properties
};

} // namespace vt

#endif // VT_PAGE_MANAGER_DIALOG_H
