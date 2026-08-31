#include "gui/main_window.h"
#include "utils.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QHeaderView>

MainWindow::MainWindow(std::vector<trash_candidate> candidates, QWidget* parent)
    : QMainWindow(parent), candidates_(std::move(candidates))
{
    setWindowTitle("Mac Sweep");
    resize(1180, 720);
    setMinimumSize(920, 580);

    auto* central = new QWidget;
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    auto* title = new QLabel("MacSweep");
    auto* subtitle = new QLabel("삭제 후보를 확인하고 macOS 휴지통으로 안전하게 이동합니다.");
    root->addWidget(title);
    root->addWidget(subtitle);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    root->addWidget(splitter, 1);

    // 왼쪽: 테이블 + 필터
    auto* table_frame = new QFrame;
    auto* table_layout = new QVBoxLayout(table_frame);
    table_layout->setContentsMargins(14, 14, 14, 14);

    auto* filter_layout = new QHBoxLayout;
    image_checkbox_ = new QCheckBox("Images");
    cache_checkbox_ = new QCheckBox("Cache");
    developer_cache_checkbox_ = new QCheckBox("Developer Cache");
    installer_checkbox_ = new QCheckBox("Installers");
    duplicate_checkbox_ = new QCheckBox("Duplicate Files");

    for (auto* cb : {image_checkbox_, cache_checkbox_, developer_cache_checkbox_,
                      installer_checkbox_, duplicate_checkbox_}) {
        cb->setChecked(true);
        connect(cb, &QCheckBox::stateChanged, this, &MainWindow::populate_table);
        filter_layout->addWidget(cb);
    }
    filter_layout->addStretch();
    table_layout->addLayout(filter_layout);

    table_ = new QTableWidget(0, 4);
    table_->setHorizontalHeaderLabels({"파일명", "용량", "카테고리", "삭제 사유"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 220);
    table_->setColumnWidth(1, 90);
    table_->setColumnWidth(2, 120);

    connect(table_, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::show_selected_candidate);

    table_layout->addWidget(table_);
    splitter->addWidget(table_frame);

    // 오른쪽: 미리보기
    auto* preview_frame = new QFrame;
    auto* preview_layout = new QVBoxLayout(preview_frame);
    preview_layout->setContentsMargins(18, 18, 18, 18);
    preview_layout->setSpacing(12);

    preview_layout->addWidget(new QLabel("미리보기"));

    thumbnail_ = new QLabel("항목을 선택하세요");
    thumbnail_->setAlignment(Qt::AlignCenter);
    thumbnail_->setMinimumHeight(230);
    thumbnail_->setWordWrap(true);
    preview_layout->addWidget(thumbnail_);

    name_value_ = add_detail_row(preview_layout, "파일명");
    size_value_ = add_detail_row(preview_layout, "용량");
    category_value_ = add_detail_row(preview_layout, "카테고리");
    reason_value_ = add_detail_row(preview_layout, "삭제 사유");
    path_value_ = add_detail_row(preview_layout, "전체 경로");
    preview_layout->addStretch();

    splitter->addWidget(preview_frame);
    splitter->setSizes({700, 420});

    // 버튼
    auto* button_layout = new QHBoxLayout;
    button_layout->addStretch();

    selected_button_ = new QPushButton("선택 항목 휴지통으로 이동");
    connect(selected_button_, &QPushButton::clicked, this, &MainWindow::move_selected_to_trash);

    all_button_ = new QPushButton("현재 보이는 후보 전체 이동");
    connect(all_button_, &QPushButton::clicked, this, &MainWindow::move_all_to_trash);

    button_layout->addWidget(selected_button_);
    button_layout->addWidget(all_button_);
    root->addLayout(button_layout);

    populate_table();
    update_buttons();
}

QLabel* MainWindow::add_detail_row(QVBoxLayout* layout, const QString& label_text) {
    layout->addWidget(new QLabel(label_text));
    auto* value = new QLabel("-");
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(value);
    return value;
}

bool MainWindow::is_category_visible(const std::string& category) const {
    if (category == "screenshot" || category == "image") return image_checkbox_->isChecked();
    if (category == "user_cache") return cache_checkbox_->isChecked();
    if (category == "developer_cache") return developer_cache_checkbox_->isChecked();
    if (category == "installer") return installer_checkbox_->isChecked();
    if (category == "duplicate") return duplicate_checkbox_->isChecked();
    return true;
}

void MainWindow::populate_table() {
    table_->setRowCount(0);
    visible_indices_.clear();

    for (size_t i = 0; i < candidates_.size(); ++i) {
        const auto& c = candidates_[i];
        if (!is_category_visible(c.category)) continue;

        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.path.filename().string())));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(c.size_mb, 'f', 2) + " MB"));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c.category)));
        table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(c.reason)));

        visible_indices_.push_back(i);
    }

    if (table_->rowCount() > 0) table_->selectRow(0);
    update_buttons();
}

