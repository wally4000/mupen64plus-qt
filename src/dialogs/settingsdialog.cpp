/***
 * Copyright (c) 2013, Dan Hasting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organization nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ***/

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "../global.h"
#include "../common.h"

#include <QDesktopWidget>
#include <QFileDialog>
#include <QListWidget>
#include <QTranslator>


SettingsDialog::SettingsDialog(QWidget *parent, int activeTab) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(activeTab);


    //Populate Paths tab
    ui->emulatorPath->setText(settings.value("Paths/mupen64plus", "").toString());
    ui->pluginPath->setText(settings.value("Paths/plugins", "").toString());
    ui->dataPath->setText(settings.value("Paths/data", "").toString());
    ui->configPath->setText(settings.value("Paths/config", "").toString());

    QStringList romDirectories = settings.value("Paths/roms", "").toString().split("|");
    romDirectories.removeAll("");
    foreach (QString directory, romDirectories)
        ui->romList->addItem(directory);

    connect(ui->emulatorButton, SIGNAL(clicked()), this, SLOT(browseEmulator()));
    connect(ui->pluginButton, SIGNAL(clicked()), this, SLOT(browsePlugin()));
    connect(ui->dataButton, SIGNAL(clicked()), this, SLOT(browseData()));
    connect(ui->configButton, SIGNAL(clicked()), this, SLOT(browseConfig()));
    connect(ui->romAddButton, SIGNAL(clicked()), this, SLOT(addRomDirectory()));
    connect(ui->romRemoveButton, SIGNAL(clicked()), this, SLOT(removeRomDirectory()));


    //Populate Emulation tab
    QString emuMode = settings.value("Emulation/mode", "").toString();
    if (emuMode == "0")
        ui->pureButton->setChecked(true);
    else if (emuMode == "1")
        ui->cachedButton->setChecked(true);
    else
        ui->dynamicButton->setChecked(true);


    //Populate Graphics tab
    if (settings.value("Graphics/osd", "true").toString() == "true")
        ui->osdOption->setChecked(true);
    if (settings.value("Graphics/fullscreen", "").toString() == "true")
        ui->fullscreenOption->setChecked(true);

    QStringList useableModes, modes;
    useableModes << tr("default"); //Allow users to use the screen resolution set in the config file

    modes << "2560x1600"
          << "2560x1440"
          << "2048x1152"
          << "1920x1200"
          << "1920x1080"
          << "1680x1050"
          << "1600x1200"
          << "1600x900"
          << "1440x900"
          << "1400x1050"
          << "1366x768"
          << "1360x768"
          << "1280x1024"
          << "1280x960"
          << "1280x800"
          << "1280x768"
          << "1280x720"
          << "1152x864"
          << "1024x768"
          << "1024x600"
          << "800x600"
          << "640x480";


    desktop = new QDesktopWidget;
    int screenWidth = desktop->width();
    int screenHeight = desktop->height();

    foreach (QString mode, modes)
    {
        QStringList values = mode.split("x");

        if (values.value(0).toInt() <= screenWidth && values.value(1).toInt() <= screenHeight)
            useableModes << mode;
    }

    ui->resolutionBox->insertItems(0, useableModes);
    int resIndex = useableModes.indexOf(settings.value("Graphics/resolution","").toString());
    if (resIndex >= 0) ui->resolutionBox->setCurrentIndex(resIndex);


    //Populate Plugins tab
    QStringList audioPlugins, inputPlugins, rspPlugins, videoPlugins;
    pluginsDir = QDir(settings.value("Paths/plugins", "").toString());

    if (pluginsDir.exists()) {
        QStringList files = pluginsDir.entryList(QStringList() << "*", QDir::Files);

        if (files.size() > 0) {
            foreach (QString fileName, files)
            {
                QString baseName = QFileInfo(fileName).completeBaseName();

                if (fileName.contains("-audio-"))
                    audioPlugins << baseName;
                else if (fileName.contains("-input-"))
                    inputPlugins << baseName;
                else if (fileName.contains("-rsp-"))
                    rspPlugins << baseName;
                else if (fileName.contains("-video-"))
                    videoPlugins << baseName;
            }
        }
    }

    ui->videoBox->insertItems(0, videoPlugins);
    ui->audioBox->insertItems(0, audioPlugins);
    ui->inputBox->insertItems(0, inputPlugins);
    ui->rspBox->insertItems(0, rspPlugins);

    //Set Rice as default
    QString videoDefault = "";
    if (videoPlugins.contains("mupen64plus-video-rice"))
        videoDefault = "mupen64plus-video-rice";

    int videoIndex = videoPlugins.indexOf(settings.value("Plugins/video",videoDefault).toString());
    int audioIndex = audioPlugins.indexOf(settings.value("Plugins/audio","").toString());
    int inputIndex = inputPlugins.indexOf(settings.value("Plugins/input","").toString());
    int rspIndex = rspPlugins.indexOf(settings.value("Plugins/rsp","").toString());

    if (videoIndex >= 0) ui->videoBox->setCurrentIndex(videoIndex);
    if (audioIndex >= 0) ui->audioBox->setCurrentIndex(audioIndex);
    if (inputIndex >= 0) ui->inputBox->setCurrentIndex(inputIndex);
    if (rspIndex >= 0) ui->rspBox->setCurrentIndex(rspIndex);


    //Populate Table tab
    int tableSizeIndex = 0;
    QString currentTableSize = settings.value("Table/imagesize","Medium").toString();

    QList<QStringList> sizes;
    sizes << (QStringList() << tr("Extra Small") << "Extra Small")
          << (QStringList() << tr("Small")       << "Small")
          << (QStringList() << tr("Medium")      << "Medium")
          << (QStringList() << tr("Large")       << "Large")
          << (QStringList() << tr("Extra Large") << "Extra Large")
          << (QStringList() << tr("Super")       << "Super");


    if (settings.value("Other/downloadinfo", "").toString() == "true")
        populateTableAndListTab(true);
    else
        populateTableAndListTab(false);

    if (settings.value("Table/stretchfirstcolumn", "true").toString() == "true")
        ui->tableStretchOption->setChecked(true);

    for (int i = 0; i < sizes.length(); i++)
    {
        ui->tableSizeBox->insertItem(i, sizes.at(i).at(0), sizes.at(i).at(1));
        if (currentTableSize == sizes.at(i).at(1))
            tableSizeIndex = i;
    }
    if (tableSizeIndex >= 0) ui->tableSizeBox->setCurrentIndex(tableSizeIndex);

    connect(ui->tableAddButton, SIGNAL(clicked()), this, SLOT(tableAddColumn()));
    connect(ui->tableRemoveButton, SIGNAL(clicked()), this, SLOT(tableRemoveColumn()));
    connect(ui->tableSortUpButton, SIGNAL(clicked()), this, SLOT(tableSortUp()));
    connect(ui->tableSortDownButton, SIGNAL(clicked()), this, SLOT(tableSortDown()));


    //Populate Grid tab
    int gridSizeIndex = 0, activeIndex = 0, inactiveIndex = 0, labelColorIndex = 0, bgThemeIndex = 0;
    QString currentGridSize = settings.value("Grid/imagesize","Medium").toString();
    QString currentActiveColor = settings.value("Grid/activecolor","Cyan").toString();
    QString currentInactiveColor = settings.value("Grid/inactivecolor","Black").toString();
    QString currentLabelColor = settings.value("Grid/labelcolor","White").toString();
    QString currentBGTheme = settings.value("Grid/theme","Normal").toString();

    QList<QStringList> colors;
    colors << (QStringList() << tr("Black")      << "Black")
           << (QStringList() << tr("White")      << "White")
           << (QStringList() << tr("Light Gray") << "Light Gray")
           << (QStringList() << tr("Dark Gray")  << "Dark Gray")
           << (QStringList() << tr("Green")      << "Green")
           << (QStringList() << tr("Cyan")       << "Cyan")
           << (QStringList() << tr("Blue")       << "Blue")
           << (QStringList() << tr("Purple")     << "Purple")
           << (QStringList() << tr("Red")        << "Red")
           << (QStringList() << tr("Pink")       << "Pink")
           << (QStringList() << tr("Orange")     << "Orange")
           << (QStringList() << tr("Yellow")     << "Yellow")
           << (QStringList() << tr("Brown")      << "Brown");

    QList<QStringList> bgThemes;
    bgThemes << (QStringList() << tr("Light")   << "Light")
             << (QStringList() << tr("Normal")  << "Normal")
             << (QStringList() << tr("Dark")    << "Dark");

    for (int i = 0; i < sizes.length(); i++)
    {
        ui->gridSizeBox->insertItem(i, sizes.at(i).at(0), sizes.at(i).at(1));
        if (currentGridSize == sizes.at(i).at(1))
            gridSizeIndex = i;
    }
    if (gridSizeIndex >= 0) ui->gridSizeBox->setCurrentIndex(gridSizeIndex);

    int gridColumnCount = settings.value("Grid/columncount","4").toInt();
    ui->columnCountBox->setValue(gridColumnCount);

    if (settings.value("Grid/autocolumns", "true").toString() == "true") {
        toggleGridColumn(true);
        ui->autoColumnOption->setChecked(true);
    } else
        toggleGridColumn(false);

    for (int i = 0; i < colors.length(); i++)
    {
        ui->shadowActiveBox->insertItem(i, colors.at(i).at(0), colors.at(i).at(1));
        if (currentActiveColor == colors.at(i).at(1))
            activeIndex = i;

        ui->shadowInactiveBox->insertItem(i, colors.at(i).at(0), colors.at(i).at(1));
        if (currentInactiveColor == colors.at(i).at(1))
            inactiveIndex = i;

        ui->labelColorBox->insertItem(i, colors.at(i).at(0), colors.at(i).at(1));
        if (currentLabelColor == colors.at(i).at(1))
            labelColorIndex = i;
    }
    if (activeIndex >= 0) ui->shadowActiveBox->setCurrentIndex(activeIndex);
    if (inactiveIndex >= 0) ui->shadowInactiveBox->setCurrentIndex(inactiveIndex);
    if (labelColorIndex >= 0) ui->labelColorBox->setCurrentIndex(labelColorIndex);

    //Widgets to enable when label active
    labelEnable << ui->labelTextLabel
                << ui->labelTextBox
                << ui->labelColorLabel
                << ui->labelColorBox;

    if (settings.value("Grid/label", "true").toString() == "true") {
        toggleLabel(true);
        ui->labelOption->setChecked(true);
    } else
        toggleLabel(false);

    for (int i = 0; i < bgThemes.length(); i++)
    {
        ui->bgThemeBox->insertItem(i, bgThemes.at(i).at(0), bgThemes.at(i).at(1));
        if (currentBGTheme == bgThemes.at(i).at(1))
            bgThemeIndex = i;
    }
    if (bgThemeIndex >= 0) ui->bgThemeBox->setCurrentIndex(bgThemeIndex);

    QString imagePath = settings.value("Grid/background", "").toString();
    ui->backgroundPath->setText(imagePath);
    hideBGTheme(imagePath);

    if (settings.value("Grid/sortdirection", "ascending").toString() == "descending")
        ui->gridDescendingOption->setChecked(true);

    connect(ui->autoColumnOption, SIGNAL(toggled(bool)), this, SLOT(toggleGridColumn(bool)));
    connect(ui->backgroundPath, SIGNAL(textChanged(QString)), this, SLOT(hideBGTheme(QString)));
    connect(ui->backgroundButton, SIGNAL(clicked()), this, SLOT(browseBackground()));
    connect(ui->labelOption, SIGNAL(toggled(bool)), this, SLOT(toggleLabel(bool)));


    //Populate List tab
    int listSizeIndex = 0, listTextIndex = 0, listThemeIndex = 0;
    QString currentListSize = settings.value("List/imagesize","Medium").toString();
    QString currentListText = settings.value("List/textsize","Medium").toString();
    QString currentListTheme = settings.value("List/theme","Light").toString();

    QList<QStringList> themes;
    themes << (QStringList() << tr("Light") << "Light")
           << (QStringList() << tr("Dark")  << "Dark");

    listCoverEnable << ui->listSizeLabel
                    << ui->listSizeBox;

    if (settings.value("List/displaycover", "").toString() == "true") {
        toggleListCover(true);
        ui->listCoverOption->setChecked(true);
    } else
        toggleListCover(false);

    if (settings.value("List/firstitemheader", "true").toString() == "true")
        ui->listHeaderOption->setChecked(true);

    for (int i = 0; i < sizes.length(); i++)
    {
        ui->listSizeBox->insertItem(i, sizes.at(i).at(0), sizes.at(i).at(1));
        if (currentListSize == sizes.at(i).at(1))
            listSizeIndex = i;
        ui->listTextBox->insertItem(i, sizes.at(i).at(0), sizes.at(i).at(1));
        if (currentListText == sizes.at(i).at(1))
            listTextIndex = i;
    }
    if (listSizeIndex >= 0) ui->listSizeBox->setCurrentIndex(listSizeIndex);
    if (listTextIndex >= 0) ui->listTextBox->setCurrentIndex(listTextIndex);

    for (int i = 0; i < themes.length(); i++)
    {
        ui->listThemeBox->insertItem(i, themes.at(i).at(0), themes.at(i).at(1));
        if (currentListTheme == themes.at(i).at(1))
            listThemeIndex = i;
    }
    if (listThemeIndex >= 0) ui->listThemeBox->setCurrentIndex(listThemeIndex);

    if (settings.value("List/sortdirection", "ascending").toString() == "descending")
        ui->listDescendingOption->setChecked(true);


    connect(ui->listCoverOption, SIGNAL(toggled(bool)), this, SLOT(toggleListCover(bool)));
    connect(ui->listAddButton, SIGNAL(clicked()), this, SLOT(listAddColumn()));
    connect(ui->listRemoveButton, SIGNAL(clicked()), this, SLOT(listRemoveColumn()));
    connect(ui->listSortUpButton, SIGNAL(clicked()), this, SLOT(listSortUp()));
    connect(ui->listSortDownButton, SIGNAL(clicked()), this, SLOT(listSortDown()));


    //Populate Other tab
    int languageIndex = 0;
    QString currentLanguage = settings.value("language", getDefaultLanguage()).toString();

    QList<QStringList> languages;
    languages << (QStringList() << QString::fromUtf8("English")  << "EN")
              << (QStringList() << QString::fromUtf8("Français") << "FR")
              << (QStringList() << QString::fromUtf8("Русский") << "RU");

    downloadEnable << ui->tableSizeLabel
                   << ui->tableSizeBox
                   << ui->listCoverOption
                   << ui->listSizeLabel
                   << ui->listSizeBox;

    if (settings.value("Other/downloadinfo", "").toString() == "true") {
        toggleDownload(true);
        ui->downloadOption->setChecked(true);
    } else
        toggleDownload(false);

    if (settings.value("saveoptions", "").toString() == "true")
        ui->saveOption->setChecked(true);

    ui->parametersLine->setText(settings.value("Other/parameters", "").toString());

    for (int i = 0; i < languages.length(); i++)
    {
        ui->languageBox->insertItem(i, languages.at(i).at(0), languages.at(i).at(1));
        if (currentLanguage == languages.at(i).at(1))
            languageIndex = i;
    }
    ui->languageBox->setCurrentIndex(languageIndex);

    ui->languageInfoLabel->setHidden(true);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    connect(ui->downloadOption, SIGNAL(toggled(bool)), this, SLOT(toggleDownload(bool)));
    connect(ui->downloadOption, SIGNAL(toggled(bool)), this, SLOT(populateTableAndListTab(bool)));
    connect(ui->languageBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateLanguageInfo()));


    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(editSettings()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}


