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

    // R-type instructions (opcode, funct3, funct7)
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

    // I-type instructions
    opcodeMap["addi"] = 0x13;  funct3Map["addi"] = 0x0;
    opcodeMap["slti"] = 0x13;  funct3Map["slti"] = 0x2;
    opcodeMap["sltiu"] = 0x13; funct3Map["sltiu"] = 0x3;
    opcodeMap["xori"] = 0x13;  funct3Map["xori"] = 0x4;
    opcodeMap["ori"] = 0x13;   funct3Map["ori"] = 0x6;
    opcodeMap["andi"] = 0x13;  funct3Map["andi"] = 0x7;
    opcodeMap["slli"] = 0x13;  funct3Map["slli"] = 0x1; funct7Map["slli"] = 0x00;
    opcodeMap["srli"] = 0x13;  funct3Map["srli"] = 0x5; funct7Map["srli"] = 0x00;
    opcodeMap["srai"] = 0x13;  funct3Map["srai"] = 0x5; funct7Map["srai"] = 0x20;

    // Load instructions (I-type)
    opcodeMap["lb"] = 0x03;   funct3Map["lb"] = 0x0;
    opcodeMap["lh"] = 0x03;   funct3Map["lh"] = 0x1;
    opcodeMap["lw"] = 0x03;   funct3Map["lw"] = 0x2;
    opcodeMap["lbu"] = 0x03;  funct3Map["lbu"] = 0x4;
    opcodeMap["lhu"] = 0x03;  funct3Map["lhu"] = 0x5;

    // JALR (I-type)
    opcodeMap["jalr"] = 0x67; funct3Map["jalr"] = 0x0;

    // S-type instructions (Store)
    opcodeMap["sb"] = 0x23;   funct3Map["sb"] = 0x0;
    opcodeMap["sh"] = 0x23;   funct3Map["sh"] = 0x1;
    opcodeMap["sw"] = 0x23;   funct3Map["sw"] = 0x2;

    // B-type instructions (Branch)
    opcodeMap["beq"] = 0x63;  funct3Map["beq"] = 0x0;
    opcodeMap["bne"] = 0x63;  funct3Map["bne"] = 0x1;
    opcodeMap["blt"] = 0x63;  funct3Map["blt"] = 0x4;
    opcodeMap["bge"] = 0x63;  funct3Map["bge"] = 0x5;
    opcodeMap["bltu"] = 0x63; funct3Map["bltu"] = 0x6;
    opcodeMap["bgeu"] = 0x63; funct3Map["bgeu"] = 0x7;

    // U-type instructions
    opcodeMap["lui"] = 0x37;
    opcodeMap["auipc"] = 0x17;

    // J-type instructions
    opcodeMap["jal"] = 0x6f;

    // SYSTEM instructions (I-type)
    opcodeMap["ecall"] = 0x73;  funct3Map["ecall"] = 0x0; funct12Map["ecall"] = 0x000;
    opcodeMap["ebreak"] = 0x73; funct3Map["ebreak"] = 0x0; funct12Map["ebreak"] = 0x001;
    opcodeMap["csrrw"] = 0x73;  funct3Map["csrrw"] = 0x1;
    opcodeMap["csrrs"] = 0x73;  funct3Map["csrrs"] = 0x2;
    opcodeMap["csrrc"] = 0x73;  funct3Map["csrrc"] = 0x3;
    opcodeMap["csrrwi"] = 0x73; funct3Map["csrrwi"] = 0x5;
    opcodeMap["csrrsi"] = 0x73; funct3Map["csrrsi"] = 0x6;
    opcodeMap["csrrci"] = 0x73; funct3Map["csrrci"] = 0x7;

    // FENCE instruction
    opcodeMap["fence"] = 0x0f; funct3Map["fence"] = 0x0;
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

    if (!opcodeMap.contains(mnemonic)) {
        errorMessage = QString("Unknown instruction: '%1'").arg(mnemonic);
        return false;
    }

    quint32 opcode = opcodeMap[mnemonic];

    // Route to appropriate parser based on opcode
    switch (opcode) {
    case 0x33: // R-type
        return parseRTypeInstruction(parts, machineCode, errorMessage);
    case 0x13: // I-type (ALU immediate)
    case 0x03: // I-type (Load)
    case 0x67: // I-type (JALR)
    case 0x73: // I-type (SYSTEM)
        return parseITypeInstruction(parts, machineCode, errorMessage);
    case 0x23: // S-type
        return parseSTypeInstruction(parts, machineCode, errorMessage);
    case 0x63: // B-type
        return parseBTypeInstruction(parts, machineCode, errorMessage);
    case 0x37: // U-type (LUI)
    case 0x17: // U-type (AUIPC)
        return parseUTypeInstruction(parts, machineCode, errorMessage);
    case 0x6f: // J-type (JAL)
        return parseJTypeInstruction(parts, machineCode, errorMessage);
    case 0x0f: // FENCE
        // FENCE instruction - simple implementation
        machineCode = 0x0000000f; // fence
        return true;
    default:
        errorMessage = QString("Instruction type not implemented: '%1'").arg(mnemonic);
        return false;
    }
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
    if (!parseRegister(parts[2], rs1, errorMessage)) return false;
    if (!parseRegister(parts[3], rs2, errorMessage)) return false;

    // Validate registers
    if (rd < 0 || rd > 31 || rs1 < 0 || rs1 > 31 || rs2 < 0 || rs2 > 31) {
        errorMessage = "Register numbers must be between 0 and 31";
        return false;
    }

    // Build machine code
    quint32 opcode = opcodeMap[mnemonic];
    quint32 funct3 = funct3Map[mnemonic];
    quint32 funct7 = funct7Map[mnemonic];

    machineCode = (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    return true;
}

