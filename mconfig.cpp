//
//   Copyright (C) 2003-2010 by Warren Woodford
//   Copyright (C) 2014 by Timothy E. Harris
//   for modifications applicable to the MX Linux project.
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#include "mconfig.h"
#include <stdio.h>
#include <unistd.h>
#include <QFileDialog>

#include <QDebug>


MConfig::MConfig(QWidget* parent) : QDialog(parent) {
    setupUi(this);
    setWindowIcon(QApplication::windowIcon());

    proc = new QProcess(this);
    timer = new QTimer(this);

    tabWidget->setCurrentIndex(0);
    refresh();
}

MConfig::~MConfig(){
}

/////////////////////////////////////////////////////////////////////////
// util functions

QString MConfig::getCmdOut(QString cmd) {
    char line[260];
    const char* ret = "";
    FILE* fp = popen(cmd.toUtf8(), "r");
    if (fp == NULL) {
        return QString (ret);
    }
    int i;
    if (fgets(line, sizeof line, fp) != NULL) {
        i = strlen(line);
        line[--i] = '\0';
        ret = line;
    }
    pclose(fp);
    return QString (ret);
}

QStringList MConfig::getCmdOuts(QString cmd) {
    char line[260];
    FILE* fp = popen(cmd.toUtf8(), "r");
    QStringList results;
    if (fp == NULL) {
        return results;
    }
    int i;
    while (fgets(line, sizeof line, fp) != NULL) {
        i = strlen(line);
        line[--i] = '\0';
        results.append(line);
    }
    pclose(fp);
    return results;
}

QString MConfig::getCmdValue(QString cmd, QString key, QString keydel, QString valdel) {
    const char *ret = "";
    char line[260];

    QStringList strings = getCmdOuts(cmd);
    for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
        strcpy(line, ((QString)*it).toUtf8());
        char* keyptr = strstr(line, key.toUtf8());
        if (keyptr != NULL) {
            // key found
            strtok(keyptr, keydel.toUtf8());
            const char* val = strtok(NULL, valdel.toUtf8());
            if (val != NULL) {
                ret = val;
            }
            break;
        }
    }
    return QString (ret);
}

QStringList MConfig::getCmdValues(QString cmd, QString key, QString keydel, QString valdel) {
    char line[130];
    FILE* fp = popen(cmd.toUtf8(), "r");
    QStringList results;
    if (fp == NULL) {
        return results;
    }
    int i;
    while (fgets(line, sizeof line, fp) != NULL) {
        i = strlen(line);
        line[--i] = '\0';
        char* keyptr = strstr(line, key.toUtf8());
        if (keyptr != NULL) {
            // key found
            strtok(keyptr, keydel.toUtf8());
            const char* val = strtok(NULL, valdel.toUtf8());
            if (val != NULL) {
                results.append(val);
            }
        }
    }
    pclose(fp);
    return results;
}

bool MConfig::replaceStringInFile(QString oldtext, QString newtext, QString filepath) {

    QString cmd = QString("sed -i 's/%1/%2/g' %3").arg(oldtext).arg(newtext).arg(filepath);
    if (system(cmd.toUtf8()) != 0) {
        return false;
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////
// common

void MConfig::refresh() {
    setCursor(QCursor(Qt::ArrowCursor));
    syncProgressBar->setValue(0);
    int i = tabWidget->currentIndex();
    switch (i) {

    case 1:
        refreshRestore();
        buttonApply->setEnabled(false);
        break;

    case 2:
        refreshDesktop();
        buttonApply->setEnabled(true);
        break;

    case 3:
        refreshGroups();
        buttonApply->setEnabled(false);
        break;

    case 4:
        refreshMembership();
        buttonApply->setEnabled(false);
        break;

    default:
        refreshAdd();
        refreshDelete();
        refreshChangePass();
        buttonApply->setEnabled(false);
        break;
    }
}

/////////////////////////////////////////////////////////////////////////
// special

void MConfig::refreshRestore() {
    char line[130];
    char line2[130];
    char *tok;
    FILE *fp;
    int i;
    // locale
    userComboBox->clear();
    userComboBox->addItem(tr("none"));
    userComboBox->addItem("root");
    fp = popen("ls -1 /home", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                sprintf(line2, "grep '^%s' /etc/passwd >/dev/null", tok);
                if (system(line2) == 0) {
                    userComboBox->addItem(tok);
                }
            }
        }
        pclose(fp);
    }
    checkGroups->setChecked(false);
    checkMozilla->setChecked(false);
    checkApt->setChecked(false);
    radioAutologinNo->setAutoExclusive(false);
    radioAutologinNo->setChecked(false);
    radioAutologinNo->setAutoExclusive(true);
    radioAutologinYes->setAutoExclusive(false);
    radioAutologinYes->setChecked(false);
    radioAutologinYes->setAutoExclusive(true);
}

