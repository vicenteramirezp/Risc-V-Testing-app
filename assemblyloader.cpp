#include "assemblyloader.h"
#include "ui_assemblyloader.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTextBlock>
#include <QScrollBar>

AssemblyLoader::AssemblyLoader(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AssemblyLoader)
    , currentInstructionIndex(-1)
{
    ui->setupUi(this);
    
    // Connect signals and slots
    connect(ui->loadFileButton, &QPushButton::clicked, this, &AssemblyLoader::loadAssemblyFile);
    connect(ui->stepButton, &QPushButton::clicked, this, &AssemblyLoader::stepInstruction);
    connect(ui->resetButton, &QPushButton::clicked, this, &AssemblyLoader::resetStepping);
    connect(ui->sendInstructionButton, &QPushButton::clicked, this, &AssemblyLoader::sendCurrentInstruction);
}

AssemblyLoader::~AssemblyLoader()
{
    delete ui;
}

void AssemblyLoader::loadAssemblyFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open RISC-V Assembly File", "", "Assembly Files (*.s *.S);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "File Error", 
            QString("Could not open file: %1").arg(file.errorString()));
        return;
    }
    
    // Clear previous content
    instructions.clear();
    ui->assemblyTextEdit->clear();
    
    QTextStream in(&file);
    QString fileContent;
    int lineNumber = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("//")) {
            fileContent += line + "\n";
            continue;
        }
        
        // Remove inline comments
        int commentIndex = line.indexOf('#');
        if (commentIndex != -1) {
            line = line.left(commentIndex).trimmed();
        }
        
        commentIndex = line.indexOf("//");
        if (commentIndex != -1) {
            line = line.left(commentIndex).trimmed();
        }
        
        if (!line.isEmpty()) {
            instructions.append(line);
            fileContent += line + "\n";
        }
    }
    
    file.close();
    
    // Display the assembly code
    ui->assemblyTextEdit->setPlainText(fileContent);
    
    // Update UI
    ui->statusLabel->setText(QString("Loaded: %1 instructions").arg(instructions.size()));
    ui->stepButton->setEnabled(!instructions.isEmpty());
    ui->resetButton->setEnabled(!instructions.isEmpty());
    ui->sendInstructionButton->setEnabled(false);
    
    // Reset stepping
    resetStepping();
}

void AssemblyLoader::stepInstruction()
{
    if (instructions.isEmpty() || currentInstructionIndex >= instructions.size() - 1) {
        return;
    }
    
    currentInstructionIndex++;
    highlightCurrentInstruction();
    updateStatus();
    
    ui->sendInstructionButton->setEnabled(true);
    
    // Disable step button if we've reached the end
    if (currentInstructionIndex >= instructions.size() - 1) {
        ui->stepButton->setEnabled(false);
    }
}

void AssemblyLoader::resetStepping()
{
    currentInstructionIndex = -1;
    
    // Clear any highlighting
    QTextCursor cursor(ui->assemblyTextEdit->document());
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setBackground(Qt::white);
    cursor.setCharFormat(format);
    
    updateStatus();
    ui->stepButton->setEnabled(!instructions.isEmpty());
    ui->sendInstructionButton->setEnabled(false);
}

void AssemblyLoader::sendCurrentInstruction()
{
    if (currentInstructionIndex >= 0 && currentInstructionIndex < instructions.size()) {
        QString instruction = instructions[currentInstructionIndex];
        emit instructionSelected(instruction);
    }
}

void AssemblyLoader::highlightCurrentInstruction()
{
    if (currentInstructionIndex < 0 || currentInstructionIndex >= instructions.size()) {
        return;
    }
    
    QTextDocument *document = ui->assemblyTextEdit->document();
    QTextCursor cursor(document);
    
    // Clear previous highlighting
    cursor.select(QTextCursor::Document);
    QTextCharFormat defaultFormat;
    defaultFormat.setBackground(Qt::white);
    cursor.setCharFormat(defaultFormat);
    
    // Find and highlight the current instruction
    QString targetLine = instructions[currentInstructionIndex];
    QTextCursor findCursor(document);
    
    while (!findCursor.isNull() && !findCursor.atEnd()) {
        findCursor = document->find(targetLine, findCursor);
        if (!findCursor.isNull()) {
            QTextCharFormat highlightFormat;
            highlightFormat.setBackground(Qt::yellow);
            findCursor.mergeCharFormat(highlightFormat);
            
            // Ensure the highlighted line is visible
            ui->assemblyTextEdit->setTextCursor(findCursor);
            ui->assemblyTextEdit->ensureCursorVisible();
            break;
        }
    }
}

void AssemblyLoader::updateStatus()
{
    if (currentInstructionIndex >= 0 && currentInstructionIndex < instructions.size()) {
        QString currentInstruction = instructions[currentInstructionIndex];
        ui->currentInstructionLabel->setText(
            QString("Current Instruction: [%1/%2] %3")
                .arg(currentInstructionIndex + 1)
                .arg(instructions.size())
                .arg(currentInstruction));
    } else {
        ui->currentInstructionLabel->setText("Current Instruction: None");
    }
}
