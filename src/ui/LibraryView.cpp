#include "ui/LibraryView.h"

#include "ui/PhotoDelegate.h"
#include "ui/PhotoModel.h"

#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStackedLayout>
#include <QVBoxLayout>

namespace svitlo {

namespace {
constexpr int kAllFoldersRole = Qt::UserRole + 10;
}

LibraryView::LibraryView(PhotoModel* model, QWidget* parent) : QWidget(parent), m_model(model) {
    // --- Left: folders ---------------------------------------------------
    auto* left = new QWidget;
    left->setMinimumWidth(180);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(10, 10, 6, 10);
    auto* foldersTitle = new QLabel(tr("FOLDERS"));
    foldersTitle->setObjectName("sectionTitle");
    m_folders = new QListWidget;
    m_folders->setObjectName("folderList");
    auto* importButton = new QPushButton(tr("Import Folder…"));
    leftLayout->addWidget(foldersTitle);
    leftLayout->addWidget(m_folders, 1);
    leftLayout->addWidget(importButton);
    connect(importButton, &QPushButton::clicked, this, &LibraryView::importRequested);

    // --- Filter bar --------------------------------------------------------
    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Search file name or camera"));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(260);

    m_ratingFilter = new QComboBox;
    m_ratingFilter->addItem(tr("Any rating"), 0);
    for (int i = 1; i <= 5; ++i)
        m_ratingFilter->addItem(tr("%1+ ●").arg(i), i);

    m_flagFilter = new QComboBox;
    m_flagFilter->addItem(tr("Hide rejected"), int(PhotoModel::FlagFilter::HideRejected));
    m_flagFilter->addItem(tr("All photos"), int(PhotoModel::FlagFilter::All));
    m_flagFilter->addItem(tr("Picked only"), int(PhotoModel::FlagFilter::PickedOnly));
    m_flagFilter->addItem(tr("Rejected only"), int(PhotoModel::FlagFilter::RejectedOnly));

    m_sort = new QComboBox;
    m_sort->addItem(tr("Sort: capture time"), int(PhotoModel::SortOrder::CaptureTime));
    m_sort->addItem(tr("Sort: file name"), int(PhotoModel::SortOrder::FileName));
    m_sort->addItem(tr("Sort: import time"), int(PhotoModel::SortOrder::ImportTime));

    m_size = new QSlider(Qt::Horizontal);
    m_size->setRange(100, 420);
    m_size->setValue(200);
    m_size->setFixedWidth(120);
    m_size->setToolTip(tr("Thumbnail size"));

    auto* bar = new QHBoxLayout;
    bar->setContentsMargins(8, 8, 8, 8);
    bar->addWidget(m_search);
    bar->addWidget(m_ratingFilter);
    bar->addWidget(m_flagFilter);
    bar->addWidget(m_sort);
    bar->addStretch();
    bar->addWidget(new QLabel(tr("Size")));
    bar->addWidget(m_size);

    // --- Grid ----------------------------------------------------------------
    m_grid = new QListView;
    m_delegate = new PhotoDelegate(m_grid);
    m_grid->setModel(m_model);
    m_grid->setItemDelegate(m_delegate);
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Static);
    m_grid->setUniformItemSizes(true);
    m_grid->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_grid->setSpacing(2);
    m_grid->setMouseTracking(true);
    m_grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_emptyState = new QLabel(tr("<p style='font-size:18px'>No photos here yet</p>"
                                 "<p>Use <b>Import Folder…</b> to add a folder of photos.<br>"
                                 "Your originals are never changed.</p>"));
    static_cast<QLabel*>(m_emptyState)->setAlignment(Qt::AlignCenter);
    m_emptyState->setStyleSheet("color: #8D9095; background: #151617;");

    auto* centreStack = new QWidget;
    auto* stack = new QStackedLayout(centreStack);
    stack->addWidget(m_grid);
    stack->addWidget(m_emptyState);

    auto* centre = new QWidget;
    auto* centreLayout = new QVBoxLayout(centre);
    centreLayout->setContentsMargins(0, 0, 0, 0);
    centreLayout->setSpacing(0);
    centreLayout->addLayout(bar);
    centreLayout->addWidget(centreStack, 1);

    auto* splitter = new QSplitter;
    splitter->addWidget(left);
    splitter->addWidget(centre);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 1000});

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    // --- Behaviour -------------------------------------------------------------
    auto updateEmpty = [this, stack] { stack->setCurrentIndex(m_model->rowCount() == 0 ? 1 : 0); };
    connect(m_model, &QAbstractItemModel::modelReset, this, updateEmpty);
    connect(m_model, &QAbstractItemModel::modelReset, this, &LibraryView::refreshFolders);
    updateEmpty();

    connect(m_folders, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item) {
        if (!item)
            return;
        m_model->setFolder(item->data(kAllFoldersRole).toBool() ? QString()
                                                                 : item->data(Qt::UserRole).toString());
    });
    connect(m_search, &QLineEdit::textChanged, m_model, &PhotoModel::setSearchText);
    connect(m_ratingFilter, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { m_model->setMinRating(m_ratingFilter->currentData().toInt()); });
    connect(m_flagFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        m_model->setFlagFilter(PhotoModel::FlagFilter(m_flagFilter->currentData().toInt()));
    });
    connect(m_sort, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        m_model->setSortOrder(PhotoModel::SortOrder(m_sort->currentData().toInt()));
    });
    connect(m_size, &QSlider::valueChanged, this, [this](int v) {
        m_delegate->setCellSize(v);
        m_grid->doItemsLayout();
    });
    connect(m_grid, &QListView::doubleClicked, this, [this](const QModelIndex& idx) {
        emit openInDevelop(idx.data(PhotoModel::IdRole).toLongLong());
    });

    refreshFolders();
}

void LibraryView::refreshFolders() {
    const QString current = m_model->folder();
    const QSignalBlocker block(m_folders);
    m_folders->clear();

    const auto folders = m_model->folders();
    int total = 0;
    for (const auto& f : folders)
        total += f.second;

    auto* all = new QListWidgetItem(tr("All Photographs   %1").arg(total));
    all->setData(kAllFoldersRole, true);
    m_folders->addItem(all);
    if (current.isEmpty())
        m_folders->setCurrentItem(all);

    for (const auto& f : folders) {
        auto* item = new QListWidgetItem(QStringLiteral("%1   %2").arg(QDir(f.first).dirName()).arg(f.second));
        item->setData(Qt::UserRole, f.first);
        item->setToolTip(QDir::toNativeSeparators(f.first));
        m_folders->addItem(item);
        if (f.first == current)
            m_folders->setCurrentItem(item);
    }
}

QVector<qint64> LibraryView::selectedIds() const {
    QVector<qint64> ids;
    for (const QModelIndex& idx : m_grid->selectionModel()->selectedIndexes())
        ids.append(idx.data(PhotoModel::IdRole).toLongLong());
    if (ids.isEmpty() && m_grid->currentIndex().isValid())
        ids.append(m_grid->currentIndex().data(PhotoModel::IdRole).toLongLong());
    return ids;
}

qint64 LibraryView::currentId() const {
    const QModelIndex idx = m_grid->currentIndex();
    return idx.isValid() ? idx.data(PhotoModel::IdRole).toLongLong() : 0;
}

void LibraryView::selectId(qint64 id) {
    const int row = m_model->rowOf(id);
    if (row < 0)
        return;
    const QModelIndex idx = m_model->index(row);
    m_grid->setCurrentIndex(idx);
    m_grid->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
    m_grid->scrollTo(idx);
}

} // namespace svitlo
