#pragma once
#include <QMainWindow>
#include <QCheckBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <vector>
#include "trash_candidate.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::vector<trash_candidate> candidates, QWidget* parent = nullptr);

private slots:
    void populate_table();
    void show_selected_candidate();
    void move_selected_to_trash();
    void move_all_to_trash();

private:
    QLabel* add_detail_row(QVBoxLayout* layout, const QString& label_text);
    void clear_preview();
    void update_buttons();
    const trash_candidate* current_candidate() const;
    bool is_category_visible(const std::string& category) const;

    std::vector<trash_candidate> candidates_;
    std::vector<size_t> visible_indices_;  // 테이블 row -> candidates_ 인덱스

    QCheckBox* image_checkbox_;
    QCheckBox* cache_checkbox_;
    QCheckBox* developer_cache_checkbox_;
    QCheckBox* installer_checkbox_;
    QCheckBox* duplicate_checkbox_;

    QTableWidget* table_;
    QLabel* thumbnail_;
    QLabel* name_value_;
    QLabel* size_value_;
    QLabel* category_value_;
    QLabel* reason_value_;
    QLabel* path_value_;

    QPushButton* selected_button_;
    QPushButton* all_button_;
};