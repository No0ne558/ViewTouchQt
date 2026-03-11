// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.cpp

#include "PropertyDialog.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QHBoxLayout>
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
    // Force a proper top-level window with title bar so the dialog is
    // movable on every platform — even Wayland with a frameless parent.
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
                   | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
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
    // Only allow Button type in the simplified editor.
    m_typeCombo->addItem(QStringLiteral("Button"), static_cast<int>(ElementType::Button));
    // Ensure combo index is valid (default to Button)
    int idx = m_typeCombo->findData(static_cast<int>(ElementType::Button));
    m_typeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyDialog::onTypeChanged);
    idForm->addRow(QStringLiteral("Type:"), m_typeCombo);

    m_inheritableChk = new QCheckBox(QStringLiteral("Inheritable"));
    m_inheritableChk->setToolTip(QStringLiteral("If checked, this element can be inherited by other pages."));
    m_inheritableChk->setChecked(m_element->isInheritable());
    idForm->addRow(m_inheritableChk);

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

    m_fontFamilyCombo = new QComboBox;
    for (const QString &fam : UiElement::availableFontFamilies())
        m_fontFamilyCombo->addItem(fam);
    // Set current family
    int famIdx = m_fontFamilyCombo->findText(m_element->fontFamily());
    if (famIdx >= 0)
        m_fontFamilyCombo->setCurrentIndex(famIdx);
    else {
        m_fontFamilyCombo->addItem(m_element->fontFamily());
        m_fontFamilyCombo->setCurrentIndex(m_fontFamilyCombo->count() - 1);
    }
    textForm->addRow(QStringLiteral("Font:"), m_fontFamilyCombo);

    m_fontBoldChk = new QCheckBox(QStringLiteral("Bold"));
    m_fontBoldChk->setChecked(m_element->fontBold());
    textForm->addRow(m_fontBoldChk);

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
        // Uncheck transparent when user picks a colour
        if (m_bgColor.alpha() == 255 && m_bgTransChk->isChecked())
            m_bgTransChk->setChecked(false);
    });

    m_bgTransChk = new QCheckBox(QStringLiteral("Transparent"));
    m_bgTransChk->setChecked(m_bgColor == Qt::transparent || m_bgColor.alpha() == 0);
    m_bgColorBtn->setEnabled(!m_bgTransChk->isChecked());
    connect(m_bgTransChk, &QCheckBox::toggled, this, [this](bool on) {
        m_bgColorBtn->setEnabled(!on);
        if (on) {
            m_bgColor = Qt::transparent;
            setButtonColor(m_bgColorBtn, m_bgColor);
        }
    });

    auto *bgRow = new QHBoxLayout;
    bgRow->addWidget(m_bgColorBtn);
    bgRow->addWidget(m_bgTransChk);
    bgRow->addStretch();
    styleForm->addRow(QStringLiteral("Background:"), bgRow);

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

    // Type-specific groups removed — editor only supports Button elements.

    // Only Button type supported; nothing to toggle.
    adjustSize();

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
    if (c.alpha() == 0) {
        // Checkerboard pattern signals "transparent"
        btn->setStyleSheet(QStringLiteral(
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            "stop:0 #ccc, stop:0.5 #ccc, stop:0.5 #888, stop:1 #888);"
            "border: 1px solid #888;"));
    } else {
        btn->setStyleSheet(
            QStringLiteral("background-color: %1; border: 1px solid #888;")
                .arg(c.name()));
    }
}

void PropertyDialog::applyChanges()
{
    // Store the new type from the combo box
    m_newType = static_cast<ElementType>(m_typeCombo->currentData().toInt());

    m_element->setLabel(m_labelEdit->text());
    m_element->setFontSize(m_fontSizeBox->value());
    m_element->setFontFamily(m_fontFamilyCombo->currentText());
    m_element->setFontBold(m_fontBoldChk->isChecked());
    m_element->moveTo(m_xBox->value(), m_yBox->value());
    m_element->resizeTo(m_wBox->value(), m_hBox->value());
    m_element->setBgColor(m_bgColor);
    m_element->setTextColor(m_textColor);
    m_element->setCornerRadius(m_radiusBox->value());
    m_element->setInheritable(m_inheritableChk->isChecked());

    // ── Type-specific properties (only apply if type NOT changing) ───────
    if (!typeChanged()) {
        // No additional type-specific properties to apply (only Button supported).
    }

    accept();
}

void PropertyDialog::onTypeChanged(int index)
{
    Q_UNUSED(index);
    // No-op: only Button type is supported in this simplified editor.
    adjustSize();
}

int PropertyDialog::actionTypeValue() const
{
    return m_actionTypeCombo ? m_actionTypeCombo->currentData().toInt() : 0;
}

bool PropertyDialog::inheritable() const
{
    return m_inheritableChk ? m_inheritableChk->isChecked() : false;
}

void PropertyDialog::setPageNames(const QStringList &names)
{
    Q_UNUSED(names);
    // No-op: navigation targets are only relevant for ActionButton types,
    // which are not supported in the simplified editor.
}

} // namespace vt
