#include "data/DataPath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace AppPaths
{
    QString databasePath()
    {
        const QString appDir = QCoreApplication::applicationDirPath();

        // 开发阶段优先尝试这些路径
        const QStringList candidates = {
            QDir(appDir).filePath("../../../data/database.db"),
            QDir(appDir).filePath("../../data/database.db"),
            QDir(appDir).filePath("../data/database.db"),
            QDir(appDir).filePath("data/database.db")
        };

        for (const QString& path : candidates) {
            if (QFileInfo::exists(path)) {
                return QDir::cleanPath(path);
            }
        }

        // 都找不到时，返回第一候选，方便调试输出
        return QDir::cleanPath(candidates.first());
    }
}