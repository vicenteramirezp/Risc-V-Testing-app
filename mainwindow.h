#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "riscvmachinecodeconverter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshSerialPorts();
    void connectSerialPort();
    void disconnectSerialPort();
    void handleSerialError(QSerialPort::SerialPortError error);
    void sendData();
    void readData();
    void clearLog();
    void getPC();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    QByteArray receiveBuffer;
    RiscVMachineCodeConverter riscvConverter;  // Private converter instance


    void updateStatus(const QString &message, bool isConnected = false);
    void appendToLog(const QString &data, bool isSent = false);
};

#endif // MAINWINDOW_H
