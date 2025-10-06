#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>

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

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    void updateStatus(const QString &message, bool isConnected = false);
};

#endif // MAINWINDOW_H