SettingsDialog::~SettingsDialog()
{
    delete ui;
}


void SettingsDialog::addColumn(QListWidget *currentList, QListWidget *availableList)
{
    int row = availableList->currentRow();

    if (row >= 0)
        currentList->addItem(availableList->takeItem(row));
}


void SettingsDialog::addRomDirectory()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("ROM Directory"));
    if (path != "") {
        //check for duplicates
        bool found = false;
        foreach (QListWidgetItem *item, ui->romList->findItems("*", Qt::MatchWildcard))
            if (path == item->text())
                found = true;

        if (!found)
            ui->romList->addItem(path);
    }
}


void SettingsDialog::browseBackground()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Background Image"));
    if (path != "")
        ui->backgroundPath->setText(path);
}


void SettingsDialog::browseEmulator()
{
    QString path = QFileDialog::getOpenFileName(this, tr("<ParentName> Executable")
                                                .replace("<ParentName>",ParentName));
    if (path != "")
        ui->emulatorPath->setText(path);

#ifdef Q_OS_OSX
    //Allow OSX users to just select the .app directory and auto-populate for them
    if (path.right(15) == "mupen64plus.app") {
        ui->emulatorPath->setText(path+"/Contents/MacOS/mupen64plus");
        ui->pluginPath->setText(path+"/Contents/MacOS");
        ui->dataPath->setText(path+"/Contents/Resources");
    }
#endif
}


