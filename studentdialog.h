#ifndef STUDENTDIALOG_H
#define STUDENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include "student.h"

class StudentDialog : public QDialog {
    Q_OBJECT
public:
    explicit StudentDialog(Student* student = nullptr, QWidget* parent = nullptr);
    
    Student getStudent() const;

private:
    QLineEdit* m_nameEdit;
    QSpinBox* m_recordSpin;
    QSpinBox* m_gradeSpins[5];
    Student* m_existingStudent;
};

#endif // STUDENTDIALOG_H
