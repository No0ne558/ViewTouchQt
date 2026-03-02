// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageManagerDialog.cpp

#include "PageManagerDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
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
    setMinimumSize(400, 350);

    auto *mainLayout = new QVBoxLayout(this);

    // ── Header ──────────────────────────────────────────────────────────
    auto *header = new QLabel(QStringLiteral("Pages"));
    QFont hFont = header->font();
    hFont.setPointSize(14);
    hFont.setBold(true);
    header->setFont(hFont);
    mainLayout->addWidget(header);

    // ── List + buttons ──────────────────────────────────────────────────
    auto *bodyLayout = new QHBoxLayout;

    m_list = new QListWidget;
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setStyleSheet(
        "QListWidget { font-size: 14px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #0078D4; color: white; }"
    );
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &PageManagerDialog::onItemDoubleClicked);
    bodyLayout->addWidget(m_list, 1);

    auto *btnLayout = new QVBoxLayout;
    m_btnNew    = new QPushButton(QStringLiteral("New Page"));
    m_btnRename = new QPushButton(QStringLiteral("Rename"));
    m_btnDelete = new QPushButton(QStringLiteral("Delete"));
    m_btnGoTo   = new QPushButton(QStringLiteral("Go To Page"));

    m_btnNew->setFixedWidth(120);
    m_btnRename->setFixedWidth(120);
    m_btnDelete->setFixedWidth(120);
    m_btnGoTo->setFixedWidth(120);

    connect(m_btnNew,    &QPushButton::clicked, this, &PageManagerDialog::onNewPage);
    connect(m_btnRename, &QPushButton::clicked, this, &PageManagerDialog::onRenamePage);
    connect(m_btnDelete, &QPushButton::clicked, this, &PageManagerDialog::onDeletePage);
    connect(m_btnGoTo,   &QPushButton::clicked, this, &PageManagerDialog::onGoToPage);

    btnLayout->addWidget(m_btnNew);
    btnLayout->addWidget(m_btnRename);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(m_btnGoTo);
    btnLayout->addStretch();
    bodyLayout->addLayout(btnLayout);

    mainLayout->addLayout(bodyLayout);

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

    // If the renamed page was selected as the go-to target, update it.
    if (m_selectedPage == oldName)
        m_selectedPage = newName;

    refreshList();
}

void PageManagerDialog::onDeletePage()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();

    // Cannot delete the active page if it's the only one
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

void PageManagerDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    m_selectedPage = item->data(Qt::UserRole).toString();
    m_engine->showPage(m_selectedPage);
    accept();
}

// ── Helpers ─────────────────────────────────────────────────────────────────

void PageManagerDialog::refreshList()
{
    m_list->clear();

    QString activeName = m_engine->activePageName();
    QStringList pages  = m_engine->pageNames();
    pages.sort();

    for (const QString &name : pages) {
        auto *pg = m_engine->page(name);
        int count = pg ? pg->elements().size() : 0;

        QString display = QStringLiteral("%1  (%2 elements)").arg(name).arg(count);
        if (name == activeName)
            display = QStringLiteral("▸ ") + display;

        auto *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, name);
        m_list->addItem(item);

        if (name == activeName)
            m_list->setCurrentItem(item);
    }
}

} // namespace vt
