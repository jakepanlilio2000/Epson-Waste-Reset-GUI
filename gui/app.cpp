#include "app.h"
#include "style.h"
#include "ewr/updater.h"
#include "ewr/version.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ewr_gui {

namespace {

std::string CurrentTimeStr()
{
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &tt);
#else
    localtime_r(&tt, &tmNow);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return std::string(buf);
}

std::string LocateDatabase()
{
    // Check local working dir first
    std::ifstream test("database.json");
    if (test.good())
        return "database.json";

#ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0)
    {
        std::string dir(exePath);
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
        {
            std::string candidate = dir.substr(0, lastSlash + 1) + "database.json";
            std::ifstream testCandidate(candidate);
            if (testCandidate.good())
                return candidate;
        }
    }
#endif

    return "database.json";
}

} // namespace

// -------------------------------------------------------------------------
// Construction & Lifecycle
// -------------------------------------------------------------------------

App::App()
{
    databasePath_ = LocateDatabase();
    workerThread_ = std::thread(&App::WorkerLoop, this);
}

App::~App()
{
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        isBusy_ = false;
        hasActiveTask_ = false;
        // Unblock any modal promise
        if (modal_.decisionPromise)
        {
            try { modal_.decisionPromise->set_value(false); } catch (...) {}
            modal_.decisionPromise.reset();
        }
    }

    // Signal worker thread exit
    StartWorkerTask("Exiting", []() {});
    if (workerThread_.joinable())
    {
        // Cancel task if needed
        workerThread_.detach(); // Clean exit
    }
}

void App::Initialize()
{
    // Add startup log
    LogEntry startLog;
    startLog.timestamp = CurrentTimeStr();
    startLog.level = 1;
    startLog.code = "app.start";
    startLog.message = "Epson Waste Reset (EWR) GUI v" + std::string(EWR_VERSION) + " initialized.";
    logs_.push_back(startLog);

    // Initial load: database models and auto-detect USB printer
    StartWorkerTask("Initializing database and scanning for printer...", [this]()
    {
        LoadModelsDatabase();
        ActionDetectModel();
        if (!selectedModel_.empty())
        {
            ActionReadStatus();
        }
    });
}

void App::WorkerLoop()
{
    while (true)
    {
        std::function<void()> currentTask;
        {
            std::unique_lock<std::mutex> lock(dataMutex_);
            if (!hasActiveTask_)
            {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            currentTask = pendingTask_;
            pendingTask_ = nullptr;
            hasActiveTask_ = false;
            isBusy_ = true;
        }

        if (currentTaskDesc_ == "Exiting")
            break;

        try
        {
            if (currentTask)
                currentTask();
        }
        catch (const std::exception& ex)
        {
            LogEntry err;
            err.timestamp = CurrentTimeStr();
            err.level = 3;
            err.code = "worker.exception";
            err.message = "Error in background task: " + std::string(ex.what());
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(err);
            }
        }
        catch (...)
        {
            LogEntry err;
            err.timestamp = CurrentTimeStr();
            err.level = 3;
            err.code = "worker.exception";
            err.message = "Unknown error in background task.";
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(err);
            }
        }

        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            isBusy_ = false;
            currentTaskDesc_.clear();
            taskProgress_ = 0.0f;
        }
    }
}

void App::StartWorkerTask(const std::string& desc, std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (isBusy_ && desc != "Exiting")
        return; // Don't stack tasks if busy

    currentTaskDesc_ = desc;
    pendingTask_ = task;
    hasActiveTask_ = true;
    taskProgress_ = 0.0f;
}

// -------------------------------------------------------------------------
// Callbacks & Synchronization
// -------------------------------------------------------------------------

static void StaticEventCallback(const ewr_event* event, void* user)
{
    if (user && event)
        static_cast<App*>(user)->AddLogEvent(event);
}

static int StaticBlockerCallback(const char* json, void* user)
{
    if (user && json)
        return static_cast<App*>(user)->RequestBlockerDecision(json) ? 1 : 0;
    return 0;
}

static int StaticConfirmCallback(const char* json, void* user)
{
    if (user && json)
        return static_cast<App*>(user)->RequestConfirmDecision(json) ? 1 : 0;
    return 0;
}

void App::AddLogEvent(const ewr_event* event)
{
    LogEntry entry;
    entry.timestamp = CurrentTimeStr();
    entry.level = event->level;
    entry.stage = event->stage;
    entry.code = event->code ? event->code : "";
    entry.message = event->message ? event->message : "";
    entry.index = event->index;
    entry.total = event->total;
    entry.fieldsJson = event->fields_json ? event->fields_json : "{}";

    std::lock_guard<std::mutex> lock(dataMutex_);
    logs_.push_back(entry);
    if (logs_.size() > maxLogs_)
        logs_.pop_front();

    if (event->total > 0 && event->index >= 0)
        taskProgress_ = static_cast<float>(event->index) / static_cast<float>(event->total);
}

bool App::RequestConfirmDecision(const std::string& json)
{
    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        modal_.type = ModalType::ConfirmReset;
        modal_.title = "Confirm Waste Ink Pad Reset";
        modal_.message = "Please confirm that you want to reset the EEPROM waste ink counters.";
        modal_.jsonPayload = json;
        modal_.isError = false;
        modal_.decisionPromise = prom;
        openModalFlag_ = true;
    }

    return fut.get();
}

bool App::RequestBlockerDecision(const std::string& json)
{
    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        modal_.type = ModalType::BlockerWarning;
        modal_.title = "Printer Condition Warning";
        modal_.message = "The printer reported a blocker condition. Do you want to cancel or force continue?";
        modal_.jsonPayload = json;
        modal_.isError = true;
        modal_.decisionPromise = prom;
        openModalFlag_ = true;
    }

    return fut.get();
}

// -------------------------------------------------------------------------
// Engine Actions
// -------------------------------------------------------------------------

void App::LoadModelsDatabase()
{
    ewr_session* session = nullptr;
    int rc = ewr_session_open(databasePath_.c_str(), &session);
    if (rc != EWR_OK && rc != EWR_ERR_DEVICE_NOT_FOUND)
    {
        if (session) ewr_session_close(session);
        return;
    }

    char* outJson = nullptr;
    if (ewr_list_models(session, &outJson) == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            std::vector<ModelItem> list;
            if (j.contains("models") && j["models"].is_array())
            {
                for (const auto& m : j["models"])
                {
                    ModelItem item;
                    item.name = m.value("name", "");
                    if (m.contains("aliases") && m["aliases"].is_array())
                    {
                        for (const auto& a : m["aliases"])
                            item.aliases.push_back(a.get<std::string>());
                    }
                    item.resettable = m.value("resettable", false);
                    item.hasInkReset = m.value("has_ink_reset", false);
                    list.push_back(item);
                }
            }

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                allModels_ = std::move(list);
                FilterModelsList();
            }
        } catch (...) {}
        ewr_string_free(outJson);
    }

    ewr_session_close(session);
}