void MConfig::refreshDesktop() {
    char line[130];
    QString cmd;
    fromUserComboBox->clear();
    FILE *fp = popen("ls -1 /home", "r");
    int i;
    char *tok;
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                cmd = QString("grep '^%1' /etc/passwd >/dev/null").arg(tok);
                if (system(cmd.toUtf8()) == 0) {
                    fromUserComboBox->addItem(tok);
                }
            }
        }
        pclose(fp);
    }
    copyRadioButton->setChecked(true);
    entireRadioButton->setChecked(true);
    on_fromUserComboBox_activated();
}

void MConfig::refreshAdd() {
    userNameEdit->setText(tr(""));
    userPasswordEdit->setText("");
    userPassword2Edit->setText("");
    addUserBox->setEnabled(true);
}

void MConfig::refreshDelete() {
    char line[130];
    char line2[130];
    char *tok;
    FILE *fp;
    int i;
    // locale
    deleteUserCombo->clear();
    deleteUserCombo->addItem(tr("none"));
    deleteUserBox->setEnabled(true);
    fp = popen("ls -1 /home", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                sprintf(line2, "grep '^%s' /etc/passwd >/dev/null", tok);
                if (system(line2) == 0) {
                    deleteUserCombo->addItem(tok);
                }
            }
        }
        pclose(fp);
    }
}

void MConfig::refreshChangePass()
{
    char line[130];
    char line2[130];
    char *tok;
    FILE *fp;
    int i;

    comboChangePass->clear();
    comboChangePass->addItem(tr("none"));
    userBoxChangePass->setEnabled(true);
    comboChangePass->addItem("root");
    lineEditChangePass->setText("");
    lineEditChangePassConf->setText("");

    fp = popen("ls -1 /home", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                sprintf(line2, "grep '^%s' /etc/passwd >/dev/null", tok);
                if (system(line2) == 0) {
                    comboChangePass->addItem(tok);
                }
            }
        }
        pclose(fp);
    }
}


void MConfig::refreshGroups() {
    char line[130];
    FILE *fp;
    int i;
    groupNameEdit->setText(tr(""));
    addBox->setEnabled(true);
    deleteGroupCombo->clear();
    deleteGroupCombo->addItem(tr("none"));
    deleteBox->setEnabled(true);
    fp = popen("cat /etc/group | cut -f 1 -d :", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            if (line != NULL && strlen(line) > 1 && strcmp(line, "root") != 0 ) {
                deleteGroupCombo->addItem(line);
            }
        }
        pclose(fp);
    }
}

void MConfig::refreshMembership() {
    char line[130];
    char line2[130];
    char *tok;
    FILE *fp;
    int i;
    userComboMembership->clear();
    userComboMembership->addItem(tr("none"));
    listGroups->clear();
    fp = popen("ls -1 /home", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                sprintf(line2, "grep '^%s' /etc/passwd >/dev/null", tok);
                if (system(line2) == 0) {
                    userComboMembership->addItem(tok);
                }
            }
        }
        pclose(fp);
    }
}


