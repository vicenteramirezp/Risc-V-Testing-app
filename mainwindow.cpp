#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QScrollBar>
#include <QThread>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialPort(nullptr)
    , riscvConverter()
    , csvStream(&csvFile)
    , assemblyLoader(nullptr)  // Initialize pointer
{
    ui->setupUi(this);

    // Initialize CSV file
    initializeCsvFile();

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

    // For now, let's add it programmatically or you can add it in Qt Designer
    connect(ui->openAssemblyLoaderButton, &QPushButton::clicked, this, &MainWindow::openAssemblyLoader);

    // Initial refresh of available ports
    refreshSerialPorts();

    // Set initial state
    updateStatus("Status: Disconnected", false);
    PC_counter=false;
    // Update placeholder text for RISC-V instructions
    ui->sendLineEdit->setPlaceholderText("Enter RISC-V instruction (e.g., add x5, x6, x7)...");
}

void MainWindow::openAssemblyLoader()
{
    if (!assemblyLoader) {
        assemblyLoader = new AssemblyLoader(this);
        connect(assemblyLoader, &AssemblyLoader::instructionSelected,
                this, &MainWindow::handleInstructionFromLoader);
    }

    assemblyLoader->show();
    assemblyLoader->raise();
    assemblyLoader->activateWindow();
}

void MainWindow::handleInstructionFromLoader(const QString& instruction)
{
    // Set the instruction in the send line edit and send it
    ui->sendLineEdit->setText(instruction);
    sendData();
}


void MainWindow::initializeCsvFile()
{
    QString fileName = "memory_map.csv";
    QString filePath = QDir::current().filePath(fileName);

    csvFile.setFileName(filePath);

    if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        csvStream << "DataValue\n";
        csvStream.flush();

        appendToLog(QString("Memory map CSV created: %1").arg(filePath));
        qDebug() << "Memory map CSV created:" << filePath;
    } else {
        QMessageBox::warning(this, "File Error",
                             QString("Failed to create CSV file: %1").arg(csvFile.errorString()));
        qDebug() << "Failed to create CSV file:" << csvFile.errorString();
    }
}