void App::ActionDetectModel()
{
    ewr_session* session = nullptr;
    int rc = (connectionMode_ == ConnectionMode::USB)
        ? ewr_session_open(databasePath_.c_str(), &session)
        : ewr_session_open_network(databasePath_.c_str(), manualIp_.c_str(), &session);

    if (rc != EWR_OK && rc != EWR_ERR_DEVICE_NOT_FOUND)
    {
        LogEntry entry;
        entry.timestamp = CurrentTimeStr();
        entry.level = 2;
        entry.code = "detect.failed";
        entry.message = "Could not open session to detect printer: " + std::string(ewr_status_name(rc));
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(entry);
        }
        if (session) ewr_session_close(session);
        return;
    }

    char* outJson = nullptr;
    rc = ewr_detect_model(session, &outJson);
    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            std::string detectedModel = j.value("model", "");
            std::string reportedModel = j.value("reported_model", "");
            std::string deviceId = j.value("device_id", "");

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                condition_.reportedModel = reportedModel;
                condition_.deviceId = deviceId;
                if (!detectedModel.empty())
                    selectedModel_ = detectedModel;
                else if (!reportedModel.empty() && selectedModel_.empty())
                    selectedModel_ = reportedModel;
            }

            LogEntry entry;
            entry.timestamp = CurrentTimeStr();
            entry.level = 1;
            entry.code = "detect.ok";
            entry.message = "Detected: " + (detectedModel.empty() ? reportedModel : detectedModel);
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(entry);
            }
        } catch (...) {}
        ewr_string_free(outJson);
    }
    else
    {
        LogEntry entry;
        entry.timestamp = CurrentTimeStr();
        entry.level = 2;
        entry.code = "detect.not_found";
        entry.message = (connectionMode_ == ConnectionMode::USB)
            ? "No Epson printer detected over USB."
            : "No response from network printer at " + manualIp_;
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(entry);
        }
    }

    ewr_session_close(session);
}

void App::ActionReadStatus()
{
    std::string model;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        model = selectedModel_;
    }

    if (model.empty())
    {
        ActionDetectModel();
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            model = selectedModel_;
        }
        if (model.empty())
        {
            LogEntry entry;
            entry.timestamp = CurrentTimeStr();
            entry.level = 2;
            entry.code = "status.no_model";
            entry.message = "No printer model selected or detected.";
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(entry);
            }
            return;
        }
    }

    ewr_session* session = nullptr;
    int rc = (connectionMode_ == ConnectionMode::USB)
        ? ewr_session_open(databasePath_.c_str(), &session)
        : ewr_session_open_network(databasePath_.c_str(), manualIp_.c_str(), &session);

    if (rc != EWR_OK)
    {
        LogEntry entry;
        entry.timestamp = CurrentTimeStr();
        entry.level = 3;
        entry.code = "status.open_failed";
        entry.message = "Failed to open printer session: " + std::string(ewr_session_last_error(session));
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(entry);
        }
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);
    ewr_session_set_interface(session, interfaceCandidate_);
    ewr_session_set_soft_reset(session, usbSoftReset_ ? 1 : 0);

    char* outJson = nullptr;
    rc = ewr_read_status(session, model.c_str(), &outJson);

    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);

            PrinterCondition cond;
            if (j.contains("printer") && j["printer"].is_object())
            {
                auto p = j["printer"];
                cond.state = p.value("state", "READY");
                cond.stateCode = p.value("state_code", 0);
                cond.error = p.value("error", "");
                cond.errorCode = p.value("error_code", 0);
                cond.serial = p.value("serial", "");
                cond.truncated = p.value("truncated", false);
            }

            std::vector<InkCartridge> inks;
            if (j.contains("printer") && j["printer"].contains("inks") && j["printer"]["inks"].is_array())
            {
                for (const auto& item : j["printer"]["inks"])
                {
                    InkCartridge ink;
                    ink.color = item.value("color", "");
                    ink.code = item.value("code", 0);
                    ink.level = item.value("level", 0);
                    ink.status = item.value("status", "OK");
                    inks.push_back(ink);
                }
            }

            std::vector<WastePad> pads;
            if (j.contains("pads") && j["pads"].is_array())
            {
                for (const auto& item : j["pads"])
                {
                    WastePad pad;
                    pad.name = item.value("name", "Waste Ink Pad");
                    pad.kind = item.value("kind", "main");
                    pad.used = item.value("used", 0);
                    pad.max = item.value("max", 0);
                    pad.percent = item.value("percent", 0.0);
                    pads.push_back(pad);
                }
            }

            std::vector<RawCounter> counters;
            if (j.contains("counters") && j["counters"].is_array())
            {
                for (const auto& item : j["counters"])
                {
                    RawCounter c;
                    c.address = item.value("address", 0);
                    c.value = item.value("value", 0);
                    counters.push_back(c);
                }
            }

            std::vector<PlannedWrite> writes;
            if (j.contains("planned_writes") && j["planned_writes"].is_array())
            {
                for (const auto& item : j["planned_writes"])
                {
                    PlannedWrite w;
                    w.address = item.value("address", 0);
                    w.value = static_cast<uint8_t>(item.value("value", 0));
                    writes.push_back(w);
                }
            }

            std::vector<ResetCover> covers;
            if (j.contains("reset_covers") && j["reset_covers"].is_array())
            {
                for (const auto& item : j["reset_covers"])
                {
                    ResetCover cov;
                    cov.kind = item.value("kind", "");
                    cov.name = item.value("name", "");
                    cov.readable = item.value("readable", true);
                    covers.push_back(cov);
                }
            }

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                condition_.state = cond.state;
                condition_.stateCode = cond.stateCode;
                condition_.error = cond.error;
                condition_.errorCode = cond.errorCode;
                if (!cond.serial.empty()) condition_.serial = cond.serial;
                condition_.truncated = cond.truncated;

                inkCartridges_ = std::move(inks);
                wastePads_ = std::move(pads);
                rawCounters_ = std::move(counters);
                plannedWrites_ = std::move(writes);
                resetCovers_ = std::move(covers);
                hasStatusData_ = true;
            }

            LogEntry okLog;
            okLog.timestamp = CurrentTimeStr();
            okLog.level = 1;
            okLog.code = "status.ok";
            okLog.message = "Status query complete for " + model + ". State: " + cond.state + (cond.error.empty() ? "" : " (" + cond.error + ")");
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(okLog);
            }
        } catch (const std::exception& e) {
            LogEntry err;
            err.timestamp = CurrentTimeStr();
            err.level = 3;
            err.code = "status.parse_err";
            err.message = "Failed to parse printer status reply: " + std::string(e.what());
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(err);
            }
        }
        ewr_string_free(outJson);
    }
    else
    {
        LogEntry err;
        err.timestamp = CurrentTimeStr();
        err.level = 3;
        err.code = "status.failed";
        err.message = "Failed to read printer status: " + std::string(ewr_session_last_error(session));
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(err);
        }
    }

    ewr_session_close(session);
}

