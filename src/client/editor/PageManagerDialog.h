// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageManagerDialog.h — Dialog for creating, renaming, and deleting pages.

#ifndef VT_PAGE_MANAGER_DIALOG_H
#define VT_PAGE_MANAGER_DIALOG_H

#include "../layout/LayoutEngine.h"

#include <QDialog>
#include <QListWidget>
#include <QPushButton>

namespace vt {

/// Modal dialog that shows all pages and lets the user create, rename,
/// delete, or switch to any page.
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
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void refreshList();

    LayoutEngine *m_engine;
    QListWidget  *m_list          = nullptr;
    QPushButton  *m_btnNew        = nullptr;
    QPushButton  *m_btnRename     = nullptr;
    QPushButton  *m_btnDelete     = nullptr;
    QPushButton  *m_btnGoTo       = nullptr;
    QString       m_selectedPage;
};

} // namespace vt

#endif // VT_PAGE_MANAGER_DIALOG_H