// apply but do not close
void MConfig::applyRestore() {
    QString user = userComboBox->currentText();
    if (user.compare(tr("none")) == 0) {
        // no user selected
        return;
    }
    QString home = user;
    if (user.compare("root") != 0) {
        home = QString("/home/%1").arg(user);
    }
    QString cmd;

    if (checkApt->isChecked() || checkGroups->isChecked() || checkMozilla->isChecked()) {
        int ans = QMessageBox::warning(0, QString::null,
                                       tr("The user configuration will be repaired. Please close all other applications now. When finished, please logout or reboot. Are you sure you want to repair now?"),
                                       tr("Yes"), tr("No"));
        if (ans != 0) {
            return;
        }
    }
    setCursor(QCursor(Qt::WaitCursor));

    // restore groups
    if (checkGroups->isChecked() && user.compare("root") != 0) {
        cmd = QString("sed -n '/^EXTRA_GROUPS=/s/^EXTRA_GROUPS=//p' /etc/adduser.conf | sed  -e 's/ /,/g' -e 's/\"//g'");
        cmd = "usermod -G " + getCmdOut(cmd) + " " + user;
        system(cmd.toUtf8());
    }
    // restore Mozilla configs
    if (checkMozilla->isChecked()) {
        cmd = QString("/bin/rm -r %1/.mozilla").arg(home);
        system(cmd.toUtf8());
    }
    // restore APT configs
    if (checkApt->isChecked()) {
        QString mx_version = getCmdOut("lsb_release -rs").left(2);
        if (mx_version.toInt() < 15) {
            qDebug() << "MX version not detected or out of range: " << mx_version;
            return;
        }
        // create temp folder
        QString path = getCmdOut("mktemp -d /tmp/mx-sources.XXXXXX");
        // download source files from
        cmd = QString("wget -q https://github.com/mx-linux/MX-%1_sources/archive/master.zip -P %2").arg(mx_version).arg(path);
        system(cmd.toUtf8());
        // extract master.zip to temp folder
        cmd = QString("unzip -q %1/master.zip -d %1/").arg(path);
        system(cmd.toUtf8());
        // move the files from the temporary directory to /etc/apt/sources.list.d/
        cmd = QString("mv -b %1/MX-*_sources-master/* /etc/apt/sources.list.d/").arg(path);
        system(cmd.toUtf8());
        // delete temp folder
        cmd = QString("rm -rf %1").arg(path);
        system(cmd.toUtf8());
        // localize repos
        system("localize-repo $(cat /etc/timezone)");
    }
    if (radioAutologinNo->isChecked()) {
        cmd = QString("sed -i -r '/^autologin-user=%1/ s/^/#/' /etc/lightdm/lightdm.conf").arg(user);
        system(cmd.toUtf8());
        QMessageBox::information(0, tr("Autologin options"),
                                 (tr("Autologin has been disabled for the '%1' account.").arg(user)));
    } else if (radioAutologinYes->isChecked()) {
        cmd = QString("grep -qE '^#autologin-user=%1'\\|'^autologin-user=%1' /etc/lightdm/lightdm.conf").arg(user);
        if (system(cmd.toUtf8()) == 0) {
            cmd = QString("sed -i -r '/^#autologin-user=%1/ s/^#//' /etc/lightdm/lightdm.conf").arg(user);
            system(cmd.toUtf8());
        } else {
            cmd = QString("echo 'autologin-user=%1' >> /etc/lightdm/lightdm.conf").arg(user);
            system(cmd.toUtf8());
        }
        QMessageBox::information(0, tr("Autologin options"),
                                 (tr("Autologin has been enabled for the '%1' account.").arg(user)));
    }
    setCursor(QCursor(Qt::ArrowCursor));

    refresh();
}