void SettingsDialog::browsePlugin()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Plugin Directory"));
    if (path != "")
        ui->pluginPath->setText(path);

}


void SettingsDialog::browseData()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Data Directory"));
    if (path != "")
        ui->dataPath->setText(path);
}


void SettingsDialog::browseConfig()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Config Directory"));
    if (path != "")
        ui->configPath->setText(path);
}


void SettingsDialog::editSettings()
{
    //Set download option first
    if (ui->downloadOption->isChecked()) {
        settings.setValue("Other/downloadinfo", true);
        populateAvailable(true); //This removes thegamesdb.net options if user unselects download
    } else {
        settings.setValue("Other/downloadinfo", "");
        populateAvailable(false);
    }


    //Paths tab
    settings.setValue("Paths/mupen64plus", ui->emulatorPath->text());
    settings.setValue("Paths/plugins", ui->pluginPath->text());
    settings.setValue("Paths/data", ui->dataPath->text());
    settings.setValue("Paths/config", ui->configPath->text());

    QStringList romDirectories;
    foreach (QListWidgetItem *item, ui->romList->findItems("*", Qt::MatchWildcard))
        romDirectories << item->text();

    settings.setValue("Paths/roms", romDirectories.join("|"));


    //Emulation tab
    if (ui->pureButton->isChecked())
        settings.setValue("Emulation/mode", "0");
    else if (ui->cachedButton->isChecked())
        settings.setValue("Emulation/mode", "1");
    else
        settings.setValue("Emulation/mode", "2");


    //Graphics tab
    if (ui->osdOption->isChecked())
        settings.setValue("Graphics/osd", true);
    else
        settings.setValue("Graphics/osd", "");

    if (ui->fullscreenOption->isChecked())
        settings.setValue("Graphics/fullscreen", true);
    else
        settings.setValue("Graphics/fullscreen", "");

    if (ui->resolutionBox->currentText() != "default")
        settings.setValue("Graphics/resolution", ui->resolutionBox->currentText());
    else
        settings.setValue("Graphics/resolution", "");


    //Plugins tab
    settings.setValue("Plugins/video", ui->videoBox->currentText());
    settings.setValue("Plugins/audio", ui->audioBox->currentText());
    settings.setValue("Plugins/input", ui->inputBox->currentText());
    settings.setValue("Plugins/rsp", ui->rspBox->currentText());


    //Table tab
    QStringList tableVisibleItems;
    foreach (QListWidgetItem *item, ui->tableCurrentList->findItems("*", Qt::MatchWildcard))
        if (available.contains(item->data(Qt::UserRole).toString()))
            tableVisibleItems << item->data(Qt::UserRole).toString();

    settings.setValue("Table/columns", tableVisibleItems.join("|"));

    if (ui->tableStretchOption->isChecked())
        settings.setValue("Table/stretchfirstcolumn", true);
    else
        settings.setValue("Table/stretchfirstcolumn", "");

    settings.setValue("Table/imagesize", ui->tableSizeBox->itemData(ui->tableSizeBox->currentIndex()));


    //Grid tab
    settings.setValue("Grid/imagesize", ui->gridSizeBox->itemData(ui->gridSizeBox->currentIndex()));
    settings.setValue("Grid/columncount", ui->columnCountBox->value());

    if (ui->autoColumnOption->isChecked())
        settings.setValue("Grid/autocolumns", true);
    else
        settings.setValue("Grid/autocolumns", "");

    settings.setValue("Grid/inactivecolor", ui->shadowInactiveBox->itemData(ui->shadowInactiveBox->currentIndex()));
    settings.setValue("Grid/activecolor", ui->shadowActiveBox->itemData(ui->shadowActiveBox->currentIndex()));
    settings.setValue("Grid/theme", ui->bgThemeBox->itemData(ui->bgThemeBox->currentIndex()));
    settings.setValue("Grid/background", ui->backgroundPath->text());

    if (ui->labelOption->isChecked())
        settings.setValue("Grid/label", true);
    else
        settings.setValue("Grid/label", "");

    settings.setValue("Grid/labeltext", ui->labelTextBox->itemData(ui->labelTextBox->currentIndex()));
    settings.setValue("Grid/labelcolor", ui->labelColorBox->itemData(ui->labelColorBox->currentIndex()));
    settings.setValue("Grid/sort", ui->gridSortBox->itemData(ui->gridSortBox->currentIndex()));

    if (ui->gridDescendingOption->isChecked())
        settings.setValue("Grid/sortdirection", "descending");
    else
        settings.setValue("Grid/sortdirection", "ascending");


    //List tab
    QStringList listVisibleItems;
    foreach (QListWidgetItem *item, ui->listCurrentList->findItems("*", Qt::MatchWildcard))
        if (available.contains(item->data(Qt::UserRole).toString()))
            listVisibleItems << item->data(Qt::UserRole).toString();

    settings.setValue("List/columns", listVisibleItems.join("|"));

    if (ui->listHeaderOption->isChecked())
        settings.setValue("List/firstitemheader", true);
    else
        settings.setValue("List/firstitemheader", "");

    if (ui->listCoverOption->isChecked() && ui->downloadOption->isChecked())
        settings.setValue("List/displaycover", true);
    else
        settings.setValue("List/displaycover", "");

    settings.setValue("List/imagesize", ui->listSizeBox->itemData(ui->listSizeBox->currentIndex()));
    settings.setValue("List/textsize", ui->listTextBox->itemData(ui->listTextBox->currentIndex()));
    settings.setValue("List/theme", ui->listThemeBox->itemData(ui->listThemeBox->currentIndex()));
    settings.setValue("List/sort", ui->listSortBox->itemData(ui->listSortBox->currentIndex()));

    if (ui->listDescendingOption->isChecked())
        settings.setValue("List/sortdirection", "descending");
    else
        settings.setValue("List/sortdirection", "ascending");


    //Other tab
    if (ui->saveOption->isChecked())
        settings.setValue("saveoptions", true);
    else
        settings.setValue("saveoptions", "");

    settings.setValue("Other/parameters", ui->parametersLine->text());
    settings.setValue("language", ui->languageBox->itemData(ui->languageBox->currentIndex()));

    close();
}


