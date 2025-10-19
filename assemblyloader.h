#ifndef ASSEMBLYLOADER_H
#define ASSEMBLYLOADER_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QTextCursor>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
namespace Ui {
class AssemblyLoader;
}
QT_END_NAMESPACE

class AssemblyLoader : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssemblyLoader(QWidget *parent = nullptr);
    ~AssemblyLoader();

signals:
    void instructionSelected(const QString& instruction);

private slots:
    void loadAssemblyFile();
    void stepInstruction();
    void resetStepping();
    void sendCurrentInstruction();

private:
    Ui::AssemblyLoader *ui;
    QVector<QString> instructions;
    int currentInstructionIndex;
    
    void highlightCurrentInstruction();
    void updateStatus();
};

#endif // ASSEMBLYLOADER_H