void App::ActionResetWastePads()
{
    std::string model;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        model = selectedModel_;
    }

    if (model.empty())
    {
        LogEntry err;
        err.timestamp = CurrentTimeStr();
        err.level = 3;
        err.code = "reset.no_model";
        err.message = "No model selected. Please detect or pick a model first.";
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(err);
        }
        return;
    }

    ewr_session* session = nullptr;
    int rc = (connectionMode_ == ConnectionMode::USB)
        ? ewr_session_open(databasePath_.c_str(), &session)
        : ewr_session_open_network(databasePath_.c_str(), manualIp_.c_str(), &session);

    if (rc != EWR_OK)
    {
        LogEntry err;
        err.timestamp = CurrentTimeStr();
        err.level = 3;
        err.code = "reset.open_failed";
        err.message = "Failed to open printer session: " + std::string(ewr_session_last_error(session));
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(err);
        }
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);
    ewr_session_set_blocker_callback(session, StaticBlockerCallback, this);
    ewr_session_set_confirm_callback(session, StaticConfirmCallback, this);
    ewr_session_set_interface(session, interfaceCandidate_);
    ewr_session_set_soft_reset(session, usbSoftReset_ ? 1 : 0);
    ewr_session_set_allow_model_mismatch(session, allowModelMismatch_ ? 1 : 0);

    char* outJson = nullptr;
    rc = ewr_reset(session, model.c_str(), 0, &outJson);

    std::string summary;
    bool success = (rc == EWR_OK);

    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            summary = "Waste ink pad counters successfully reset!\n\n"
                      "IMPORTANT COMMIT STEP:\n"
                      "Turn the printer OFF using its physical power button, then turn it back ON.\n"
                      "This commits the new zeroed EEPROM counters into the printer firmware.";

            LogEntry log;
            log.timestamp = CurrentTimeStr();
            log.level = 1;
            log.code = "reset.success";
            log.message = "Reset completed and verified successfully! Cycle printer power now.";
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(log);
            }
        } catch (...) {
            summary = "Reset command completed.";
        }
        ewr_string_free(outJson);
    }
    else
    {
        const char* err = ewr_session_last_error(session);
        summary = "Reset failed (" + std::string(ewr_status_name(rc)) + "):\n" + (err ? err : "Unknown error");

        LogEntry log;
        log.timestamp = CurrentTimeStr();
        log.level = 3;
        log.code = "reset.error";
        log.message = summary;
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(log);
        }
    }

    ewr_session_close(session);

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        lastOpSummary_ = summary;
        lastOpSuccess_ = success;
        modal_.type = ModalType::OperationResultAlert;
        modal_.title = success ? "Reset Succeeded!" : "Reset Failed";
        modal_.message = summary;
        modal_.isError = !success;
        openModalFlag_ = true;
    }

    if (success)
    {
        // Re-read status
        ActionReadStatus();
    }
}

void App::ActionResetCartridgeInk()
{
    std::string model;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        model = selectedModel_;
    }

    if (model.empty())
        return;

    ewr_session* session = nullptr;
    int rc = (connectionMode_ == ConnectionMode::USB)
        ? ewr_session_open(databasePath_.c_str(), &session)
        : ewr_session_open_network(databasePath_.c_str(), manualIp_.c_str(), &session);

    if (rc != EWR_OK)
    {
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);
    ewr_session_set_blocker_callback(session, StaticBlockerCallback, this);
    ewr_session_set_confirm_callback(session, StaticConfirmCallback, this);
    ewr_session_set_interface(session, interfaceCandidate_);
    ewr_session_set_soft_reset(session, usbSoftReset_ ? 1 : 0);
    ewr_session_set_allow_model_mismatch(session, allowModelMismatch_ ? 1 : 0);

    char* outJson = nullptr;
    rc = ewr_reset(session, model.c_str(), 1, &outJson);

    std::string summary;
    bool success = (rc == EWR_OK);

    if (rc == EWR_OK)
    {
        summary = "Cartridge ink level counters reset successfully!\nCycle printer power now.";
        ewr_string_free(outJson);
    }
    else
    {
        summary = "Ink reset failed: " + std::string(ewr_session_last_error(session));
    }

    ewr_session_close(session);

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        lastOpSummary_ = summary;
        lastOpSuccess_ = success;
        modal_.type = ModalType::OperationResultAlert;
        modal_.title = success ? "Ink Reset Succeeded" : "Ink Reset Failed";
        modal_.message = summary;
        modal_.isError = !success;
        openModalFlag_ = true;
    }

    if (success)
        ActionReadStatus();
}

void App::ActionDryRun()
{
    std::string model;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        model = selectedModel_;
    }

    if (model.empty())
        return;

    ewr_session* session = nullptr;
    int rc = (connectionMode_ == ConnectionMode::USB)
        ? ewr_session_open(databasePath_.c_str(), &session)
        : ewr_session_open_network(databasePath_.c_str(), manualIp_.c_str(), &session);

    if (rc != EWR_OK)
    {
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);
    char* outJson = nullptr;
    rc = ewr_plan(session, model.c_str(), 0, &outJson);

    std::string summary;
    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            std::ostringstream ss;
            ss << "Dry run simulation succeeded for " << model << ".\n\n"
               << "Planned Writes (" << j["planned_writes"].size() << " operations):\n";
            for (const auto& w : j["planned_writes"])
            {
                ss << "  Address 0x" << std::hex << std::uppercase << w["address"].get<int>()
                   << " -> Value 0x" << w["value"].get<int>() << std::dec << "\n";
            }
            summary = ss.str();
        } catch (...) {
            summary = "Dry run completed.";
        }
        ewr_string_free(outJson);
    }
    else
    {
        summary = "Dry run failed: " + std::string(ewr_session_last_error(session));
    }

    ewr_session_close(session);

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        lastOpSummary_ = summary;
        lastOpSuccess_ = (rc == EWR_OK);
        modal_.type = ModalType::OperationResultAlert;
        modal_.title = "Dry Run Simulation";
        modal_.message = summary;
        modal_.isError = (rc != EWR_OK);
        openModalFlag_ = true;
    }
}

void App::ActionDumpEeprom()
{
    std::string model;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        model = selectedModel_;
    }

    if (model.empty())
        return;

    ewr_session* session = nullptr;
    int rc = ewr_session_open(databasePath_.c_str(), &session);
    if (rc != EWR_OK)
    {
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);

    char* outJson = nullptr;
    rc = ewr_dump(session, model.c_str(), &outJson);

    if ((rc == EWR_OK || rc == EWR_ERR_INCOMPLETE_DUMP) && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            int answered = j.value("answered", 0);
            int total = j.value("total", 0);
            std::vector<EepromByte> bytes;
            if (j.contains("values") && j["values"].is_array())
            {
                for (const auto& v : j["values"])
                {
                    EepromByte eb;
                    eb.address = v.value("address", 0);
                    eb.value = v.value("value", -1);
                    bytes.push_back(eb);
                }
            }

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                eepromBytes_ = std::move(bytes);
                eepromAnswered_ = answered;
                eepromTotal_ = total;
            }

            LogEntry log;
            log.timestamp = CurrentTimeStr();
            log.level = 1;
            log.code = "dump.complete";
            log.message = "Dump complete: " + std::to_string(answered) + " / " + std::to_string(total) + " bytes read.";
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(log);
            }
        } catch (...) {}
        ewr_string_free(outJson);
    }
    else
    {
        LogEntry log;
        log.timestamp = CurrentTimeStr();
        log.level = 3;
        log.code = "dump.error";
        log.message = "EEPROM dump failed: " + std::string(ewr_session_last_error(session));
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            logs_.push_back(log);
        }
    }

    ewr_session_close(session);
}

