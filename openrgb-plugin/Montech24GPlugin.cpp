// SPDX-License-Identifier: GPL-2.0-or-later
#include "Montech24GPlugin.h"
#include "RGBController.h"
#include <QColor>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>
#include <hidapi.h>

namespace {
constexpr unsigned KeyCount = 81;
constexpr unsigned RingCount = 7;
constexpr unsigned ReportLength = 32;
constexpr int TimeoutMs = 350;
constexpr unsigned short Vendor = 0x362d;
constexpr unsigned short Usb = 0x1700;
constexpr unsigned short Receiver = 0xd101;
constexpr unsigned int Gap = static_cast<unsigned int>(-1);

constexpr std::array<unsigned int, 90> KeyMatrix = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, Gap,
    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, Gap, 56, 57,
    58, Gap, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, Gap,
    71, 72, 73, Gap, Gap, Gap, 74, Gap, Gap, 75, 76, 77, 78, 79, 80
};
constexpr std::array<const char *, KeyCount> KeyNames = {
    "Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Delete",
    "Grave", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Minus", "Equals", "Backspace", "Home",
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "Left Bracket", "Right Bracket", "Backslash", "Page Up",
    "Caps Lock", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Semicolon", "Quote", "Enter", "Page Down",
    "Left Shift", "Z", "X", "C", "V", "B", "N", "M", "Comma", "Period", "Slash", "Right Shift", "Up",
    "Left Ctrl", "Win", "Left Alt", "Space", "Right Alt", "Fn", "Right Ctrl", "Left", "Down", "Right"
};

using Report = std::array<unsigned char, ReportLength + 1>;
using Reply = std::array<unsigned char, ReportLength>;
using Device = std::unique_ptr<hid_device, decltype(&hid_close)>;

bool Exchange(hid_device *device, const Report &request, Reply &reply)
{
    if (hid_write(device, request.data(), request.size()) != static_cast<int>(request.size()))
        return false;
    using Clock = std::chrono::steady_clock;
    const auto deadline = Clock::now() + std::chrono::milliseconds(TimeoutMs);
    for (auto now = Clock::now(); now < deadline; now = Clock::now())
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        const int size = hid_read_timeout(device, reply.data(), reply.size(), static_cast<int>(remaining) + 1);
        if (size <= 0) return false;
        if (size == static_cast<int>(reply.size()) && reply[0] == request[1] && reply[1] == request[2])
            return true;
    }
    return false;
}

std::string FindPath(unsigned short product)
{
    auto *devices = hid_enumerate(Vendor, product);
    std::string path;
    for (auto *device = devices; device; device = device->next)
    {
        if (device->usage_page == 0xff60 && device->usage == 0x61 && device->path)
        {
            path = device->path;
            break;
        }
    }
    hid_free_enumeration(devices);
    return path;
}

bool SupportsLighting(hid_device *device)
{
    Reply reply{};
    return Exchange(device, {0, 0x08, 0x40}, reply) &&
           reply[2] == 1 && reply[3] == 0 && reply[4] != 0 &&
           Exchange(device, {0, 0x08, 0x43}, reply) &&
           reply[2] == 90 && reply[3] == 0;
}
}

class MontechController final : public RGBController
{
public:
    MontechController(Device device, const std::string &path, bool wireless)
        : device_(std::move(device)), path_(path), map_data_(KeyMatrix)
    {
        name = wireless ? "MONTECH MKey PRO 75 (2.4 GHz)" : "MONTECH MKey PRO 75 (USB)";
        vendor = "MONTECH";
        description = "MONTECH MKey PRO 75 ANSI using VialRGB";
        version = "VialRGB 1";
        serial = wireless ? "362D:D101" : "362D:1700";
        location = "HID: " + path;
        type = DEVICE_TYPE_KEYBOARD;

        mode direct;
        direct.name = "Direct";
        direct.value = 1;
        direct.flags = MODE_FLAG_HAS_PER_LED_COLOR;
        direct.color_mode = MODE_COLORS_PER_LED;
        modes.push_back(direct);
        mode off;
        off.name = "Off";
        off.value = 0;
        off.color_mode = MODE_COLORS_NONE;
        modes.push_back(off);
        active_mode = 0;
        SetupZones();
    }

    ~MontechController() override { StopDeviceThread(); }

    void SetupZones() override
    {
        zones.clear();
        leds.clear();
        zone keys;
        keys.name = "Keyboard";
        keys.type = ZONE_TYPE_MATRIX;
        keys.leds_count = keys.leds_min = keys.leds_max = KeyCount;
        keys.matrix_map = &matrix_;
        zones.push_back(keys);
        zone ring;
        ring.name = "Encoder Ring";
        ring.type = ZONE_TYPE_LINEAR;
        ring.leds_count = ring.leds_min = ring.leds_max = RingCount;
        zones.push_back(ring);
        for (unsigned index = 0; index < KeyNames.size(); ++index)
            leds.push_back({KeyNames[index], index});
        for (unsigned index = 0; index < RingCount; ++index)
            leds.push_back({"Ring " + std::to_string(index + 1), KeyCount + index});
        SetupColors();
    }

