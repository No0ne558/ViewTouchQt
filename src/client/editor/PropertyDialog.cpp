// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.cpp

#include "PropertyDialog.h"
#include "../layout/PinEntryElement.h"
#include "../layout/KeypadButtonElement.h"
#include "../layout/ActionButtonElement.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>

namespace vt {

PropertyDialog::PropertyDialog(UiElement *element, QWidget *parent)
    : QDialog(parent)
    , m_element(element)
    , m_originalType(element->elementType())
    , m_newType(element->elementType())
    , m_bgColor(element->bgColor())
    , m_textColor(element->textColor())
{
    // Ensure the dialog gets its own window title bar so it can be moved,
    // even when the parent is a frameless fullscreen window.
    setWindowFlags(windowFlags() | Qt::Dialog | Qt::WindowTitleHint
                   | Qt::WindowCloseButtonHint);
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

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem(QStringLiteral("Button"),        static_cast<int>(ElementType::Button));
    m_typeCombo->addItem(QStringLiteral("Label"),         static_cast<int>(ElementType::Label));
    m_typeCombo->addItem(QStringLiteral("Panel"),         static_cast<int>(ElementType::Panel));
    m_typeCombo->addItem(QStringLiteral("PIN Entry"),     static_cast<int>(ElementType::PinEntry));
    m_typeCombo->addItem(QStringLiteral("Keypad Button"), static_cast<int>(ElementType::KeypadButton));
    m_typeCombo->addItem(QStringLiteral("Action Button"), static_cast<int>(ElementType::ActionButton));
    m_typeCombo->setCurrentIndex(static_cast<int>(m_element->elementType()));
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyDialog::onTypeChanged);
    idForm->addRow(QStringLiteral("Type:"), m_typeCombo);

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

    // ── Type-specific groups (always created, shown/hidden by type) ─────

    // PIN Entry group
    {
        m_pinGroup = new QGroupBox(QStringLiteral("PIN Entry"));
        auto *pinForm  = new QFormLayout(m_pinGroup);

        m_maskedCheck = new QCheckBox(QStringLiteral("Mask input (show dots)"));
        m_maskedCheck->setChecked(true);   // default
        pinForm->addRow(m_maskedCheck);

        m_maxLengthBox = new QSpinBox;
        m_maxLengthBox->setRange(0, 32);
        m_maxLengthBox->setValue(8);   // default
        m_maxLengthBox->setSpecialValueText(QStringLiteral("Unlimited"));
        pinForm->addRow(QStringLiteral("Max length:"), m_maxLengthBox);

        // If current element IS a PinEntry, load actual values
        if (m_element->elementType() == ElementType::PinEntry) {
            auto *pin = static_cast<PinEntryElement *>(m_element);
            m_maskedCheck->setChecked(pin->masked());
            m_maxLengthBox->setValue(pin->maxLength());
        }

        mainLayout->addWidget(m_pinGroup);
    }

    // Keypad Button group
    {
        m_kpdGroup = new QGroupBox(QStringLiteral("Keypad Button"));
        auto *kpdForm  = new QFormLayout(m_kpdGroup);

        m_keyValueEdit = new QLineEdit;
        m_keyValueEdit->setPlaceholderText(QStringLiteral("Uses label if empty. BACK or CLEAR for special."));

        // If current element IS a KeypadButton, load actual value
        if (m_element->elementType() == ElementType::KeypadButton) {
            auto *kpd = static_cast<KeypadButtonElement *>(m_element);
            m_keyValueEdit->setText(kpd->keyValue());
        }

        kpdForm->addRow(QStringLiteral("Key value:"), m_keyValueEdit);
        mainLayout->addWidget(m_kpdGroup);
    }

    // Action Button group
    {
        m_actGroup = new QGroupBox(QStringLiteral("Action Button"));
        auto *actForm  = new QFormLayout(m_actGroup);

        m_actionTypeCombo = new QComboBox;
        m_actionTypeCombo->addItem(QStringLiteral("Login (→ Tables)"), static_cast<int>(ActionType::Login));
        m_actionTypeCombo->addItem(QStringLiteral("Dine-In (→ Order)"), static_cast<int>(ActionType::DineIn));
        m_actionTypeCombo->addItem(QStringLiteral("To-Go (→ Order)"), static_cast<int>(ActionType::ToGo));

        // If current element IS an ActionButton, load actual value
        if (m_element->elementType() == ElementType::ActionButton) {
            auto *act = static_cast<ActionButtonElement *>(m_element);
            m_actionTypeCombo->setCurrentIndex(static_cast<int>(act->actionType()));
        }

        actForm->addRow(QStringLiteral("Action:"), m_actionTypeCombo);
        mainLayout->addWidget(m_actGroup);
    }

    // Show/hide the correct groups for the current type
    onTypeChanged(m_typeCombo->currentIndex());

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
    // Store the new type from the combo box
    m_newType = static_cast<ElementType>(m_typeCombo->currentData().toInt());

    m_element->setLabel(m_labelEdit->text());
    m_element->setFontSize(m_fontSizeBox->value());
    m_element->moveTo(m_xBox->value(), m_yBox->value());
    m_element->resizeTo(m_wBox->value(), m_hBox->value());
    m_element->setBgColor(m_bgColor);
    m_element->setTextColor(m_textColor);
    m_element->setCornerRadius(m_radiusBox->value());

    // ── Type-specific properties (only apply if type NOT changing) ───────
    if (!typeChanged()) {
        if (m_element->elementType() == ElementType::PinEntry) {
            auto *pin = static_cast<PinEntryElement *>(m_element);
            if (m_maskedCheck)
                pin->setMasked(m_maskedCheck->isChecked());
            if (m_maxLengthBox)
                pin->setMaxLength(m_maxLengthBox->value());
        }
        if (m_element->elementType() == ElementType::KeypadButton) {
            auto *kpd = static_cast<KeypadButtonElement *>(m_element);
            if (m_keyValueEdit)
                kpd->setKeyValue(m_keyValueEdit->text());
        }
        if (m_element->elementType() == ElementType::ActionButton) {
            auto *act = static_cast<ActionButtonElement *>(m_element);
            if (m_actionTypeCombo)
                act->setActionType(static_cast<ActionType>(m_actionTypeCombo->currentData().toInt()));
        }
    }

    accept();
}

void PropertyDialog::onTypeChanged(int index)
{
    Q_UNUSED(index);
    auto selectedType = static_cast<ElementType>(m_typeCombo->currentData().toInt());

    m_pinGroup->setVisible(selectedType == ElementType::PinEntry);
    m_kpdGroup->setVisible(selectedType == ElementType::KeypadButton);
    m_actGroup->setVisible(selectedType == ElementType::ActionButton);

    // Resize the dialog to fit the visible content
    adjustSize();
}

bool PropertyDialog::pinMasked() const
{
    return m_maskedCheck ? m_maskedCheck->isChecked() : true;
}

int PropertyDialog::pinMaxLength() const
{
    return m_maxLengthBox ? m_maxLengthBox->value() : 8;
}

QString PropertyDialog::keypadValue() const
{
    return m_keyValueEdit ? m_keyValueEdit->text() : QString();
}

int PropertyDialog::actionTypeValue() const
{
    return m_actionTypeCombo ? m_actionTypeCombo->currentData().toInt() : 0;
}

} // namespace vt