void MainWindow::writeToCsv(quint32 address, quint32 dataValue)
{
    if (!csvFile.isOpen()) {
        qDebug() << "CSV file not open!";
        return;
    }

    // Calculate target row (address / 4)
    int targetRow = address / 4;

    // Read current file content to find where we are
    csvFile.close();

    QVector<QString> lines;
    if (csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&csvFile);
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        csvFile.close();
    }

    // Ensure we have enough rows (including header)
    while (lines.size() <= targetRow + 1) {  // +1 for header
        if (lines.isEmpty()) {
            lines.append("DataValue");  // Header
        } else {
            lines.append("0x00000000");  // Default value
        }
    }

    // Update the specific row
    lines[targetRow + 1] = QString("0x%1").arg(dataValue, 8, 16, QChar('0'));

    // Write back
    if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        for (const QString& line : lines) {
            csvStream << line << "\n";
        }
        csvStream.flush();

        qDebug() << "Memory updated - Address:" << QString("0x%1").arg(address, 8, 16, QChar('0'))
                 << "-> Row:" << (targetRow + 1) << "Data:" << lines[targetRow + 1];
    }
}
MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }

    // Close CSV file
    if (csvFile.isOpen()) {
        csvFile.close();
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

    PC_counter=true;
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
        // Sw instruction
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

            quint32 readWriteValue = (quint8)receiveBuffer[0];
            receiveBuffer.remove(0, 1);
            if(readWriteValue==65){
                appendToLog("Read/Write:Write", false);
            } else {
                appendToLog("Read/Write:Read", false);
            }


            chunk = receiveBuffer.left(4);

            // Convert from little-endian bytes to 32-bit integer
            quint32 dataValue = (quint32)((unsigned char)chunk[0]) |
                                ((quint32)((unsigned char)chunk[1]) << 8) |
                                ((quint32)((unsigned char)chunk[2]) << 16) |
                                ((quint32)((unsigned char)chunk[3]) << 24);

            receiveBuffer.remove(0, 4);
            receivedData = QString("Data Value: 0x%1 (%2)").arg(dataValue, 8, 16, QChar('0')).arg(dataValue);
            appendToLog(receivedData, false);

            // Write to CSV - address and data value only
            writeToCsv(Address, dataValue);
        }
        // lw instruction
        if (receiveBuffer.size() >= 4) {
            if (PC_counter) { //Program counter Request
                // Try to process as 32-bit data first
                QByteArray chunk = receiveBuffer.left(4);

                // Convert from little-endian bytes to 32-bit integer
                quint32 value = (quint32)((unsigned char)chunk[0]) |
                                ((quint32)((unsigned char)chunk[1]) << 8) |
                                ((quint32)((unsigned char)chunk[2]) << 16) |
                                ((quint32)((unsigned char)chunk[3]) << 24);

                receiveBuffer.remove(0, 4);
                QString receivedData = QString("Program counter: 0x%1 (%2)").arg(value, 8, 16, QChar('0')).arg(value);
                appendToLog(receivedData, false);
                PC_counter=false;
                continue; // Continue processing next chunk
            } else
                {
                QByteArray chunk = receiveBuffer.left(4);

                // Convert from little-endian bytes to 32-bit integer
                quint32 address = (quint32)((unsigned char)chunk[0]) |
                                ((quint32)((unsigned char)chunk[1]) << 8) |
                                ((quint32)((unsigned char)chunk[2]) << 16) |
                                ((quint32)((unsigned char)chunk[3]) << 24);


                receiveBuffer.remove(0, 4);
                QString receivedData = QString("Address: 0x%1 (%2)").arg(address, 8, 16, QChar('0')).arg(address);
                appendToLog(receivedData, false);

                // Read value from CSV and send it back
                quint32 dataValue = readFromCsv(address);

                // Send the data value back through serial port
                QByteArray responseData;
                responseData.resize(4);
                responseData[0] = (dataValue >> 0) & 0xFF;   // LSB
                responseData[1] = (dataValue >> 8) & 0xFF;
                responseData[2] = (dataValue >> 16) & 0xFF;
                responseData[3] = (dataValue >> 24) & 0xFF;  // MSB

                quint32  size = (quint8)receiveBuffer[0];
                receiveBuffer.remove(0, 1);

                receivedData = QString("Size: 0x%1 (%2)").arg(size, 2, 16, QChar('0')).arg(size);
                appendToLog(receivedData, false);
                serialPort->write(responseData);

                QString responseLog = QString("Sent data value: 0x%1 for address: 0x%2")
                                          .arg(dataValue, 8, 16, QChar('0'))
                                          .arg(address, 8, 16, QChar('0'));
                appendToLog(responseLog, true);
                }
        }



        // If we get here, process as 8-bit data
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

quint32 MainWindow::readFromCsv(quint32 address)
{
    if (!csvFile.isOpen()) {
        qDebug() << "CSV file not open!";
        return 0;
    }

    // Calculate target row (address / 4)
    int targetRow = address / 4;

    // Read current file content
    csvFile.close();

    quint32 dataValue = 0;

    if (csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&csvFile);
        int currentRow = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();

            // Skip header row and check if we're at the target row
            if (currentRow == targetRow + 1 && currentRow > 0) {
                // Parse the hex value
                bool ok;
                dataValue = line.toUInt(&ok, 16);
                if (!ok) {
                    qDebug() << "Failed to parse CSV value:" << line;
                    dataValue = 0;
                }
                break;
            }
            currentRow++;
        }
        csvFile.close();

        // Reopen for writing if needed later
        csvFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    }

    qDebug() << "Read from CSV - Address:" << QString("0x%1").arg(address, 8, 16, QChar('0'))
             << "-> Row:" << (targetRow + 1) << "Data:" << QString("0x%1").arg(dataValue, 8, 16, QChar('0'));

    return dataValue;
}
