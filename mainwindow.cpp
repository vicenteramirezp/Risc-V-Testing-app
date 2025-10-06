#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialPort(nullptr)
{
    ui->setupUi(this);

    // Initialize serial port
    serialPort = new QSerialPort(this);

    // Connect signals and slots
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectSerialPort);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectSerialPort);
    connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleSerialError);

    // Initial refresh of available ports
    refreshSerialPorts();

    // Set initial state
    updateStatus("Status: Disconnected", false);
}

MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    delete ui;
}

void MainWindow::refreshSerialPorts()
{
    ui->serialPortComboBox->clear();

    // Get available serial ports
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        ui->serialPortComboBox->addItem("No serial ports found");
        ui->connectButton->setEnabled(false);
    } else {
        for (const QSerialPortInfo &port : ports) {
            QString portInfo = QString("%1 - %2").arg(port.portName()).arg(port.description());
            ui->serialPortComboBox->addItem(portInfo, port.portName());
        }
        ui->connectButton->setEnabled(true);
    }
}

void MainWindow::connectSerialPort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    QString selectedPort = ui->serialPortComboBox->currentData().toString();

    if (selectedPort.isEmpty()) {
        QMessageBox::warning(this, "Connection Error", "Please select a valid serial port.");
        return;
    }

    serialPort->setPortName(selectedPort);

    // Set common serial port settings (you can make these configurable)
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (serialPort->open(QIODevice::ReadWrite)) {
        updateStatus(QString("Status: Connected to %1").arg(selectedPort), true);
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(true);
        ui->refreshButton->setEnabled(false);
        ui->serialPortComboBox->setEnabled(false);
    } else {
        QMessageBox::critical(this, "Connection Error",
                              QString("Failed to connect to %1: %2").arg(selectedPort).arg(serialPort->errorString()));
        updateStatus("Status: Connection failed", false);
    }
}

void MainWindow::disconnectSerialPort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    updateStatus("Status: Disconnected", false);
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->refreshButton->setEnabled(true);
    ui->serialPortComboBox->setEnabled(true);
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, "Serial Port Error",
                              QString("Serial port error: %1").arg(serialPort->errorString()));
        disconnectSerialPort();
    }
}

void MainWindow::updateStatus(const QString &message, bool isConnected)
{
    ui->statusLabel->setText(message);

    // Update style based on connection status
    QString styleSheet;
    if (isConnected) {
        styleSheet = "margin-top: 10px; padding: 5px; background-color: #d4edda; border: 1px solid #c3e6cb; color: #155724;";
    } else {
        styleSheet = "margin-top: 10px; padding: 5px; background-color: #f8d7da; border: 1px solid #f5c6cb; color: #721c24;";
    }
    ui->statusLabel->setStyleSheet(styleSheet);
}
