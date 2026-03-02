// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageManagerDialog.cpp

#include "PageManagerDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QFont>

namespace vt {

PageManagerDialog::PageManagerDialog(LayoutEngine *engine, QWidget *parent)
    : QDialog(parent)
    , m_engine(engine)
{
    // Force a proper top-level window with title bar so the dialog is
    // movable on every platform — even Wayland with a frameless parent.
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
                   | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(QStringLiteral("Page Manager"));
    setMinimumSize(700, 420);

    auto *mainLayout = new QVBoxLayout(this);

    // ── Splitter: left (page list) | right (properties) ─────────────────
    auto *splitter = new QSplitter(Qt::Horizontal);

    // ── Left panel — page list + action buttons ─────────────────────────
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto *header = new QLabel(QStringLiteral("Pages"));
    QFont hFont = header->font();
    hFont.setPointSize(14);
    hFont.setBold(true);
    header->setFont(hFont);
    leftLayout->addWidget(header);

    m_list = new QListWidget;
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setStyleSheet(
        "QListWidget { font-size: 14px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #0078D4; color: white; }"
    );
    connect(m_list, &QListWidget::currentItemChanged,
            this, [this]() { onPageSelectionChanged(); });
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *item) {
                if (!item) return;
                m_selectedPage = item->data(Qt::UserRole).toString();
                m_engine->showPage(m_selectedPage);
                accept();
            });
    leftLayout->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout;
    m_btnNew    = new QPushButton(QStringLiteral("New"));
    m_btnRename = new QPushButton(QStringLiteral("Rename"));
    m_btnDelete = new QPushButton(QStringLiteral("Delete"));
    m_btnGoTo   = new QPushButton(QStringLiteral("Go To"));

    connect(m_btnNew,    &QPushButton::clicked, this, &PageManagerDialog::onNewPage);
    connect(m_btnRename, &QPushButton::clicked, this, &PageManagerDialog::onRenamePage);
    connect(m_btnDelete, &QPushButton::clicked, this, &PageManagerDialog::onDeletePage);
    connect(m_btnGoTo,   &QPushButton::clicked, this, &PageManagerDialog::onGoToPage);

    btnRow->addWidget(m_btnNew);
    btnRow->addWidget(m_btnRename);
    btnRow->addWidget(m_btnDelete);
    btnRow->addWidget(m_btnGoTo);
    leftLayout->addLayout(btnRow);

    splitter->addWidget(leftWidget);

    // ── Right panel — page properties ───────────────────────────────────
    m_propsGroup = new QGroupBox(QStringLiteral("Page Properties"));
    auto *propsLayout = new QFormLayout(m_propsGroup);

    m_propNameEdit = new QLineEdit;
    m_propNameEdit->setReadOnly(true);
    m_propNameEdit->setStyleSheet(QStringLiteral("color: #aaa;"));
    propsLayout->addRow(QStringLiteral("Name:"), m_propNameEdit);

    m_propSystemChk = new QCheckBox(QStringLiteral("System page"));
    m_propSystemChk->setToolTip(QStringLiteral("System pages are part of the POS flow (Login, Tables, Order)."));
    propsLayout->addRow(m_propSystemChk);

    m_propInheritCombo = new QComboBox;
    m_propInheritCombo->setToolTip(
        QStringLiteral("Inherit elements marked as 'inheritable' from another page."));
    propsLayout->addRow(QStringLiteral("Inherit from:"), m_propInheritCombo);

    m_propApplyBtn = new QPushButton(QStringLiteral("Apply"));
    connect(m_propApplyBtn, &QPushButton::clicked, this, &PageManagerDialog::onApplyProperties);
    propsLayout->addRow(m_propApplyBtn);

    m_propsGroup->setEnabled(false);  // disabled until a page is selected
    splitter->addWidget(m_propsGroup);

    // Set initial sizes (roughly 40% list, 60% properties)
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1);

    // ── Close button ────────────────────────────────────────────────────
    auto *closeBtn = new QPushButton(QStringLiteral("Close"));
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);

    refreshList();
}

// ── Slots ───────────────────────────────────────────────────────────────────

void PageManagerDialog::onNewPage()
{
    bool ok = false;
    QString name = QInputDialog::getText(this,
        QStringLiteral("New Page"),
        QStringLiteral("Page name:"),
        QLineEdit::Normal, QString(), &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    name = name.trimmed();

    if (m_engine->page(name)) {
        QMessageBox::warning(this, QStringLiteral("Duplicate"),
            QStringLiteral("A page named \"%1\" already exists.").arg(name));
        return;
    }

    m_engine->createPage(name);
    refreshList();

    // Select the newly created page
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toString() == name) {
            m_list->setCurrentRow(i);
            break;
        }
    }
}

