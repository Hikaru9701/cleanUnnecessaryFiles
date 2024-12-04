#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSet>
#include <QDebug>

struct ConfigPanel {
    int programMode;
    QString Suffix;
};

void logWrite(const QString &message) {
    QFile logFile("cleaner.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << message << "\n";
        logFile.close();
    }
}

bool readConfig(const QString &filePath, ConfigPanel &config, QSet<QString> &fileList) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logWrite("Cannot open the file: " + filePath);
        return false;
    }

    QTextStream in(&file);
    QString line;
    bool modeSection = false;
    bool fileListSection = false;

    while (!in.atEnd()) {
        line = in.readLine().trimmed();

        if (line.isEmpty()) {
            continue; // Skip empty lines
        }

        if (line == "[MODE]") {
            modeSection = true;
            fileListSection = false;
            continue;
        }

        if (line == "[FILE_LIST]") {
            fileListSection = true;
            modeSection = false;
            continue;
        }

        if (modeSection) {
            if (line.startsWith("programMode=")) {
                QStringRef modeRef = line.midRef(QString("programMode=").length());
                bool ok;
                int mode = modeRef.toInt(&ok);
                if (!ok || (mode != 1 && mode != 2)) {
                    logWrite("Invalid program mode: " + line);
                    return false;
                }
                config.programMode = mode;
            } else if (line.startsWith("Suffix=")) {
                config.Suffix = line.mid(QString("Suffix=").length());
                if (config.Suffix.isEmpty()) {
                    logWrite("Suffix is empty. Exiting.");
                    return false;
                }
            }
        }

        if (fileListSection) {
            if (!line.isEmpty()) {
                fileList.insert(line);
            }
        }
    }

    file.close();
    return true;
}

void deleteFiles(const ConfigPanel &config, const QSet<QString> &fileList) {
    // Add the program's own executable, important DLLs, and the keep file to the keep list
    QSet<QString> keepFiles;
    keepFiles.insert(QCoreApplication::applicationName() + ".exe");
    keepFiles.insert("Qt5Core.dll");
    keepFiles.insert("config.ini");
    keepFiles.insert("cleaner.log");

    // Get all files in the current directory
    QDir dir(QDir::currentPath());
    QStringList allFiles = dir.entryList(QDir::Files);

    if (config.programMode == 1) {
        // Delete files with the specified suffix that are not in the keep list
        QStringList targetFiles = dir.entryList(QStringList() << ("*." + config.Suffix), QDir::Files);
        foreach (const QString &fileName, targetFiles) {
            if (!fileList.contains(fileName) && !keepFiles.contains(fileName)) {
                if (QFile::remove(dir.absoluteFilePath(fileName))) {
                    logWrite("File deleted: " + fileName);
                } else {
                    logWrite("Cannot delete the file: " + fileName);
                }
            }
        }
    } else if (config.programMode == 2) {
        // Delete files in the list but skip the keep files
        foreach (const QString &fileName, allFiles) {
            if (fileList.contains(fileName) && !keepFiles.contains(fileName)) {
                if (QFile::remove(dir.absoluteFilePath(fileName))) {
                    logWrite("File deleted: " + fileName);
                } else {
                    logWrite("Cannot delete the file: " + fileName);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Assume the config file path is "config.ini"
    QString filePath = "config.ini";

    // Configuration panel
    ConfigPanel config;
    QSet<QString> fileList;

    if (!readConfig(filePath, config, fileList)) {
        return -1; // Exit if config is invalid
    } else {
        deleteFiles(config, fileList);
        return 0; // Ensure the application quits after processing
    }
}
