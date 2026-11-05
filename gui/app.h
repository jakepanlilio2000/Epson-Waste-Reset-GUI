#pragma once

#include "ewr/ewr_c.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <deque>
#include <chrono>

namespace ewr_gui {

enum class ConnectionMode {
    USB,
    Network
};

struct InkCartridge {
    std::string color;
    int code = 0;
    int level = 0;
    std::string status;
};

struct WastePad {
    std::string name;
    std::string kind;
    int used = 0;
    int max = 0;
    double percent = 0.0;
};

struct RawCounter {
    uint16_t address = 0;
    int value = 0;
};

struct PlannedWrite {
    uint16_t address = 0;
    uint8_t value = 0;
};

struct ResetCover {
    std::string kind;
    std::string name;
    bool readable = true;
};

struct PrinterCondition {
    std::string state = "UNKNOWN";
    int stateCode = -1;
    std::string error;
    int errorCode = -1;
    std::string serial;
    std::string deviceId;
    std::string reportedModel;
    bool truncated = false;
};

struct LogEntry {
    std::string timestamp;
    int level = 1; // ewr_level
    int stage = 0; // ewr_stage
    std::string code;
    std::string message;
    int index = -1;
    int total = -1;
    std::string fieldsJson;
};

struct NetworkPrinterItem {
    std::string address;
    std::string deviceId;
    std::string reportedModel;
    std::string modelMatch;
};

struct UsbInterfaceItem {
    int index = 0;
    std::string transport;
    std::string path;
    std::string deviceId;
    std::string modelMatch;
};

struct ModelItem {
    std::string name;
    std::vector<std::string> aliases;
    bool resettable = false;
    bool hasInkReset = false;
};

struct EepromByte {
    uint16_t address = 0;
    int value = -1;
};

enum class ModalType {
    None,
    ConfirmReset,
    BlockerWarning,
    ModelMismatchAlert,
    OperationResultAlert
};

struct ModalState {
    ModalType type = ModalType::None;
    std::string title;
    std::string message;
    std::string jsonPayload;
    bool isError = false;
    std::shared_ptr<std::promise<bool>> decisionPromise;
};

class App {
public:
    App();
    ~App();

    void Initialize();
    void Render();

    // Callbacks from worker/engine
    void AddLogEvent(const ewr_event* event);
    bool RequestConfirmDecision(const std::string& json);
    bool RequestBlockerDecision(const std::string& json);

private:
    // Background worker
    void StartWorkerTask(const std::string& desc, std::function<void()> task);
    void WorkerLoop();

    // Actions
    void ActionDetectModel();
    void ActionReadStatus();
    void ActionResetWastePads();
    void ActionResetCartridgeInk();
    void ActionDryRun();
    void ActionDumpEeprom();
    void ActionScanNetwork();
    void ActionScanUsbInterfaces();
    void ActionUpdateDatabase();
    void LoadModelsDatabase();
    void ExportEepromToFile(const std::string& path, bool asJson);

    // Tab rendering
    void RenderTopBar();
    void RenderDashboardTab();
    void RenderDashboardEmptyState();
    void RenderDashboardActiveState(float availWidth);
    void RenderModelPickerPopup();
    void RenderNetworkTab();
    void RenderModelsTab();
    void RenderEepromTab();
    void RenderLogsTab();
    void RenderSettingsTab();
    void RenderModals();

    // UI helpers
    void FilterModelsList();

    // Mutex & thread safety
    mutable std::mutex dataMutex_;
    std::thread workerThread_;
    std::atomic<bool> isBusy_{false};
    std::atomic<bool> hasActiveTask_{false};
    std::function<void()> pendingTask_;
    std::string currentTaskDesc_;
    float taskProgress_{0.0f};

    // Configuration / state
    std::string databasePath_;
    ConnectionMode connectionMode_ = ConnectionMode::USB;
    std::string manualIp_ = "192.168.1.100";
    std::string selectedModel_;
    int interfaceCandidate_ = 0;
    bool usbSoftReset_ = false;
    bool allowModelMismatch_ = false;
    bool autoUpdateCheck_ = true;

    // Printer data
    bool hasStatusData_ = false;
    PrinterCondition condition_;
    std::vector<WastePad> wastePads_;
    std::vector<InkCartridge> inkCartridges_;
    std::vector<RawCounter> rawCounters_;
    std::vector<PlannedWrite> plannedWrites_;
    std::vector<ResetCover> resetCovers_;

    // Models database
    std::vector<ModelItem> allModels_;
    std::vector<ModelItem> filteredModels_;
    char modelSearchBuf_[128] = "";
    bool showOnlyResettable_ = false;

    // Network & USB scans
    std::vector<NetworkPrinterItem> networkPrinters_;
    std::vector<UsbInterfaceItem> usbInterfaces_;
    bool isScanningNetwork_ = false;
    bool isScanningUsb_ = false;

    // EEPROM dump
    std::vector<EepromByte> eepromBytes_;
    int eepromAnswered_ = 0;
    int eepromTotal_ = 0;
    char eepromSearchBuf_[32] = "";

    // Logging
    std::deque<LogEntry> logs_;
    size_t maxLogs_ = 2000;
    bool autoScrollLogs_ = true;
    int logFilterLevel_ = 0; // 0: all, 1: info+, 2: warn+, 3: err
    char logSearchBuf_[128] = "";

    // Modals
    ModalState modal_;
    bool openModalFlag_ = false;

    // Last operation summary
    std::string lastOpSummary_;
    bool lastOpSuccess_ = true;
};

} // namespace ewr_gui