void MConfig::applyDesktop() {

    if (toUserComboBox->currentText().isEmpty()) {
        QMessageBox::information(0, QString::null,
                                 tr("You must specify a 'copy to' destination. You cannot copy to the desktop you are logged in to."));
        return;
    }
    // verify
    int ans = QMessageBox::critical(0, QString::null, tr("Before copying, close all other applications. Be sure the copy to destination is large enough to contain the files you are copying. Copying between desktops may overwrite or delete your files or preferences on the destination desktop. Are you sure you want to proceed?"),
                                    tr("Yes"), tr("No"));
    if (ans != 0) {
        return;
    }

    QString fromDir = QString("/home/%1").arg(fromUserComboBox->currentText());
    QString toDir = QString("/home/%1").arg(toUserComboBox->currentText());
    if (toUserComboBox->currentText().contains("/")) {  // if a directory rather than a user name
        toDir = toUserComboBox->currentText();
    }
    if (docsRadioButton->isChecked()) {
        fromDir.append("/Documents");
        toDir.append("/Documents");
    } else if (mozillaRadioButton->isChecked()) {
        fromDir.append("/.mozilla");
        toDir.append("/.mozilla");
    } else if (sharedRadioButton->isChecked()) {
        fromDir.append("/Shared");
        toDir.append("/Shared");
    }
    fromDir.append("/");

    setCursor(QCursor(Qt::WaitCursor));
    if (syncRadioButton->isChecked()) {
        syncStatusEdit->setText(tr("Synchronizing desktop..."));
    } else {
        syncStatusEdit->setText(tr("Copying desktop..."));
    }
    disconnect(timer, SIGNAL(timeout()), 0, 0);
    connect(timer, SIGNAL(timeout()), this, SLOT(syncTime()));
    disconnect(proc, SIGNAL(started()), 0, 0);
    connect(proc, SIGNAL(started()), this, SLOT(syncStart()));
    disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(syncDone(int, QProcess::ExitStatus)));
    QString cmd = QString("rsync -qa ");
    if (syncRadioButton->isChecked()) {
        cmd.append("--delete-after ");
    }
    cmd.append(fromDir);
    cmd.append(" ");
    cmd.append(toDir);
    proc->start(cmd);
}

void MConfig::applyAdd() {
    //validate data before proceeding
    // see if username is reasonable length
    if (userNameEdit->text().length() < 2) {
        QMessageBox::critical(0, QString::null,
                              tr("The user name needs to be at least 2 characters long. Please select a longer name before proceeding."));
        return;
    } else if (!userNameEdit->text().contains(QRegExp("^[a-z_][a-z0-9_-]*[$]?$"))) {
        QMessageBox::critical(0, QString::null,
                              tr("The user name needs to be lower case and it\n"
                                 "cannot contain special characters or spaces.\n"
                                 "Please choose another name before proceeding."));
        return;
    }
    // check that user name is not already used
    QString cmd = QString("grep '^%1' /etc/passwd >/dev/null").arg( userNameEdit->text());
    if (system(cmd.toUtf8()) == 0) {
        QMessageBox::critical(0, QString::null,
                              tr("Sorry that name is in use. Please select a different name."));
        return;
    }
    if (userPasswordEdit->text().compare(userPassword2Edit->text()) != 0) {
        QMessageBox::critical(0, QString::null,
                              tr("Password entries do not match. Please try again."));
        return;
    }
    if (userPasswordEdit->text().length() < 2) {
        QMessageBox::critical(0, QString::null,
                              tr("Password needs to be at least 2 characters long. Please enter a longer password before proceeding."));
        return;
    }

    cmd = QString("adduser --disabled-login --force-badname --gecos %1 %2").arg( userNameEdit->text()).arg(userNameEdit->text());
    system(cmd.toUtf8());
    cmd = QString("passwd %1").arg(userNameEdit->text());
    FILE *fp = popen(cmd.toUtf8(), "w");
    bool fpok = true;
    cmd = QString("%1\n").arg(userPasswordEdit->text());
    if (fp != NULL) {
        sleep(1);
        if (fputs(cmd.toUtf8(), fp) >= 0) {
            fflush(fp);
            sleep(1);
            if (fputs(cmd.toUtf8(), fp) < 0) {
                fpok = false;
            }
        } else {
            fpok = false;
        }
        pclose(fp);
    } else {
        fpok = false;
    }

    if (fpok) {
        QMessageBox::information(0, QString::null,
                                 tr("The user was added ok."));
        refresh();
    } else {
        QMessageBox::critical(0, QString::null,
                              tr("Failed to add the user."));
    }
}

// change user password
void MConfig::applyChangePass()
{
    if (lineEditChangePass->text().compare(lineEditChangePassConf->text()) != 0) {
        QMessageBox::critical(0, QString::null,
                              tr("Password entries do not match. Please try again."));
        return;
    }
    if (lineEditChangePass->text().length() < 2) {
        QMessageBox::critical(0, QString::null,
                              tr("Password needs to be at least 2 characters long. Please enter a longer password before proceeding."));
        return;
    }
    QString cmd = QString("passwd %1").arg(comboChangePass->currentText());
    FILE *fp = popen(cmd.toUtf8(), "w");
    bool fpok = true;
    cmd = QString("%1\n").arg(lineEditChangePass->text());
    if (fp != NULL) {
        sleep(1);
        if (fputs(cmd.toUtf8(), fp) >= 0) {
            fflush(fp);
            sleep(1);
            if (fputs(cmd.toUtf8(), fp) < 0) {
                fpok = false;
            }
        } else {
            fpok = false;
        }
        pclose(fp);
    } else {
        fpok = false;
    }

    if (fpok) {
        QMessageBox::information(0, QString::null,
                                 tr("Password successfully changed."));
        refresh();
    } else {
        QMessageBox::critical(0, QString::null,
                              tr("Failed to change password."));
    }
}

