#include "studentdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

StudentDialog::StudentDialog(Student* student, QWidget* parent)
    : QDialog(parent), m_existingStudent(student) {
    
    setWindowTitle(m_existingStudent ? "Редактировать студента" : "Добавить студента");
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);

    // ФИО
    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("ФИО:"));
    m_nameEdit = new QLineEdit();
    if (m_existingStudent) {
        m_nameEdit->setText(m_existingStudent->fullName);
    }
    nameLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(nameLayout);

    // Номер зачетки
    auto* recordLayout = new QHBoxLayout();
    recordLayout->addWidget(new QLabel("№ зачетки:"));
    m_recordSpin = new QSpinBox();
    m_recordSpin->setRange(1, 999999);
    if (m_existingStudent) {
        m_recordSpin->setValue(m_existingStudent->recordNumber);
    }
    recordLayout->addWidget(m_recordSpin);
    recordLayout->addStretch();
    mainLayout->addLayout(recordLayout);

    // Оценки
    auto* gradesGroup = new QGroupBox("Оценки (5 предметов)");
    auto* gradesLayout = new QHBoxLayout();
    
    for (int i = 0; i < 5; ++i) {
        auto* layout = new QVBoxLayout();
        layout->addWidget(new QLabel(QString("Предмет %1").arg(i + 1)));
        m_gradeSpins[i] = new QSpinBox();
        m_gradeSpins[i]->setRange(2, 5);
        if (m_existingStudent) {
            m_gradeSpins[i]->setValue(m_existingStudent->grades[i]);
        } else {
            m_gradeSpins[i]->setValue(4);
        }
        layout->addWidget(m_gradeSpins[i]);
        gradesLayout->addLayout(layout);
    }
    
    gradesGroup->setLayout(gradesLayout);
    mainLayout->addWidget(gradesGroup);

    // Кнопки
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* okButton = new QPushButton("OK");
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(okButton);
    
    auto* cancelButton = new QPushButton("Отмена");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

Student StudentDialog::getStudent() const {
    Student s;
    s.fullName = m_nameEdit->text();
    s.recordNumber = m_recordSpin->value();
    for (int i = 0; i < 5; ++i) {
        s.grades[i] = m_gradeSpins[i]->value();
    }
    return s;
}