void SettingsDialog::hideBGTheme(QString imagePath)
{
    if (imagePath == "") {
        ui->bgThemeLabel->setEnabled(true);
        ui->bgThemeBox->setEnabled(true);
    } else {
        ui->bgThemeLabel->setEnabled(false);
        ui->bgThemeBox->setEnabled(false);
    }
}


void SettingsDialog::listAddColumn()
{
    addColumn(ui->listCurrentList, ui->listAvailableList);
}


void SettingsDialog::listRemoveColumn()
{
    removeColumn(ui->listCurrentList, ui->listAvailableList);
}


void SettingsDialog::listSortDown()
{
    sortDown(ui->listCurrentList);
}


void SettingsDialog::listSortUp()
{
    sortUp(ui->listCurrentList);
}


void SettingsDialog::populateAvailable(bool downloadItems) {
    available.clear();
    labelOptions.clear();
    sortOptions.clear();

    available << "Filename"
              << "Filename (extension)"
              << "Zip File"
              << "GoodName"
              << "Internal Name"
              << "Size"
              << "MD5"
              << "CRC1"
              << "CRC2"
              << "Players"
              << "Rumble"
              << "Save Type";

    labelOptions << "Filename"
                 << "Filename (extension)"
                 << "GoodName"
                 << "Internal Name";

    sortOptions << "Filename"
                << "GoodName"
                << "Internal Name"
                << "Size";

    if (downloadItems) {
        available << "Game Title"
                  << "Release Date"
                  << "Overview"
                  << "ESRB"
                  << "Genre"
                  << "Publisher"
                  << "Developer"
                  << "Game Cover";

        labelOptions << "Game Title"
                     << "Release Date"
                     << "Genre";

        sortOptions << "Game Title"
                    << "Release Date"
                    << "ESRB"
                    << "Genre"
                    << "Publisher"
                    << "Developer";
    }

    available.sort();
    labelOptions.sort();
    sortOptions.sort();
}