void MConfig::applyDelete() {
    QString cmd = QString(tr("This action cannot be undone. Are you sure you want to delete user %1?")).arg(deleteUserCombo->currentText());
    int ans = QMessageBox::warning(this, QString::null, cmd,
                                   tr("Yes"), tr("No"));
    if (ans == 0) {
        if (deleteHomeCheckBox->isChecked()) {
            cmd = QString("killall -u %1").arg( deleteUserCombo->currentText());
            system(cmd.toUtf8());
            cmd = QString("deluser --force --remove-home %1").arg( deleteUserCombo->currentText());
        } else {
            cmd = QString("deluser %1").arg(deleteUserCombo->currentText());
        }
        if (system(cmd.toUtf8()) == 0) {
            QMessageBox::information(0, QString::null,
                                     tr("The user has been deleted."));
        } else {
            QMessageBox::critical(0, QString::null,
                                  tr("Failed to delete the user."));
        }
        refresh();
    }
}

void MConfig::applyGroup() {
    //checks if adding or removing groups
    if (addBox->isEnabled()) {
        //validate data before proceeding
        // see if groupname is reasonable length
        if (groupNameEdit->text().length() < 2) {
            QMessageBox::critical(0, QString::null,
                                  tr("The group name needs to be at least 2 characters long. Please select a longer name before proceeding."));
            return;
        } else if (!groupNameEdit->text().contains(QRegExp("^[a-z_][a-z0-9_-]*[$]?$"))) {
            QMessageBox::critical(0, QString::null,
                                  tr("The group name needs to be lower case and it \n"
                                     "cannot contain special characters or spaces.\n"
                                     "Please choose another name before proceeding."));
            return;
        }
        // check that group name is not already used
        QString cmd = QString("grep '^%1' /etc/group >/dev/null").arg( groupNameEdit->text());
        if (system(cmd.toUtf8()) == 0) {
            QMessageBox::critical(0, QString::null,
                                  tr("Sorry that group name already exists. Please select a different name."));
            return;
        }
        // run addgroup command
        cmd = QString("addgroup --system %1").arg( groupNameEdit->text());
        if (system(cmd.toUtf8()) == 0) {
            QMessageBox::information(0, QString::null,
                                     tr("The system group was added ok."));
        } else {
            QMessageBox::critical(0, QString::null,
                                  tr("Failed to add the system group."));
        }
    }  else { //deleting group if addBox disabled
        QString cmd = QString(tr("This action cannot be undone. Are you sure you want to delete group %1?")).arg(deleteGroupCombo->currentText());
        int ans = QMessageBox::warning(this, QString::null, cmd,
                                       tr("Yes"), tr("No"));
        if (ans == 0) {
            cmd = QString("delgroup %1").arg(deleteGroupCombo->currentText());
            if (system(cmd.toUtf8()) == 0) {
                QMessageBox::information(0, QString::null,
                                         tr("The group has been deleted."));
            } else {
                QMessageBox::critical(0, QString::null,
                                      tr("Failed to delete the group."));
            }
        }
    }
    refresh();
}

void MConfig::applyMembership() {
    QString cmd;
    //Add all WidgetItems from listGroups
    QList<QListWidgetItem *> items = listGroups->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    while (!items.isEmpty()) {
        QListWidgetItem *item = items.takeFirst();
        if (item->checkState() == 2) {
            cmd += item->text() + ",";
        }
    }
    cmd.chop(1);
    int ans = QMessageBox::warning(this, QString::null, tr("Are you sure you want to make these changes?"),
                                   tr("Yes"), tr("No"));
    if (ans == 0) {
        cmd = QString("usermod -G %1 %2").arg(cmd).arg(userComboMembership->currentText());
        if (system(cmd.toUtf8()) == 0) {
            QMessageBox::information(0, QString::null,
                                     tr("The changes have been applied."));
        } else {
            QMessageBox::critical(0, QString::null,
                                  tr("Failed to apply group changes"));
        }
    }
}