void PageManagerDialog::onRenamePage()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    QString oldName = item->data(Qt::UserRole).toString();

    bool ok = false;
    QString newName = QInputDialog::getText(this,
        QStringLiteral("Rename Page"),
        QStringLiteral("New name for \"%1\":").arg(oldName),
        QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.trimmed().isEmpty())
        return;

    newName = newName.trimmed();
    if (newName == oldName) return;

    if (m_engine->page(newName)) {
        QMessageBox::warning(this, QStringLiteral("Duplicate"),
            QStringLiteral("A page named \"%1\" already exists.").arg(newName));
        return;
    }

    if (!m_engine->renamePage(oldName, newName)) {
        QMessageBox::warning(this, QStringLiteral("Rename Failed"),
            QStringLiteral("Could not rename the page."));
        return;
    }

    // Update any pages that inherit from the old name
    for (const QString &pn : m_engine->pageNames()) {
        auto *pg = m_engine->page(pn);
        if (pg && pg->inheritFrom() == oldName)
            pg->setInheritFrom(newName);
    }

    if (m_selectedPage == oldName)
        m_selectedPage = newName;
    if (m_editingPage == oldName)
        m_editingPage = newName;

    refreshList();
}

void PageManagerDialog::onDeletePage()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();

    QStringList pages = m_engine->pageNames();
    if (pages.size() <= 1) {
        QMessageBox::warning(this, QStringLiteral("Cannot Delete"),
            QStringLiteral("You must have at least one page."));
        return;
    }

    int ret = QMessageBox::question(this,
        QStringLiteral("Delete Page"),
        QStringLiteral("Delete page \"%1\" and all its elements?").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes)
        return;

    // If deleting the active page, switch to another first
    if (m_engine->activePageName() == name) {
        for (const QString &other : pages) {
            if (other != name) {
                m_engine->showPage(other);
                m_selectedPage = other;
                break;
            }
        }
    }

    // Clear inheritance references to the deleted page
    for (const QString &pn : m_engine->pageNames()) {
        auto *pg = m_engine->page(pn);
        if (pg && pg->inheritFrom() == name)
            pg->setInheritFrom(QString());
    }

    m_engine->removePage(name);
    refreshList();
}

void PageManagerDialog::onGoToPage()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    m_selectedPage = item->data(Qt::UserRole).toString();
    m_engine->showPage(m_selectedPage);
    accept();
}

void PageManagerDialog::onPageSelectionChanged()
{
    auto *item = m_list->currentItem();
    if (!item) {
        m_propsGroup->setEnabled(false);
        m_editingPage.clear();
        return;
    }

    QString pageName = item->data(Qt::UserRole).toString();
    loadPageProperties(pageName);
}

void PageManagerDialog::onApplyProperties()
{
    if (m_editingPage.isEmpty()) return;

    auto *pg = m_engine->page(m_editingPage);
    if (!pg) return;

    pg->setSystemPage(m_propSystemChk->isChecked());

    // Set inheritance
    QString inherit = m_propInheritCombo->currentData().toString();
    pg->setInheritFrom(inherit);

    refreshList();
}

// ── Helpers ─────────────────────────────────────────────────────────────────

void PageManagerDialog::refreshList()
{
    // Remember current selection
    QString currentSel;
    if (auto *item = m_list->currentItem())
        currentSel = item->data(Qt::UserRole).toString();

    m_list->clear();

    QString activeName = m_engine->activePageName();
    QStringList pages  = m_engine->pageNames();
    pages.sort();

    int selectRow = -1;
    for (int i = 0; i < pages.size(); ++i) {
        const QString &name = pages[i];
        auto *pg = m_engine->page(name);
        int count = pg ? pg->elements().size() : 0;

        QString display = QStringLiteral("%1  (%2 elements)").arg(name).arg(count);
        if (name == activeName)
            display = QStringLiteral("▸ ") + display;
        if (pg && pg->isSystemPage())
            display += QStringLiteral("  [system]");

        auto *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, name);
        m_list->addItem(item);

        if (name == currentSel)
            selectRow = i;
        else if (selectRow < 0 && name == activeName)
            selectRow = i;
    }

    if (selectRow >= 0)
        m_list->setCurrentRow(selectRow);
}

void PageManagerDialog::loadPageProperties(const QString &pageName)
{
    m_editingPage = pageName;
    auto *pg = m_engine->page(pageName);
    if (!pg) {
        m_propsGroup->setEnabled(false);
        return;
    }

    m_propsGroup->setEnabled(true);
    m_propNameEdit->setText(pageName);
    m_propSystemChk->setChecked(pg->isSystemPage());

    // Build inherit combo — all pages except this one
    m_propInheritCombo->clear();
    m_propInheritCombo->addItem(QStringLiteral("(none)"), QString());

    QStringList pages = m_engine->pageNames();
    pages.sort();
    int currentIdx = 0;
    for (const QString &pn : pages) {
        if (pn == pageName) continue;  // can't inherit from self
        m_propInheritCombo->addItem(pn, pn);
        if (pn == pg->inheritFrom())
            currentIdx = m_propInheritCombo->count() - 1;
    }
    m_propInheritCombo->setCurrentIndex(currentIdx);
}

} // namespace vt