void SettingsDialog::populateTableAndListTab(bool downloadItems)
{
    populateAvailable(downloadItems);

    QStringList tableCurrent, tableAvailable;
    tableCurrent = settings.value("Table/columns", "Filename|Size").toString().split("|");
    tableAvailable = available;

    foreach (QString cur, tableCurrent)
    {
        if (tableAvailable.contains(cur))
            tableAvailable.removeOne(cur);
        else //Someone added an invalid item
            tableCurrent.removeOne(cur);
    }

    ui->tableAvailableList->clear();
    foreach (QString listItem, tableAvailable)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(getTranslation(listItem));
        item->setData(Qt::UserRole, listItem);

        ui->tableAvailableList->addItem(item);
    }
    ui->tableAvailableList->sortItems();

    ui->tableCurrentList->clear();
    foreach (QString listItem, tableCurrent)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(getTranslation(listItem));
        item->setData(Qt::UserRole, listItem);

        ui->tableCurrentList->addItem(item);
    }


    //Grid sort field and label text
    int labelTextIndex = 0, gridSortIndex = 0;
    QString currentLabelText = settings.value("Grid/labeltext","Filename").toString();
    QString currentGridSort = settings.value("Grid/sort","Filename").toString();

    ui->labelTextBox->clear();
    for (int i = 0; i < labelOptions.length(); i++)
    {
        ui->labelTextBox->insertItem(i, getTranslation(labelOptions.at(i)), labelOptions.at(i));
        if (currentLabelText == labelOptions.at(i))
            labelTextIndex = i;
    }
    if (labelTextIndex >= 0) ui->labelTextBox->setCurrentIndex(labelTextIndex);

    ui->gridSortBox->clear();
    for (int i = 0; i < sortOptions.length(); i++)
    {
        ui->gridSortBox->insertItem(i, getTranslation(sortOptions.at(i)), sortOptions.at(i));
        if (currentGridSort == sortOptions.at(i))
            gridSortIndex = i;
    }
    if (gridSortIndex >= 0) ui->gridSortBox->setCurrentIndex(gridSortIndex);


    //List items and sort field
    QStringList listCurrent, listAvailable;
    listCurrent = settings.value("List/columns", "Filename|Internal Name|Size").toString().split("|");
    listAvailable = available;
    listAvailable.removeOne("Game Cover"); //Game Cover handled separately

    foreach (QString cur, listCurrent)
    {
        if (listAvailable.contains(cur))
            listAvailable.removeOne(cur);
        else //Someone added an invalid item
            listCurrent.removeOne(cur);
    }

    ui->listAvailableList->clear();
    foreach (QString listItem, listAvailable)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(getTranslation(listItem));
        item->setData(Qt::UserRole, listItem);

        ui->listAvailableList->addItem(item);
    }
    ui->listAvailableList->sortItems();

    ui->listCurrentList->clear();
    foreach (QString listItem, listCurrent)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(getTranslation(listItem));
        item->setData(Qt::UserRole, listItem);

        ui->listCurrentList->addItem(item);
    }

    int listSortIndex = 0;
    QString currentListSort = settings.value("List/sort","Filename").toString();

    ui->listSortBox->clear();
    for (int i = 0; i < sortOptions.length(); i++)
    {
        ui->listSortBox->insertItem(i, getTranslation(sortOptions.at(i)), sortOptions.at(i));
        if (currentListSort == sortOptions.at(i))
            listSortIndex = i;
    }
    if (listSortIndex >= 0) ui->listSortBox->setCurrentIndex(listSortIndex);
}