void App::ActionScanNetwork()
{
    ewr_session* session = nullptr;
    int rc = ewr_session_open(databasePath_.c_str(), &session);
    if (rc != EWR_OK && rc != EWR_ERR_DEVICE_NOT_FOUND)
    {
        if (session) ewr_session_close(session);
        return;
    }

    ewr_session_set_event_callback(session, StaticEventCallback, this);

    char* outJson = nullptr;
    rc = ewr_discover_network(session, &outJson);

    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            std::vector<NetworkPrinterItem> list;
            if (j.contains("printers") && j["printers"].is_array())
            {
                for (const auto& p : j["printers"])
                {
                    NetworkPrinterItem item;
                    item.address = p.value("address", "");
                    item.deviceId = p.value("device_id", "");
                    item.reportedModel = p.value("reported_model", "");
                    item.modelMatch = p.value("model_match", "");
                    list.push_back(item);
                }
            }

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                networkPrinters_ = std::move(list);
            }

            LogEntry log;
            log.timestamp = CurrentTimeStr();
            log.level = 1;
            log.code = "net.scan_ok";
            log.message = "Network scan found " + std::to_string(networkPrinters_.size()) + " Epson printer(s).";
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                logs_.push_back(log);
            }
        } catch (...) {}
        ewr_string_free(outJson);
    }

    ewr_session_close(session);
}

void App::ActionScanUsbInterfaces()
{
    ewr_session* session = nullptr;
    int rc = ewr_session_open(databasePath_.c_str(), &session);
    if (rc != EWR_OK && rc != EWR_ERR_DEVICE_NOT_FOUND)
    {
        if (session) ewr_session_close(session);
        return;
    }

    char* outJson = nullptr;
    rc = ewr_list_interfaces(session, &outJson);

    if (rc == EWR_OK && outJson)
    {
        try {
            auto j = nlohmann::json::parse(outJson);
            std::vector<UsbInterfaceItem> list;
            if (j.contains("interfaces") && j["interfaces"].is_array())
            {
                for (const auto& item : j["interfaces"])
                {
                    UsbInterfaceItem ui;
                    ui.index = item.value("candidate", 0);
                    ui.transport = item.value("transport", "");
                    ui.path = item.value("path", "");
                    ui.deviceId = item.value("device_id", "");
                    ui.modelMatch = item.value("model", "");
                    list.push_back(ui);
                }
            }

            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                usbInterfaces_ = std::move(list);
            }
        } catch (...) {}
        ewr_string_free(outJson);
    }

    ewr_session_close(session);
}

void App::ActionUpdateDatabase()
{
    LogEntry startLog;
    startLog.timestamp = CurrentTimeStr();
    startLog.level = 1;
    startLog.code = "update.start";
    startLog.message = "Checking for printer database updates from GitHub...";
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        logs_.push_back(startLog);
    }

    auto res = ewr::update::UpdateDatabaseNow(databasePath_);
    std::string msg;
    if (res == ewr::DatabaseUpdate::Updated)
    {
        msg = "Printer database updated successfully!";
        LoadModelsDatabase();
    }
    else if (res == ewr::DatabaseUpdate::UpToDate)
    {
        msg = "Printer database is already up to date.";
    }
    else
    {
        msg = "Database update check failed (offline or GitHub unreachable).";
    }

    LogEntry endLog;
    endLog.timestamp = CurrentTimeStr();
    endLog.level = (res == ewr::DatabaseUpdate::Failed) ? 2 : 1;
    endLog.code = "update.done";
    endLog.message = msg;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        logs_.push_back(endLog);
    }
}

void App::FilterModelsList()
{
    std::string search = modelSearchBuf_;
    std::transform(search.begin(), search.end(), search.begin(), [](unsigned char c) { return std::tolower(c); });

    filteredModels_.clear();
    for (const auto& m : allModels_)
    {
        if (showOnlyResettable_ && !m.resettable)
            continue;

        if (search.empty())
        {
            filteredModels_.push_back(m);
            continue;
        }

        std::string lowerName = m.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });
        if (lowerName.find(search) != std::string::npos)
        {
            filteredModels_.push_back(m);
            continue;
        }

        bool matchAlias = false;
        for (const auto& a : m.aliases)
        {
            std::string lowerAlias = a;
            std::transform(lowerAlias.begin(), lowerAlias.end(), lowerAlias.begin(), [](unsigned char c) { return std::tolower(c); });
            if (lowerAlias.find(search) != std::string::npos)
            {
                matchAlias = true;
                break;
            }
        }
        if (matchAlias)
            filteredModels_.push_back(m);
    }
}

void App::ExportEepromToFile(const std::string& path, bool asJson)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (eepromBytes_.empty())
        return;

    if (asJson)
    {
        nlohmann::json j;
        j["model"] = selectedModel_;
        j["answered"] = eepromAnswered_;
        j["total"] = eepromTotal_;
        nlohmann::json vals = nlohmann::json::array();
        for (const auto& eb : eepromBytes_)
        {
            nlohmann::json v;
            v["address"] = eb.address;
            v["value"] = eb.value;
            vals.push_back(v);
        }
        j["values"] = vals;

        std::ofstream f(path);
        if (f.good())
            f << j.dump(2);
    }
    else
    {
        std::ofstream f(path, std::ios::binary);
        if (f.good())
        {
            for (const auto& eb : eepromBytes_)
            {
                uint8_t byteVal = (eb.value >= 0) ? static_cast<uint8_t>(eb.value) : 0;
                f.write(reinterpret_cast<const char*>(&byteVal), 1);
            }
        }
    }
}

// -------------------------------------------------------------------------
// Main Render Loop
// -------------------------------------------------------------------------