bool RiscVMachineCodeConverter::parseITypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    QString mnemonic = parts[0];
    quint32 opcode = opcodeMap[mnemonic];

    // Handle special cases
    if (mnemonic == "ecall" || mnemonic == "ebreak") {
        // ECALL/EBREAK have no operands
        if (parts.size() != 1) {
            errorMessage = QString("%1 takes no operands").arg(mnemonic);
            return false;
        }
        machineCode = (funct12Map[mnemonic] << 20) | opcode;
        return true;
    }

    if (mnemonic.startsWith("csrr")) {
        // CSR instructions have different formats
        if (parts.size() != 3) {
            errorMessage = QString("CSR instruction requires 2 operands (got %1)").arg(parts.size() - 1);
            return false;
        }

        int rd, csr;
        if (!parseRegister(parts[1], rd, errorMessage)) return false;
        if (!parseImmediate(parts[2], csr, errorMessage)) return false;

        quint32 funct3 = funct3Map[mnemonic];
        machineCode = (csr << 20) | (rd << 7) | (funct3 << 12) | opcode;
        return true;
    }

    // Standard I-type: rd, rs1, imm
    if (parts.size() != 4) {
        errorMessage = QString("I-type instruction requires 3 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    int rd, rs1, imm;
    if (!parseRegister(parts[1], rd, errorMessage)) return false;
    if (!parseRegister(parts[2], rs1, errorMessage)) return false;
    if (!parseImmediate(parts[3], imm, errorMessage)) return false;

    // Handle shift instructions specially
    if (mnemonic == "slli" || mnemonic == "srli" || mnemonic == "srai") {
        if (imm < 0 || imm > 31) {
            errorMessage = "Shift amount must be between 0 and 31";
            return false;
        }
        quint32 shamt = imm & 0x1F;
        quint32 funct3 = funct3Map[mnemonic];
        quint32 funct7 = funct7Map[mnemonic];
        machineCode = (funct7 << 25) | (shamt << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    } else {
        // Standard I-type
        quint32 imm12 = imm & 0xFFF;
        quint32 funct3 = funct3Map[mnemonic];
        machineCode = (imm12 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    }

    return true;
}

bool RiscVMachineCodeConverter::parseSTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    // S-type format: rs2, offset(rs1)
    if (parts.size() != 3) {
        errorMessage = QString("S-type instruction requires 2 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    QString mnemonic = parts[0];
    int rs2, rs1, offset;

    // Parse source register
    if (!parseRegister(parts[1], rs2, errorMessage)) return false;

    // Parse offset(rs1) format
    QString offsetRs1 = parts[2];
    QRegularExpression offsetPattern(R"(([-+]?\d+)\((\w+)\))");
    QRegularExpressionMatch match = offsetPattern.match(offsetRs1);

    if (!match.hasMatch()) {
        errorMessage = "S-type instruction must be in format: offset(rs1)";
        return false;
    }

    if (!parseImmediate(match.captured(1), offset, errorMessage)) return false;
    if (!parseRegister(match.captured(2), rs1, errorMessage)) return false;

    // Build machine code
    quint32 opcode = opcodeMap[mnemonic];
    quint32 funct3 = funct3Map[mnemonic];

    // S-type: imm[11:5] | rs2 | rs1 | funct3 | imm[4:0] | opcode
    quint32 imm11_5 = (offset >> 5) & 0x7F;
    quint32 imm4_0 = offset & 0x1F;

    machineCode = (imm11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm4_0 << 7) | opcode;
    return true;
}

bool RiscVMachineCodeConverter::parseBTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    // B-type format: rs1, rs2, offset
    if (parts.size() != 4) {
        errorMessage = QString("B-type instruction requires 3 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    QString mnemonic = parts[0];
    int rs1, rs2, offset;

    if (!parseRegister(parts[1], rs1, errorMessage)) return false;
    if (!parseRegister(parts[2], rs2, errorMessage)) return false;
    if (!parseOffset(parts[3], offset, errorMessage)) return false;

    // Build machine code
    quint32 opcode = opcodeMap[mnemonic];
    quint32 funct3 = funct3Map[mnemonic];

    // B-type: imm[12] | imm[10:5] | rs2 | rs1 | funct3 | imm[4:1] | imm[11] | opcode
    quint32 imm12 = (offset >> 12) & 0x1;
    quint32 imm10_5 = (offset >> 5) & 0x3F;
    quint32 imm4_1 = (offset >> 1) & 0xF;
    quint32 imm11 = (offset >> 11) & 0x1;

    machineCode = (imm12 << 31) | (imm10_5 << 25) | (rs2 << 20) | (rs1 << 15) |
                  (funct3 << 12) | (imm4_1 << 8) | (imm11 << 7) | opcode;
    return true;
}

bool RiscVMachineCodeConverter::parseUTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    // U-type format: rd, imm
    if (parts.size() != 3) {
        errorMessage = QString("U-type instruction requires 2 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    QString mnemonic = parts[0];
    int rd, imm;

    if (!parseRegister(parts[1], rd, errorMessage)) return false;
    if (!parseImmediate(parts[2], imm, errorMessage)) return false;

    // Build machine code
    quint32 opcode = opcodeMap[mnemonic];

    // U-type: imm[31:12] | rd | opcode
    quint32 imm31_12 = (imm >> 12) & 0xFFFFF;

    machineCode = (imm31_12 << 12) | (rd << 7) | opcode;
    return true;
}

bool RiscVMachineCodeConverter::parseJTypeInstruction(const QStringList& parts, quint32& machineCode, QString& errorMessage)
{
    // J-type format: rd, offset
    if (parts.size() != 3) {
        errorMessage = QString("J-type instruction requires 2 operands (got %1)").arg(parts.size() - 1);
        return false;
    }

    QString mnemonic = parts[0];
    int rd, offset;

    if (!parseRegister(parts[1], rd, errorMessage)) return false;
    if (!parseOffset(parts[2], offset, errorMessage)) return false;

    // Build machine code
    quint32 opcode = opcodeMap[mnemonic];

    // J-type: imm[20] | imm[10:1] | imm[11] | imm[19:12] | rd | opcode
    quint32 imm20 = (offset >> 20) & 0x1;
    quint32 imm10_1 = (offset >> 1) & 0x3FF;
    quint32 imm11 = (offset >> 11) & 0x1;
    quint32 imm19_12 = (offset >> 12) & 0xFF;

    machineCode = (imm20 << 31) | (imm19_12 << 12) | (imm11 << 20) | (imm10_1 << 21) | (rd << 7) | opcode;
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

bool RiscVMachineCodeConverter::parseOffset(const QString& offsetStr, int& offsetValue, QString& errorMessage)
{
    // For now, treat offset same as immediate
    return parseImmediate(offsetStr, offsetValue, errorMessage);
}

QString RiscVMachineCodeConverter::formatMachineCode(quint32 machineCode)
{
    return QString("0x%1").arg(machineCode, 8, 16, QChar('0'));
}