void SettingsDialog::removeColumn(QListWidget *currentList, QListWidget *availableList)
{
    int row = currentList->currentRow();

    if (row >= 0) {
        availableList->addItem(currentList->takeItem(row));
        availableList->sortItems();
    }
}


void SettingsDialog::removeRomDirectory()
{
    int row = ui->romList->currentRow();

    if (row >= 0)
        delete ui->romList->takeItem(row);
}



void SettingsDialog::sortDown(QListWidget *currentList)
{
    int row = currentList->currentRow();

    if (row > 0) {
        QListWidgetItem *item = currentList->takeItem(row);
        currentList->insertItem(row - 1, item);
        currentList->setCurrentRow(row - 1);
    }
}


void SettingsDialog::sortUp(QListWidget *currentList)
{
    int row = currentList->currentRow();

    if (row >= 0 && row < currentList->count() - 1) {
        QListWidgetItem *item = currentList->takeItem(row);
        currentList->insertItem(row + 1, item);
        currentList->setCurrentRow(row + 1);
    }
}


void SettingsDialog::tableAddColumn()
{
    addColumn(ui->tableCurrentList, ui->tableAvailableList);
}


void SettingsDialog::tableRemoveColumn()
{
    removeColumn(ui->tableCurrentList, ui->tableAvailableList);
}


