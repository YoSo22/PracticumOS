#include "mainwindow.h"
#include "studentdialog.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollArea>
#include <QGridLayout>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_isTableView(true), m_currentFile("students.csv") {
    
    setWindowTitle("Менеджер студентов");
    resize(800, 600);

    // Виджеты
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Таблица
    m_tableWidget = new QTableWidget();
    m_tableWidget->setColumnCount(7);
    m_tableWidget->setHorizontalHeaderLabels({"ФИО", "№ зачетки", "О1", "О2", "О3", "О4", "О5"});
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_stackedWidget->addWidget(m_tableWidget);

    // Карточки (в скролл-области)
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    m_cardContainer = new QWidget();
    scrollArea->setWidget(m_cardContainer);
    m_stackedWidget->addWidget(scrollArea);

    // Меню
    auto* fileMenu = menuBar()->addMenu("Файл");
    fileMenu->addAction("Открыть...", this, &MainWindow::loadFile, QKeySequence::Open);
    fileMenu->addAction("Сохранить", this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("Выход", qApp, &QApplication::quit);

    auto* editMenu = menuBar()->addMenu("Правка");
    editMenu->addAction("Добавить", this, &MainWindow::addStudent, QKeySequence::New);
    editMenu->addAction("Изменить", this, &MainWindow::editStudent, QKeySequence{Qt::CTRL | Qt::Key_E});
    editMenu->addAction("Удалить", this, &MainWindow::deleteStudent, QKeySequence::Delete);

    auto* viewMenu = menuBar()->addMenu("Вид");
    viewMenu->addAction("Таблица/Карточки", this, &MainWindow::toggleViewMode, QKeySequence{Qt::CTRL | Qt::Key_V});

    auto* sortMenu = menuBar()->addMenu("Сортировка");
    sortMenu->addAction("По ФИО (А-Я)", this, &MainWindow::sortByNameAsc);
    sortMenu->addAction("По ФИО (Я-А)", this, &MainWindow::sortByNameDesc);
    sortMenu->addSeparator();
    sortMenu->addAction("По среднему баллу (возр.)", this, &MainWindow::sortByAverageAsc);
    sortMenu->addAction("По среднему баллу (убыв.)", this, &MainWindow::sortByAverageDesc);

    // Статус бар
    statusBar()->showMessage("Готово");

    // Загрузка данных
    StudentManager::loadFromFile(m_currentFile, m_students);
    refreshView();
}

MainWindow::~MainWindow() = default;

void MainWindow::addStudent() {
    StudentDialog dialog(nullptr, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_students.push_back(dialog.getStudent());
        StudentManager::saveToFile(m_currentFile, m_students);
        refreshView();
        statusBar()->showMessage("Студент добавлен");
    }
}

void MainWindow::editStudent() {
    if (m_isTableView) {
        int row = m_tableWidget->currentRow();
        if (row < 0 || row >= static_cast<int>(m_students.size())) {
            QMessageBox::warning(this, "Ошибка", "Выберите студента для редактирования");
            return;
        }
        StudentDialog dialog(&m_students[row], this);
        if (dialog.exec() == QDialog::Accepted) {
            m_students[row] = dialog.getStudent();
            StudentManager::saveToFile(m_currentFile, m_students);
            refreshView();
            statusBar()->showMessage("Студент обновлен");
        }
    } else {
        QMessageBox::information(this, "Инфо", "Переключитесь в режим таблицы для редактирования");
    }
}

void MainWindow::deleteStudent() {
    if (m_isTableView) {
        int row = m_tableWidget->currentRow();
        if (row < 0 || row >= static_cast<int>(m_students.size())) {
            QMessageBox::warning(this, "Ошибка", "Выберите студента для удаления");
            return;
        }
        if (QMessageBox::question(this, "Подтверждение", "Удалить студента?") == QMessageBox::Yes) {
            m_students.erase(m_students.begin() + row);
            StudentManager::saveToFile(m_currentFile, m_students);
            refreshView();
            statusBar()->showMessage("Студент удален");
        }
    } else {
        QMessageBox::information(this, "Инфо", "Переключитесь в режим таблицы для удаления");
    }
}

