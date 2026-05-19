#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QStackedWidget>
#include <QLabel>
#include <vector>
#include "student.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void addStudent();
    void editStudent();
    void deleteStudent();
    void toggleViewMode();
    void sortByNameAsc();
    void sortByNameDesc();
    void sortByAverageAsc();
    void sortByAverageDesc();
    void loadFile();
    void saveFile();

private:
    void updateTable();
    void updateCards();
    void refreshView();
    
    std::vector<Student> m_students;
    QTableWidget* m_tableWidget;
    QStackedWidget* m_stackedWidget;
    QWidget* m_cardContainer;
    bool m_isTableView;
    QString m_currentFile;
};

#endif // MAINWINDOW_H