void SettingsDialog::tableSortDown()
{
    sortDown(ui->tableCurrentList);
}


void SettingsDialog::tableSortUp()
{
    sortUp(ui->tableCurrentList);
}


void SettingsDialog::toggleDownload(bool active)
{
    foreach (QWidget *next, downloadEnable)
        next->setEnabled(active);

    if (active)
        toggleListCover(ui->listCoverOption->isChecked());
}


void SettingsDialog::toggleGridColumn(bool active)
{
    if (active)
        ui->columnCountBox->setEnabled(false);
    else
        ui->columnCountBox->setEnabled(true);
}


void SettingsDialog::toggleLabel(bool active)
{
    foreach (QWidget *next, labelEnable)
        next->setEnabled(active);
}


void SettingsDialog::toggleListCover(bool active)
{
    foreach (QWidget *next, listCoverEnable)
        next->setEnabled(active);
}


void SettingsDialog::updateLanguageInfo()
{
    ui->languageInfoLabel->setHidden(false);

    const char *sourceText = "<b>Note:</b> Language changes will not take place until application restart.";

    QTranslator translator;
    QString language = ui->languageBox->itemData(ui->languageBox->currentIndex()).toString().toLower();
    QString resource = ":/locale/"+AppNameLower+"_"+language+".qm";
    if (QFileInfo(resource).exists()) {
        translator.load(resource);
        ui->languageInfoLabel->setText(translator.translate("SettingsDialog", sourceText));
    } else
        ui->languageInfoLabel->setText(sourceText);
}
