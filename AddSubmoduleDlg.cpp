#include "AddSubmoduleDlg.h"
#include "ui_AddSubmoduleDlg.h"

#include <git.h>

#include <QFile>
#include <QDir>

AddSubmoduleDlg::AddSubmoduleDlg(QSharedPointer<Git> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::AddSubmoduleDlg)
   , mGit(git)
{
   ui->setupUi(this);

   connect(ui->lePath, &QLineEdit::returnPressed, this, &AddSubmoduleDlg::accept);
   connect(ui->leUrl, &QLineEdit::returnPressed, this, &AddSubmoduleDlg::accept);

   connect(ui->pbAccept, &QPushButton::clicked, this, &AddSubmoduleDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddSubmoduleDlg::~AddSubmoduleDlg()
{
   delete ui;
}

void AddSubmoduleDlg::accept()
{
   const auto submodulePath = ui->lePath->text();
   const auto submoduleUrl = ui->leUrl->text();

   const auto ret = mGit->submoduleAdd(submoduleUrl, submodulePath);

   if (ret)
      QDialog::accept();
}