const trash_candidate* MainWindow::current_candidate() const {
    int row = table_->currentRow();
    if (row < 0 || static_cast<size_t>(row) >= visible_indices_.size()) return nullptr;
    return &candidates_[visible_indices_[row]];
}

void MainWindow::show_selected_candidate() {
    const auto* c = current_candidate();
    if (!c) { clear_preview(); return; }

    name_value_->setText(QString::fromStdString(c->path.filename().string()));
    size_value_->setText(QString::number(c->size_mb, 'f', 2) + " MB");
    category_value_->setText(QString::fromStdString(c->category));
    reason_value_->setText(QString::fromStdString(c->reason));
    path_value_->setText(QString::fromStdString(c->path.string()));

    if (is_image_file(c->path) && fs::is_regular_file(c->path)) {
        QPixmap pixmap(QString::fromStdString(c->path.string()));
        if (!pixmap.isNull()) {
            thumbnail_->setPixmap(pixmap.scaled(360, 230, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            thumbnail_->setText("");
            return;
        }
    }
    thumbnail_->setPixmap(QPixmap());
    thumbnail_->setText("이미지 미리보기를 사용할 수 없습니다.");
}

void MainWindow::clear_preview() {
    thumbnail_->setPixmap(QPixmap());
    thumbnail_->setText("항목을 선택하세요");
    for (auto* label : {name_value_, size_value_, category_value_, reason_value_, path_value_}) {
        label->setText("-");
    }
}

void MainWindow::move_selected_to_trash() {
    const auto* c = current_candidate();
    if (!c) return;

    auto answer = QMessageBox::question(this, "휴지통으로 이동",
        QString("'%1' 항목을 휴지통으로 이동할까요?").arg(QString::fromStdString(c->path.filename().string())));
    if (answer != QMessageBox::Yes) return;

    size_t index = visible_indices_[table_->currentRow()];
    if (!move_to_trash(c->path)) {
        QMessageBox::critical(this, "이동 실패", "휴지통으로 이동하지 못했습니다.");
        return;
    }

    candidates_.erase(candidates_.begin() + index);
    populate_table();
    clear_preview();
    QMessageBox::information(this, "이동 완료", "선택한 항목을 휴지통으로 이동했습니다.");
}

void MainWindow::move_all_to_trash() {
    if (visible_indices_.empty()) return;

    auto answer = QMessageBox::question(this, "현재 후보 이동",
        QString("현재 보이는 삭제 후보 %1개를 모두 휴지통으로 이동할까요?").arg(visible_indices_.size()));
    if (answer != QMessageBox::Yes) return;

    std::vector<fs::path> to_remove;
    int moved = 0, failed = 0;

    for (size_t idx : visible_indices_) {
        if (move_to_trash(candidates_[idx].path)) {
            to_remove.push_back(candidates_[idx].path);
            ++moved;
        } else {
            ++failed;
        }
    }

    candidates_.erase(
        std::remove_if(candidates_.begin(), candidates_.end(),
            [&](const trash_candidate& c) {
                return std::find(to_remove.begin(), to_remove.end(), c.path) != to_remove.end();
            }),
        candidates_.end());

    populate_table();
    clear_preview();

    if (failed > 0) {
        QMessageBox::warning(this, "일부 이동 실패",
            QString("%1개 이동 완료, %2개 이동 실패").arg(moved).arg(failed));
    } else {
        QMessageBox::information(this, "이동 완료", QString("%1개 항목을 휴지통으로 이동했습니다.").arg(moved));
    }
}

void MainWindow::update_buttons() {
    bool has_items = table_->rowCount() > 0;
    selected_button_->setEnabled(has_items);
    all_button_->setEnabled(has_items);
}