void App::Render()
{
    RenderTopBar();

    ImGuiTabBarFlags tabFlags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MainTabs", tabFlags))
    {
        if (ImGui::BeginTabItem("Dashboard & Reset"))
        {
            RenderDashboardTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Network Printers"))
        {
            RenderNetworkTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Model Database"))
        {
            RenderModelsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EEPROM Inspector"))
        {
            RenderEepromTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Activity Log"))
        {
            RenderLogsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            RenderSettingsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    RenderModals();
}

// -------------------------------------------------------------------------
// UI Panels & Popups
// -------------------------------------------------------------------------

void App::RenderModelPickerPopup()
{
    if (ImGui::BeginPopup("ModelPickerPopup"))
    {
        ImGui::TextColored(colors::PrimaryHover, "Select Printer Model");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        if (ImGui::InputTextWithHint("##ModelSearchPopup", "Search e.g. L3150, ET-2800, XP...", modelSearchBuf_, sizeof(modelSearchBuf_)))
        {
            FilterModelsList();
        }

        ImGui::Spacing();
        ImGui::BeginChild("ModelPickerList", ImVec2(300.0f, 220.0f), true);
        for (const auto& m : filteredModels_)
        {
            bool isSelected = (m.name == selectedModel_);
            std::string label = m.name;
            if (!m.aliases.empty())
                label += " (" + m.aliases[0] + ")";

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selectedModel_ = m.name;
                ImGui::CloseCurrentPopup();
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        if (filteredModels_.empty())
        {
            ImGui::TextDisabled("No matching models found.");
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::TextDisabled("Showing %zu of %zu models", filteredModels_.size(), allModels_.size());
        ImGui::EndPopup();
    }
}

void App::RenderTopBar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::HeaderBg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::BeginChild("TopHeaderBar", ImVec2(0, 80.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const float availWidth = ImGui::GetContentRegionAvail().x;

    // --- Line 1: Branding, Status Pill, Mode Switch ---
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(colors::PrimaryHover, "EPSON WASTE RESET");
    ImGui::SameLine();
    ImGui::TextDisabled("v%s", EWR_VERSION);

    // Status pill badge
    ImGui::SameLine();
    if (isBusy_)
    {
        DrawStatusBadge("WORKING...", colors::Warning);
    }
    else if (hasStatusData_)
    {
        if (condition_.state == "READY")
            DrawStatusBadge("READY", colors::Success);
        else if (condition_.state == "ERROR")
            DrawStatusBadge(condition_.error.empty() ? "ERROR" : condition_.error.c_str(), colors::Danger);
        else
            DrawStatusBadge(condition_.state.c_str(), colors::Warning);
    }
    else if (!selectedModel_.empty())
    {
        DrawStatusBadge(selectedModel_.c_str(), colors::Primary);
    }
    else
    {
        DrawStatusBadge("IDLE", colors::TextMuted);
    }

    // Connection mode toggle on the right
    const float modeWidth = 190.0f;
    if (availWidth > 560.0f)
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - modeWidth - 14.0f);
        ImGui::Text("Mode:");
        ImGui::SameLine();
        if (ImGui::RadioButton("USB", connectionMode_ == ConnectionMode::USB))
            connectionMode_ = ConnectionMode::USB;
        DrawTooltip("Direct USB connection via Win32 SetupAPI & libusb");
        ImGui::SameLine();
        if (ImGui::RadioButton("Network", connectionMode_ == ConnectionMode::Network))
            connectionMode_ = ConnectionMode::Network;
        DrawTooltip("Network printer communication over SNMP (UDP 161)");
    }

    // --- Line 2: Active Target Model, Auto-Detect, Read Status, IP input ---
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Target:");
    ImGui::SameLine();

    std::string modelBtnLabel = selectedModel_.empty() ? "Select Model Profile ▾" : (selectedModel_ + " ▾");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
    if (ImGui::Button(modelBtnLabel.c_str(), ImVec2(220.0f, 0)))
    {
        ImGui::OpenPopup("ModelPickerPopup");
    }
    ImGui::PopStyleColor();
    DrawTooltip("Click to search through 1,450+ printer models");

    RenderModelPickerPopup();

    ImGui::SameLine();
    if (ImGui::Button("Auto-Detect USB"))
    {
        StartWorkerTask("Detecting connected Epson printer...", [this]()
        {
            ActionDetectModel();
            if (!selectedModel_.empty())
                ActionReadStatus();
        });
    }
    DrawTooltip("Scans USB tree for connected Epson printers and matches IEEE 1284 Device ID");

    ImGui::SameLine();
    if (ImGui::Button("Read Status"))
    {
        StartWorkerTask("Reading printer counters and status...", [this]()
        {
            ActionReadStatus();
        });
    }
    DrawTooltip("Queries current EEPROM waste pad counters and ink levels");

    if (connectionMode_ == ConnectionMode::Network)
    {
        ImGui::SameLine();
        ImGui::Text("IP:");
        ImGui::SameLine();
        char ipBuf[64];
        std::snprintf(ipBuf, sizeof(ipBuf), "%s", manualIp_.c_str());
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputText("##TopIP", ipBuf, sizeof(ipBuf)))
        {
            manualIp_ = ipBuf;
        }
        DrawTooltip("Enter the printer's local IP address (e.g. 192.168.1.100)");
    }

    // Live busy task description
    if (isBusy_)
    {
        ImGui::SameLine();
        ImGui::TextColored(colors::Warning, " %s", currentTaskDesc_.c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void App::RenderDashboardTab()
{
    const float availWidth = ImGui::GetContentRegionAvail().x;

    if (!hasStatusData_)
    {
        RenderDashboardEmptyState();
    }
    else
    {
        RenderDashboardActiveState(availWidth);
    }
}

void App::RenderDashboardEmptyState()
{
    const float availWidth = ImGui::GetContentRegionAvail().x;

    // Welcome & Quick Action Card
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("WelcomeCard", ImVec2(0, 0), true);

    DrawSectionHeader("PRINTER CONNECTION & SETUP", "Follow the steps below to connect and inspect your printer");

    ImGui::Spacing();
    ImGui::TextWrapped("Epson Waste Reset (EWR) communicates directly with your printer's motherboard over USB or Network (SNMP) "
                       "to safely inspect and reset the EEPROM waste ink pad counters without third-party reset keys.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 3 Responsive Action Cards
    const float cardWidth = (availWidth > 720.0f) ? ((availWidth - 40.0f) / 3.0f) : -1.0f;
    const float cardHeight = 160.0f;

    // Card 1: USB Auto-Detect
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBgAlt);
    ImGui::BeginChild("Step1Card", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(colors::PrimaryHover, "1. USB Auto-Detect");
    ImGui::TextDisabled("(Recommended)");
    ImGui::Spacing();
    ImGui::TextWrapped("Plug printer into USB. Automatically scans SetupAPI and libusb interfaces.");
    ImGui::Spacing();
    if (ImGui::Button("Auto-Detect USB Printer", ImVec2(-1.0f, 34.0f)))
    {
        connectionMode_ = ConnectionMode::USB;
        StartWorkerTask("Detecting connected Epson printer...", [this]()
        {
            ActionDetectModel();
            if (!selectedModel_.empty())
                ActionReadStatus();
        });
    }
    ImGui::EndChild();

    if (availWidth > 720.0f)
        ImGui::SameLine();

    // Card 2: Network Scan
    ImGui::BeginChild("Step2Card", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(colors::PrimaryHover, "2. Network Scan");
    ImGui::TextDisabled("(Wi-Fi / Ethernet)");
    ImGui::Spacing();
    ImGui::TextWrapped("Discovers Epson printers answering SNMP broadcasts across your LAN in ~2s.");
    ImGui::Spacing();
    if (ImGui::Button("Scan Local Network", ImVec2(-1.0f, 34.0f)))
    {
        connectionMode_ = ConnectionMode::Network;
        StartWorkerTask("Scanning local network for Epson printers...", [this]()
        {
            ActionScanNetwork();
        });
    }
    ImGui::EndChild();

    if (availWidth > 720.0f)
        ImGui::SameLine();

    // Card 3: Manual Model Selection
    ImGui::BeginChild("Step3Card", ImVec2(cardWidth, cardHeight), true);
    ImGui::TextColored(colors::PrimaryHover, "3. Manual Model");
    ImGui::TextDisabled("(Offline Database)");
    ImGui::Spacing();
    ImGui::TextWrapped("Select your specific printer model from the offline database of 1,450+ profiles.");
    ImGui::Spacing();
    if (ImGui::Button("Choose From Database...", ImVec2(-1.0f, 34.0f)))
    {
        ImGui::OpenPopup("ModelPickerPopup");
    }
    RenderModelPickerPopup();
    ImGui::EndChild();
    ImGui::PopStyleColor(); // CardBgAlt

    ImGui::Spacing();
    ImGui::Spacing();

    // Quick read status if model is already selected
    if (!selectedModel_.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.22f, 0.32f, 0.85f));
        ImGui::BeginChild("ReadyToReadCard", ImVec2(0, 75.0f), true);
        ImGui::Text("Active Model Profile: ");
        ImGui::SameLine();
        ImGui::TextColored(colors::PrimaryHover, "%s", selectedModel_.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
        if (ImGui::Button("Read Status & Counters Now", ImVec2(220.0f, 32.0f)))
        {
            StartWorkerTask("Reading printer counters and status...", [this]()
            {
                ActionReadStatus();
            });
        }
        ImGui::TextDisabled("Ready to query printer hardware condition and EEPROM waste pad values.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Helpful checklist
    DrawSectionHeader("BEFORE YOU BEGIN");
    ImGui::BulletText("Ensure your Epson printer is powered ON with its physical power button.");
    ImGui::BulletText("Make sure no stuck print jobs are pending in the Windows Print Spooler queue.");
    ImGui::BulletText("EcoTank / L-Series: Maintenance box or waste pads must have sufficient physical absorbent capacity.");
    ImGui::BulletText("Zero installation required: Uses native Windows SetupAPI and statically linked libusb.");

    ImGui::EndChild();
    ImGui::PopStyleColor(); // CardBg
}

void App::RenderDashboardActiveState(float availWidth)
{
    // 1. Compact Status Summary Bar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("StatusSummaryBar", ImVec2(0, 60.0f), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(colors::PrimaryHover, "%s", selectedModel_.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| Connection: %s", (connectionMode_ == ConnectionMode::USB) ? "USB Direct" : manualIp_.c_str());

    ImGui::SameLine();
    ImGui::TextDisabled("| State:");
    ImGui::SameLine();
    if (condition_.state == "READY")
        ImGui::TextColored(colors::Success, "READY");
    else if (condition_.state == "ERROR")
        ImGui::TextColored(colors::Danger, "ERROR (%s)", condition_.error.empty() ? "Hardware Error" : condition_.error.c_str());
    else
        ImGui::TextColored(colors::Warning, "%s", condition_.state.c_str());

    if (!condition_.serial.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| S/N: %s", condition_.serial.c_str());
    }

    // Buttons on top right of the summary bar
    const float rightBtnWidth = 240.0f;
    if (availWidth > 680.0f)
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - rightBtnWidth - 10.0f);
        if (ImGui::Button("Refresh Status", ImVec2(115.0f, 0)))
        {
            StartWorkerTask("Reading printer counters and status...", [this]()
            {
                ActionReadStatus();
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Change Model", ImVec2(115.0f, 0)))
        {
            ImGui::OpenPopup("ModelPickerPopup");
        }
        RenderModelPickerPopup();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // 2. Responsive 2-column or stacked layout
    const bool twoColumns = (availWidth >= 750.0f);
    const float leftWidth = twoColumns ? ((availWidth - 12.0f) * 0.55f) : -1.0f;
    const float rightWidth = twoColumns ? ((availWidth - 12.0f) * 0.45f) : -1.0f;

    // LEFT PANE: Waste Ink Pad Gauges & Ink Levels
    if (twoColumns)
        ImGui::BeginChild("DashboardLeftPane", ImVec2(leftWidth, 0), false);

    // Waste Ink Pad Gauges Card
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("WasteGaugesCard", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);
    DrawSectionHeader("WASTE INK PAD GAUGES");

    if (!wastePads_.empty())
    {
        for (const auto& pad : wastePads_)
        {
            DrawWasteGauge(pad.name.c_str(), pad.kind.c_str(), pad.used, pad.max, pad.percent);
            ImGui::Spacing();
        }

        if (!resetCovers_.empty())
        {
            ImGui::Spacing();
            ImGui::TextDisabled("EEPROM Reset Coverage:");
            for (const auto& cov : resetCovers_)
            {
                ImGui::Bullet();
                ImGui::TextDisabled("%s (%s)", cov.name.c_str(), cov.readable ? "Directly readable" : "Protected / shadow");
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No waste counters reported. Click 'Refresh Status' above.");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Cartridge Ink Levels Card
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("CartridgeInkCard", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);
    DrawSectionHeader("CARTRIDGE INK LEVELS");

    if (!inkCartridges_.empty())
    {
        for (const auto& ink : inkCartridges_)
        {
            DrawInkGauge(ink.color.c_str(), ink.level, ink.status.c_str());
            ImGui::Spacing();
        }
    }
    else
    {
        ImGui::TextDisabled("Integrated EcoTank model or ink levels not electronically reported.\n"
                            "Inspect ink levels visually through front tank windows.");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (twoColumns)
    {
        ImGui::EndChild(); // DashboardLeftPane
        ImGui::SameLine();
        ImGui::BeginChild("DashboardRightPane", ImVec2(rightWidth, 0), false);
    }
    else
    {
        ImGui::Spacing();
    }

    // RIGHT PANE: Maintenance Actions & Hardware Instructions
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("MaintenanceActionsCard", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);
    DrawSectionHeader("MAINTENANCE ACTIONS");

    ImGui::Spacing();

    // Primary Reset Button
    bool has100Percent = false;
    for (const auto& p : wastePads_)
    {
        if (p.percent >= 98.0)
            has100Percent = true;
    }

    const ImVec4 btnCol = has100Percent ? colors::Danger : colors::Primary;
    const ImVec4 btnColHover = has100Percent ? colors::DangerHover : colors::PrimaryHover;
    const ImVec4 btnColActive = has100Percent ? colors::DangerActive : colors::PrimaryActive;

    ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnColHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnColActive);

    bool canReset = !isBusy_ && !selectedModel_.empty();
    if (!canReset)
        ImGui::BeginDisabled();

    if (ImGui::Button("RESET WASTE INK PAD COUNTERS", ImVec2(-1.0f, 44.0f)))
    {
        StartWorkerTask("Executing Waste Ink Counter Reset...", [this]()
        {
            ActionResetWastePads();
        });
    }
    DrawTooltip("Zeroes out the EEPROM waste pad counters and verifies the new values.");

    if (!canReset)
        ImGui::EndDisabled();

    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // Secondary actions in a responsive row
    const float subBtnWidth = (ImGui::GetContentRegionAvail().x - 12.0f) / 3.0f;

    if (ImGui::Button("Reset Ink Levels", ImVec2(subBtnWidth, 32.0f)))
    {
        StartWorkerTask("Resetting Cartridge Ink Levels...", [this]()
        {
            ActionResetCartridgeInk();
        });
    }
    DrawTooltip("Resets cartridge ink levels for models with per-color ink map");

    ImGui::SameLine();
    if (ImGui::Button("Dry Run / Test", ImVec2(subBtnWidth, 32.0f)))
    {
        StartWorkerTask("Simulating planned writes...", [this]()
        {
            ActionDryRun();
        });
    }
    DrawTooltip("Tests communication and displays planned writes without touching EEPROM");

    ImGui::SameLine();
    if (ImGui::Button("Dump EEPROM", ImVec2(subBtnWidth, 32.0f)))
    {
        StartWorkerTask("Reading full EEPROM dump...", [this]()
        {
            ActionDumpEeprom();
        });
    }
    DrawTooltip("Reads full EEPROM into memory for inspection and backup");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Planned Writes Details (Collapsible)
    if (!plannedWrites_.empty())
    {
        if (ImGui::CollapsingHeader("Planned EEPROM Writes"))
        {
            ImGui::TextDisabled("Writes that will be applied by reset:");
            for (const auto& w : plannedWrites_)
            {
                ImGui::BulletText("Address 0x%04X -> Value 0x%02X (%d)", w.address, w.value, w.value);
            }
            ImGui::Spacing();
        }
    }

    // Hardware Commit Box
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.18f, 0.22f, 0.85f));
    ImGui::BeginChild("CommitBox", ImVec2(0, 115.0f), true);
    ImGui::TextColored(colors::Warning, "IMPORTANT COMMIT NOTICE:");
    ImGui::TextWrapped("1. After resetting, turn printer OFF with physical power button.\n"
                       "2. Wait 5 seconds, then turn printer back ON to apply changes.\n"
                       "3. Ensure physical waste ink pads have absorbent capacity.");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::EndChild(); // MaintenanceActionsCard
    ImGui::PopStyleColor();

    if (twoColumns)
    {
        ImGui::EndChild(); // DashboardRightPane
    }
}

void App::RenderNetworkTab()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("NetScanControls", ImVec2(0, 90.0f), true);

    DrawSectionHeader("LOCAL NETWORK DISCOVERY (SNMP BROADCAST)");

    ImGui::AlignTextToFramePadding();
    if (ImGui::Button("Scan Local Subnet for Epson Printers", ImVec2(280.0f, 30.0f)))
    {
        StartWorkerTask("Scanning local network broadcast for Epson printers (UDP 161)...", [this]()
        {
            ActionScanNetwork();
        });
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(Broadcasts SNMP query to all active network adapters; takes ~2 seconds)");

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Table of discovered network printers
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("NetPrintersTable", 5, tableFlags))
    {
        ImGui::TableSetupColumn("IP Address", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Reported Model", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Database Match", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupColumn("Device ID", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableHeadersRow();

        for (const auto& p : networkPrinters_)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(colors::PrimaryHover, "%s", p.address.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(p.reportedModel.c_str());

            ImGui::TableSetColumnIndex(2);
            if (!p.modelMatch.empty())
                ImGui::TextColored(colors::Success, "%s", p.modelMatch.c_str());
            else
                ImGui::TextDisabled("No direct match");

            ImGui::TableSetColumnIndex(3);
            ImGui::TextDisabled("%s", p.deviceId.c_str());

            ImGui::TableSetColumnIndex(4);
            std::string btnId = "Connect##" + p.address;
            if (ImGui::Button(btnId.c_str(), ImVec2(-1, 0)))
            {
                manualIp_ = p.address;
                connectionMode_ = ConnectionMode::Network;
                if (!p.modelMatch.empty())
                    selectedModel_ = p.modelMatch;
                StartWorkerTask("Connecting to network printer at " + p.address + "...", [this]()
                {
                    ActionReadStatus();
                });
            }
        }

        ImGui::EndTable();
    }
}

void App::RenderModelsTab()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("ModelFilters", ImVec2(0, 80.0f), true);

    DrawSectionHeader("PRINTER DATABASE EXPLORER", ("Total: " + std::to_string(allModels_.size()) + " models").c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::InputTextWithHint("##ModelSearch", "Search e.g. L3150, ET-2800, Stylus...", modelSearchBuf_, sizeof(modelSearchBuf_)))
    {
        FilterModelsList();
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Only Resettable", &showOnlyResettable_))
    {
        FilterModelsList();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(Showing %zu models)", filteredModels_.size());

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Table of models
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("ModelsTable", 5, tableFlags))
    {
        ImGui::TableSetupColumn("Model Name", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("Aliases", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Waste Reset", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Ink Reset", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        for (const auto& m : filteredModels_)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(colors::PrimaryHover, "%s", m.name.c_str());

            ImGui::TableSetColumnIndex(1);
            std::string aliasStr;
            for (size_t i = 0; i < m.aliases.size(); ++i)
            {
                aliasStr += m.aliases[i];
                if (i + 1 < m.aliases.size()) aliasStr += ", ";
            }
            ImGui::TextDisabled("%s", aliasStr.empty() ? "-" : aliasStr.c_str());

            ImGui::TableSetColumnIndex(2);
            if (m.resettable)
                ImGui::TextColored(colors::Success, "Supported");
            else
                ImGui::TextDisabled("No");

            ImGui::TableSetColumnIndex(3);
            if (m.hasInkReset)
                ImGui::TextColored(colors::Success, "Yes");
            else
                ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(4);
            std::string btnId = "Select##" + m.name;
            if (ImGui::Button(btnId.c_str(), ImVec2(-1, 0)))
            {
                selectedModel_ = m.name;
            }
        }

        ImGui::EndTable();
    }
}

void App::RenderEepromTab()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("EepromControls", ImVec2(0, 80.0f), true);

    DrawSectionHeader("EEPROM HEX DUMP & INSPECTOR");

    if (ImGui::Button("Read EEPROM Dump", ImVec2(180.0f, 30.0f)))
    {
        StartWorkerTask("Dumping full printer EEPROM...", [this]()
        {
            ActionDumpEeprom();
        });
    }

    ImGui::SameLine();
    if (!eepromBytes_.empty())
    {
        if (ImGui::Button("Export to .bin", ImVec2(130.0f, 30.0f)))
        {
            ExportEepromToFile("eeprom_dump.bin", false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export to .json", ImVec2(130.0f, 30.0f)))
        {
            ExportEepromToFile("eeprom_dump.json", true);
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Read: %d / %d bytes", eepromAnswered_, eepromTotal_);

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Hex view
    if (eepromBytes_.empty())
    {
        ImGui::TextDisabled("No EEPROM data loaded. Click 'Read EEPROM Dump' above.");
        return;
    }

    ImGui::BeginChild("HexScrollRegion", ImVec2(0, 0), true);

    const int bytesPerRow = 16;
    for (size_t row = 0; row < eepromBytes_.size(); row += bytesPerRow)
    {
        // Address offset
        ImGui::TextColored(colors::PrimaryHover, "%04zX: ", row);
        ImGui::SameLine();

        // Hex bytes
        for (int col = 0; col < bytesPerRow; ++col)
        {
            size_t idx = row + col;
            if (idx < eepromBytes_.size())
            {
                int val = eepromBytes_[idx].value;
                if (val >= 0)
                    ImGui::Text("%02X ", static_cast<unsigned int>(val));
                else
                    ImGui::TextDisabled("?? ");
            }
            else
            {
                ImGui::Text("   ");
            }
            ImGui::SameLine();
        }

        ImGui::TextDisabled("| ");
        ImGui::SameLine();

        // ASCII representation
        char ascii[bytesPerRow + 1];
        for (int col = 0; col < bytesPerRow; ++col)
        {
            size_t idx = row + col;
            if (idx < eepromBytes_.size())
            {
                int val = eepromBytes_[idx].value;
                ascii[col] = (val >= 32 && val <= 126) ? static_cast<char>(val) : '.';
            }
            else
            {
                ascii[col] = ' ';
            }
        }
        ascii[bytesPerRow] = '\0';
        ImGui::TextUnformatted(ascii);
    }

    ImGui::EndChild();
}

void App::RenderLogsTab()
{
    // Filter & control header
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("LogControls", ImVec2(0, 50.0f), true);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Level:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    const char* levels[] = { "All", "Info+", "Warnings+", "Errors" };
    ImGui::Combo("##LogLevel", &logFilterLevel_, levels, IM_ARRAYSIZE(levels));

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScrollLogs_);

    ImGui::SameLine();
    if (ImGui::Button("Clear Logs"))
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        logs_.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy Log to Clipboard"))
    {
        std::string fullText;
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            for (const auto& log : logs_)
            {
                fullText += "[" + log.timestamp + "] " + log.code + ": " + log.message + "\n";
            }
        }
        ImGui::SetClipboardText(fullText.c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Log list
    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), true);

    std::lock_guard<std::mutex> lock(dataMutex_);
    for (const auto& entry : logs_)
    {
        if (entry.level < logFilterLevel_)
            continue;

        ImGui::TextDisabled("[%s]", entry.timestamp.c_str());
        ImGui::SameLine();

        ImVec4 col = colors::TextMuted;
        const char* lvlTag = "INFO";
        if (entry.level == 0) { col = colors::TextMuted; lvlTag = "TRACE"; }
        else if (entry.level == 1) { col = colors::Success; lvlTag = "INFO"; }
        else if (entry.level == 2) { col = colors::Warning; lvlTag = "WARN"; }
        else if (entry.level >= 3) { col = colors::Danger; lvlTag = "ERROR"; }

        ImGui::TextColored(col, "[%-5s]", lvlTag);
        ImGui::SameLine();
        ImGui::TextColored(colors::PrimaryHover, "%-18s", entry.code.c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted(entry.message.c_str());
    }

    if (autoScrollLogs_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

void App::RenderSettingsTab()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::CardBg);
    ImGui::BeginChild("SettingsCard", ImVec2(0, 0), true);

    DrawSectionHeader("ADVANCED COMMUNICATION SETTINGS");

    ImGui::Checkbox("Allow Model Mismatch (--force-yes override)", &allowModelMismatch_);
    ImGui::TextDisabled("Allows writing EEPROM values even if printer IEEE 1284 ID reports a different model.");
    if (allowModelMismatch_)
    {
        ImGui::TextColored(colors::Danger, "WARNING: Writing incorrect values into an incompatible printer may brick the motherboard.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("USB Soft-Reset (Diagnostic)", &usbSoftReset_);
    ImGui::TextDisabled("Resets the USB bus channel on Windows before connecting. Useful if print spooler locks up.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("USB Interface Candidate:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    ImGui::InputInt("##InterfaceCandidate", &interfaceCandidate_);
    ImGui::SameLine();
    ImGui::TextDisabled("(0 = Automatic interface detection)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawSectionHeader("DATABASE UPDATES & MAINTENANCE");
    ImGui::Text("Database Path: %s", databasePath_.c_str());
    ImGui::Spacing();

    if (ImGui::Button("Check for Database Updates on GitHub", ImVec2(280.0f, 32.0f)))
    {
        StartWorkerTask("Checking for database updates...", [this]()
        {
            ActionUpdateDatabase();
        });
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawSectionHeader("ABOUT EPSON WASTE RESET (EWR)");
    ImGui::Text("EWR GUI Version: %s", EWR_VERSION);
    ImGui::Text("License: Apache License 2.0 (Free, Open-Source)");
    ImGui::TextDisabled("Bypasses third-party reset keys and malicious utilities using direct IEEE 1284.4 EEPROM packet generation.");

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void App::RenderModals()
{
    if (openModalFlag_)
    {
        openModalFlag_ = false;
        ImGui::OpenPopup("ModalDialog");
    }

    // Set modal size and position
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("ModalDialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(modal_.isError ? colors::Danger : colors::PrimaryHover, "%s", modal_.title.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("%s", modal_.message.c_str());
        ImGui::Spacing();

        if (modal_.type == ModalType::ConfirmReset)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.20f, 0.24f, 0.80f));
            ImGui::BeginChild("ModalDetails", ImVec2(0, 100.0f), true);
            ImGui::TextColored(colors::Warning, "HARDWARE NOTICE:");
            ImGui::TextWrapped("1. The waste counters in EEPROM will be zeroed.\n"
                               "2. After write finishes, you MUST turn the printer OFF and ON with its power button.\n"
                               "3. Verify that physical waste ink pads are cleaned or an external tank is attached.");
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, colors::Danger);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::DangerHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::DangerActive);
            if (ImGui::Button("YES - PROCEED WITH RESET", ImVec2(240.0f, 36.0f)))
            {
                if (modal_.decisionPromise)
                {
                    modal_.decisionPromise->set_value(true);
                    modal_.decisionPromise.reset();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 36.0f)))
            {
                if (modal_.decisionPromise)
                {
                    modal_.decisionPromise->set_value(false);
                    modal_.decisionPromise.reset();
                }
                ImGui::CloseCurrentPopup();
            }
        }
        else if (modal_.type == ModalType::BlockerWarning)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, colors::Danger);
            if (ImGui::Button("Overrule Blocker & Force Write", ImVec2(220.0f, 36.0f)))
            {
                if (modal_.decisionPromise)
                {
                    modal_.decisionPromise->set_value(true);
                    modal_.decisionPromise.reset();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            if (ImGui::Button("Cancel / Safe Exit", ImVec2(150.0f, 36.0f)))
            {
                if (modal_.decisionPromise)
                {
                    modal_.decisionPromise->set_value(false);
                    modal_.decisionPromise.reset();
                }
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            // Simple OK button
            if (ImGui::Button("OK", ImVec2(120.0f, 34.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

} // namespace ewr_gui

