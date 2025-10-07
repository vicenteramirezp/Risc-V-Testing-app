#include "riscvmachinecodeconverter.h"
#include <QStringList>
#include <QRegularExpression>
#include <QDebug>

RiscVMachineCodeConverter::RiscVMachineCodeConverter()
{
    initializeMaps();
}

void RiscVMachineCodeConverter::initializeMaps()
{
    // Initialize register mappings
    registerMap.clear();
    for (int i = 0; i < 32; i++) {
        registerMap[QString("x%1").arg(i)] = i;
    }
    // ABI names
    registerMap["zero"] = 0;
    registerMap["ra"] = 1;
    registerMap["sp"] = 2;
    registerMap["gp"] = 3;
    registerMap["tp"] = 4;
    registerMap["t0"] = 5; registerMap["t1"] = 6; registerMap["t2"] = 7;
    registerMap["s0"] = 8; registerMap["s1"] = 9;
    registerMap["a0"] = 10; registerMap["a1"] = 11; registerMap["a2"] = 12;
    registerMap["a3"] = 13; registerMap["a4"] = 14; registerMap["a5"] = 15;
    registerMap["a6"] = 16; registerMap["a7"] = 17;
    registerMap["s2"] = 18; registerMap["s3"] = 19; registerMap["s4"] = 20;
    registerMap["s5"] = 21; registerMap["s6"] = 22; registerMap["s7"] = 23;
    registerMap["s8"] = 24; registerMap["s9"] = 25; registerMap["s10"] = 26;
    registerMap["s11"] = 27;
    registerMap["t3"] = 28; registerMap["t4"] = 29; registerMap["t5"] = 30;
    registerMap["t6"] = 31;

    // Initialize R-type instructions (opcode, funct3, funct7)
    // Format: ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    opcodeMap["add"] = 0x33;  funct3Map["add"] = 0x0;  funct7Map["add"] = 0x00;
    opcodeMap["sub"] = 0x33;  funct3Map["sub"] = 0x0;  funct7Map["sub"] = 0x20;
    opcodeMap["sll"] = 0x33;  funct3Map["sll"] = 0x1;  funct7Map["sll"] = 0x00;
    opcodeMap["slt"] = 0x33;  funct3Map["slt"] = 0x2;  funct7Map["slt"] = 0x00;
    opcodeMap["sltu"] = 0x33; funct3Map["sltu"] = 0x3; funct7Map["sltu"] = 0x00;
    opcodeMap["xor"] = 0x33;  funct3Map["xor"] = 0x4;  funct7Map["xor"] = 0x00;
    opcodeMap["srl"] = 0x33;  funct3Map["srl"] = 0x5;  funct7Map["srl"] = 0x00;
    opcodeMap["sra"] = 0x33;  funct3Map["sra"] = 0x5;  funct7Map["sra"] = 0x20;
    opcodeMap["or"] = 0x33;   funct3Map["or"] = 0x6;   funct7Map["or"] = 0x00;
    opcodeMap["and"] = 0x33;  funct3Map["and"] = 0x7;  funct7Map["and"] = 0x00;
}

bool RiscVMachineCodeConverter::convertToMachineCode(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    // Clean and split the instruction
    QString cleanInstruction = instruction.trimmed().toLower();
    QStringList parts = cleanInstruction.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        errorMessage = "Empty instruction";
        return false;
    }

    QString mnemonic = parts[0];

    // Check if it's an R-type instruction
    if (opcodeMap.contains(mnemonic) && opcodeMap[mnemonic] == 0x33) {
        return parseRTypeInstruction(parts, machineCode, errorMessage);
    }

    errorMessage = QString("Instruction '%1' not supported yet").arg(mnemonic);
    return false;
}

bool RiscVMachineCodeConverter::parseRTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    // R-type format: rd, rs1, rs2
    if (parts.size() != 4) {
        errorMessage = QString("R-type instruction requires 3 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    QString mnemonic = parts[0];
    int rd, rs1, rs2;

    // Parse destination register
    if (!parseRegister(parts[1], rd, errorMessage)) return false;

    // Parse first source register
    if (!parseRegister(parts[2], rs1, errorMessage)) return false;

    // Parse second source register
    if (!parseRegister(parts[3], rs2, errorMessage)) return false;

    // Validate registers
    if (rd < 0 || rd > 31 || rs1 < 0 || rs1 > 31 || rs2 < 0 || rs2 > 31) {
        errorMessage = "Register numbers must be between 0 and 31";
        return false;
    }

    // Build machine code
    // R-type format: funct7[31:25] | rs2[24:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | opcode[6:0]
    quint32 opcode = opcodeMap[mnemonic];
    quint32 funct3 = funct3Map[mnemonic];
    quint32 funct7 = funct7Map[mnemonic];

    machineCode = (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;

    return true;
}

bool RiscVMachineCodeConverter::parseRegister(const QString& regStr, int& regNum, QString& errorMessage)
{
    QString cleanReg = regStr.trimmed().toLower();

    if (registerMap.contains(cleanReg)) {
        regNum = registerMap[cleanReg];
        return true;
    }

    // Check if it's in x0-x31 format
    if (cleanReg.startsWith('x')) {
        bool ok;
        regNum = cleanReg.mid(1).toInt(&ok);
        if (ok && regNum >= 0 && regNum <= 31) {
            return true;
        }
    }

    errorMessage = QString("Invalid register: '%1'").arg(regStr);
    return false;
}

bool RiscVMachineCodeConverter::parseImmediate(const QString& immStr, int& immValue, QString& errorMessage)
{
    QString cleanImm = immStr.trimmed();
    bool ok;

    if (cleanImm.startsWith("0x")) {
        // Hexadecimal
        immValue = cleanImm.mid(2).toInt(&ok, 16);
    } else if (cleanImm.startsWith("0b")) {
        // Binary
        immValue = cleanImm.mid(2).toInt(&ok, 2);
    } else {
        // Decimal
        immValue = cleanImm.toInt(&ok, 10);
    }

    if (!ok) {
        errorMessage = QString("Invalid immediate value: '%1'").arg(immStr);
        return false;
    }

    return true;
}

QString RiscVMachineCodeConverter::formatMachineCode(quint32 machineCode)
{
    return QString("0x%1").arg(machineCode, 8, 16, QChar('0'));
}

// TODO: Implement other instruction types
bool RiscVMachineCodeConverter::parseITypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    errorMessage = "I-type parsing not implemented";
    return false;
}

bool RiscVMachineCodeConverter::parseSTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    errorMessage = "S-type parsing not implemented";
    return false;
}

bool RiscVMachineCodeConverter::parseBTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    errorMessage = "B-type parsing not implemented";
    return false;
}

bool RiscVMachineCodeConverter::parseUTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    errorMessage = "U-type parsing not implemented";
    return false;
}

bool RiscVMachineCodeConverter::parseJTypeInstruction(const QString& instruction, quint32& machineCode, QString& errorMessage)
{
    errorMessage = "J-type parsing not implemented";
    return false;
}