    void ResizeZone(int, int) override {}
    void DeviceUpdateLEDs() override { WriteRange(0, KeyCount + RingCount); }
    void UpdateZoneLEDs(int index) override
    {
        if (index == 0) WriteRange(0, KeyCount);
        else if (index == 1) WriteRange(KeyCount, RingCount);
    }
    void UpdateSingleLED(int index) override
    {
        if (index >= 0 && static_cast<unsigned>(index) < KeyCount + RingCount)
            WriteRange(static_cast<unsigned>(index), 1);
    }

    void DeviceUpdateMode() override
    {
        std::lock_guard<std::mutex> lock(io_);
        const bool enabled = active_mode == 0;
        direct_active_ = SetFirmwareMode(enabled) && enabled;
        if (direct_active_) SendColors(0, KeyCount + RingCount);
    }

    bool Matches(const std::string &path) const { return path_ == path && !unresponsive_; }

private:
    bool Send(const Report &report)
    {
        Reply reply{};
        if (!Exchange(device_.get(), report, reply))
        {
            if (++failed_reports_ >= 2) unresponsive_ = true;
            return false;
        }
        failed_reports_ = 0;
        return true;
    }

    bool SetFirmwareMode(bool enabled)
    {
        return Send({0, 0x07, 0x41, static_cast<unsigned char>(enabled), 0, 127, 0, 255, 255});
    }

    void WriteRange(unsigned start, unsigned count)
    {
        std::lock_guard<std::mutex> lock(io_);
        if (active_mode != 0) return;
        if (!direct_active_) direct_active_ = SetFirmwareMode(true);
        if (direct_active_) SendColors(start, count);
    }

    void SendColors(unsigned start, unsigned count)
    {
        while (count)
        {
            const auto batch = static_cast<unsigned char>(std::min(count, 9u));
            Report report = {0, 0x07, 0x42, static_cast<unsigned char>(start), 0, batch};
            for (unsigned index = 0; index < batch; ++index)
            {
                const RGBColor rgb = colors[start + index];
                int hue, saturation, value;
                QColor(RGBGetRValue(rgb), RGBGetGValue(rgb), RGBGetBValue(rgb))
                    .getHsv(&hue, &saturation, &value);
                report[6 + index * 3] = static_cast<unsigned char>((std::max(hue, 0) * 255 + 180) / 360);
                report[7 + index * 3] = static_cast<unsigned char>(saturation);
                report[8 + index * 3] = static_cast<unsigned char>(value);
            }
            if (!Send(report))
            {
                direct_active_ = false;
                return;
            }
            start += batch;
            count -= batch;
        }
    }

    Device device_;
    std::string path_;
    std::array<unsigned int, KeyMatrix.size()> map_data_;
    matrix_map_type matrix_{6, 15, map_data_.data()};
    std::mutex io_;
    bool direct_active_ = false;
    unsigned failed_reports_ = 0;
    std::atomic<bool> unresponsive_{false};
};

Montech24GPlugin::Montech24GPlugin() = default;
Montech24GPlugin::~Montech24GPlugin() = default;

OpenRGBPluginInfo Montech24GPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info{};
    info.Name = "MONTECH MKey PRO 75 Wireless";
    info.Description = "VialRGB keyboard and encoder ring over USB or 2.4 GHz.";
    info.Version = "1.1.0";
    info.URL = "https://github.com/ruslan200065/-MONTECH_Mkey_Pro_75";
    info.Location = static_cast<unsigned int>(-1); // Controller supplies its own device page.
    return info;
}

unsigned int Montech24GPlugin::GetPluginAPIVersion() { return OPENRGB_PLUGIN_API_VERSION; }

void Montech24GPlugin::Load(ResourceManagerInterface *manager)
{
    manager_ = manager;
    connect(&timer_, &QTimer::timeout, this, &Montech24GPlugin::Refresh, Qt::UniqueConnection);
    timer_.start(2000);
    Refresh();
}

void Montech24GPlugin::Unload()
{
    timer_.stop();
    RemoveController();
    manager_ = nullptr;
}

void Montech24GPlugin::RemoveController()
{
    if (controller_)
    {
        manager_->UnregisterRGBController(controller_.get());
        controller_.reset();
    }
}

void Montech24GPlugin::Refresh()
{
    if (!manager_) return;
    // Prefer USB when both links are present to avoid two pages for one keyboard.
    for (unsigned short product : {Usb, Receiver})
    {
        const std::string path = FindPath(product);
        if (path.empty()) continue;
        if (controller_ && controller_->Matches(path)) return;
        Device device(hid_open_path(path.c_str()), &hid_close);
        if (!device || !SupportsLighting(device.get())) continue;
        RemoveController();
        controller_ = std::make_unique<MontechController>(std::move(device), path, product == Receiver);
        manager_->RegisterRGBController(controller_.get());
        return;
    }
    RemoveController();
}
