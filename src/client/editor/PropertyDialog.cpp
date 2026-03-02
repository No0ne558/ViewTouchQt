// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.cpp

#include "PropertyDialog.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>

namespace vt {

PropertyDialog::PropertyDialog(UiElement *element, QWidget *parent)
    : QDialog(parent)
    , m_element(element)
    , m_bgColor(element->bgColor())
    , m_textColor(element->textColor())
{
    setWindowTitle(QStringLiteral("Element Properties — ") + element->elementId());
    setMinimumWidth(360);
    setupUi();
}

void PropertyDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // ── Identity group ──────────────────────────────────────────────────
    auto *idGroup = new QGroupBox(QStringLiteral("Identity"));
    auto *idForm  = new QFormLayout(idGroup);

    m_idEdit = new QLineEdit(m_element->elementId());
    m_idEdit->setReadOnly(true);  // ID is read-only for now
    m_idEdit->setStyleSheet(QStringLiteral("color: #aaa;"));
    idForm->addRow(QStringLiteral("ID:"), m_idEdit);

    QString typeStr;
    switch (m_element->elementType()) {
    case ElementType::Button: typeStr = QStringLiteral("Button"); break;
    case ElementType::Label:  typeStr = QStringLiteral("Label"); break;
    case ElementType::Panel:  typeStr = QStringLiteral("Panel"); break;
    }
    auto *typeLabel = new QLabel(typeStr);
    idForm->addRow(QStringLiteral("Type:"), typeLabel);

    mainLayout->addWidget(idGroup);

    // ── Text group ──────────────────────────────────────────────────────
    auto *textGroup = new QGroupBox(QStringLiteral("Text"));
    auto *textForm  = new QFormLayout(textGroup);

    m_labelEdit = new QLineEdit(m_element->label());
    textForm->addRow(QStringLiteral("Label:"), m_labelEdit);

    m_fontSizeBox = new QSpinBox;
    m_fontSizeBox->setRange(8, 120);
    m_fontSizeBox->setValue(m_element->fontSize());
    textForm->addRow(QStringLiteral("Font size:"), m_fontSizeBox);

    mainLayout->addWidget(textGroup);

    // ── Position / size group ───────────────────────────────────────────
    auto *geomGroup = new QGroupBox(QStringLiteral("Geometry"));
    auto *geomForm  = new QFormLayout(geomGroup);

    m_xBox = new QSpinBox;
    m_xBox->setRange(0, 1920);
    m_xBox->setValue(static_cast<int>(m_element->pos().x()));
    geomForm->addRow(QStringLiteral("X:"), m_xBox);

    m_yBox = new QSpinBox;
    m_yBox->setRange(0, 1080);
    m_yBox->setValue(static_cast<int>(m_element->pos().y()));
    geomForm->addRow(QStringLiteral("Y:"), m_yBox);

    m_wBox = new QSpinBox;
    m_wBox->setRange(40, 1920);
    m_wBox->setValue(static_cast<int>(m_element->elementW()));
    geomForm->addRow(QStringLiteral("Width:"), m_wBox);

    m_hBox = new QSpinBox;
    m_hBox->setRange(40, 1080);
    m_hBox->setValue(static_cast<int>(m_element->elementH()));
    geomForm->addRow(QStringLiteral("Height:"), m_hBox);

    mainLayout->addWidget(geomGroup);

    // ── Appearance group ────────────────────────────────────────────────
    auto *styleGroup = new QGroupBox(QStringLiteral("Appearance"));
    auto *styleForm  = new QFormLayout(styleGroup);

    m_bgColorBtn = new QPushButton;
    m_bgColorBtn->setFixedSize(60, 30);
    setButtonColor(m_bgColorBtn, m_bgColor);
    connect(m_bgColorBtn, &QPushButton::clicked, this, [this]() {
        chooseColor(m_bgColorBtn, m_bgColor);
    });
    styleForm->addRow(QStringLiteral("Background:"), m_bgColorBtn);

    m_textColorBtn = new QPushButton;
    m_textColorBtn->setFixedSize(60, 30);
    setButtonColor(m_textColorBtn, m_textColor);
    connect(m_textColorBtn, &QPushButton::clicked, this, [this]() {
        chooseColor(m_textColorBtn, m_textColor);
    });
    styleForm->addRow(QStringLiteral("Text colour:"), m_textColorBtn);

    m_radiusBox = new QSpinBox;
    m_radiusBox->setRange(0, 100);
    m_radiusBox->setValue(static_cast<int>(m_element->cornerRadius()));
    styleForm->addRow(QStringLiteral("Corner radius:"), m_radiusBox);

    mainLayout->addWidget(styleGroup);

    // ── Buttons ─────────────────────────────────────────────────────────
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &PropertyDialog::applyChanges);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void PropertyDialog::chooseColor(QPushButton *btn, QColor &target)
{
    QColor c = QColorDialog::getColor(target, this, QStringLiteral("Choose colour"));
    if (c.isValid()) {
        target = c;
        setButtonColor(btn, c);
    }
}

void PropertyDialog::setButtonColor(QPushButton *btn, const QColor &c)
{
    btn->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid #888;")
            .arg(c.name()));
}

void PropertyDialog::applyChanges()
{
    m_element->setLabel(m_labelEdit->text());
    m_element->setFontSize(m_fontSizeBox->value());
    m_element->moveTo(m_xBox->value(), m_yBox->value());
    m_element->resizeTo(m_wBox->value(), m_hBox->value());
    m_element->setBgColor(m_bgColor);
    m_element->setTextColor(m_textColor);
    m_element->setCornerRadius(m_radiusBox->value());

    accept();
}

} // namespace vt