void MConfig::syncDone(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit) {
        QString fromDir = QString("/home/%1").arg(fromUserComboBox->currentText());
        QString toDir = QString("/home/%1").arg(toUserComboBox->currentText());

        // if a directory rather than a user name
        if (toUserComboBox->currentText().contains("/")) {
            if (syncRadioButton->isChecked()) {
                syncStatusEdit->setText(tr("Synchronizing desktop...ok"));
            } else {
                syncStatusEdit->setText(tr("Copying desktop...ok"));
            }
            timer->stop();
            syncProgressBar->setValue(100);
            setCursor(QCursor(Qt::ArrowCursor));
            return;
        }

        // fix owner
        QString cmd = QString("chown -R %1:users %2").arg(toUserComboBox->currentText()).arg(toDir);
        system(cmd.toUtf8());

        // fix /home/username in some files
        if (entireRadioButton->isChecked() || mozillaRadioButton->isChecked()) {
            // fix mozilla tree
            cmd = QString("find %1/.mozilla -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
        }

        if (entireRadioButton->isChecked()) {
            //delete some files
            cmd = QString("rm -f %1/.recently-used").arg(toDir);
            system(cmd.toUtf8());
            cmd = QString("rm -f %1/.openoffice.org/*/.lock").arg(toDir);
            system(cmd.toUtf8());
            cmd = QString("find %1/.openoffice.org -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
            cmd = QString("find %1/.thunderbird -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
            cmd = QString("find %1/.adobe -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
            cmd = QString("find %1/.gimp-2.4 -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
            cmd = QString("find %1/.xine -type f -exec sed -i 's|home/%2|home/%3|g' '{}' \\;").arg(toDir).arg(fromUserComboBox->currentText()).arg(toUserComboBox->currentText());
            system(cmd.toUtf8());
        }
        if (syncRadioButton->isChecked()) {
            syncStatusEdit->setText(tr("Synchronizing desktop...ok"));
        } else {
            syncStatusEdit->setText(tr("Copying desktop...ok"));
        }
    } else {
        if (syncRadioButton->isChecked()) {
            syncStatusEdit->setText(tr("Synchronizing desktop...failed"));
        } else {
            syncStatusEdit->setText(tr("Copying desktop...failed"));
        }
    }
    timer->stop();
    syncProgressBar->setValue(100);
    setCursor(QCursor(Qt::ArrowCursor));
}

/////////////////////////////////////////////////////////////////////////
// slots

void MConfig::on_fromUserComboBox_activated() {
    char line[130];
    QString cmd;

    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
    toUserComboBox->clear();
    FILE *fp = popen("ls -1 /home", "r");
    int i;
    char *tok;
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            tok = strtok(line, " ");
            if (tok != NULL && strlen(tok) > 1 && strncmp(tok, "ftp", 3) != 0) {
                cmd = QString("grep '^%1' /etc/passwd >/dev/null").arg(tok);
                if (system(cmd.toUtf8()) == 0 && fromUserComboBox->currentText().compare(tok) != 0) {
                    cmd = QString("who | grep '%1'").arg(tok);
                    if (system(cmd.toUtf8()) != 0) {
                        toUserComboBox->addItem(tok);
                    }
                }
            }
        }
        pclose(fp);
    }
    toUserComboBox->addItem(tr("browse..."));
}

void MConfig::on_userComboBox_activated() {
    buttonApply->setEnabled(true);
    if (userComboBox->currentText() == tr("none")) {
        refresh();
    }
    radioAutologinNo->setAutoExclusive(false);
    radioAutologinNo->setChecked(false);
    radioAutologinNo->setAutoExclusive(true);
    radioAutologinYes->setAutoExclusive(false);
    radioAutologinYes->setChecked(false);
    radioAutologinYes->setAutoExclusive(true);
}

void MConfig::on_deleteUserCombo_activated() {
    addUserBox->setEnabled(false);
    userBoxChangePass->setEnabled(false);
    buttonApply->setEnabled(true);
    if (deleteUserCombo->currentText() == tr("none")) {
        refresh();
    }
}

void MConfig::on_userNameEdit_textEdited() {
    deleteUserBox->setEnabled(false);
    userBoxChangePass->setEnabled(false);
    buttonApply->setEnabled(true);
    if (userNameEdit->text() == "") {
        refresh();
    }
}

void MConfig::on_groupNameEdit_textEdited() {
    deleteBox->setEnabled(false);
    buttonApply->setEnabled(true);
    if (groupNameEdit->text() == "") {
        refresh();
    }
}

void MConfig::on_deleteGroupCombo_activated() {
    addBox->setEnabled(false);
    buttonApply->setEnabled(true);
    if (deleteGroupCombo->currentText() == tr("none")) {
        refresh();
    }
}

void MConfig::on_userComboMembership_activated() {
    buildListGroups();
    buttonApply->setEnabled(true);
    if (userComboMembership->currentText() == tr("none")) {
        refresh();
    }
}


void MConfig::buildListGroups(){
    char line[130];
    FILE *fp;
    int i;
    listGroups->clear();
    //read /etc/group and add all the groups in the listGroups
    fp = popen("cat /etc/group | cut -f 1 -d :", "r");
    if (fp != NULL) {
        while (fgets(line, sizeof line, fp) != NULL) {
            i = strlen(line);
            line[--i] = '\0';
            if (line != NULL && strlen(line) > 1) {
                QListWidgetItem *item = new QListWidgetItem;
                item->setText(line);
                item->setCheckState(Qt::Unchecked);
                listGroups->addItem(item);
            }
        }
        pclose(fp);
    }
    //check the boxes for the groups that the current user belongs to
    QString cmd = QString("id -nG %1").arg(userComboMembership->currentText());
    QString out = getCmdOut(cmd);
    QStringList out_tok = out.split(" ");
    while (!out_tok.isEmpty()) {
        QString text = out_tok.takeFirst();
        QList<QListWidgetItem*> list = listGroups->findItems(text, Qt::MatchExactly);
        while (!list.isEmpty()) {
            list.takeFirst()->setCheckState(Qt::Checked);
        }
    }
}

void MConfig::displayDoc(QString url)
{
    QString exec = "xdg-open";
    QString user = getCmdOut("logname");
    if (system("command -v mx-viewer") == 0) { // use mx-viewer if available
        exec = "mx-viewer";
    }
    QString cmd = "su " + user + " -c \"" + exec + " " + url + "\"&";
    system(cmd.toUtf8());
}

// apply but do not close
void MConfig::on_buttonApply_clicked() {
    if (!buttonApply->isEnabled()) {
        return;
    }

    int i = tabWidget->currentIndex();

    switch (i) {

    case 1:
        setCursor(QCursor(Qt::WaitCursor));
        applyRestore();
        setCursor(QCursor(Qt::ArrowCursor));
        buttonApply->setEnabled(false);
        break;

    case 2:
        applyDesktop();
        buttonApply->setEnabled(false);
        break;

    case 3:
        setCursor(QCursor(Qt::WaitCursor));
        applyGroup();
        setCursor(QCursor(Qt::ArrowCursor));
        buttonApply->setEnabled(false);
        break;

    case 4:
        setCursor(QCursor(Qt::WaitCursor));
        applyMembership();
        setCursor(QCursor(Qt::ArrowCursor));
        break;

    default:
        setCursor(QCursor(Qt::WaitCursor));
        if (addUserBox->isEnabled()) {
            applyAdd();
        } else if (deleteUserBox->isEnabled()) {
            applyDelete();
            buttonApply->setEnabled(false);
        } else if (userBoxChangePass->isEnabled()) {
            applyChangePass();
        }
        setCursor(QCursor(Qt::ArrowCursor));
        break;
    }
}

// install and run baobab
void MConfig::on_baobabPushButton_clicked()
{
    if (system("[ -f /usr/bin/baobab ]") != 0) {
        setCursor(QCursor(Qt::BusyCursor));
        mbox = new QMessageBox();
        mbox->setWindowTitle(tr("Baobab installation"));
        mbox->setText(tr("Wait while Baobab is installing..."));
        mbox->setStandardButtons(0);
        this->hide();
        mbox->show();
        disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
        connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(installDone(int, QProcess::ExitStatus)));
        proc->start("/bin/bash  -c \"apt-get update && apt-get install baobab\"");
    } else {
        this->hide();
        system("baobab");
        this->show();
    }
}

// after installing baobab
void MConfig::installDone(int exitCode, QProcess::ExitStatus) {
    setCursor(QCursor(Qt::ArrowCursor));
    delete mbox;
    if (exitCode == 0) {
        system("baobab");
    }
    this->show();
}

void MConfig::on_tabWidget_currentChanged() {
    refresh();
}


// close but do not apply
void MConfig::on_buttonCancel_clicked() {
    close();
}

bool MConfig::hasInternetConnection()
{
    bool internetConnection  = false;
    // Query network interface status
    QStringList interfaceList  = getCmdOuts("ifconfig -a -s");
    int i=1;
    while (i<interfaceList.size()) {
        QString interface = interfaceList.at(i);
        interface = interface.left(interface.indexOf(" "));
        if ((interface != "lo") && (interface != "wmaster0") && (interface != "wifi0")) {
            QStringList ifStatus  = getCmdOuts(QString("ifconfig %1").arg(interface));
            QString unwrappedList = ifStatus.join(" ");
            if (unwrappedList.indexOf("UP ") != -1) {
                if (unwrappedList.indexOf(" RUNNING ") != -1) {
                    internetConnection  = true;
                }
            }
        }
        ++i;
    }
    return internetConnection;
}


// Get version of the program
QString MConfig::getVersion(QString name) {
    QString cmd = QString("dpkg -l %1 | awk 'NR==6 {print $3}'").arg(name);
    return getCmdOut(cmd);
}

// show about
void MConfig::on_buttonAbout_clicked() {
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX User Manager"), "<p align=\"center\"><b><h2>" +
                       tr("MX User Manager") + "</h2></b></p><p align=\"center\">" + "Version: " +
                       getVersion("mx-user") + "</p><p align=\"center\"><h3>" +
                       tr("Simple user configuration for MX Linux") + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>", 0, this);
    msgBox.addButton(tr("License"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    if (msgBox.exec() == QMessageBox::AcceptRole) {
        displayDoc("file:///usr/share/doc/mx-user/license.html");
    }
    this->show();
}

// Help button clicked
void MConfig::on_buttonHelp_clicked() {
    QLocale locale;
    QString lang = locale.bcp47Name();

    QString url = "https://mxlinux.org/wiki/help-files/help-mx-user-manager";

    if (lang.startsWith("fr")) {
        url = "https://mxlinux.org/wiki/help-files/help-gestionnaire-des-utilisateurs";
    }
    displayDoc(url);
}


void MConfig::restartPanel(QString user)
{
    QString cmd = QString("pkill xfconfd; sudo -Eu %1 bash -c 'xfce4-panel -r'").arg(user);
    system(cmd.toUtf8());
}


void MConfig::on_comboChangePass_activated()
{
    addUserBox->setEnabled(false);
    deleteUserBox->setEnabled(false);
    buttonApply->setEnabled(true);
    if (comboChangePass->currentText() == tr("none")) {
        refresh();
    }
}


void MConfig::on_toUserComboBox_activated()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_copyRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_syncRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_entireRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_docsRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_mozillaRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}

void MConfig::on_sharedRadioButton_clicked()
{
    buttonApply->setEnabled(true);
    syncProgressBar->setValue(0);
}



void MConfig::on_toUserComboBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 == tr("browse...")) {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder to copy to"), "/",QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
        if (dir != "") {
            toUserComboBox->removeItem(toUserComboBox->currentIndex());
            toUserComboBox->addItem(dir);
            int idx = toUserComboBox->findText(dir, Qt::MatchExactly | Qt::MatchCaseSensitive);
            toUserComboBox->setCurrentIndex(idx);
            toUserComboBox->addItem(tr("browse..."));
        } else {
            toUserComboBox->setCurrentIndex(toUserComboBox->currentIndex() - 1);
        }
    }
}


