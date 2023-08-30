// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"

#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QRegularExpression>

#include <chrono>

static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};

//! [0]
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
//! [0]
    m_status(new QLabel),
    m_settings(new SettingsDialog(this)),
    m_timer(new QTimer(this)),
//! [1]
    m_serial(new QSerialPort(this))
{
//! [1]
    m_ui->setupUi(this);

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);

    m_ui->statusBar->addWidget(m_status);

    initActionsConnections();

    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
    m_timer->setSingleShot(true);

//! [2]
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(m_serial, &QSerialPort::bytesWritten, this, &MainWindow::handleBytesWritten);
}
//! [3]

MainWindow::~MainWindow()
{
    delete m_settings;
    delete m_ui;
}


static void updateSerialValue(const QString& received_data, QRegularExpression& re, QLabel* label)
{
    QRegularExpressionMatch match1 = re.match(received_data);
    if (match1.hasMatch()) {
        double num1 = match1.captured(0).toDouble();
        label->setText(QString::number(num1, 'g', 10));
    }
}




//! [4]
void MainWindow::openSerialPort()
{
    const SettingsDialog::Settings p = m_settings->settings();
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if(m_serial->open(QIODevice::ReadWrite)){
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name, p.stringBaudRate, p.stringDataBits,
                               p.stringParity, p.stringStopBits, p.stringFlowControl));
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());
        showStatusMessage(tr("Open error"));
    }
}
//! [4]

//! [5]
void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}
//! [5]

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Serial Terminal"),
                       tr("The <b>Serial Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

//! [6]
void MainWindow::writeData(const QByteArray &data)
{
    const qint64 written = m_serial->write(data);
    if (written == data.size()) {
        m_bytesToWrite += written;
        m_timer->start(kWriteTimeout);
    } else {
        const QString error = tr("Failed to write all data to port %1.\n"
                                 "Error: %2").arg(m_serial->portName(),
                                                  m_serial->errorString());
        showWriteError(error);
    }
}
//! [6]

//! [7]
void MainWindow::readData()
{
    const QByteArray data = m_serial->readAll();  //reads serial data

    //received_data->append(data.toStdString());   //appends serial data
    received_data += data;

    if(received_data.contains("\n"))  //append until "\n" is received
    {
        if(received_data.contains(":"))
        {
            qInfo() << received_data;
            QStringList parameter = received_data.split(":");
            QRegularExpression re("-?\\d+(\\.\\d+)?"); //matches one or more digits, optionally preceded by a - sign and followed by a decimal point and one or more digits.

//            if(list[0] == "temperature") //list is not empty
//            {
//                QRegularExpressionMatch match1 = re.match(received_data); //used to search for the first occurrence of the regular expression within the input string.

//                if(match1.hasMatch())
//                {
//                    double num1 = match1.captured(0).toDouble(); //used to extract the matched substring
//                    m_ui->PowerStep_value->setText(QString::number(num1, 'g', 10)); //'g' format specifier to convert the value variable to a string without trailing zeros, 10 is the number of digits.
//                }
//            }

            if(parameter[0] == "PowerStep")
                updateSerialValue(received_data, re, m_ui->PowerStep_value);
            if(parameter[0] == "ErrorBuf")
                updateSerialValue(received_data, re, m_ui->ErrorBuf_value);
            if(parameter[0] == "Mains input")
                updateSerialValue(received_data, re, m_ui->Mains_input_value);
            if(parameter[0] == "AD Mains input")
                updateSerialValue(received_data, re, m_ui->AD_Mains_input_value);
            if(parameter[0] == "ADC Mains input value")
                updateSerialValue(received_data, re, m_ui->ADC_Mains_input_value_value);
            if(parameter[0] == "Current")
                updateSerialValue(received_data, re, m_ui->Current_value);
            if(parameter[0] == "ADC Current")
                updateSerialValue(received_data, re, m_ui->ADC_current_value);
            if(parameter[0] == "Power")
                updateSerialValue(received_data, re, m_ui->Power_value);
        }
        received_data.clear();
    }
}
//! [7]

//! [8]
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}
//! [8]

//! [9]
void MainWindow::handleBytesWritten(qint64 bytes)
{
    m_bytesToWrite -= bytes;
    if (m_bytesToWrite == 0)
        m_timer->stop();
}
//! [9]

void MainWindow::handleWriteTimeout()
{
    const QString error = tr("Write operation timed out for port %1.\n"
                             "Error: %2").arg(m_serial->portName(),
                                              m_serial->errorString());
    showWriteError(error);
}

void MainWindow::ClearSerialValue()
{
    m_ui->PowerStep_value->setText("0");
    m_ui->ErrorBuf_value->setText("0");
    m_ui->Mains_input_value->setText("0");
    m_ui->AD_Mains_input_value->setText("0");
    m_ui->ADC_Mains_input_value_value->setText("0");
    m_ui->Current_value->setText("0");
    m_ui->ADC_current_value->setText("0");
    m_ui->Power_value->setText("0");
}

void MainWindow::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionClear, &QAction::triggered, this, &MainWindow::ClearSerialValue);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}

void MainWindow::showWriteError(const QString &message)
{
    QMessageBox::warning(this, tr("Warning"), message);
}
