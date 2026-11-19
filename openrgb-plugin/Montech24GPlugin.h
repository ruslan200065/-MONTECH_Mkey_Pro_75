// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "OpenRGBPluginInterface.h"

class MontechController;

class Montech24GPlugin final : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID)
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    Montech24GPlugin();
    ~Montech24GPlugin() override;
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override;
    void Load(ResourceManagerInterface *manager) override;
    QWidget *GetWidget() override { return nullptr; }
    QMenu *GetTrayMenu() override { return nullptr; }
    void Unload() override;

private:
    void Refresh();
    void RemoveController();

    ResourceManagerInterface *manager_ = nullptr;
    QTimer timer_;
    std::unique_ptr<MontechController> controller_;
};
