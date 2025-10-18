#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QScrollBar>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialPort(nullptr)
    , riscvConverter()  // Initialize the converter
{
    ui->setupUi(this);

    // Initialize serial port
    serialPort = new QSerialPort(this);

    // Connect signals and slots
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectSerialPort);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectSerialPort);
    connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleSerialError);
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readData);

    // New connections for log/send functionality
    connect(ui->getPC, &QPushButton::clicked, this, &MainWindow::getPC);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendData);
    connect(ui->clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(ui->sendLineEdit, &QLineEdit::returnPressed, this, &MainWindow::sendData);

    // Initial refresh of available ports
    refreshSerialPorts();

    // Set initial state
    updateStatus("Status: Disconnected", false);

    // Update placeholder text for RISC-V instructions
    ui->sendLineEdit->setPlaceholderText("Enter RISC-V instruction (e.g., add x5, x6, x7)...");
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

    // Set common serial port settings
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

        // Log connection
        appendToLog(QString("Connected to %1").arg(selectedPort));
    } else {
        QMessageBox::critical(this, "Connection Error",
                              QString("Failed to connect to %1: %2").arg(selectedPort).arg(serialPort->errorString()));
        updateStatus("Status: Connection failed", false);
    }
}

void MainWindow::disconnectSerialPort()
{
    if (serialPort->isOpen()) {
        appendToLog("Disconnected from serial port");
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

void MainWindow::sendData()
{
    if (!serialPort || !serialPort->isOpen()) {
        QMessageBox::warning(this, "Send Error", "Not connected to any serial port.");
        return;
    }

    QString instruction = ui->sendLineEdit->text().trimmed();
    if (instruction.isEmpty()) {
        return;
    }

    // Convert RISC-V instruction to machine code using our private converter
    quint32 machineCode;
    QString errorMessage;

    if (!riscvConverter.convertToMachineCode(instruction, machineCode, errorMessage)) {
        QMessageBox::warning(this, "Instruction Error",
                             QString("Invalid RISC-V instruction: %1").arg(errorMessage));
        return;
    }

    // Send protocol: first send byte 3 to trigger wait_for_inst state
    QByteArray protocolByte;
    protocolByte.resize(1);
    protocolByte[0] = 0x03;  // Trigger wait_for_inst state

    // Send instruction as 32-bit little-endian
    QByteArray instructionData;
    instructionData.resize(4);
    instructionData[0] = (machineCode >> 0) & 0xFF;   // LSB
    instructionData[1] = (machineCode >> 8) & 0xFF;
    instructionData[2] = (machineCode >> 16) & 0xFF;
    instructionData[3] = (machineCode >> 24) & 0xFF;  // MSB

    // Send protocol byte first
    qint64 protocolBytesWritten = serialPort->write(protocolByte);
    if (protocolBytesWritten == -1) {
        QMessageBox::critical(this, "Send Error",
                              QString("Failed to send protocol byte: %1").arg(serialPort->errorString()));
        return;
    }

    // Small delay to ensure protocol byte is processed
    QThread::msleep(10);

    // Send instruction data
    qint64 instructionBytesWritten = serialPort->write(instructionData);

    if (instructionBytesWritten == -1) {
        QMessageBox::critical(this, "Send Error",
                              QString("Failed to send instruction: %1").arg(serialPort->errorString()));
    } else if (instructionBytesWritten != 4) {
        QMessageBox::warning(this, "Send Error",
                             QString("Only sent %1 out of 4 instruction bytes").arg(instructionBytesWritten));
    } else {
        // Log both protocol and instruction
        QString displayInstruction = QString("32-bit: %1 - %2").arg(riscvConverter.formatMachineCode(machineCode)).arg(instruction);

        appendToLog(displayInstruction, true);

        // Clear the input field after sending
        ui->sendLineEdit->clear();
    }
}

void MainWindow::getPC()
{
    if (!serialPort || !serialPort->isOpen()) {
        QMessageBox::warning(this, "Send Error", "Not connected to any serial port.");
        return;
    }

    // According to README: Send byte 2 to request Program Counter
    quint8 pcRequest = 2;
    QByteArray data;
    data.resize(1);
    data[0] = pcRequest;

    QString displayData = QString("8-bit: 0x%1 (%2) - PC Request").arg(pcRequest, 2, 16, QChar('0')).arg(pcRequest);

    qint64 bytesWritten = serialPort->write(data);

    if (bytesWritten == -1) {
        QMessageBox::critical(this, "Send Error",
                              QString("Failed to send data: %1").arg(serialPort->errorString()));
    } else if (bytesWritten != data.size()) {
        QMessageBox::warning(this, "Send Error",
                             QString("Only sent %1 out of %2 bytes").arg(bytesWritten).arg(data.size()));
    } else {
        // Log sent data
        appendToLog(displayData, true);
    }
}

void MainWindow::readData()
{
    if (!serialPort || !serialPort->isOpen()) {
        return;
    }

    // Accumulate received bytes
    receiveBuffer += serialPort->readAll();

    // Process data based on buffer content
    while (!receiveBuffer.isEmpty()) {
        // Check if we have at least 4 bytes for 32-bit data
        if (receiveBuffer.size() >= 10) {
            // Try to process as 32-bit data first
            QByteArray chunk = receiveBuffer.left(4);

            // Convert from little-endian bytes to 32-bit integer
            quint32 Address = (quint32)((unsigned char)chunk[0]) |
                            ((quint32)((unsigned char)chunk[1]) << 8) |
                            ((quint32)((unsigned char)chunk[2]) << 16) |
                            ((quint32)((unsigned char)chunk[3]) << 24);


            receiveBuffer.remove(0, 4);
            QString receivedData = QString("Address: 0x%1 (%2)").arg(Address, 8, 16, QChar('0')).arg(Address);
            appendToLog(receivedData, false);


            quint32 value = (quint8)receiveBuffer[0];
            receiveBuffer.remove(0, 1);

             receivedData = QString("Read/Write: 0x%1 (%2)").arg(value, 2, 16, QChar('0')).arg(value);
            appendToLog(receivedData, false);

            chunk = receiveBuffer.left(4);

            // Convert from little-endian bytes to 32-bit integer
            value = (quint32)((unsigned char)chunk[0]) |
                            ((quint32)((unsigned char)chunk[1]) << 8) |
                            ((quint32)((unsigned char)chunk[2]) << 16) |
                            ((quint32)((unsigned char)chunk[3]) << 24);


            receiveBuffer.remove(0, 4);
            receivedData = QString("Data Value: 0x%1 (%2)").arg(value, 8, 16, QChar('0')).arg(value);
            appendToLog(receivedData, false);
        }

        if (receiveBuffer.size() >= 4) {
            // Try to process as 32-bit data first
            QByteArray chunk = receiveBuffer.left(4);

            // Convert from little-endian bytes to 32-bit integer
            quint32 value = (quint32)((unsigned char)chunk[0]) |
                            ((quint32)((unsigned char)chunk[1]) << 8) |
                            ((quint32)((unsigned char)chunk[2]) << 16) |
                            ((quint32)((unsigned char)chunk[3]) << 24);

            // Check if this might be valid 32-bit data
            // You might want to adjust this logic based on your protocol
            bool isLikely32Bit = true;

            // Optional: Add validation logic here based on your protocol
            // For example, check if the value is within expected ranges

            if (isLikely32Bit) {
                receiveBuffer.remove(0, 4);
                QString receivedData = QString("Program counter: 0x%1 (%2)").arg(value, 8, 16, QChar('0')).arg(value);
                appendToLog(receivedData, false);
                continue; // Continue processing next chunk
            }
        }

        // If we get here, process as 8-bit data
        // Process single byte as 8-bit data
        quint8 value = (quint8)receiveBuffer[0];
        receiveBuffer.remove(0, 1);

        QString receivedData = QString("8-bit: 0x%1 (%2)").arg(value, 2, 16, QChar('0')).arg(value);
        appendToLog(receivedData, false);

        // Special handling for reset confirmation (8'b1 = 0x01)
        if (value == 0x01) {
            appendToLog("*** CPU Ready Confirmation received ***", false);
        }
    }
}

void MainWindow::appendToLog(const QString &data, bool isSent)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    QString timeStr = timestamp.toString("hh:mm:ss.zzz");

    QString direction = isSent ? "SENT" : "RECV";
    QString color = isSent ? "blue" : "green";

    QString logEntry = QString("[%1] <span style='color:%2;'>%3:</span> %4")
                           .arg(timeStr, color, direction, data.toHtmlEscaped());

    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(logEntry + "<br>");

    QScrollBar *scrollBar = ui->logTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::clearLog()
{
    ui->logTextEdit->clear();
}
