// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.cpp

#include "PropertyDialog.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
// Image support
#include <QFileDialog>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include "../layout/ButtonElement.h"

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
    // Allow Button and Image types in the editor.
    m_typeCombo->addItem(QStringLiteral("Button"), static_cast<int>(ElementType::Button));
    m_typeCombo->addItem(QStringLiteral("Image"), static_cast<int>(ElementType::Image));
    // Ensure combo index reflects the element's current type
    int idx = m_typeCombo->findData(static_cast<int>(m_element->elementType()));
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
    m_textGroup = new QGroupBox(QStringLiteral("Text"));
    auto *textForm  = new QFormLayout(m_textGroup);

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

    mainLayout->addWidget(m_textGroup);

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

    m_edgeStyleCombo = new QComboBox;
    m_edgeStyleCombo->addItem(QStringLiteral("Flat"), static_cast<int>(UiElement::EdgeStyle::Flat));
    m_edgeStyleCombo->addItem(QStringLiteral("Raised"), static_cast<int>(UiElement::EdgeStyle::Raised));
    m_edgeStyleCombo->addItem(QStringLiteral("Raised (2)"), static_cast<int>(UiElement::EdgeStyle::Raised2));
    m_edgeStyleCombo->addItem(QStringLiteral("Raised (3)"), static_cast<int>(UiElement::EdgeStyle::Raised3));
    m_edgeStyleCombo->addItem(QStringLiteral("Inset"), static_cast<int>(UiElement::EdgeStyle::Inset));
    m_edgeStyleCombo->addItem(QStringLiteral("Inset (2)"), static_cast<int>(UiElement::EdgeStyle::Inset2));
    m_edgeStyleCombo->addItem(QStringLiteral("Inset (3)"), static_cast<int>(UiElement::EdgeStyle::Inset3));
    m_edgeStyleCombo->addItem(QStringLiteral("Double"), static_cast<int>(UiElement::EdgeStyle::Double));
    m_edgeStyleCombo->addItem(QStringLiteral("Border"), static_cast<int>(UiElement::EdgeStyle::Border));
    m_edgeStyleCombo->addItem(QStringLiteral("Outline"), static_cast<int>(UiElement::EdgeStyle::Outline));
    m_edgeStyleCombo->addItem(QStringLiteral("Rounded"), static_cast<int>(UiElement::EdgeStyle::Rounded));
    m_edgeStyleCombo->addItem(QStringLiteral("None"), static_cast<int>(UiElement::EdgeStyle::None));
    int esIdx = m_edgeStyleCombo->findData(static_cast<int>(m_element->edgeStyle()));
    if (esIdx >= 0) m_edgeStyleCombo->setCurrentIndex(esIdx);
    styleForm->addRow(QStringLiteral("Edge style:"), m_edgeStyleCombo);

    // Behaviour selector (Button only)
    m_behaviorCombo = new QComboBox;
    m_behaviorCombo->addItem(QStringLiteral("Blink"), static_cast<int>(UiElement::ButtonBehavior::Blink));
    m_behaviorCombo->addItem(QStringLiteral("Toggle"), static_cast<int>(UiElement::ButtonBehavior::Toggle));
    m_behaviorCombo->addItem(QStringLiteral("None"), static_cast<int>(UiElement::ButtonBehavior::None));
    m_behaviorCombo->addItem(QStringLiteral("Click Pass Through"), static_cast<int>(UiElement::ButtonBehavior::PassThrough));
    m_behaviorCombo->addItem(QStringLiteral("Double Tap"), static_cast<int>(UiElement::ButtonBehavior::DoubleTap));
    int behIdx = m_behaviorCombo->findData(static_cast<int>(m_element->behavior()));
    if (behIdx >= 0) m_behaviorCombo->setCurrentIndex(behIdx);
    styleForm->addRow(QStringLiteral("Behaviour:"), m_behaviorCombo);

    

    m_layerBox = new QSpinBox;
    m_layerBox->setRange(-10, 10);
    m_layerBox->setValue(m_element->layer());
    styleForm->addRow(QStringLiteral("Layer (-10..10):"), m_layerBox);

    // Image property (optional)
    m_imageCombo = new QComboBox;
    m_imagePreview = new QLabel;
    m_imagePreview->setFixedSize(48, 48);
    m_imagePreview->setFrameStyle(QFrame::Box | QFrame::Plain);
    m_imagePreview->setAlignment(Qt::AlignCenter);

    auto *imgRow = new QHBoxLayout;
    imgRow->addWidget(m_imageCombo);
    imgRow->addWidget(m_imagePreview);
    styleForm->addRow(QStringLiteral("Image:"), imgRow);

    // Image button toggle: when checked, the element is an Image Button (no text)
    m_imageOnlyChk = new QCheckBox(QStringLiteral("Image Button (no text)"));
    bool isImageOnly = false;
    if (auto *btn = qobject_cast<ButtonElement *>(m_element))
        isImageOnly = btn->imageOnly();
    m_imageOnlyChk->setChecked(isImageOnly);
    styleForm->addRow(m_imageOnlyChk);

    // Populate the image combo from the system image directory
    QByteArray env = qgetenv("VIEWTOUCH_IMG_DIR");
    QString sysDir = env.isEmpty() ? QStringLiteral("/opt/viewtouch/img") : QString::fromUtf8(env);
    QDir dir(sysDir);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.svg";
    // Add a None option first
    m_imageCombo->addItem(QStringLiteral("None"), QString());
    if (dir.exists()) {
        const QStringList entries = dir.entryList(filters, QDir::Files, QDir::Name);
        for (const QString &fn : entries) {
            m_imageCombo->addItem(fn, dir.filePath(fn));
        }
    }

    // Populate initial selection if element has an image set and exists in the dir
    if (auto *btn = qobject_cast<ButtonElement *>(m_element)) {
        QString cur = btn->imagePath();
        if (!cur.isEmpty()) {
            QString name = QFileInfo(cur).fileName();
            int idx = m_imageCombo->findText(name);
            if (idx >= 0) {
                m_imageCombo->setCurrentIndex(idx);
                QString full = m_imageCombo->itemData(idx).toString();
                QPixmap p(full);
                if (!p.isNull())
                    m_imagePreview->setPixmap(p.scaled(m_imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                // Try to match full path in data
                int didx = m_imageCombo->findData(cur);
                if (didx >= 0) {
                    m_imageCombo->setCurrentIndex(didx);
                    QString full = m_imageCombo->itemData(didx).toString();
                    QPixmap p(full);
                    if (!p.isNull())
                        m_imagePreview->setPixmap(p.scaled(m_imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        } else {
            // No image -> select None
            m_imageCombo->setCurrentIndex(0);
        }
    }

    connect(m_imageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx < 0) { m_imagePreview->clear(); return; }
        QString full = m_imageCombo->itemData(idx).toString();
        if (full.isEmpty()) {
            m_imagePreview->clear();
            return;
        }
        QPixmap p(full);
        if (!p.isNull())
            m_imagePreview->setPixmap(p.scaled(m_imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else
            m_imagePreview->clear();
    });

    mainLayout->addWidget(styleGroup);

    // ── Type-specific groups (always created, shown/hidden by type) ─────
    // Ensure UI reflects current type selection (hides/shows text group)
    onTypeChanged(m_typeCombo->currentIndex());

    // ── Buttons ─────────────────────────────────────────────────────────
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &PropertyDialog::applyChanges);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

QString PropertyDialog::imagePathValue() const
{
    if (!m_imageCombo) return QString();
    int idx = m_imageCombo->currentIndex();
    if (idx < 0) return QString();
    return m_imageCombo->itemData(idx).toString();
}

bool PropertyDialog::imageOnlyValue() const
{
    return m_imageOnlyChk ? m_imageOnlyChk->isChecked() : false;
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
    if (m_edgeStyleCombo)
        m_element->setEdgeStyle(static_cast<UiElement::EdgeStyle>(m_edgeStyleCombo->currentData().toInt()));
    m_element->setInheritable(m_inheritableChk->isChecked());
    if (m_behaviorCombo)
        m_element->setBehavior(static_cast<UiElement::ButtonBehavior>(m_behaviorCombo->currentData().toInt()));
    if (m_layerBox)
        m_element->setLayer(m_layerBox->value());

    // Behaviour is handled via the behaviour combo (includes PassThrough option).

    // ── Type-specific properties (only apply if type NOT changing) ───────
    if (!typeChanged()) {
        // Apply Button-specific properties: image path and image-only mode
        if (auto *btn = qobject_cast<ButtonElement *>(m_element)) {
            if (m_imageCombo && m_imageCombo->currentIndex() >= 0)
                btn->setImagePath(m_imageCombo->itemData(m_imageCombo->currentIndex()).toString());
            if (m_imageOnlyChk)
                btn->setImageOnly(m_imageOnlyChk->isChecked());
        }
    }

    accept();
}

void PropertyDialog::onTypeChanged(int index)
{
    ElementType t = static_cast<ElementType>(m_typeCombo->itemData(index).toInt());
    // Hide the full Text group (label + font controls) when Image type is selected.
    if (t == ElementType::Image) {
        if (m_textGroup) m_textGroup->setVisible(false);
        if (m_imageOnlyChk) {
            m_imageOnlyChk->setChecked(true);
            m_imageOnlyChk->setEnabled(false);
        }
    } else {
        if (m_textGroup) m_textGroup->setVisible(true);
        if (m_imageOnlyChk) m_imageOnlyChk->setEnabled(true);
    }
    adjustSize();
}

int PropertyDialog::actionTypeValue() const
{
    // Action types are no longer supported in the simplified editor.
    return 0;
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
