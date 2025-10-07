#ifndef RISCVMACHINECODECONVERTER_H
#define RISCVMACHINECODECONVERTER_H

#include <QString>
#include <QMap>

class RiscVMachineCodeConverter
{
public:
    RiscVMachineCodeConverter();

    // Main conversion function
    bool convertToMachineCode(const QString& instruction, quint32& machineCode, QString& errorMessage);

    // Utility function to format machine code as string
    static QString formatMachineCode(quint32 machineCode);

private:
    void initializeMaps();
    bool parseRTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage);
    bool parseITypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage);
    bool parseSTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage);
    bool parseBTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage);
    bool parseUTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage);
    bool parseJTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage);

    // Helper functions
    bool parseRegister(const QString& regStr, int& regNum, QString& errorMessage);
    bool parseImmediate(const QString& immStr, int& immValue, QString& errorMessage);

    QMap<QString, int> registerMap;
    QMap<QString, quint32> opcodeMap;
    QMap<QString, quint32> funct3Map;
    QMap<QString, quint32> funct7Map;
};

#endif // RISCVMACHINECODECONVERTER_H