void MainWindow::toggleViewMode() {
    m_isTableView = !m_isTableView;
    m_stackedWidget->setCurrentIndex(m_isTableView ? 0 : 1);
    if (!m_isTableView) {
        updateCards();
    }
    statusBar()->showMessage(m_isTableView ? "Режим: Таблица" : "Режим: Карточки");
}

void MainWindow::sortByNameAsc() {
    StudentManager::sortByNameRecursive(m_students, 0, m_students.size() - 1, true);
    StudentManager::saveToFile(m_currentFile, m_students);
    refreshView();
    statusBar()->showMessage("Сортировка по ФИО (А-Я)");
}

void MainWindow::sortByNameDesc() {
    StudentManager::sortByNameRecursive(m_students, 0, m_students.size() - 1, false);
    StudentManager::saveToFile(m_currentFile, m_students);
    refreshView();
    statusBar()->showMessage("Сортировка по ФИО (Я-А)");
}

void MainWindow::sortByAverageAsc() {
    StudentManager::sortByAverageNonRecursive(m_students, true);
    StudentManager::saveToFile(m_currentFile, m_students);
    refreshView();
    statusBar()->showMessage("Сортировка по среднему баллу (возрастание)");
}

void MainWindow::sortByAverageDesc() {
    StudentManager::sortByAverageNonRecursive(m_students, false);
    StudentManager::saveToFile(m_currentFile, m_students);
    refreshView();
    statusBar()->showMessage("Сортировка по среднему баллу (убывание)");
}

void MainWindow::loadFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Открыть файл", "", "CSV Files (*.csv)");
    if (!filename.isEmpty()) {
        m_currentFile = filename;
        StudentManager::loadFromFile(filename, m_students);
        refreshView();
        statusBar()->showMessage("Файл загружен: " + filename);
    }
}

void MainWindow::saveFile() {
    QString filename = QFileDialog::getSaveFileName(this, "Сохранить файл", m_currentFile, "CSV Files (*.csv)");
    if (!filename.isEmpty()) {
        m_currentFile = filename;
        StudentManager::saveToFile(filename, m_students);
        statusBar()->showMessage("Файл сохранен: " + filename);
    }
}

void MainWindow::updateTable() {
    m_tableWidget->setRowCount(m_students.size());
    for (size_t i = 0; i < m_students.size(); ++i) {
        const auto& s = m_students[i];
        m_tableWidget->setItem(i, 0, new QTableWidgetItem(s.fullName));
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(s.recordNumber)));
        for (int j = 0; j < 5; ++j) {
            m_tableWidget->setItem(i, j + 2, new QTableWidgetItem(QString::number(s.grades[j])));
        }
    }
}

void MainWindow::updateCards() {
    // Очистка контейнера
    QLayout* layout = m_cardContainer->layout();
    if (layout) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout;
    }

    auto* gridLayout = new QGridLayout();
    int row = 0, col = 0;
    
    for (size_t i = 0; i < m_students.size(); ++i) {
        const auto& s = m_students[i];
        auto* card = new QGroupBox(QString("Студент %1").arg(i + 1));
        auto* cardLayout = new QFormLayout();
        
        cardLayout->addRow("ФИО:", new QLabel(s.fullName));
        cardLayout->addRow("№ зачетки:", new QLabel(QString::number(s.recordNumber)));
        cardLayout->addRow("Средний балл:", new QLabel(QString::number(s.getAverage(), 'f', 2)));
        
        QString gradesStr;
        for (int j = 0; j < 5; ++j) {
            gradesStr += QString::number(s.grades[j]) + " ";
        }
        cardLayout->addRow("Оценки:", new QLabel(gradesStr.trimmed()));
        
        card->setLayout(cardLayout);
        gridLayout->addWidget(card, row, col);
        
        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
    }
    
    gridLayout->addStretch();
    m_cardContainer->setLayout(gridLayout);
}

void MainWindow::refreshView() {
    updateTable();
    if (!m_isTableView) {
        updateCards();
    }
}
